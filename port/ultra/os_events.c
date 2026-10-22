/* Dr. Mario 64 -> Wii native port: external events, VI, timers, time, boot
 * wiring (task 06b3 part).
 *
 * IRQ/helper code (VI retrace callback, timer helper LWP) must never call
 * osSendMesg directly. port_post_event pushes {mq,msg} into a ring under a real
 * IRQ guard (port_irq_disable/port_irq_restore from port/wii/irqlock.c, wrapping
 * libogc's IRQ_Disable/IRQ_Restore) because it has TWO producers: the VI IRQ
 * callback and the timer LWP. Ring-full drops are counted and logged.
 * LWP (priority 100, above the game LWPs at 64) waits on event_sem, drains
 * the ring and delivers each event by calling the real osSendMesg NOBLOCK,
 * which already does the whole delivery under sched_lock: push the message,
 * wake a recv waiter (make it READY), and if the CPU is idle start the
 * highest READY thread. A RUNNING lower-priority game thread is never parked
 * from here; it yields at its next OS call via port_preempt_check.
 *
 * Includes ONLY ultra headers; libogc functions used here are forward-declared
 * with plain C types so libogc's u32/s32 typedefs never clash with ultratypes.
 */

static unsigned g_dbg_cb, g_dbg_post, g_dbg_pump;
#include <PR/ultratypes.h>
#include <PR/os_thread.h>
#include <PR/os_message.h>
#include <PR/os_time.h>
#include <PR/os_vi.h>
#include <PR/os_exception.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "os_sched.h"
#ifdef TARGET_WII
#include "port_log.h"
extern void port_guard_canary(const char *name, void *stack_low);
extern void port_guard_range(const char *name, const void *low, unsigned size);
extern void port_guard_vi(void);
#endif

/* libogc API, forward-declared (handles are plain ints in libogc) */
typedef int lwp_t;
typedef int sem_t;
int LWP_SemInit(sem_t *sem, int value, int max);
int LWP_SemWait(sem_t sem);
int LWP_SemPost(sem_t sem);
int LWP_SemGetValue(sem_t sem, unsigned int *value);
int LWP_CreateThread(lwp_t *lwp, void *(*aFunction)(void *), void *arg,
                     void *user_context, int stack_size, int prio);
/* gettime(): libogc's is header-inline (ogc/lwp_watchdog.h -> PPCGetTickCount),
 * so a forward declaration would not link. Read the PowerPC time base directly
 * (TBL/TBU), same as PPCGetTickCount: 64-bit ticks at PPC_TIMER_CLOCK =
 * PPC_BUS_CLOCK/4 = 60.75e6/s on Wii. */
static inline unsigned long long gettime(void)
{
    unsigned int hi, lo, hi2;
    do {
        __asm__ __volatile__("mftbu %0" : "=r"(hi));
        __asm__ __volatile__("mftb %0" : "=r"(lo));
        __asm__ __volatile__("mftbu %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((unsigned long long)hi << 32) | (unsigned int)lo;
}
void VIDEO_SetPostRetraceCallback(void (*callback)(uint32_t retraceCnt));

/* Controller scan (task 11, port/ultra/os_cont.c): WPAD/PAD polling must run
 * from a normal LWP, never the IRQ VI callback. The pump LWP calls it once per
 * retrace (VI) event drained from the ring, BEFORE delivering that event, so the
 * cached OSContPad/OSContStatus state is fresh when the game reads it this frame.
 * It is NOT called once per pump wake: the pump wakes once per event (retrace +
 * timer + SP/DP), which would tick the frame counter faster than the VI rate. */
void port_cont_scan(void);

/* ---------- event ring ---------- */
#define EVENT_RING_SIZE 256
typedef struct {
    OSMesgQueue *mq;
    OSMesg       msg;
} EventEntry;

static EventEntry g_event_ring[EVENT_RING_SIZE];
static volatile unsigned g_ring_head;   /* producer writes ring[head], head++ */
static volatile unsigned g_ring_tail;   /* pump reads ring[tail], tail++ */
static sem_t g_event_sem;
static int   g_event_sem_ready;

#ifdef TARGET_WII
unsigned g_deliver_ok;
unsigned g_deliver_drop;
unsigned g_deliver_pend;   /* task 33: events deferred because the queue was full */
void *g_defer_mq;          /* last queue a defer went to */
void *g_defer_msg;         /* last deferred message (event type) */
#endif

/* ---------- VI state ---------- */
static OSMesgQueue *g_vi_mq;
static OSMesg       g_vi_msg;
static uint32_t     g_vi_retrace;
static uint32_t     g_vi_count;

/* ---------- timer state (we keep our own table; game OSTimer is the key) -- */
typedef struct {
    OSTimer  *t;
    uint64_t  expiry;      /* counter ticks */
    uint64_t  interval;    /* 0 = one-shot */
} TimerEntry;
#define MAX_TIMERS 16
static TimerEntry g_timers[MAX_TIMERS];
static sem_t g_timer_sem;   /* woken by osSetTimer/osStopTimer */
static int   g_timer_sem_ready;

/* ---------- pump / timer stacks ---------- */
#define PUMP_STACK (64 * 1024)
static unsigned char g_pump_stack[PUMP_STACK + 32] __attribute__((aligned(32)));
static unsigned char g_timer_stack[PUMP_STACK + 32] __attribute__((aligned(32)));
static lwp_t g_pump_lwp;
static lwp_t g_timer_lwp;
static int   g_pump_created;
static int   g_timer_created;

/* ---------- counter ticks: 46.875 MHz ---------- */
#define OS_CPU_COUNTER 46875000ULL
/* gettime() ticks are TBL ticks at PPC_TIMER_CLOCK = PPC_BUS_CLOCK/4 =
 * 60.75e6/s on Wii. The 46.875 MHz counter is the exact ratio
 * 46875000/60750000 = 125/162, so one multiply + one divide (no double
 * truncation): ticks46 = ticks * 125 / 162. */
static uint64_t counter_ticks_now(void)
{
    return (gettime() * 125ULL) / 162ULL;
}

/* Raw wall-clock TBL ticks (60.75e6/s on Wii). Exposed for the T7 accuracy
 * self-test so it can compare osGetTime's 46.875e6 counter against real time. */
unsigned long long port_wall_ticks_now(void)
{
    return gettime();
}

/* ---------- port_post_event (IRQ-safe push + semaphore post) ---------- */
extern unsigned port_irq_disable(void);
extern void port_irq_restore(unsigned level);
void port_post_event(void *mq, void *msg)
{
    unsigned h;
    int dropped = 0;
    g_dbg_post++;
    unsigned sr = port_irq_disable();
    h = g_ring_head;
    if (((h + 1u) % EVENT_RING_SIZE) != g_ring_tail) {
        g_event_ring[h].mq  = (OSMesgQueue *)mq;
        g_event_ring[h].msg = *(OSMesg *)msg;   /* msg points at the OSMesg value */
        g_ring_head = (h + 1u) % EVENT_RING_SIZE;
    } else {
        dropped = 1;
    }
    port_irq_restore(sr);
    if (dropped) {
        static unsigned g_ring_drops;
        g_ring_drops++;
        if (g_ring_drops == 1u || (g_ring_drops % 64u) == 0u)
            PORT_LOG("event ring full drops=%u", g_ring_drops);
    }
    if (g_event_sem_ready)
        LWP_SemPost(g_event_sem);
}

/* ---------- event pump LWP (priority 100) ----------
 * Delivers each queued event by calling the real osSendMesg NOBLOCK, which
 * takes/releases sched_lock itself and already performs wake + idle-start. */
static void *event_pump_body(void *arg)
{
    (void)arg;
    for (;;) {
        LWP_SemWait(g_event_sem); g_dbg_pump++;
#ifdef PORT_SCHED_DUMP
        { extern void port_sched_dump(unsigned); if ((g_dbg_pump % 1200u) == 0u) port_sched_dump(g_dbg_pump); }
#endif
#ifdef PORT_LOG_VERBOSE
        if (g_dbg_pump <= 5u || (g_dbg_pump % 60u) == 0u)
            { extern void port_sched_debug(int*,int*); int c, r; port_sched_debug(&c, &r); PORT_LOG("pump cb=%x post=%x pump=%x cur=%d ready=%x", g_dbg_cb, g_dbg_post, g_dbg_pump, c, r); }
#endif
#ifdef TARGET_WII
        if ((g_dbg_pump % 60u) == 0u) {
            extern unsigned g_deliver_pend;
            extern void *g_defer_mq, *g_defer_msg;
            static unsigned last_pend;
            PORT_LOG("ev/sec: post=%u ok=%u drop=%u", g_dbg_post, g_deliver_ok, g_deliver_drop);
            if (g_deliver_pend != last_pend) {
                PORT_LOG("defer/sec: mq=%p msg=%p n=%u", g_defer_mq, g_defer_msg, g_deliver_pend);
                last_pend = g_deliver_pend;
            }
        }
#endif
        while (g_ring_tail != g_ring_head) {
            EventEntry e = g_event_ring[g_ring_tail];
            g_ring_tail = (g_ring_tail + 1u) % EVENT_RING_SIZE;
            if (e.mq) {
                if (e.mq == g_vi_mq) {
                    port_cont_scan();
#ifdef TARGET_WII
                    port_guard_vi();
#endif
                }
                port_deliver_event(e.mq, e.msg);
            }
        }
        { extern int port_sched_kick_idle(void); static unsigned kicks;
          if (port_sched_kick_idle() && ((++kicks & (kicks - 1)) == 0)) PORT_LOG("sched: idle kick %u", kicks); }
    }
    return NULL;
}

/* ---------- VI: callback counts, posts every retraceCount ---------- */

static void vi_retrace_callback(uint32_t retraceCnt)
{
    (void)retraceCnt; g_dbg_cb++;
#ifdef PORT_SENTINEL
    { extern void port_sentinel_sample(void); port_sentinel_sample(); }
#endif
    if (g_vi_mq == NULL || g_vi_retrace == 0) return;
    g_vi_count++;
    if (g_vi_count >= g_vi_retrace) {
        g_vi_count = 0;
        port_post_event(g_vi_mq, (void *)&g_vi_msg);
    }
}

void osViSetEvent(OSMesgQueue *mq, OSMesg msg, u32 retraceCount)
{
    g_vi_mq = mq;
    g_vi_msg = msg;
    g_vi_retrace = (uint32_t)retraceCount;
    g_vi_count = 0;
    PORT_LOG("vi set rc=%x", (u32)retraceCount);
    VIDEO_SetPostRetraceCallback(vi_retrace_callback);
}

/* ---------- timer helper LWP (priority 90) ---------- */
static void *timer_body(void *arg)
{
    (void)arg;
    for (;;) {
        uint64_t now, best = 0;
        int i, has = 0;
        now = counter_ticks_now();

        /* Scan the table under the IRQ guard: the timer LWP must not read a
         * half-written entry while a game thread (osSetTimer/osStopTimer)
         * mutates g_timers. */
        {
            unsigned sr = port_irq_disable();
            for (i = 0; i < MAX_TIMERS; i++) {
                if (!g_timers[i].t) continue;
                if (!has || g_timers[i].expiry < best) {
                    best = g_timers[i].expiry;
                    has = 1;
                }
            }
            port_irq_restore(sr);
        }

        if (has && best <= now) {
            /* Collect due timers under the guard, update expiry / clear one-shots,
             * then post the events AFTER re-enabling IRQs (never port_post_event
             * while IRQs are disabled). */
            EventEntry due[MAX_TIMERS];
            unsigned ndue = 0;
            unsigned sr = port_irq_disable();
            for (i = 0; i < MAX_TIMERS; i++) {
                if (!g_timers[i].t) continue;
                if (g_timers[i].expiry <= now) {
                    due[ndue].mq  = g_timers[i].t->mq;
                    due[ndue].msg = g_timers[i].t->msg;
                    ndue++;
                    if (g_timers[i].interval != 0) {
                        /* Periodic: advance by the interval so the schedule does
                         * not drift; if that is still in the past, jump to now +
                         * interval to avoid a runaway catch-up burst. */
                        uint64_t ne = g_timers[i].expiry + g_timers[i].interval;
                        g_timers[i].expiry = (ne <= now) ? (now + g_timers[i].interval) : ne;
                    } else {
                        g_timers[i].t = NULL; /* one-shot done */
                    }
                }
            }
            port_irq_restore(sr);
            for (i = 0; i < ndue; i++)
                port_post_event(due[i].mq, (void *)&due[i].msg);
            continue; /* re-scan immediately */
        }

        if (!has) {
            /* No timer armed: block on the semaphore instead of busy-waiting
             * every 10 ms. osSetTimer/osStopTimer post it to wake us. */
            LWP_SemWait(g_timer_sem);
            continue;
        }

        {
            /* Timers armed but none due: sleep until the earliest expiry, in
             * <= 10 ms steps. */
            uint64_t wait_ticks = 10 * (OS_CPU_COUNTER / 100);
            uint64_t d = best - now;
            if (d < wait_ticks) wait_ticks = d;
            /* ticks -> microseconds */
            unsigned usec = (unsigned)((wait_ticks * 1000000ULL + OS_CPU_COUNTER - 1)
                                       / OS_CPU_COUNTER);
            if (usec < 100u) usec = 100u;
            usleep(usec);
        }
        {
            /* Non-blocking drain of any osSetTimer/osStopTimer wakeups that
             * arrived during the sleep. A blocking LWP_SemWait here would stall
             * the helper until the NEXT timer_note_change post, so periodic
             * timers would never re-scan and fire. Read the count and consume
             * credits only while count > 0, then loop back to re-scan. */
            unsigned int val;
            while (LWP_SemGetValue(g_timer_sem, &val) == 0 && val > 0u)
                LWP_SemWait(g_timer_sem);
        }
    }
    return NULL;
}

static void timer_ensure_running(void)
{
    if (!g_event_sem_ready) {
        LWP_SemInit(&g_event_sem, 0, EVENT_RING_SIZE);
        g_event_sem_ready = 1;
    }
    if (!g_timer_sem_ready) {
        LWP_SemInit(&g_timer_sem, 0, 1000);
        g_timer_sem_ready = 1;
    }
    if (!g_timer_created) {
        g_timer_created = 1;
        LWP_CreateThread(&g_timer_lwp, timer_body, NULL,
                         (void *)(((unsigned long)&g_timer_stack[0] + 31u)
                                   & ~(unsigned long)31u),
                         PUMP_STACK, 90);
#ifdef TARGET_WII
        port_guard_canary("timer", g_timer_stack);
        port_guard_range("timer", g_timer_stack, sizeof(g_timer_stack));
#endif
    }
}

static void timer_note_change(void)
{
    if (g_timer_sem_ready) LWP_SemPost(g_timer_sem);
}

/* ---------- osSetTimer / osStopTimer ---------- */
int osSetTimer(OSTimer *t, OSTime delay, OSTime interval, OSMesgQueue *mq, OSMesg msg)
{
    int i, found = 0;
    uint64_t now = counter_ticks_now();
    timer_ensure_running();
    {
        unsigned sr = port_irq_disable();
        for (i = 0; i < MAX_TIMERS; i++) {
            if (!g_timers[i].t) {
                g_timers[i].t = t;
                g_timers[i].expiry = now + (uint64_t)delay;
                g_timers[i].interval = (uint64_t)interval;
                t->value = (OSTime)g_timers[i].expiry;
                t->interval = interval;
                t->mq = mq;
                t->msg = msg;
                found = 1;
                break;
            }
        }
        port_irq_restore(sr);
    }
    if (!found)
        PORT_LOG("timer table full");
    timer_note_change();
    return 0;
}

int osStopTimer(OSTimer *t)
{
    int i;
    unsigned sr = port_irq_disable();
    for (i = 0; i < MAX_TIMERS; i++)
        if (g_timers[i].t == t) g_timers[i].t = NULL;
    port_irq_restore(sr);
    timer_note_change();
    return 0;
}

/* ---------- time ---------- */
/* osGetTime counts from the last osSetTime, like libultra: the game calls osSetTime(0) on every retrace
 * (nnScEventHandler) and paces frames with OS_CYCLES_TO_USEC(osGetTime()) in dm_game_graphic_onDoneSawp. Ignoring
 * osSetTime made that a time-since-boot value, the s32 subtraction wrapped and the game could sleep ~30 minutes
 * ("frozen picture" at random moments). Timers and osGetCount keep using the raw counter. */
static volatile uint64_t g_time_base;   /* counter value at the last osSetTime */
static volatile uint64_t g_time_set;    /* value passed to the last osSetTime */

OSTime osGetTime(void)
{
    port_preempt_check();
    return (OSTime)(counter_ticks_now() - g_time_base + g_time_set);
}

void osSetTime(OSTime time)
{
    unsigned sr = port_irq_disable();
    g_time_base = counter_ticks_now();
    g_time_set = (uint64_t)time;
    port_irq_restore(sr);
}

u32 osGetCount(void)
{
    port_preempt_check();
    return (u32)counter_ticks_now();
}

/* ---------- IRQ mask: no real IRQ disabling on Wii ---------- */
OSIntMask osSetIntMask(OSIntMask mask)
{
    /* Pure mask save/restore: MUST NOT switch threads. Game code (nnScAddClient)
     * wraps short critical sections in osSetIntMask(OS_IM_NONE)..osSetIntMask(old). */
    return mask;
}

void __osDisableInt(void) { }
void __osRestoreInt(void) { }

/* ---------- boot: start pump + timer helper before bootproc ---------- */
void port_sched_boot(void)
{
    if (!g_event_sem_ready) {
        LWP_SemInit(&g_event_sem, 0, EVENT_RING_SIZE);
        g_event_sem_ready = 1;
    }
    if (!g_pump_created) {
        g_pump_created = 1;
        LWP_CreateThread(&g_pump_lwp, event_pump_body, NULL,
                         (void *)(((unsigned long)&g_pump_stack[0] + 31u)
                                   & ~(unsigned long)31u),
                         PUMP_STACK, 100);
#ifdef TARGET_WII
        port_guard_canary("pump", g_pump_stack);
        port_guard_range("pump", g_pump_stack, sizeof(g_pump_stack));
#endif
    }
}

/* ---------- idle-thread blocker (unused on Wii; boot uses osStopThread) -- */
void port_idle_block(void)
{
    static sem_t idle_sem;
    static int   idle_init;
    if (!idle_init) {
        LWP_SemInit(&idle_sem, 0, 0);
        idle_init = 1;
    }
    LWP_SemWait(idle_sem);
}

/* Debug: counters for the self-test failure message. */
void port_events_debug(unsigned *cb, unsigned *post, unsigned *pump, unsigned *head, unsigned *tail)
{ *cb = g_dbg_cb; *post = g_dbg_post; *pump = g_dbg_pump; *head = g_ring_head; *tail = g_ring_tail; }
