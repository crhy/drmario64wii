# Task 12f3: remove 12f2 debug code, make the log flush on its own
Claude found the 12f2 self-test regression: in port/ultra/os_events.c event_pump_body, task 12d had put
`#ifdef PORT_LOG_VERBOSE` BETWEEN an `if (...)` and its body, so in normal builds that `if` guarded the event-ring drain
loop: events were delivered only on the first 5 pump wakes and then every 60th. Claude fixed it (the #ifdef now wraps the
whole if). Do NOT reintroduce that pattern: an #ifdef must wrap a whole statement.
1. Remove the diagnostic code added by task 12f2: every "pchk ...", "dliv ...", "T5 start ..." PORT_LOG and its counters
   (g_n_guard etc.) in os_sched.c, os_events.c and test_threads.c, plus port_take_pending_event / g_event_pending and the
   LWP_YieldThread() call in port_preempt_check (they were a wrong theory). Keep 12f's real fixes (waiter removal on stop,
   pick_best_ready_locked, g_current_st cache, xf_vtx bounds, one-time "waiter list full"/"mq table full" logs).
2. port/wii/log.c buffers lines and only syncs when another line is logged, so the LAST lines before a hang or exit never
   reach the SD card (the self-test's "T7 ok"/"SELFTEST PASS" were lost). Add a priority-10 LWP (static 16 KB stack)
   started by port_log_init that sleeps 1 s (usleep) in a loop and calls port_log_flush() when lines are pending
   (keep a pending counter; flush only if > 0). Also call port_log_flush() right after the self-test logs
   "SELFTEST PASS"/its last result line.
GATE: ! grep -nE "pchk|dliv|port_take_pending_event|g_event_pending" port/ultra/*.c && make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t12f3_st >/dev/null && grep -aq "SELFTEST PASS" .strata/scratch/t12f3_st/sd/drmario/log.txt && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12f3 >/dev/null && .strata/imgmax.py .strata/scratch/t12f3 --min-colors 128
