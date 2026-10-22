# Task 06b3: N64-style scheduler, part 3 of 3: events, VI, timers, time, boot wiring
Design: `.strata/scratch/06b-spec-full.md` (read the "Model" bullets about external events, VI, timers, idle thread,
DOL main(), osSetIntMask, PORT_LOG). Implement in a new `port/ultra/os_events.c`:
- `void port_post_event(OSMesgQueue *mq, OSMesg msg)`: IRQ-safe ring of 256 entries (disable IRQs around the push with
  _CPU_ISR_Disable/_CPU_ISR_Restore), then LWP_SemPost(event_sem).
- event pump LWP thread at LWP priority 100: waits on event_sem, drains the ring; for each event takes sched_lock and
  does what osSendMesg NOBLOCK does (wake waiter, READY); if current == NULL start the highest READY thread.
- osViSetEvent + VIDEO_SetPostRetraceCallback: the callback only counts and calls port_post_event every retraceCount.
- osSetTimer/osStopTimer: timer helper LWP (priority 90) sleeping until the earliest expiry (usleep in <= 10 ms steps
  is fine), periodic and one-shot, delivers via port_post_event.
- osGetTime/osGetCount at 46.875 MHz from gettime(); osSetIntMask/__osDisableInt/__osRestoreInt do nothing.
- sched_preempt_check() is called at the start of osRecvMesg, osSendMesg, osGetTime, osGetCount, osPiStartDma,
  osWritebackDCache, osInvalDCache.
- Delete now-dead code: old VI/timer/watchdog/pump/idle-block code in os_backend.c / os_thread.c / port/wii/main.c,
  mkdir markers. The game's Idle_ThreadEntry under TARGET_WII: after starting the main thread, osStopThread(NULL).
- PORT_LOG: open the log file ONCE at init, keep it open, mutex, fflush per line.
- DOL main(): video/GX/log/WPAD init, start the event pump, then bootproc() (or port_selftest() when WII_SELFTEST),
  then block forever on an LWP semaphore.
Self-test: add T5 (>= 50 VI retrace messages within 1 s measured by osGetTime), T6 (one-shot 100 ms timer fires in
90-150 ms; periodic 50 ms timer fires 18-22 times in 1 s), T7 (osGetTime advances 46.875e6 +-5 % per second). Keep T1-T4.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t06b3 && grep -a "T[0-9]" .strata/scratch/t06b3/sd/drmario/log.txt; for t in T1 T2 T3 T4 T5 T6 T7; do grep -aq "$t ok" .strata/scratch/t06b3/sd/drmario/log.txt || exit 1; done; make -f Makefile.wii clean >/dev/null; make -f Makefile.wii >/dev/null
