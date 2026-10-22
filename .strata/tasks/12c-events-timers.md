# Task 12c: event ring and timer races (port/ultra/os_events.c)
1. port_post_event has TWO producers (the VI IRQ callback and the timer LWP at priority 90), but its
   _CPU_ISR_Disable/_CPU_ISR_Restore are no-op macros because no libogc header defines them in this TU. An IRQ landing
   mid-push from the timer LWP corrupts or loses an event (a lost VI event can stall the scheduler).
   Add port/wii/irqlock.c (includes <ogc/irq.h>) exporting `unsigned port_irq_disable(void)` and
   `void port_irq_restore(unsigned)` wrapping libogc's inline IRQ_Disable/IRQ_Restore; use them in port_post_event
   (and delete the no-op macros from os_events.c and os_sched.c). Add irqlock.c to Makefile.wii.
2. When the ring is full the event is dropped silently: count drops and PORT_LOG the first and every 64th
   ("event ring full drops=%u").
3. g_timers[] is written by game threads (osSetTimer/osStopTimer) and read/written by the timer LWP with no lock.
   Guard every access to g_timers in osSetTimer, osStopTimer and timer_body with port_irq_disable/restore (short
   sections only; never call usleep or port_post_event while disabled — collect the due timers first, then post).
   osSetTimer with a full table must PORT_LOG "timer table full".
4. Periodic timers drift: on fire use `expiry += interval` (if that is still <= now, set expiry = now + interval).
5. counter_ticks_now: replace the two 64-bit divisions with the exact ratio ticks * 125 / 162 (46.875 MHz / 60.75 MHz).
6. The timer LWP wakes every 10 ms even with no timers: when no timer is armed, block on g_timer_sem (LWP_SemWait)
   instead of usleep, since osSetTimer posts it. Keep the non-blocking drain when timers are armed.
Self-test: the existing threads self-test (WII_SELFTEST build, port/ultra/test_threads.c, T1-T7 incl. T7 timer
accuracy) must still pass.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t12c_st >/dev/null && grep -aq "SELFTEST PASS" .strata/scratch/t12c_st/sd/drmario/log.txt && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12c >/dev/null && .strata/imgmax.py .strata/scratch/t12c --min-colors 128
