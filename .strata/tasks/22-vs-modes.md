# Task 22: reach a 4-player VS game and a 2-player VS game with the input script
Needs task 21 (multi-port script, 5th column = port 1..4). Harness: DR_PADS=4 plugs 4 GameCube pads into Dolphin.
1. Read src/main_segment/main_menu.c (main menu panels, MainMenuMode, how 2P/4P are chosen, how each player joins /
   confirms; joyResponseCheck() counts present controllers) and write two scripts:
   .strata/scratch/t22/in4p.txt -> from the title into a 4-player VS game (all four players confirm with A on their own
   port), .strata/scratch/t22/in2p.txt -> a 2-player VS game. Use START/A/D-pad presses of 6 frames, 60+ frames apart.
2. In src/main_segment/dm_game_main.c dm_game_main (under #ifdef TARGET_WII) log once per game start:
   "game: start players=%d gamesel=%d" using evs_playcnt and evs_gamesel.
3. Run each with DR_PADS=4 for 120 s; the game must start with the right player count and keep rendering (gameplay
   frames non-black, no CRASH). Write the scripts' menu path into NOTES.md.
GATE: make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t22/in4p.txt >/dev/null 2>&1 && DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 120 .strata/scratch/t22/r4 >/dev/null; grep -aq "game: start players=4" .strata/scratch/t22/r4/sd/drmario/log.txt && ! grep -aq CRASH .strata/scratch/t22/r4/sd/drmario/log.txt && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t22/in2p.txt >/dev/null 2>&1 && DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 120 .strata/scratch/t22/r2 >/dev/null; grep -aq "game: start players=2" .strata/scratch/t22/r2/sd/drmario/log.txt && ! grep -aq CRASH .strata/scratch/t22/r2/sd/drmario/log.txt && .strata/imgmax.py .strata/scratch/t22/r4 --min-colors 64
