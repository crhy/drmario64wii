/* Dr. Mario 64 -> Wii native port: stubs for libultra services the game
 * reaches through libmus/libal but that have no Wii implementation yet
 * (task 07 link support). Audio DMA and the libultra scheduler client API
 * are inert here; the game's own NNSched (nnsched.c) does the real work. */

#include <PR/ultratypes.h>
#include <PR/os_message.h>
#include <PR/sched.h>
#include <stdint.h>
#include <PR/os_ai.h>
#include <PR/os_cache.h>
#include <PR/os_convert.h>
#include <PR/os_system.h>
#include <PR/os_vi.h>
#include <PR/os_exception.h>
#include <PR/os_eeprom.h>
#include <PR/os_pi.h>
#include <PR/gu.h>
#include <PR/gbi.h>
#include <PR/os_cont.h>
#include <PR/os_host.h>

#ifdef TARGET_WII
#include <stdio.h>
#include "port_log.h"
#include "os_sched.h"
#include "../audio/naudio.h"
#include <PR/rcp.h>

/* text canary (task 15d) */
extern void port_canary_boot(void);
extern void port_canary_check(int kind, unsigned n, uint32_t a, void *p, uint32_t s);

/* Forward-declare the libogc audio-interface (AI DMA) bits with plain C types
 * so they do not clash with the libultra u32/s32 typedefs.  Only linkable
 * (non-inline) symbols are declared; the inline wrappers are replicated
 * locally.  The N64 AI is a plain big-endian 16-bit stereo DMA FIFO, which the
 * Wii AI DMA plays with no DSP microcode, so we drive it directly. */
void AUDIO_Init(unsigned char *);
void AUDIO_SetDSPSampleRate(unsigned char);
void AUDIO_RegisterDMACallback(void (*)(void));
void AUDIO_InitDMA(unsigned int, unsigned int);
void AUDIO_StartDMA(void);
void AUDIO_StopDMA(void);
unsigned int AUDIO_GetDMABytesLeft(void);
void PPCDCacheFlushAsync(const void *, unsigned int);

/* DCFlushRange() in libogc is an inline; the linkable flush primitive is
 * PPCDCacheFlushAsync (the no-sync variant), so call it and sync ourselves. */
static inline void ai_dc_flush(const void *buf, unsigned int size)
{
    PPCDCacheFlushAsync(buf, size);
    __asm__ volatile("sync");
}

#define AI_SAMPLERATE_32KHZ 0x00000000u
#define AI_SAMPLERATE_48KHZ 0x00000001u
#endif

/* system */
s32 osTvType = 1; /* NTSC */

void osWritebackDCacheAll(void) {
#ifdef TARGET_WII
    port_preempt_check();
#endif
    /* PPC/Wii: no MIPS-style writeback needed */
}

u32 osVirtualToPhysical(void *addr) {
#ifdef TARGET_WII
    port_preempt_check();
#endif
    return (u32)(uintptr_t)addr;
}

/* audio interface (AI DMA) */
#ifdef TARGET_WII
#define AI_RING_BUFS      4
#define AI_RING_BYTES     8192
#define AI_SILENCE_BYTES  2048

/* Ring of 4 DMA buffers (32-byte aligned) plus a 32-byte-aligned silence
 * buffer.  The N64 AI is a plain big-endian 16-bit stereo FIFO; the Wii AI DMA
 * plays exactly that with no DSP microcode, so libmus's frame buffers are
 * copied straight into the ring and fed to the AI one block at a time. */
static u8 g_ai_ring[AI_RING_BUFS][AI_RING_BYTES] __attribute__((aligned(32)));
static u8 g_ai_silence[AI_SILENCE_BYTES] __attribute__((aligned(32)));
static u32 g_ai_len[AI_RING_BUFS];
static u32 g_ai_head;   /* index of the block the DMA is currently playing */
static u32 g_ai_tail;   /* index of the next free slot to enqueue into */
static u32 g_ai_count;  /* blocks in the ring: 1 playing + (count-1) queued */
static u32 g_ai_frames;
static u32 g_ai_underruns;
static int g_ai_playing; /* DMA has been started and is consuming a block */

/* The ring indices are shared with the AI DMA IRQ callback, so they are guarded
 * by disabling IRQs (never a LWP mutex, never PORT_LOG inside the callback). */
extern unsigned int port_irq_disable(void);
extern void port_irq_restore(unsigned int level);
static unsigned int g_ai_irq;
static void ai_lock(void) { g_ai_irq = port_irq_disable(); }
static void ai_unlock(void) { port_irq_restore(g_ai_irq); }

/* AI DMA interrupt: fires when the hardware has latched the current block and
 * is ready for the next address, i.e. the block that was playing is finished.
 * Drop it, then feed the next queued block (or silence on an underrun).  Feed
 * with AUDIO_InitDMA only - never AUDIO_StartDMA again. */
static void ai_dma_cb(void)
{
    const void *buf;
    u32 len;

    ai_lock();
    if (g_ai_count > 0u) {
        /* the playing block (head) just finished: drop it */
        g_ai_head = (g_ai_head + 1u) % AI_RING_BUFS;
        g_ai_count--;
    }
    if (g_ai_count > 0u) {
        u32 idx = g_ai_head;
        buf = g_ai_ring[idx];
        len = g_ai_len[idx];
    } else {
        buf = g_ai_silence;
        len = AI_SILENCE_BYTES;
        g_ai_underruns++;
    }
    ai_unlock();

    AUDIO_InitDMA((u32)(uintptr_t)buf, len);
}

u32 osAiGetStatus(void)
{
    u32 full = 0;
    ai_lock();
    if (g_ai_count >= AI_RING_BUFS)
        full = AI_STATUS_FIFO_FULL;
    ai_unlock();
    return full;
}

/* Bytes still to play: the remaining bytes of the playing block (hardware
 * register) plus the bytes of every block queued behind it.  This is what
 * libmus expects from the N64 AI: never more than ~2 frames. */
u32 osAiGetLength(void)
{
    u32 i;
    u32 total = 0u;
    ai_lock();
    if (g_ai_playing && g_ai_count > 0u) {
        total = AUDIO_GetDMABytesLeft();
        for (i = 1u; i < g_ai_count; i++) {
            u32 idx = (g_ai_head + i) % AI_RING_BUFS;
            total += g_ai_len[idx];
        }
    }
    ai_unlock();
    return total;
}

s32 osAiSetFrequency(u32 freq)
{
    u8 rate;
    if (freq == 48000u) {
        rate = (u8)AI_SAMPLERATE_48KHZ;
    } else if (freq == 32000u) {
        rate = (u8)AI_SAMPLERATE_32KHZ;
    } else {
        AUDIO_SetDSPSampleRate((u8)AI_SAMPLERATE_32KHZ);
        return 32000;
    }
    AUDIO_SetDSPSampleRate(rate);
    return (s32)freq;
}

s32 osAiSetNextBuffer(void *buf, u32 size)
{
    u32 idx;
    u32 len;
    u32 frames;
    u32 underruns;
    int start = 0;

    if (!buf || size == 0u || size > AI_RING_BYTES)
        return -1;

    len = (size + 31u) & ~31u;   /* round up to a multiple of 32 */

    ai_lock();
    if (g_ai_count >= AI_RING_BUFS) {
        ai_unlock();
        return -1;
    }

    idx = g_ai_tail;
    __builtin_memcpy(g_ai_ring[idx], buf, size);
    if (len > size)
        __builtin_memset((u8 *)g_ai_ring[idx] + size, 0, len - size);
    ai_dc_flush(g_ai_ring[idx], len);
    g_ai_len[idx] = len;
    g_ai_tail = (g_ai_tail + 1u) % AI_RING_BUFS;
    g_ai_count++;
    g_ai_frames++;

    if (!g_ai_playing) {
        /* First block: start the DMA atomically (InitDMA then StartDMA once). */
        g_ai_playing = 1;
        start = 1;
    }

    frames = g_ai_frames;
    underruns = g_ai_underruns;
    ai_unlock();

    if (start) {
        AUDIO_RegisterDMACallback(ai_dma_cb);
        AUDIO_InitDMA((u32)(uintptr_t)g_ai_ring[idx], len);
        AUDIO_StartDMA();
    }

    if (frames == 1u || (frames % 600u) == 0u)
        PORT_LOG("audio: frames=%u underruns=%u", frames, underruns);

    return 0;
}
#else
u32 osAiGetStatus(void) {
    return 0;
}

u32 osAiGetLength(void) {
    return 0;
}

s32 osAiSetFrequency(u32 freq) {
    /* libmus derives its frame timing from the returned rate; 0 makes alAudioFrame loop forever. */
    return (s32)freq;
}

s32 osAiSetNextBuffer(void *buf, u32 size) {
    (void)buf;
    (void)size;
    return 0;
}
#endif

/* libultra scheduler client API: the game uses its own NNSched; these exist
 * only so libmus's default audio-scheduler code links. The command queue is
 * a static queue nobody drains, so the audio thread stalls harmlessly if it
 * ever tries to submit a task through it. */
static OSMesg s_scStubCmdMsgs[8];
static OSMesgQueue s_scStubCmdQ;
static int s_scStubCmdQInit = 0;

void osScAddClient(OSSched *s, OSScClient *c, OSMesgQueue *msgQ) {
    if (!s || !c)
        return;
    c->msgQ = msgQ;
    c->next = s->clientList;
    s->clientList = c;
}

void osScRemoveClient(OSSched *s, OSScClient *c) {
    OSScClient **link;
    if (!s || !c)
        return;
    for (link = &s->clientList; *link != NULL; link = &(*link)->next) {
        if (*link == c) {
            *link = c->next;
            return;
        }
    }
}

OSMesgQueue *osScGetCmdQ(OSSched *s) {
    if (!s_scStubCmdQInit) {
        s_scStubCmdQInit = 1;
        osCreateMesgQueue(&s_scStubCmdQ, s_scStubCmdMsgs, 8);
    }
    return &s_scStubCmdQ;
}

/* RSP ucode addresses (N64 ROM symbols; unused on Wii).
 * Types must match the extern declarations in ucode.h / gs2dex.h. */
long long rspbootTextStart[1];
long long rspbootTextEnd[1];
long long aspMainTextStart[1];
long long aspMainDataStart[1];
long long gspF3DEX2_fifoTextStart[1];
long long gspF3DEX2_fifoDataStart[1];
u64 gspS2DEX_fifoTextStart[1];
u64 gspS2DEX_fifoDataStart[1];

/* libal low-pass filter init (libmus calls it; libultra's copy lives in
 * drvrnew.c under the renamed symbol) */
void _init_lpfilter(void *lp) {
    (void)lp;
}

/* VI manager stubs: the game's NNSched calls these but the real VI work is
 * handled by the Wii's own video output (task 05's bt_vi_start). */
OSViMode osViModeTable[32];

void osCreateViManager(OSPri pri) {
    (void)pri;
}

void osViSetMode(OSViMode *mode) {
    (void)mode;
}

void osViBlack(u8 black) {
    (void)black;
}

static void *g_vi_cur_fb;
static void *g_vi_next_fb;

void *osViGetCurrentFramebuffer(void) {
    return g_vi_cur_fb;
}

void *osViGetNextFramebuffer(void) {
    return g_vi_next_fb;
}

extern void port_present(void *fb);
extern void rsp_enqueue(int type, OSTask *tp, void *fb);

void osViSwapBuffer(void *frameptr) {
    g_vi_next_fb = frameptr;
    rsp_enqueue(2, NULL, frameptr);   /* RSP LWP calls port_present(fb) */
    g_vi_cur_fb = frameptr;
}

void osViSetSpecialFeatures(u32 func) {
    (void)func;
}

/* Interrupt mask now owned by port/ultra/os_events.c (task 06b3). */

/* EEPROM: implemented in port/ultra/os_eeprom.c (task 14c). */

/* Cache: PPC/Wii has no MIPS-style D-cache invalidate */
void osInvalDCache(void *buf, s32 size) {
#ifdef TARGET_WII
    port_preempt_check();
#ifdef PORT_LOG_VERBOSE
    PORT_LOG("inv v=%x s=%x", (u32)(uintptr_t)buf, (u32)size);
    PORT_LOG("inv2");
#endif
#endif
    (void)buf;
    (void)size;
}

/* PI DMA: no N64 PI hardware on Wii; ROM data is already in RAM.
 * devAddr is a ROM file offset into the embedded image (port_rom) when it is
 * below port_rom_size; otherwise it is a direct RAM address (DRAM->DRAM copy).
 * Copy into vAddr and post a completion message to mq so the caller's
 * osRecvMesg(..., OS_MESG_BLOCK) unblocks. */
extern u8 port_rom[];
extern u8 port_rom_end[];
/* port_rom_size from objcopy is an ABS symbol (the size value itself), so
 * reading it as a C variable dereferences that value. Compute the real size
 * from the two data symbols' addresses instead. */
static u32 port_rom_size_bytes(void) {
    return (u32)((uintptr_t)port_rom_end - (uintptr_t)port_rom);
}

extern char __text_start[];
extern char __etext[];
static int dma_text_rejected;

static void *dma_resolve_dst(void *vAddr) {
    uint32_t a = (uint32_t)(uintptr_t)vAddr;
    uint32_t p;
    if (a >= 0x80000000u)
        p = a;
    else if (((a >> 24) & 0xF) == 0)
        p = 0x80000000u | (a & 0xFFFFFFu);
    else
        p = a;
    if (p >= (uint32_t)(uintptr_t)__text_start && p < (uint32_t)(uintptr_t)__etext) {
        if (!dma_text_rejected) {
            dma_text_rejected = 1;
            PORT_LOG("pi: dma dst %08x lands in .text, rejected", a);
        }
        return NULL;
    }
    return (void *)(uintptr_t)p;
}

s32 osPiStartDma(OSIoMesg *mb, s32 priority, s32 direction, u32 devAddr,
                  void *vAddr, u32 size, OSMesgQueue *mq) {
#ifdef TARGET_WII
#ifdef PORT_LOG_VERBOSE
    PORT_LOG("pi in d=%x v=%x s=%x rsz=%x", (u32)devAddr, (u32)(uintptr_t)vAddr,
             (u32)size, (u32)port_rom_size_bytes());
#endif
    port_preempt_check();
#ifdef PORT_LOG_VERBOSE
    PORT_LOG("pi pre");
#endif
#endif
    (void)mb;
    (void)priority;
    (void)direction;
    if (vAddr != NULL && size > 0) {
        const u8 *src;
        void *dst = dma_resolve_dst(vAddr);
        if (dst == NULL) {
            if (mq != NULL) osSendMesg(mq, (OSMesg)0, OS_MESG_NOBLOCK);
            return 0;
        }
        if ((u64)devAddr + size <= (u64)port_rom_size_bytes()) {
            src = port_rom + devAddr;
        } else {
            src = (const u8 *)(uintptr_t)devAddr;
        }
        __builtin_memcpy(dst, src, size);
#ifdef PORT_TEXT_CANARY
        port_canary_check(2, 0u, (u32)devAddr, vAddr, size);
#endif
#ifdef TARGET_WII
#ifdef PORT_LOG_VERBOSE
        PORT_LOG("pi cp v=%x s=%x", (u32)(uintptr_t)vAddr, (u32)size);
#endif
#endif
    }
    if (mq != NULL) {
        osSendMesg(mq, (OSMesg)0, OS_MESG_NOBLOCK);
#ifdef TARGET_WII
#ifdef PORT_LOG_VERBOSE
        { extern void port_sched_debug(int*,int*); int c,r; port_sched_debug(&c,&r);
          PORT_LOG("pi sent mq=%x cur=%d ready=%x", (u32)(uintptr_t)mq, c, r); }
#endif
#endif
    }
    return 0;
}

s32 osEPiStartDma(OSPiHandle *p, OSIoMesg *mb, s32 direction) {
    (void)p;
    (void)direction;
    if (mb != NULL && mb->dramAddr != NULL && mb->size > 0 &&
        (u64)mb->devAddr + mb->size <= (u64)port_rom_size_bytes()) {
        void *dst = dma_resolve_dst((void *)mb->dramAddr);
        if (dst != NULL)
            __builtin_memcpy(dst, port_rom + mb->devAddr, mb->size);
#ifdef PORT_TEXT_CANARY
        port_canary_check(2, 0u, (u32)mb->devAddr, (void *)mb->dramAddr, mb->size);
#endif
    } else if (mb != NULL) {
        static int n;
        if (n++ < 8) PORT_LOG("EPI DMA out of range: dev=%08x size=%x dram=%p", (u32)mb->devAddr, (u32)mb->size, (void *)mb->dramAddr);
    }
    if (mb != NULL && mb->hdr.retQueue != NULL) {
        osSendMesg(mb->hdr.retQueue, (OSMesg)0, OS_MESG_NOBLOCK);
    }
    return 0;
}

/* Cart ROM init: no N64 cart on Wii */
OSPiHandle *osCartRomInit(void) {
    return NULL;
}

/* GU matrix helpers (gu.h): stubs leave matrices unchanged */
/* guTranslate: real libultra version from lib/ultralib/src/gu (renamed n64_*) */

/* guRotateRPYF: real libultra version from lib/ultralib/src/gu (renamed n64_*) */

/* guMtxF2L: real libultra version from lib/ultralib/src/gu (renamed n64_*) */

/* ctype lookup table (include/libc/ctype.h): zero-init means "no class bits
 * set", so isalpha/isdigit/etc. all return 0; the game only uses these for
 * menu text classification, which is non-critical for the GATE. */
unsigned char __ctype_map[0x100];

/* sine lookup (gu.h): returns 0; only used for decorative animation */
/* sins: real libultra version from lib/ultralib/src/gu (renamed n64_*) */

/* PI manager: no N64 PI hardware on Wii */
void osCreatePiManager(OSPri pri, OSMesgQueue *mq, OSMesg *msg, s32 count) {
    (void)pri;
    (void)mq;
    (void)msg;
    (void)count;
}

/* osInitialize() is a macro that calls __osInitialize_common() (os_host.h) */
void __osInitialize_common(void) {
}

/* VI Y-scale: no N64 VI hardware on Wii */
void osViSetYScale(f32 scale) {
    (void)scale;
}

/* Controller: the real libultra controller API (osContSetCh/Init/StartQuery/
 * GetQuery/StartReadData/GetReadData) now lives in port/ultra/os_cont.c
 * (task 11), which maps Wii WPAD/PAD input into the N64 controller model. */
