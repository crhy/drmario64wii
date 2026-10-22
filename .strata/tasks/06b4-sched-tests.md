# Task 06b4: self-tests for events, VI, timers and time
port/ultra/os_events.c (task 06b3) implements port_post_event, the event pump, osViSetEvent, osSetTimer/osStopTimer,
osGetTime/osGetCount. port/ultra/test_threads.c has T1-T4 (passing). Claude fixed PORT_LOG (port/wii/log.c) to
fsync after each line: libfat keeps writes in its cache otherwise and the log was empty.
Add to test_threads.c, run after T4 inside the same selftest, each as game-style threads (osCreateThread/osStartThread):
T5: osViSetEvent(&q, msg, 1); a pri-20 thread counts messages received for 1 s of osGetTime: needs >= 50.
T6: one-shot osSetTimer 100 ms: fires after 90-150 ms (osGetTime); periodic 50 ms timer: 18-22 fires in 1 s.
T7: osGetTime advances 46.875e6 +-5 % across 1 s measured with libogc gettime()/ticks_to_millisecs in a port helper.
Log "T5 ok"/"T5 FAIL <values>" etc. If a test fails, fix os_events.c / os_sched.c (not the test thresholds).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t06b4 && grep -a "T[0-9]" .strata/scratch/t06b4/sd/drmario/log.txt; for t in T1 T2 T3 T4 T5 T6 T7; do grep -aq "$t ok" .strata/scratch/t06b4/sd/drmario/log.txt || exit 1; done; make -f Makefile.wii clean >/dev/null; make -f Makefile.wii >/dev/null
