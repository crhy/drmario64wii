# Task 12d: cheap logging, no debug I/O by default, clean boot (port/wii/log.c, main.c, sentinel.c, os_events.c)
1. log.c: PORT_LOG does fflush + fsync on EVERY line (an SD-card sector write each time) and main.c's port_present logs
   EVERY frame, so the game thread does ~60+ SD syncs a second. Change log.c to: init the mutex inside port_log_init
   (not lazily — lazy init races when two threads log first); setvbuf the FILE with a 16 KB static buffer; fflush+fsync
   only when 32 lines are pending, when 1 second (time base) has passed since the last sync, or when the line starts
   with "CRASH". Add `void port_log_flush(void)` and call it from port_crash_report (crash.c) before parking.
2. Per-frame / periodic debug lines become opt-in: wrap the port_present "present f=..." log (main.c) and the
   event pump "pump cb=..." log (os_events.c) in `#ifdef PORT_LOG_VERBOSE`.
3. sentinel.c: the VI callback samples every retrace and a priority-127 LWP opens/appends/closes sentinel.txt every
   half second, forever. Compile sentinel.c's body and the port_sentinel_sample() call (os_events.c) and the
   port_sentinel_start() call (main.c) only under `#ifdef PORT_SENTINEL`. Same for the dead XFB colour-toggle code in
   os_events.c (g_sent_xfb, bt_dc_flush) and the `if (0) port_vi_sentinel_set_xfbs` block in main.c: remove them.
4. main.c allocates both XFBs twice (main() and port_gx_setup()), leaking ~1.2 MB of MEM1, and calls VIDEO_Configure
   twice. Allocate once (in main, before port_gx_setup, which must reuse them) and configure once.
5. Per-frame debug lines added during tasks 11/11b also become opt-in under #ifdef PORT_LOG_VERBOSE: "scan gf=" in
   port/ultra/os_cont.c, "demo_title ..." in src/main_segment/main_story.c, "joycore ENTER" and "joy0 button=" in
   src/main_segment/joy.c (keep them inside their existing #ifdef TARGET_WII too).
GATE: make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12d >/dev/null && .strata/imgmax.py .strata/scratch/t12d --min-colors 128 && L=.strata/scratch/t12d/sd/drmario/log.txt && test $(grep -ac "present f=\|scan gf=\|demo_title" $L) -eq 0 && test ! -e .strata/scratch/t12d/sd/drmario/sentinel.txt && test $(grep -c "SYS_AllocateFramebuffer" port/wii/main.c) -eq 2
