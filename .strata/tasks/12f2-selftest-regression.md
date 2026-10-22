# Task 12f2: the threads self-test regressed (T5 VI events and T6 timers fail)
The WII_SELFTEST build passed "SELFTEST PASS" after task 12c (03:49). After 12d (logging/boot), 12e (GX state) and 12f
(scheduler robustness) it fails:
  T5 FAIL 6 (vi_cb=60 posts=60 pump_wakes=60 ring=60/60)   -> the pri-20 thread got 6 VI messages in 1 s of osGetTime
  T6 FAIL 45358950 / T6 FAIL 2                             -> timers fire late / too few periodic fires
Every posted VI event reached the pump (posts == pump_wakes), so find why the test thread does not receive ~60/s.
Method (measure, do not guess):
1. In port/ultra/test_threads.c T5, log at start and end: port_wall_ticks_now(), osGetTime(), the vi_cb count
   (port_events_debug) — so you know the real VI rate and whether osGetTime matches wall time (60.75 MHz TB).
2. Log in port_deliver_event (os_sched.c, only while a static debug flag set by T5 is on, first 20 calls): the mq, whether
   a recv waiter was found, the woken thread id, and g_current id. Check whether the VI message is queued but the
   pri-20 thread is never made RUNNING (CPU "busy" with a thread that is actually the DOL main LWP / not a game thread),
   or the waiter was never registered (side table / waiter list changes from 12f).
3. The self-test main loop runs on the DOL main LWP (not a game thread) and busy-waits on osGetTime; port_preempt_check
   returns early for non-game LWPs. Make sure a READY game thread still gets started when g_current is NULL
   (port_sched_kick_idle / idle hand-off in port_deliver_event) — compare with how 12f changed these paths.
4. Fix the cause in the port code (never weaken the test thresholds). Write what you found to .strata/NOTES.md.
Previous file versions: .strata/snapshots/*.tgz (after-12f-* is the current state; per-task snapshots from now on).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t12f2_st >/dev/null && grep -aq "SELFTEST PASS" .strata/scratch/t12f2_st/sd/drmario/log.txt && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12f2 >/dev/null && .strata/imgmax.py .strata/scratch/t12f2 --min-colors 128
