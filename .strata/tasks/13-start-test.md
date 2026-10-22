# Task 13: START leaves the title screen (re-test after the speed fixes 12b/12d/12e)
Task 11c moved graphics onto an RSP LWP, but the title still ran at ~16 fps and the main game thread (id 3,
joyProcCore) starved: the single Wii CPU was busy rendering and SD-logging. After 12b/12d/12e, re-run the START test.
If it still fails: read 'rsp: ... fps=' and the 'pump cb=... cur= ready=' lines, find what uses the CPU (add a
per-thread run-time counter in os_sched.c switch_to_next_locked, logged every 600 frames as 'sched time id=ms ...'),
fix the cause, and write findings to .strata/NOTES.md. Do not lower the bar of the test.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t11in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t13 >/dev/null; L=.strata/scratch/t13/sd/drmario/log.txt; grep -a "input script loaded" $L && grep -aq "input script loaded [1-9]" $L && grep -aq "input press" $L && grep -a "main_no=" $L | grep -av "main_no=[03]$" | grep -a "main_no=" && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t13_reg >/dev/null && .strata/imgmax.py .strata/scratch/t13_reg --min-colors 128
