/* Dr. Mario 64 -> Wii native port: N64-style single-CPU scheduler on libogc
 * (task 06b1). Replaces the preemptive thread layer in os_thread.c / os_backend.c
 * with the "ultramodern" model from N64Recomp: exactly ONE game thread holds
 * the CPU at a time; every OSThread gets an LWP thread at the SAME libogc
 * priority (64), each with its own run semaphore (init 0) and 64 KB stack.
 * A thread only executes game code while it holds the CPU; otherwise it is
 * parked in LWP_SemWait(run_sem).
 *
 * Includes ONLY libogc headers so its typedefs (u32=uint32_t, s32=int32_t)
 * never clash with ultratypes (u32=unsigned long). The public libultra
 * entry points (osCreateThread/osStartThread/...) live here and keep the
 * exact libultra names/signatures. */

#include <PR/ultratypes.h>
#include <PR/os_thread.h>
#include <PR/os_message.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "os_sched.h"
#ifdef TARGET_WII
#include "port_log.h"
#include <string.h>
#endif

/* libogc LWP/mutex/semaphore API, forward-declared here so this file can use
 * the ultratypes (OSThread/OSId/OSPri) without pulling libogc's <common/types.h>
 * u32/s32 typedefs, which clash with ultratypes' (unsigned long). In libogc
 * these handle types are plain ints; the real definitions live in libogc. */
typedef int lwp_t;
typedef int mutex_t;
typedef int sem_t;
int LWP_MutexInit(mutex_t *mutex, int initial_unlocked);
int LWP_MutexLock(mutex_t mutex);
int LWP_MutexUnlock(mutex_t mutex);
int LWP_SemInit(sem_t *sem, int value, int max);
int LWP_SemWait(sem_t sem);
int LWP_SemPost(sem_t sem);
int LWP_CreateThread(lwp_t *lwp, void *(*aFunction)(void *), void *arg,
                     void *user_context, int stack_size, int prio);
lwp_t LWP_GetSelf(void);
int LWP_SetThreadPriority(lwp_t lwp, int prio);
void LWP_YieldThread(void);

#define OS_CPU_COUNTER 46875000

#define LWP_GAME_PRI   64
#define STACK_SIZE     (64 * 1024)
#define MAX_THREADS    32

/* thread states (libultra OS_STATE_* are #defines; we keep our own ints) */
#define ST_STOPPED 0
#define ST_READY   1
#define ST_RUNNING 2
#define ST_WAITING 3

/* ---------- scheduler thread table ---------- */
typedef struct {
    OSThread   *t;          /* the OSThread* this slot stands for */
    lwp_t       lwp;
    sem_t       run_sem;    /* init 0: thread parks here when not holding CPU */
    int         state;      /* ST_STOPPED / ST_READY / ST_RUNNING / ST_WAITING */
    int         pri;        /* N64 priority (higher = more urgent) */
    void      (*entry)(void *);
    void       *arg;
    void       *stack;      /* STACK_SIZE bytes, 32-byte aligned */
    int         ready_seq;  /* FIFO tie-break: lower = queued earlier */
    OSMesgQueue *wait_mq;   /* last queue this thread blocked on (diagnostics) */
    int         wait_recv;  /* 1 = blocked receiving, 0 = sending */
    int         owed;       /* 1 = a pump delivery woke this thread and owes it
                               the CPU at the running thread's next OS call
                               (libultra send-wake hands off regardless of
                               priority; the pump cannot park a running game
                               thread mid-execution, so we defer the handoff
                               to port_preempt_check) */
} SchedThread;

static SchedThread g_sched[MAX_THREADS];
static mutex_t     g_sched_lock;
static int         g_sched_lock_ready;
static OSThread   *g_current;       /* thread holding the CPU, or NULL */
static SchedThread *g_current_st;   /* cached g_sched slot for g_current (NULL if idle) */
static int         g_ready_counter; /* monotonically increasing seq */

/* ---------- static stack arena ---------- */
static unsigned char g_thread_stacks[MAX_THREADS][STACK_SIZE + 32]
    __attribute__((aligned(32)));

#ifdef TARGET_WII
/* task 29b: per-slot context guard. A 64-byte 0x5A canary sits at the LOW
 * end of each game stack (g_thread_stacks[i][0..63]); the slot fields (t,
 * entry, lwp, stack) are snapshotted at creation and re-checked every VI.
 * Any mismatch is the writer that zeroes a thread context -> log once. */
extern void port_guard_range(const char *name, const void *low, unsigned size);
extern void port_guard_boot_ranges(void);
extern void port_guard_check(unsigned frame);
static unsigned char g_guard_copy[MAX_THREADS][64];
static OSThread   *g_guard_t[MAX_THREADS];
static void     (*g_guard_entry[MAX_THREADS])(void *);
static lwp_t     g_guard_lwp[MAX_THREADS];
static void     *g_guard_stack[MAX_THREADS];
static unsigned  g_guard_frame;
static unsigned  g_guard_ranges_done;
static unsigned  g_guard_broke;
static int       g_guard_arena_done;
#endif

/* ---------- forward declarations ---------- */
static void switch_to_next_locked(SchedThread *self);
static void sched_make_ready_locked(SchedThread *st);
static SchedThread *sched_find_locked(OSThread *t);
static SchedThread *pick_best_ready_locked(void);
static void sched_remove_waiter_locked(SchedThread *st);

/* ---------- trampoline ---------- */
static void *sched_trampoline(void *arg)
{
    int slot = (int)(long)arg;
    SchedThread *st = &g_sched[slot];
    /* Park until the scheduler posts run_sem (i.e. this thread gets the CPU). */
    LWP_SemWait(st->run_sem);
    /* Now this thread holds the CPU. Run the entry function. */
    if (st->entry)
        st->entry(st->arg);
    /* entry() returned: thread is done. Mark STOPPED and hand CPU to next. */
    LWP_MutexLock(g_sched_lock);
    st->state = ST_STOPPED;
    switch_to_next_locked(st);
    /* switch_to_next_locked waits on st->run_sem (st is STOPPED, never posted
     * again), so this trampoline thread blocks forever and retires. */
    return NULL;
}

/* ---------- pick_best_ready_locked ----------
 * Highest-priority READY thread, FIFO among equals (lowest ready_seq).
 * An OWED thread (a pump delivery handed it the CPU obligation but could not
 * park the running game thread) outranks every non-owed READY thread, so the
 * obligation is satisfied at the next switch regardless of priority.
 * Caller holds g_sched_lock. Returns NULL if none is READY. */
static SchedThread *pick_best_ready_locked(void)
{
    int i;
    SchedThread *best = NULL;
    int best_pri = -1;
    int best_seq = -1;
    int best_owed = 0;
    for (i = 0; i < MAX_THREADS; i++) {
        SchedThread *c = &g_sched[i];
        if (c->state != ST_READY) continue;
        if (c->owed > best_owed ||
            (c->owed == best_owed &&
             (c->pri > best_pri || (c->pri == best_pri && c->ready_seq < best_seq)))) {
            best = c;
            best_pri = c->pri;
            best_seq = c->ready_seq;
            best_owed = c->owed;
        }
    }
    return best;
}

/* ---------- switch_to_next_locked ----------
 * Called with g_sched_lock held by the thread giving up the CPU.
 * Picks the highest-priority READY thread (FIFO among equals), makes it
 * current/RUNNING, releases the lock, posts its run_sem (if not self),
 * then waits on self->run_sem (if self was not picked).
 * If no thread is READY: current = NULL, release lock, wait on self->run_sem. */
static void switch_to_next_locked(SchedThread *self)
{
    SchedThread *best = pick_best_ready_locked();

    if (best) {
        best->state = ST_RUNNING;
        best->owed = 0; /* obligation satisfied: it now holds the CPU */
        g_current = best->t;
        g_current_st = best;
        LWP_MutexUnlock(g_sched_lock);
        if (best != self) {
            LWP_SemPost(best->run_sem);
            LWP_SemWait(self->run_sem);
        }
        /* if best == self: lock already released, self continues running */
    } else {
        g_current = NULL;
        g_current_st = NULL;
        LWP_MutexUnlock(g_sched_lock);
        LWP_SemWait(self->run_sem);
    }
}

/* ---------- sched_make_ready_locked ----------
 * Called with g_sched_lock held. Sets state=READY and assigns a new seq. */
static void sched_make_ready_locked(SchedThread *st)
{
    st->state = ST_READY;
    g_ready_counter++;
    st->ready_seq = g_ready_counter;
}

/* ---------- sched_find_locked ----------
 * Find the SchedThread for an OSThread*. Must be called with lock held.
 * Returns NULL if not found. */
static SchedThread *sched_find_locked(OSThread *t)
{
    int i;
    if (t == NULL) {
        /* NULL means "self" = the thread currently holding the CPU */
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t && g_sched[i].t == g_current)
                return &g_sched[i];
        return NULL;
    }
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t == t)
            return &g_sched[i];
    return NULL;
}

/* ---------- sched_get_current_id ----------
 * Returns the id of the thread currently holding the CPU, or -1.
 * Replaces bt_thread_getid(NULL) in os_mesg.c. */
int sched_get_current_id(void)
{
    int i;
    if (g_current == NULL) return -1;
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t == g_current)
            return g_sched[i].t->id;
    return -1;
}

/* ---------- sched_preempt_check ----------
 * Called at hot libultra entry points. If a READY thread has higher priority
 * than the current thread, preempt: current becomes READY, switch to next.
 * Must be called from a game thread (the one holding the CPU). */
void port_preempt_check(void)
{
    SchedThread *cur_st;
    SchedThread *best;
    int best_pri;
    int best_owed;

    if (g_current == NULL) return;

    /* cached slot for the thread holding the CPU */
    cur_st = g_current_st;
    if (!cur_st) return;
    /* Only the thread that holds the CPU may give it up here. Helper LWPs (timer, event pump)
     * call osGetTime etc. too; acting for them would run two game threads at once. */
    if (cur_st->lwp != LWP_GetSelf())
        return;

    /* find highest-priority READY thread, preferring an owed one */
    best = pick_best_ready_locked();
    if (!best) return;
    best_pri = best->pri;
    best_owed = best->owed;

    if (best_owed || best_pri > cur_st->pri) {
        /* preempt: current becomes READY */
        LWP_MutexLock(g_sched_lock);
        sched_make_ready_locked(cur_st);
        switch_to_next_locked(cur_st);
    }
}

/* ---------- sched_block_current_locked ----------
 * Marks current as WAITING and switches to next. Returns with lock RELEASED.
 * Caller must hold g_sched_lock. */
static void sched_block_current_locked(void)
{
    SchedThread *cur_st = g_current_st;
    if (cur_st) {
        cur_st->state = ST_WAITING;
        switch_to_next_locked(cur_st);
    } else {
        /* current is NULL or not in table: just release lock */
        LWP_MutexUnlock(g_sched_lock);
    }
}

/* ---------- osCreateThread ---------- */
void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg,
                    void *sp, OSPri pri)
{
    int i;
    (void)sp; /* we give each thread its own 64 KB stack */

    if (!g_sched_lock_ready) {
        LWP_MutexInit(&g_sched_lock, false);
        g_sched_lock_ready = 1;
    }

    LWP_MutexLock(g_sched_lock);
    for (i = 0; i < MAX_THREADS; i++) {
        if (g_sched[i].t == NULL) {
            SchedThread *st = &g_sched[i];
            st->t = t;
            st->entry = entry;
            st->arg = arg;
            st->pri = (int)pri;
            st->state = ST_STOPPED;
            st->ready_seq = 0;
            st->owed = 0;
            st->stack = (void *)(((unsigned long)&g_thread_stacks[i][0] + 31)
                                 & ~(unsigned long)31);
            LWP_SemInit(&st->run_sem, 0, 1);
            if (LWP_CreateThread(&st->lwp, sched_trampoline,
                                 (void *)(long)i,
                                 st->stack, STACK_SIZE,
                                 (u8)LWP_GAME_PRI) < 0) {
                st->t = NULL;
                LWP_MutexUnlock(g_sched_lock);
                return;
            }
#ifdef TARGET_WII
            memset(&g_thread_stacks[i][0], 0x5A, 64);
            memcpy(g_guard_copy[i], g_thread_stacks[i], 64);
            g_guard_t[i] = t;
            g_guard_entry[i] = entry;
            g_guard_lwp[i] = st->lwp;
            g_guard_stack[i] = st->stack;
            if (!g_guard_arena_done) {
                g_guard_arena_done = 1;
                port_guard_range("gthread", g_thread_stacks[0], sizeof(g_thread_stacks));
                port_guard_range("g_sched", g_sched, sizeof(g_sched));
            }
#endif
            /* Mirror libultra: the host (main) thread runs at the lowest
             * priority so game threads outrank it. */
            {
                lwp_t self = LWP_GetSelf();
                int j, managed = 0;
                for (j = 0; j < MAX_THREADS; j++)
                    if (g_sched[j].t && g_sched[j].lwp == self) { managed = 1; break; }
                if (!managed)
                    LWP_SetThreadPriority(self, 0);
            }
            t->id = id;
            t->priority = pri;
            t->state = OS_STATE_STOPPED;
            LWP_MutexUnlock(g_sched_lock);
            return;
        }
    }
    LWP_MutexUnlock(g_sched_lock);
}

/* ---------- osStartThread ---------- */
void osStartThread(OSThread *t)
{
    int i;
    SchedThread *st = NULL;

    LWP_MutexLock(g_sched_lock);
    for (i = 0; i < MAX_THREADS; i++) {
        if (g_sched[i].t == t) { st = &g_sched[i]; break; }
    }
    if (!st) { LWP_MutexUnlock(g_sched_lock); return; }

    st->state = ST_READY;
    g_ready_counter++;
    st->ready_seq = g_ready_counter;
    t->state = OS_STATE_RUNNABLE;

    if (g_current == NULL) {
        /* CPU idle: hand it to this thread immediately */
        st->state = ST_RUNNING;
        st->owed = 0;
        g_current = t;
        g_current_st = st;
        t->state = OS_STATE_RUNNING;
        LWP_MutexUnlock(g_sched_lock);
        LWP_SemPost(st->run_sem);
    } else {
        /* find current's slot */
        SchedThread *cur_st = NULL;
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == g_current) { cur_st = &g_sched[i]; break; }

        if (cur_st && st->pri > cur_st->pri) {
            /* preemption: current becomes READY, new thread runs */
            sched_make_ready_locked(cur_st);
            switch_to_next_locked(cur_st);
        } else {
            /* just READY: unlock, thread waits on run_sem until picked */
            LWP_MutexUnlock(g_sched_lock);
        }
    }
}

/* ---------- osStopThread ---------- */
void osStopThread(OSThread *t)
{
    int i;
    SchedThread *st = NULL;

    LWP_MutexLock(g_sched_lock);

    if (t == NULL) {
        /* NULL means "self" = the thread currently holding the CPU */
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == g_current) { st = &g_sched[i]; break; }
    } else {
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == t) { st = &g_sched[i]; break; }
    }

    if (!st) { LWP_MutexUnlock(g_sched_lock); return; }

    if (st->t == g_current) {
        /* stopping the thread that holds the CPU: switch to next */
        st->state = ST_STOPPED;
        st->t->state = OS_STATE_STOPPED;
        sched_remove_waiter_locked(st);
        switch_to_next_locked(st);
    } else {
        /* stopping a non-current thread: just mark STOPPED (removed from READY) */
        st->state = ST_STOPPED;
        st->t->state = OS_STATE_STOPPED;
        sched_remove_waiter_locked(st);
        LWP_MutexUnlock(g_sched_lock);
    }
}

/* ---------- osDestroyThread ---------- */
void osDestroyThread(OSThread *t)
{
    /* no-op: stacks are static, LWP threads retire when entry() returns */
    (void)t;
}

/* ---------- osYieldThread ---------- */
void osYieldThread(void)
{
    int i;
    SchedThread *cur_st = NULL;

    LWP_MutexLock(g_sched_lock);
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t == g_current) { cur_st = &g_sched[i]; break; }

    if (!cur_st) { LWP_MutexUnlock(g_sched_lock); return; }

    /* re-queue behind equal priority: make self READY with new seq, then switch */
    sched_make_ready_locked(cur_st);
    switch_to_next_locked(cur_st);
}

/* ---------- osSetThreadPri ---------- */
void osSetThreadPri(OSThread *t, OSPri pri)
{
    int i;
    SchedThread *st = NULL;

    LWP_MutexLock(g_sched_lock);

    if (t == NULL) {
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == g_current) { st = &g_sched[i]; break; }
    } else {
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == t) { st = &g_sched[i]; break; }
    }

    if (!st) { LWP_MutexUnlock(g_sched_lock); return; }

    st->pri = (int)pri;
    if (st->t) st->t->priority = pri;

    if (st->t == g_current) {
        /* changing current thread's priority: check if a READY thread now outranks it */
        SchedThread *best = NULL;
        int best_pri = -1;
        for (i = 0; i < MAX_THREADS; i++) {
            if (g_sched[i].state != ST_READY) continue;
            if (g_sched[i].pri > best_pri) {
                best = &g_sched[i];
                best_pri = g_sched[i].pri;
            }
        }
        if (best && best_pri > st->pri) {
            sched_make_ready_locked(st);
            switch_to_next_locked(st);
        } else {
            LWP_MutexUnlock(g_sched_lock);
        }
    } else {
        LWP_MutexUnlock(g_sched_lock);
    }
}

/* ---------- osGetThreadPri ---------- */
OSPri osGetThreadPri(OSThread *t)
{
    int i;
    if (t == NULL) {
        /* NULL means current thread */
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == g_current)
                return (OSPri)g_sched[i].pri;
        return 0;
    }
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t == t)
            return (OSPri)g_sched[i].pri;
    return 0;
}

/* ---------- osGetThreadId ---------- */
OSId osGetThreadId(OSThread *t)
{
    int i;
    if (t == NULL) {
        for (i = 0; i < MAX_THREADS; i++)
            if (g_sched[i].t == g_current)
                return g_sched[i].t->id;
        return -1;
    }
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t == t)
            return g_sched[i].t->id;
    return -1;
}

/* =====================================================================
 * Message queues (task 06b2). All libultra queue functions live here.
 * The queue itself uses libultra's own fields (validCount, first, msgCount,
 * msg[]); MQ_IS_EMPTY/MQ_IS_FULL from os_message.h read them. Blocking uses
 * the scheduler: a thread blocked on an empty queue (recv) or a full queue
 * (send) is WAITING and parked on its run_sem; a side table maps each queue
 * to its recv-waiters and send-waiters (FIFO). A send that wakes a recv
 * waiter makes it READY and runs the preemption check, so a higher-priority
 * waiter starts running before osSendMesg returns. NOBLOCK returns -1 on
 * empty/full. Everything below runs under g_sched_lock.
 * ===================================================================== */

#define MAX_QUEUES         128
#define MAX_WAITERS_PER_MQ 8
#define MAX_PENDING_PER_MQ 64

typedef struct {
    OSMesgQueue  *mq;
    SchedThread  *recv_waiters[MAX_WAITERS_PER_MQ]; /* blocked on empty */
    int           recv_count;
    SchedThread  *send_waiters[MAX_WAITERS_PER_MQ]; /* blocked on full  */
    int           send_count;
    OSMesg        pend_msg[MAX_PENDING_PER_MQ]; /* deferred external events (FIFO) */
    int           pend_count;
} MqWaiters;

static MqWaiters g_mq_waiters[MAX_QUEUES];

/* ---------- event message table (moved from os_mesg.c) ---------- */
#define PORT_MAX_EVENTS 23
static OSMesgQueue *g_event_mq[PORT_MAX_EVENTS];
static OSMesg      g_event_msg[PORT_MAX_EVENTS];

/* ---------- mq_find_locked: find or create the side-table entry ----------
 * Called with g_sched_lock held. Returns NULL only if the table is full. */
static MqWaiters *mq_find_locked(OSMesgQueue *mq)
{
    int i;
    for (i = 0; i < MAX_QUEUES; i++)
        if (g_mq_waiters[i].mq == mq)
            return &g_mq_waiters[i];
    for (i = 0; i < MAX_QUEUES; i++) {
        if (g_mq_waiters[i].mq == NULL) {
            g_mq_waiters[i].mq = mq;
            g_mq_waiters[i].recv_count = 0;
            g_mq_waiters[i].send_count = 0;
            g_mq_waiters[i].pend_count = 0;
            return &g_mq_waiters[i];
        }
    }
    return NULL;
}

/* ---------- sched_current_slot_locked ----------
 * The SchedThread for the thread holding the CPU (NULL if CPU idle).
 * Caller must hold g_sched_lock. */
static SchedThread *sched_current_slot_locked(void)
{
    int i;
    lwp_t self = LWP_GetSelf();
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t && g_sched[i].lwp == self) return &g_sched[i];
    return NULL;
}

/* ---------- sched_add_waiter_locked ----------
 * Append self to the recv- or send-waiter list of mq's side entry. */
static void sched_add_waiter_locked(OSMesgQueue *mq, SchedThread *self, int is_recv)
{
    if (self) { self->wait_mq = mq; self->wait_recv = is_recv; }
    MqWaiters *w = mq_find_locked(mq);
    if (!w) {
        static int logged = 0;
        if (!logged) { logged = 1; PORT_LOG("sched: mq table full"); }
        return;
    }
    if (is_recv) {
        if (w->recv_count < MAX_WAITERS_PER_MQ)
            w->recv_waiters[w->recv_count++] = self;
        else {
            static int logged = 0;
            if (!logged) { logged = 1; PORT_LOG("sched: waiter list full mq=%p", (void *)mq); }
        }
    } else {
        if (w->send_count < MAX_WAITERS_PER_MQ)
            w->send_waiters[w->send_count++] = self;
        else {
            static int logged = 0;
            if (!logged) { logged = 1; PORT_LOG("sched: waiter list full mq=%p", (void *)mq); }
        }
    }
}

/* ---------- sched_remove_waiter_locked ----------
 * Drop st from every recv- and send-waiter list in the mq side table. A
 * STOPPED thread must not linger in a waiter list: a later send/recv would
 * otherwise make it READY and run it. Caller holds g_sched_lock. */
static void sched_remove_waiter_locked(SchedThread *st)
{
    int q, k, n;
    for (q = 0; q < MAX_QUEUES; q++) {
        MqWaiters *w = &g_mq_waiters[q];
        n = 0;
        for (k = 0; k < w->recv_count; k++)
            if (w->recv_waiters[k] != st) w->recv_waiters[n++] = w->recv_waiters[k];
        w->recv_count = n;
        n = 0;
        for (k = 0; k < w->send_count; k++)
            if (w->send_waiters[k] != st) w->send_waiters[n++] = w->send_waiters[k];
        w->send_count = n;
    }
}

/* ---------- sched_wake_waiter_locked ----------
 * Pop the FIFO waiter from mq's list, make it READY, and run the
 * preemption check: if the woken thread outranks the current one, the
 * current thread becomes READY and the woken thread takes the CPU now
 * (so it runs before the sending/recving call returns). If the CPU is
 * idle, hand it straight to the woken thread. Caller holds the lock;
 * the lock is released only if the CPU switched (switch_to_next_locked)
 * or was idle (we unlock before posting run_sem). Returns 1 if the lock is
 * STILL held on return, 0 if it was released (caller must not unlock again). */
static int sched_wake_waiter_locked(OSMesgQueue *mq, int is_recv)
{
    MqWaiters *w = mq_find_locked(mq);
    SchedThread *wt;
    SchedThread *cur;
    int i;

    if (!w) return 1;
    if (is_recv) {
        if (w->recv_count <= 0) return 1;
        wt = w->recv_waiters[0];
        for (i = 1; i < w->recv_count; i++)
            w->recv_waiters[i - 1] = w->recv_waiters[i];
        w->recv_count--;
    } else {
        if (w->send_count <= 0) return 1;
        wt = w->send_waiters[0];
        for (i = 1; i < w->send_count; i++)
            w->send_waiters[i - 1] = w->send_waiters[i];
        w->send_count--;
    }

    sched_make_ready_locked(wt);

    cur = sched_current_slot_locked();
    if (!cur) {
        /* CPU idle: start the woken thread directly */
        wt->state = ST_RUNNING;
        wt->owed = 0;
        g_current = wt->t;
        g_current_st = wt;
        if (wt->t) wt->t->state = OS_STATE_RUNNING;
        LWP_MutexUnlock(g_sched_lock);
        LWP_SemPost(wt->run_sem);
        return 0;
    }
    /* The woken thread runs to its next preemption point before the OS call
     * returns (libultra semantics), regardless of priority: current becomes
     * READY and the woken thread takes the CPU. Leaving it merely READY would
     * starve it, because port_preempt_check only switches to a STRICTLY higher
     * priority thread and never posts this thread's run_sem. */
    sched_make_ready_locked(cur);
    switch_to_next_locked(cur); /* makes wt RUNNING, posts wt->run_sem, parks cur */
    return 0;
}

/* ---------- port_deliver_event: external event delivery (task 06b3) ------
 * Called by the event pump LWP (NOT a game thread) to deliver an external
 * event exactly like osSendMesg NOBLOCK, but WITHOUT preempting the running
 * game thread: the pump cannot give up the CPU on behalf of a parked thread.
 * A woken waiter is made READY and runs when the running thread next yields
 * via port_preempt_check. If the CPU is idle, the woken thread is started.
 * Caller must NOT hold g_sched_lock. */
void port_deliver_event(void *mqv, void *msg)
{
    OSMesgQueue *mq = (OSMesgQueue *)mqv;
    MqWaiters *w;
    SchedThread *wt = NULL;
    SchedThread *cur;
    int i;

    LWP_MutexLock(g_sched_lock);
    if (MQ_IS_FULL(mq)) {
        /* Never lose an event (task 33): keep it in this queue's pending list
         * and deliver it, in order, as soon as the receiver takes a message
         * (osRecvMesg frees a slot and calls mq_deliver_pending_locked). */
        MqWaiters *pw = mq_find_locked(mq);
        if (pw && pw->pend_count < MAX_PENDING_PER_MQ) {
            pw->pend_msg[pw->pend_count++] = (OSMesg)msg;
#ifdef TARGET_WII
            { extern unsigned g_deliver_pend;
              extern void *g_defer_mq, *g_defer_msg;
              g_deliver_pend++;
              g_defer_mq = (void *)mq;
              g_defer_msg = msg;
              if (g_deliver_pend == 1u || (g_deliver_pend % 64u) == 0u)
                  PORT_LOG("deliver defer mq=%p msg=%p n=%u", (void *)mq, msg, g_deliver_pend); }
#endif
        } else {
#ifdef TARGET_WII
            { extern unsigned g_deliver_drop; g_deliver_drop++; }
#endif
        }
        LWP_MutexUnlock(g_sched_lock);
        return;
    }
#ifdef TARGET_WII
    { extern unsigned g_deliver_ok; g_deliver_ok++; }
#endif
    mq->msg[(mq->first + mq->validCount) % mq->msgCount] = msg;
    mq->validCount++;

    w = mq_find_locked(mq);
    if (w && w->recv_count > 0) {
        wt = w->recv_waiters[0];
        for (i = 1; i < w->recv_count; i++)
            w->recv_waiters[i - 1] = w->recv_waiters[i];
        w->recv_count--;
        sched_make_ready_locked(wt);
    }

    cur = g_current_st;
    if (!cur) {
        /* CPU idle: hand it to the highest READY thread. The pump is an LWP
         * thread (not a sched thread), so it must NOT wait on any run_sem;
         * it just posts the started thread and returns. */
        SchedThread *best = pick_best_ready_locked();
        if (best) {
            best->state = ST_RUNNING;
            best->owed = 0;
            g_current = best->t;
            g_current_st = best;
            if (best->t) best->t->state = OS_STATE_RUNNING;
            LWP_MutexUnlock(g_sched_lock);
            LWP_SemPost(best->run_sem);
        } else {
            LWP_MutexUnlock(g_sched_lock);
        }
        return;
    }
    /* else: the running game thread keeps the CPU; the woken thread stays
     * READY and preempts at its next OS call via port_preempt_check. Mark it
     * owed so port_preempt_check hands it the CPU even if it is LOWER priority
     * than the running thread (the pump cannot park the running thread now, so
     * the libultra immediate hand-off is deferred to the next OS call). */
    if (wt)
        wt->owed = 1;
    LWP_MutexUnlock(g_sched_lock);
}

/* ---------- osCreateMesgQueue ---------- */
void osCreateMesgQueue(OSMesgQueue *mq, OSMesg *msg, s32 msgCount)
{
    mq->mtqueue = NULL;
    mq->fullqueue = NULL;
    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = msgCount;
    mq->msg = msg;

    LWP_MutexLock(g_sched_lock);
    mq_find_locked(mq); /* register the side-table entry now */
    LWP_MutexUnlock(g_sched_lock);
}

/* ---------- osSendMesg ---------- */
/* True if the calling LWP is a game thread created by osCreateThread. Scheduler entry points
 * must only act for game threads; other LWPs (DOL main, event pump, timer, IRQ helpers) send
 * through port_post_event instead. */
static int sched_caller_is_game_thread(void)
{
    lwp_t self = LWP_GetSelf();
    int i;
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].t && g_sched[i].lwp == self) return 1;
    return 0;
}
extern void port_post_event(void *mq, void *msg);

s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 block)
{
    if (!sched_caller_is_game_thread()) { port_post_event(mq, &msg); return 0; }
    port_preempt_check();
    for (;;) {
        LWP_MutexLock(g_sched_lock);
        if (!MQ_IS_FULL(mq)) break;
        if (block == OS_MESG_NOBLOCK) {
            LWP_MutexUnlock(g_sched_lock);
            return -1;
        }
        sched_add_waiter_locked(mq, sched_current_slot_locked(), 0);
        sched_block_current_locked(); /* WAITING + park; lock released */
        /* retry when woken */
    }
    mq->msg[(mq->first + mq->validCount) % mq->msgCount] = msg;
    mq->validCount++;
    if (sched_wake_waiter_locked(mq, 1)) /* wake a recv waiter, if any */
        LWP_MutexUnlock(g_sched_lock);
    return 0;
}

/* ---------- osJamMesg: same as osSendMesg but inserts at the FRONT ----------
 * libultra semantics: the message lands before the current first element,
 * so a later osRecvMesg sees it next. */
s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 block)
{
    if (!sched_caller_is_game_thread()) { port_post_event(mq, &msg); return 0; }
    port_preempt_check();
    for (;;) {
        LWP_MutexLock(g_sched_lock);
        if (!MQ_IS_FULL(mq)) break;
        if (block == OS_MESG_NOBLOCK) {
            LWP_MutexUnlock(g_sched_lock);
            return -1;
        }
        sched_add_waiter_locked(mq, sched_current_slot_locked(), 0);
        sched_block_current_locked();
    }
    mq->first = (mq->first - 1 + mq->msgCount) % mq->msgCount;
    mq->msg[mq->first] = msg;
    mq->validCount++;
    if (sched_wake_waiter_locked(mq, 1))
        LWP_MutexUnlock(g_sched_lock);
    return 0;
}

/* ---------- mq_deliver_pending_locked (task 33) ----------
 * osRecvMesg just freed a slot in mq; if external events were deferred for
 * this queue (port_deliver_event found it full), deliver the oldest one now,
 * in arrival order, exactly like the push+wake half of port_deliver_event.
 * Invariant: pend_count > 0 implies the queue was full when the event was
 * deferred, and a full queue has no recv waiters, so the wake below is
 * defensive only. Caller holds g_sched_lock; lock state unchanged. */
static void mq_deliver_pending_locked(OSMesgQueue *mq)
{
    MqWaiters *w = mq_find_locked(mq);
    SchedThread *wt;
    int i;
    if (!w || w->pend_count <= 0) return;
    if (MQ_IS_FULL(mq)) return;
    mq->msg[(mq->first + mq->validCount) % mq->msgCount] = w->pend_msg[0];
    for (i = 1; i < w->pend_count; i++)
        w->pend_msg[i - 1] = w->pend_msg[i];
    w->pend_count--;
    mq->validCount++;
#ifdef TARGET_WII
    { extern unsigned g_deliver_ok; g_deliver_ok++; }
#endif
    if (w->recv_count > 0) {
        wt = w->recv_waiters[0];
        for (i = 1; i < w->recv_count; i++)
            w->recv_waiters[i - 1] = w->recv_waiters[i];
        w->recv_count--;
        sched_make_ready_locked(wt);
    }
}

/* ---------- osRecvMesg ---------- */
s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 block)
{
    port_preempt_check();
    for (;;) {
        LWP_MutexLock(g_sched_lock);
        if (!MQ_IS_EMPTY(mq)) break;
        if (block == OS_MESG_NOBLOCK) {
            LWP_MutexUnlock(g_sched_lock);
            return -1;
        }
        sched_add_waiter_locked(mq, sched_current_slot_locked(), 1);
#ifdef PORT_DEBUG_BLOCKS
        { /* debug: log each distinct blocking call site the first 3 times */
            static void *ra_seen[64]; static int ra_cnt[64]; void *ra = __builtin_return_address(0); int k;
            for (k = 0; k < 64 && ra_seen[k] && ra_seen[k] != ra; k++) ;
            if (k < 64) { ra_seen[k] = ra; if (((++ra_cnt[k]) & (ra_cnt[k] - 1)) == 0) PORT_LOG("blk n=%d id=%d mq=%p ra=%p", ra_cnt[k], g_current ? (int)g_current->id : -1, (void *)mq, ra); }
        }
#endif
        sched_block_current_locked(); /* WAITING + park; retry when woken */
    }
    if (msg)
        *msg = mq->msg[mq->first];
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;
    mq_deliver_pending_locked(mq); /* task 33: refill with a deferred event */
    if (sched_wake_waiter_locked(mq, 0)) /* a slot freed: wake a send waiter */
        LWP_MutexUnlock(g_sched_lock);
    return 0;
}

/* ---------- osSetEventMesg (moved from os_mesg.c) ---------- */
void osSetEventMesg(OSEvent event, OSMesgQueue *mq, OSMesg msg)
{
    if (event >= 0 && event < PORT_MAX_EVENTS) {
        g_event_mq[event] = mq;
        g_event_msg[event] = msg;
    }
}

/* ---------- port_os_event (moved from os_mesg.c) ----------
 * Called by SP/DP/SI completion code (game threads). NOBLOCK path only:
 * never blocks, never parks the caller. */
void port_os_event(int event)
{
    if (event >= 0 && event < PORT_MAX_EVENTS && g_event_mq[event])
        osSendMesg(g_event_mq[event], g_event_msg[event], OS_MESG_NOBLOCK);
}

/* Completion signal for the RSP LWP: pushes the event through the pump ring
 * (port_post_event) instead of osSendMesg, so it never touches the scheduler
 * token and never parks the RSP LWP. */
void port_os_event_post(int event)
{
    if (event >= 0 && event < PORT_MAX_EVENTS && g_event_mq[event])
        port_post_event(g_event_mq[event], &g_event_msg[event]);
}

/* Debug: id of the thread holding the CPU (-1 idle) and a READY-thread summary. */
void port_sched_debug(int *cur_id, int *ready_mask_lo)
{
    int i, m = 0;
    *cur_id = g_current ? (int)g_current->id : -1;
    for (i = 0; i < MAX_THREADS; i++)
        if (g_sched[i].state == ST_READY && g_sched[i].t) m |= 1 << (g_sched[i].t->id & 31);
    *ready_mask_lo = m;
}

/* Safety net, called by the event pump on every wake: if the CPU is idle but a thread is READY
 * (a wake path missed the idle hand-off), start the best READY thread. Returns 1 if it did. */
int port_sched_kick_idle(void)
{
    SchedThread *best;
    LWP_MutexLock(g_sched_lock);
    if (g_current == NULL) {
        best = pick_best_ready_locked();
        if (best) {
            best->state = ST_RUNNING;
            best->owed = 0;
            g_current = best->t;
            g_current_st = best;
            if (best->t) best->t->state = OS_STATE_RUNNING;
            LWP_MutexUnlock(g_sched_lock);
            LWP_SemPost(best->run_sem);
            return 1;
        }
    }
    LWP_MutexUnlock(g_sched_lock);
    return 0;
}

#ifdef TARGET_WII
/* Called by the event pump once per VI. Checks every game slot's canary and
 * slot fields against the creation-time snapshot; logs the FIRST mismatch. */
void port_guard_vi(void)
{
    int i;
    g_guard_frame++;
    if (!g_guard_ranges_done) {
        g_guard_ranges_done = 1;
        port_guard_boot_ranges();
    }
    port_guard_check(g_guard_frame);
    if (g_guard_broke)
        return;
    for (i = 0; i < MAX_THREADS; i++) {
        SchedThread *st = &g_sched[i];
        if (!st->t)
            continue;
        const char *what = NULL;
        const unsigned char *where = NULL;
        unsigned j;
        for (j = 0u; j < 64u; j++) {
            if (g_thread_stacks[i][j] != g_guard_copy[i][j]) {
                what = "canary";
                where = &g_thread_stacks[i][0];
                break;
            }
        }
        if (!what && g_guard_t[i] != st->t) { what = "t"; where = (const unsigned char *)st; }
        if (!what && g_guard_entry[i] != st->entry) { what = "entry"; where = (const unsigned char *)st; }
        if (!what && g_guard_lwp[i] != st->lwp) { what = "lwp"; where = (const unsigned char *)st; }
        if (!what && g_guard_stack[i] != st->stack) { what = "stack"; where = (const unsigned char *)st; }
        if (what && !g_guard_broke) {
            g_guard_broke = 1;
            const uint32_t *w = (const uint32_t *)(uintptr_t)where;
            PORT_LOG("guard: thread slot %d %s broken (frame %u, last running id %d) "
                     "w0=%08x w1=%08x w2=%08x w3=%08x w4=%08x w5=%08x w6=%08x w7=%08x",
                     i, what, g_guard_frame,
                     g_current ? (int)g_current->id : -1,
                     w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]);
        }
    }
}
#endif

#if defined(PORT_SCHED_DUMP) || defined(PORT_SCHED_DUMP2)
/* Diagnostics: one line per game thread (id, N64 priority, state, last queue it blocked on + that queue's fill). */
void port_sched_dump(unsigned frame)
{
    int i;
    { extern void port_events_debug(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*); unsigned a,b,c,d,e;
      port_events_debug(&a,&b,&c,&d,&e);
      PORT_LOG("dump f=%u cur=%d vi_cb=%u posts=%u pump=%u ring=%u/%u", frame, g_current ? (int)g_current->id : -1, a,b,c,d,e);
#ifdef PORT_SCHED_DUMP2
      { extern void port_nnsched_dump(void); port_nnsched_dump(); }
#endif
    }
    for (i = 0; i < MAX_THREADS; i++) {
        SchedThread *st = &g_sched[i];
        if (!st->t) continue;
        PORT_LOG(" th id=%d pri=%d st=%d mq=%p %s valid=%d/%d", (int)st->t->id, st->pri, st->state, (void *)st->wait_mq,
                 st->wait_recv ? "recv" : "send", st->wait_mq ? (int)st->wait_mq->validCount : -1,
                 st->wait_mq ? (int)st->wait_mq->msgCount : -1);
    }
}
#endif
