# Task 25: event-synced input scripts (same presses hit the same game state on the port AND the original N64)
Why (Claude): the reference renderer (port/tools/refemu/run-ref.sh: original ROM in mupen64plus+angrylion, exact
framebuffer dumps) replays our input scripts, but frame numbers count from power-on and the port boots/loads faster than
the N64, so presses land in different scenes and the runs diverge (port in the attract demo while the N64 is in menus).
Add a WAIT directive understood by both players:
  "W <n>"  - wait until main_no == n (enum_main_no, include/main.h). Frame numbers on the following lines are counted
             from the frame the wait completed. Lines before the first W keep absolute frames (old scripts unchanged).
1. Port (port/ultra/os_cont.c script player): implement W. Log "script: W %d done f=%u" each time a wait completes.
2. Reference plugin (port/tools/refemu/input_script.c, a mupen64plus input plugin; build: gcc -O2 -fPIC -shared
   -I/home/rhy/drmariowii/refemu/mupen64plus-core/src/api -o /home/rhy/drmariowii/refemu/input-script/mupen64plus-input-script.so
   port/tools/refemu/input_script.c -ldl): implement the same W by reading main_no from N64 RAM every frame with
   g_rd32(main_no_addr) (main_no_addr = env M64_MAIN_NO_ADDR, default 0x800EBCF0 from build/us/drmario64.us.map).
   Print "script: W %d done f=%u" to stderr at the same moments. Make run-ref.sh rebuild the plugin when the .c is newer.
3. Write .strata/scratch/sync/1p.txt: the 14d 1P path (title -> menu -> 1P Classic game) in W form: W 3, press START
   (and a second START) relative to it, W 6, the menu presses relative to that, W 5, then nothing.
GATE: gcc -O2 -fPIC -shared -I/home/rhy/drmariowii/refemu/mupen64plus-core/src/api -o /home/rhy/drmariowii/refemu/input-script/mupen64plus-input-script.so port/tools/refemu/input_script.c -ldl && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/sync/1p.txt >/dev/null 2>&1 && DR_43=1 .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/sync/port >/dev/null; grep -aq "script: W 5 done" .strata/scratch/sync/port/sd/drmario/log.txt && port/tools/refemu/run-ref.sh .strata/scratch/sync/1p.txt $(python3 -c "print(','.join(str(f) for f in range(30,5400,3)))") .strata/scratch/sync/ref 1200 && grep -q "script: W 5 done" .strata/scratch/sync/ref/run.log && python3 port/tools/refcompare.py .strata/scratch/sync/port .strata/scratch/sync/ref .strata/scratch/sync/cmp --window 120 && python3 -c "import statistics as s; d=[float(l.split()[2]) for l in open('.strata/scratch/sync/cmp/report.txt').readlines()[1:]]; m=s.median(d); print('median',m); assert m < 35"
