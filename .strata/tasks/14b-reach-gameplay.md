# Task 14b: get from the main menu into a 1-player game with the input script, and make it render
START now takes the title to MAIN_MENU (main_no=6). Next goal: MAIN_GAME (main_no=5, include/main.h) - a 1P game with
pills and viruses on screen. Input script format (os_cont.c, PORT_INPUT_SCRIPT, embedded from a file):
"<frame> <hex N64 buttons> <stick_x> <stick_y>", held until the next line. N64 bits: A=8000 B=4000 Z=2000 START=1000
U_JPAD=0800 D_JPAD=0400 L_JPAD=0200 R_JPAD=0100.
1. Write .strata/scratch/t14b_in/input.txt: START presses as in task 13 to leave the title, then from frame 900 press A
   (8000) for 6 frames every 90 frames up to frame 3600 (menus confirm with A; defaults are 1P / level 0 / speed).
   Read src/main_segment/main_menu.c to check which buttons each menu screen takes and adjust the script.
2. Make sure the "mp loop main_no=" log (src/main_segment/main.c) prints every main_no change (not only the first 12).
3. Once main_no=5 is reached, look at the frames from the gameplay part (imgstat: distinct colours) and log any
   unhandled display-list opcodes the renderer sees in-game (gfx.c default counter: log op values seen during main_no 5
   once each). Fix rendering gaps you find with small changes; list what is still wrong in NOTES.md.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14b_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t14b >/dev/null; grep -aq "main_no=5" .strata/scratch/t14b/sd/drmario/log.txt && .strata/imgmax.py .strata/scratch/t14b --min-colors 64
