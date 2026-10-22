# Task 14b1: the game freezes when the title hands over to the attract demo
Evidence (Claude, 2026-10-18): an idle run (no input) renders the title at ~59 fps, then at ~26-32 s - when the title's
lws animation ends and dm_title_main returns the demo path (var_s0 == 2 -> evs_gamesel = GSL_1DEMO..., then MAIN_GAME) -
frames stop (.strata/scratch/idle90: only 65 frame dumps in a 90 s run, idle50b: 53 in 50 s). No "CRASH" line. One run
ended the log with garbage bytes (memory corruption is possible). Task 14b saw a similar freeze right after the first A
press in the main menu. Find and fix the cause.
1. Make the "mp loop main_no=" log in src/main_segment/main.c NON-verbose again (it is a milestone, one line per mode
   change) and log every main_no change.
2. Locate the freeze: build with -DPORT_SENTINEL (sentinel.c samples the interrupted PC/LR every retrace and appends
   them to sd:/drmario/sentinel.txt every 30 retraces); map the stuck PCs with
   `$DEVKITPPC/bin/powerpc-eabi-addr2line -f -e build/wii/drmario64.elf <pc>`. Also check the crash hook
   (port/wii/crash.c) really logs a CPU exception (it must call port_log_flush()).
3. Typical causes to check: a DMA/segment load for the game scene (osPiStartDma / gzip expand into a buffer that is too
   small on Wii), a NULL segment in the display list (gfx_resolve returning NULL), a thread waiting on a message that the
   port never sends (e.g. an SP/DP or SI event), or the RSP job ring blocking.
4. Fix the cause, write it to .strata/NOTES.md.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t14b1 >/dev/null; grep -aq "main_no=5" .strata/scratch/t14b1/sd/drmario/log.txt && test $(ls .strata/scratch/t14b1 | grep -c framedump) -ge 160 && .strata/imgmax.py .strata/scratch/t14b1 --min-colors 64
