# Task 06b1: N64-style scheduler, part 1 of 3: threads only
Read the full design in `.strata/scratch/06b-spec-full.md` (section "Model"); this task implements ONLY the thread part.
Write `port/ultra/os_sched.c` and `port/ultra/os_sched.h` from scratch (overwrite the existing 64-line file).
Contents, all in os_sched.c:
- `typedef struct { OSThread *t; lwp_t lwp; sem_t run_sem; int state; int pri; void (*entry)(void*); void *arg; u8 *stack; } SchedThread;`
  static table of 32, lookup by OSThread*. States STOPPED, READY, RUNNING, WAITING.
- `static mutex_t sched_lock; static SchedThread *current;` ready list = scan the table for READY with the highest pri
  (ties: lowest `ready_seq` counter, set when the thread became READY).
- `static void trampoline(void *arg)`: LWP_SemWait(run_sem) then entry(arg); if entry returns: lock, state=STOPPED,
  switch_to_next_locked() and never run again.
- `static void switch_to_next_locked(SchedThread *self)`: picks next READY (may be self if self is READY), marks it
  RUNNING/current, unlocks, posts its run_sem if it is not self, and if self is not the one picked waits on
  self->run_sem. If none READY: current=NULL, unlock, wait.
- `void sched_make_ready_locked(SchedThread *t)` and `void sched_preempt_check(void)` (if a READY thread has higher
  pri than current: current becomes READY and switch_to_next_locked).
- `void sched_block_current_locked(void)` for later tasks: marks current WAITING and switches (lock held on entry,
  re-acquired before returning? -> NO: returns with the lock RELEASED; document it).
- libultra API: osCreateThread (LWP thread at LWP priority 64, 64 KB static stack, parked on run_sem, state STOPPED),
  osStartThread (READY; if current==NULL hand it the CPU immediately; else preempt if its pri > current's),
  osStopThread (self/NULL -> STOPPED and switch; other -> STOPPED, removed from READY), osDestroyThread (= stop),
  osYieldThread, osSetThreadPri (NULL = self; re-check preemption), osGetThreadPri, osGetThreadId.
  Store pri/id also in the OSThread fields the struct has (priority, id).
- Remove the old thread functions from port/ultra/os_thread.c and os_backend.c so nothing is defined twice. Leave
  message queues, VI and timers where they are for now (tasks 06b2/06b3 replace them) even if they misbehave.
Self-test (port/ultra/test_threads.c; replace its contents): when built with WII_SELFTEST=1, main() runs
`port_selftest()` instead of bootproc() and then blocks forever. Test T4 only: from main(), osCreateThread three threads
A(pri 10) B(pri 20) C(pri 20) that append their letter to a shared string and call osYieldThread() 3 times each,
then osStopThread(NULL); start them in order A,B,C from a starter thread S (pri 50) which then osStopThread(NULL)s.
Expected order string: "BCBCBCAAA". Log "T4 ok <order>" or "T4 FAIL <order>", then "SELFTEST PASS"/"SELFTEST FAIL".
(main() is not a game thread: it creates S and osStartThread(S), which hands S the CPU since current == NULL.)
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 20 .strata/scratch/t06b1 && grep -a "T4" .strata/scratch/t06b1/sd/drmario/log.txt; grep -aq "T4 ok BCBCBCAAA" .strata/scratch/t06b1/sd/drmario/log.txt
