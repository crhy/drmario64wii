/* Dr. Mario 64 -> Wii native port: boot entry (task 07, clean design).
 * Initializes video/WPAD/SD log, then hands over to the libultra boot
 * (bootproc in src/boot/boot_main.c), which starts the idle thread, the
 * PI manager, the main thread, and the game's scheduler. The DOL main
 * thread must never return or the process would tear down the game
 * threads.
 *
 * Clean frame cycle (no DOL-thread pump): main() flushes ONE bootstrap
 * frame so the VI starts advancing and the post-retrace callback fires.
 * From then on the game's scheduler drives presentation: the callback
 * (IRQ context) only posts the VI semaphore; a priority-127 vi thread
 * feeds retrace messages to the scheduler; the scheduler runs gfx tasks
 * and calls osViSwapBuffer, which calls port_present() to clear the EFB
 * to a per-frame colour, copy it into the next XFB, and VIDEO_Flush the
 * next frame. The main thread blocks forever on a never-posted semaphore
 * so it cannot starve the game threads. */

#include <gccore.h>
#include <ogc/audio.h>
#include <ogc/lwp.h>
#include <ogc/conf.h>
#include "port_log.h"
#include "heap_check.h"
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef WII_SELFTEST
void port_selftest(void);
#endif

extern void bootproc(void);
extern void port_sched_boot(void);
extern void port_rsp_boot(void);
extern void gfx_frame_start(void);

#define FIFO_SIZE (256 * 1024)

static GXRModeObj *g_screenMode;
static void *g_frameBuffer;
static void *g_frameBuffer2;

/* Pick the video mode once, shared by main() and port_gx_setup(). Prefers
 * 480p progressive when the SYSCONF asks for it and a component cable is
 * present (PAL60 -> TVEurgb60Hz480Prog, else NTSC -> TVNtsc480Prog); falls
 * back to the libogc preferred mode otherwise. When the SYSCONF asks for a
 * 16:9 TV, apply the standard libogc widescreen trick (squeeze the 640-wide
 * XFB into a 678-pixel VI window so a 16:9 set shows the 4:3 game with
 * correct proportions, no stretched sprites); the EFB/XFB stay 640 wide. */
static GXRModeObj *port_pick_mode(void)
{
    static int logged = 0;
    GXRModeObj *mode;
    const char *name;
    int wide = 0;

    if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable()) {
        if (CONF_GetEuRGB60() > 0) {
            mode = &TVEurgb60Hz480Prog;
            name = "TVEurgb60Hz480Prog";
        } else {
            mode = &TVNtsc480Prog;
            name = "TVNtsc480Prog";
        }
    } else {
        mode = VIDEO_GetPreferredMode(NULL);
        name = "preferred";
    }

    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
        mode->viWidth = 678;
        mode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678) / 2;
        wide = 1;
    }

    if (!logged) {
        logged = 1;
        PORT_LOG("video: mode=%s w=%d h=%d wide=%d", name, mode->viWidth, mode->viHeight, wide);
    }
    return mode;
}

static void port_gx_setup(void)
{
    GXColor backgroundColor = {0, 0, 0, 255};

    g_screenMode = port_pick_mode();
    /* XFBs are allocated once in main() before this call; reuse them here. */

    /* GX FIFO in MEM2: game code only ever touches MEM1, so it cannot overwrite GP commands. */
    { u8 *lo = (u8 *)(((u32)SYS_GetArena2Lo() + 31) & ~31u); SYS_SetArena2Lo(lo + FIFO_SIZE);
      void *fifo = MEM_K0_TO_K1(lo); memset(fifo, 0, FIFO_SIZE); GX_Init(fifo, FIFO_SIZE); }
    GX_SetCopyClear(backgroundColor, 0x00ffffff);
    GX_SetViewport(0, 0, g_screenMode->fbWidth, g_screenMode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)g_screenMode->xfbHeight / (f32)g_screenMode->efbHeight);
    GX_SetScissor(0, 0, g_screenMode->fbWidth, g_screenMode->efbHeight);
    GX_SetDispCopySrc(0, 0, g_screenMode->fbWidth, g_screenMode->efbHeight);
    GX_SetDispCopyDst(g_screenMode->fbWidth, g_screenMode->xfbHeight);
    GX_SetCopyFilter(g_screenMode->aa, g_screenMode->sample_pattern,
                     GX_TRUE, g_screenMode->vfilter);
    GX_SetFieldMode(g_screenMode->field_rendering,
                    ((g_screenMode->viHeight == 2 * g_screenMode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(g_frameBuffer, GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    /* Set the per-frame vertex/TEV/projection state up front so the FIRST
     * frame's gfx tasks (which run before port_present's gfx_frame_start)
     * draw with a valid vtx desc instead of asserting in the GP. */
    gfx_frame_start();
}

/* Presentation, called by the game scheduler via osViSwapBuffer. Clears the
 * EFB to a colour that changes every frame, copies it into the given XFB,
 * waits for the GP to finish, then hands that XFB to the VI as the next
 * frame and flushes. No VIDEO_WaitVSync here: this runs on a game thread and
 * blocking on vsync would stall the scheduler. */
void port_present(void *game_fb)
{
    /* game_fb is the game's 320x240 N64 colour buffer: never write the Wii XFB into it.
     * Alternate between the port's own two XFBs instead. */
    static unsigned s_frame = 0;
    void *fb = (s_frame & 1) ? g_frameBuffer2 : g_frameBuffer;
    (void)game_fb;
    s_frame++;
#ifdef PORT_LOG_VERBOSE
    { extern void port_events_debug(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*); unsigned a,b,c,d,e; port_events_debug(&a,&b,&c,&d,&e); PORT_LOG("present f=%x vi_cb=%u posts=%u pump=%u ring=%u/%u", s_frame,a,b,c,d,e); }
#endif

    /* Geometry state for the NEXT frame's gfx tasks: ortho projection, vertex
     * format, TEV pass-through, Z mode.  GX FIFO state persists, so setting it
     * here (after this frame's gfx tasks already ran) prepares the next frame. */
    gfx_frame_start();


    /* GX_SetCopyClear queues the EFB clear into the FIFO at the call site, so
     * it must NOT sit between this frame's drawing and the EFB->XFB copy (that
     * wipes the drawing -> all-black).  Copy first (EFB still holds this frame's
     * geometry), then queue the clear for the NEXT frame's gfx tasks. */
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(fb, GX_TRUE);
    GXColor backgroundColor = {0, 0, 0, 255};
    GX_SetCopyClear(backgroundColor, 0x00ffffff);
    GX_DrawDone();
    { extern void gfx_tex_frame_end(void); gfx_tex_frame_end(); }

    VIDEO_SetNextFramebuffer(fb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
}

int main(void) {
    sem_t g_main_sem;

    VIDEO_Init();
    port_log_init();
    port_heap_guard_init();
#ifdef PORT_TEXT_CANARY
    { extern void port_canary_boot(void); port_canary_boot(); }
#endif

    g_screenMode = port_pick_mode();
    g_frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_screenMode));
    g_frameBuffer2 = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_screenMode));
    /* Same order as the libogc examples: configure the VI and give it an XFB before GX setup. */
    VIDEO_Configure(g_screenMode);
    VIDEO_SetNextFramebuffer(g_frameBuffer);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (g_screenMode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    port_gx_setup();
    { extern void port_crash_install(void); port_crash_install(); }
#ifdef PORT_SENTINEL
    { extern void port_sentinel_start(void); port_sentinel_start(); }
#endif
    WPAD_Init();
    AUDIO_Init(NULL);              /* Wii audio-interface DMA for osAiSetNextBuffer (os_stub.c) */

    VIDEO_SetBlack(false);
    VIDEO_Flush();

    /* Start the event pump + timer helper LWPs before bootproc so external
     * events (VI retraces, timers) can be delivered to the game scheduler. */
    port_sched_boot();
    port_rsp_boot();

#ifdef WII_SELFTEST
    PORT_LOG("main:pre-selftest");
    port_selftest();
    PORT_LOG("main:selftest-returned");
#else
    PORT_LOG("main:pre-bootproc");
    bootproc();
    PORT_LOG("main:bootproc-returned");
#endif

    /* Block forever on a semaphore that is never posted: the DOL main thread
     * must not return (that would tear down the game threads) and must not
     * spin (that would starve them). The VI is now driven entirely by the
     * game's scheduler via port_present(). */
    LWP_SemInit(&g_main_sem, 0, 0);
    LWP_SemWait(g_main_sem);
    return 0;
}
