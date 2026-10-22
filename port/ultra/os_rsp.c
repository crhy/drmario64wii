/* Dr. Mario 64 -> Wii native port: the "RSP" thread (task 11c).
 *
 * On a real N64 the RSP/RDP do graphics work in parallel with the CPU, so the
 * game threads keep running while a display list is rasterised. Here the game
 * scheduler gives exactly ONE thread the CPU at a time, so running gfx_run_task
 * (the whole display list) and port_present (GX_DrawDone) on a game thread
 * holds the scheduler token for a whole frame and starves the other threads.
 *
 * Fix: an "RSP" LWP at libogc priority 80 (game threads 64, timer 90, pump 100)
 * owns a job ring of 16 entries {type, OSTask*, void *fb} guarded by an LWP
 * mutex plus a counting semaphore it waits on. osSpTaskStartGo and osViSwapBuffer
 * (game threads) only ENQUEUE a job and return immediately; the RSP LWP runs the
 * work and signals completion OUTSIDE the scheduler via port_os_event_post (the
 * event-pump path, never a blocking osRecvMesg/osSendMesg). The RSP LWP is the
 * ONLY thread that writes the GX FIFO, so GX_SetCurrentGXThread() is set once at
 * its top and removed from gfx_run_task and port_present. Order is preserved
 * because it is one FIFO ring. */

#include <PR/ultratypes.h>
#include <PR/sptask.h>
#include <PR/mbi.h>
#include <PR/os_message.h>
#include "port_log.h"
#include "../audio/naudio.h"
#include "port_audio.h"
#ifdef TARGET_WII
extern void port_guard_canary(const char *name, void *stack_low);
extern void port_guard_range(const char *name, const void *low, unsigned size);
#endif
#include <PR/libaudio.h>

/* libogc API, forward-declared with plain C types (handles are ints). */
typedef int lwp_t;
typedef int sem_t;
typedef int mutex_t;
int LWP_MutexInit(mutex_t *mutex, int initial_unlocked);
int LWP_MutexLock(mutex_t mutex);
int LWP_MutexUnlock(mutex_t mutex);
int LWP_SemInit(sem_t *sem, int value, int max);
int LWP_SemWait(sem_t sem);
int LWP_SemPost(sem_t sem);
int LWP_SemGetValue(sem_t sem, unsigned int *value);
int LWP_CreateThread(lwp_t *lwp, void *(*aFunction)(void *), void *arg,
                     void *user_context, int stack_size, int prio);
unsigned int GX_SetCurrentGXThread(void);

/* the renderer + presentation, now driven from this LWP */
extern void gfx_run_task(OSTask *tp);
extern void port_present(void *fb);
/* completion signal that never parks (event-pump path) */
extern void port_os_event_post(int event);

/* text canary (task 15d) */
extern void port_canary_boot(void);
extern void port_canary_check(int kind, unsigned n, uint32_t a, void *p, uint32_t s);

/* Audio tasks run SYNCHRONOUSLY on the calling thread (task 15c2): naudio is plain C and never
 * touches GX, so nnScExecuteAudio calls this instead of the N64 yield + osSpTaskStart + rspMQ wait. */
extern char __text_start[];
extern char __etext[];
static int rsp_text_rejected;

static const uint64_t *rsp_resolve_data(OSTask *t) {
    uint32_t a = (uint32_t)(uintptr_t)t->t.data_ptr;
    uint32_t p;
    if (a >= 0x80000000u)
        p = a;
    else if (((a >> 24) & 0xF) == 0)
        p = 0x80000000u | (a & 0xFFFFFFu);
    else
        p = a;
    if (p >= (uint32_t)(uintptr_t)__text_start && p < (uint32_t)(uintptr_t)__etext) {
        if (!rsp_text_rejected) {
            rsp_text_rejected = 1;
            PORT_LOG("rsp: data_ptr %08x lands in .text, rejected", a);
        }
        return NULL;
    }
    return (const uint64_t *)(uintptr_t)p;
}

void port_audio_task_run(OSTask *t)
{
    static int s_first;
    naudio_set_dram_base(0);
    if (!s_first) PORT_LOG("naudio: first task n=%d ptr=%p", (int)(t->t.data_size / 8u), (void *)t->t.data_ptr);
#ifdef PORT_SCHED_DUMP2
    { extern ALGlobals *alGlobals; void *h0 = alGlobals ? (void *)alGlobals->drvr.head : 0;
      const uint64_t *mc = rsp_resolve_data(t);
      if (mc) naudio_run(mc, (int)(t->t.data_size / 8u));
      void *h1 = alGlobals ? (void *)alGlobals->drvr.head : 0;
      static int s_h; if (h0 != h1 && s_h < 4) { s_h++; PORT_LOG("naudio: drvr.head %p -> %p (&head=%p)", h0, h1, (void *)&alGlobals->drvr.head); } }
#else
    { const uint64_t *mc = rsp_resolve_data(t); if (mc) naudio_run(mc, (int)(t->t.data_size / 8u)); }
#endif
    if (!s_first) { s_first = 1; PORT_LOG("naudio: first task done"); }
}

/* raw wall-clock TBL ticks (60.75e6/s on Wii), for the fps readout */
static inline unsigned long long rsp_gettime(void)
{
    unsigned int hi, lo, hi2;
    do {
        __asm__ __volatile__("mftbu %0" : "=r"(hi));
        __asm__ __volatile__("mftb %0" : "=r"(lo));
        __asm__ __volatile__("mftbu %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((unsigned long long)hi << 32) | (unsigned int)lo;
}
#define RSP_TICKS_PER_SEC 60750000ULL

/* ---------- job ring ---------- */
#define RSP_RING 16
typedef struct {
    int     type;     /* 0=GFX 1=AUDIO 2=PRESENT */
    OSTask *task;
    void   *fb;
} RspJob;

static RspJob g_ring[RSP_RING];
static volatile unsigned g_head;   /* producer */
static volatile unsigned g_tail;   /* consumer (RSP LWP) */
static mutex_t g_ring_mutex;
static sem_t   g_job_sem;          /* jobs ready (RSP waits on this) */
static sem_t   g_slot_sem;         /* free slots (producer waits when full) */
static int     g_ring_ready;
static int     g_full_logged;

/* ---------- counters ---------- */
static unsigned g_presents;
static unsigned g_gfx;
static unsigned g_audio;
static unsigned long long g_start_ticks;

/* ---------- producer side (called from a game thread) ---------- */
void rsp_enqueue(int type, OSTask *tp, void *fb)
{
    if (!g_ring_ready) {
        LWP_MutexInit(&g_ring_mutex, 0);
        LWP_SemInit(&g_job_sem, 0, RSP_RING);
        LWP_SemInit(&g_slot_sem, RSP_RING, RSP_RING);
        g_ring_ready = 1;
    }
    /* like the N64 CPU waiting on a busy RSP: block if the ring is full */
    {
        unsigned v = 0;
        if (LWP_SemGetValue(g_slot_sem, &v) == 0 && v == 0u && !g_full_logged) {
            g_full_logged = 1;
            PORT_LOG("rsp: ring full (first)");
        }
    }
    LWP_SemWait(g_slot_sem);          /* reserve a slot; parks if none free */
    LWP_MutexLock(g_ring_mutex);
    g_ring[g_head].type = type;
    g_ring[g_head].task = tp;
    g_ring[g_head].fb   = fb;
    g_head = (g_head + 1u) % RSP_RING;
    LWP_MutexUnlock(g_ring_mutex);
    LWP_SemPost(g_job_sem);           /* one more job ready */
}

/* ---------- the RSP LWP body ---------- */
static void *rsp_body(void *arg)
{
    (void)arg;
    GX_SetCurrentGXThread();          /* this LWP owns the GX FIFO */
    g_start_ticks = rsp_gettime();
    for (;;) {
        LWP_SemWait(g_job_sem);
        RspJob j;
        LWP_MutexLock(g_ring_mutex);
        j = g_ring[g_tail];
        g_tail = (g_tail + 1u) % RSP_RING;
        LWP_MutexUnlock(g_ring_mutex);
        LWP_SemPost(g_slot_sem);      /* a slot became free */

        switch (j.type) {
            case 0: /* GFX */
                gfx_run_task(j.task);
                port_canary_check(0, g_gfx + 1u, 0, NULL, 0);
                port_os_event_post(OS_EVENT_SP);
                port_os_event_post(OS_EVENT_DP);
                g_gfx++;
                break;
            case 1: /* AUDIO via osSpTaskStart: not expected (nnsched runs audio synchronously) */
                { static int s_v; if (!s_v) { s_v = 1; PORT_LOG("audio task via osSpTaskStart"); } }
                if (j.task) port_audio_task_run(j.task);
                port_canary_check(1, ++g_audio, 0, NULL, 0);
                port_os_event_post(OS_EVENT_SP);
                break;
            case 2: /* PRESENT */
                port_present(j.fb);
                g_presents++;
                break;
            default:
                break;
        }

        if (g_presents != 0u && (g_presents % 600u) == 0u) {
            unsigned long long now = rsp_gettime();
            unsigned long long el = now - g_start_ticks;
            unsigned long long num = (unsigned long long)g_presents * RSP_TICKS_PER_SEC;
            unsigned long long fps100 = el ? (num * 100ULL) / el : 0ULL;
            PORT_LOG("rsp: presents=%u gfx=%u fps=%u.%u",
                     g_presents, g_gfx,
                     (unsigned)(fps100 / 100u), (unsigned)(fps100 % 100u));
        }
    }
    return NULL;
}

/* ---------- boot: create the RSP LWP (priority 80, 64 KB static stack) ---------- */
#define RSP_STACK (64 * 1024)
static unsigned char g_rsp_stack[RSP_STACK + 32] __attribute__((aligned(32)));
static lwp_t g_rsp_lwp;
static int   g_rsp_created;

void port_rsp_boot(void)
{
    if (g_rsp_created) return;
    g_rsp_created = 1;
    if (!g_ring_ready) {
        LWP_MutexInit(&g_ring_mutex, 0);
        LWP_SemInit(&g_job_sem, 0, RSP_RING);
        LWP_SemInit(&g_slot_sem, RSP_RING, RSP_RING);
        g_ring_ready = 1;
    }
    LWP_CreateThread(&g_rsp_lwp, rsp_body, NULL,
                     (void *)(((unsigned long)&g_rsp_stack[0] + 31u)
                              & ~(unsigned long)31u),
                     RSP_STACK, 80);
#ifdef TARGET_WII
    port_guard_canary("rsp", g_rsp_stack);
    port_guard_range("rsp", g_rsp_stack, sizeof(g_rsp_stack));
#endif
}
