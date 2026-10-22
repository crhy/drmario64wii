# Task 12f: scheduler and vertex-load robustness (port/ultra/os_sched.c, port/gfx/gfx_xform.c)
1. osStopThread on a WAITING thread leaves it in the message-queue waiter lists; a later send then makes the STOPPED
   thread READY and runs it. Remove the thread from every recv/send waiter list when it is stopped.
2. sched_add_waiter_locked silently drops the waiter when the list (8) or the queue table (128) is full, so the thread
   waits forever. PORT_LOG "sched: waiter list full mq=%p" / "sched: mq table full" once each (do not log under the
   lock path more than once — use a static flag).
3. port_sched_kick_idle starts with bseq = 0, so FIFO order among equal priorities is ignored: init it like
   switch_to_next_locked (pick lowest ready_seq). Factor one `static SchedThread *pick_best_ready_locked(void)` and use
   it in switch_to_next_locked, port_deliver_event and port_sched_kick_idle.
4. Cache the current thread's slot: keep `static SchedThread *g_current_st` updated wherever g_current changes, and
   use it in port_preempt_check / sched_current_slot_locked / sched_block_current_locked instead of 32-slot scans
   (port_preempt_check runs on every osGetTime/osGetCount).
5. gfx_xform.c xf_vtx: v0 = ((w0>>1)&0x7F) - n can be negative and d (gfx_mem(w1)) can be NULL; both corrupt memory.
   Return early (with a one-time PORT_LOG) if d == NULL, and only write vertices with 0 <= v0+i < 32.
The threads self-test must still pass (T1-T7, "SELFTEST PASS").
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t12f_st >/dev/null && grep -aq "SELFTEST PASS" .strata/scratch/t12f_st/sd/drmario/log.txt && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12f >/dev/null && .strata/imgmax.py .strata/scratch/t12f --min-colors 128
