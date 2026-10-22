#ifndef PORT_ULTRA_OS_SCHED_H
#define PORT_ULTRA_OS_SCHED_H

/* N64-style single-CPU scheduler on libogc (task 06b).
 *
 * Exactly ONE game thread "holds the CPU" at a time. Every OSThread gets an
 * LWP thread at the SAME libogc priority (64), each with its own run
 * semaphore (init 0) and 64 KB stack. A thread only executes game code while
 * it holds the CPU; otherwise it is parked in LWP_SemWait(run_sem).
 *
 * This header deliberately includes NOTHING and uses only built-in C types,
 * so it can be included next to the ultra headers without a typedef clash.
 * The libogc includes live only in os_sched.c. */

/* ---- external event pump: IRQ/helper code posts events here ---- */
void port_post_event(void *mq, void *msg);

/* ---- deliver an external event from the pump (no preempt of a running thread) ---- */
void port_deliver_event(void *mq, void *msg);

/* ---- preemption check: called at every hot libultra entry point ---- */
void port_preempt_check(void);

/* ---- boot: start the event pump + VI callback before bootproc() ---- */
void port_sched_boot(void);

/* ---- idle-thread blocker: parks the game idle thread forever ---- */
void port_idle_block(void);

/* ---- current thread id (replaces bt_thread_getid(NULL)) ---- */
int sched_get_current_id(void);

#endif
