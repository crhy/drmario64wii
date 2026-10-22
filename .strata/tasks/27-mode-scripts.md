# Task 27: one verified, EVENT-SYNCED input script per game mode (for pixel comparison against the original)
Uses the 5-column input script (port 1..4) and the "W <main_no>" wait directive from task 26: every script must start with W 3 (title) and use W before each scene change, so the port and the original N64 (port/tools/refemu/run-ref.sh) press at the same game state. FIRST add a menu log (it does not exist yet): in src/main_segment/main_menu.c
(#ifdef TARGET_WII only) log every change of (all->nowMode, the active panel's cursor/select index) as
"menu: mode=%d sel=%d f=%u" (f = port_frame_now() from port/ultra/os_cont.c; MainMenuMode values in
include/main_menu.h). Build each script step by step: run, read the "menu:" log, add the next press.
Presses 6 frames long, >= 40 frames apart, wait for the previous transition (new mode in the log).
1. In dm_game_main (#ifdef TARGET_WII) extend the start log to "game: start players=%d gamesel=%d gamemode=%d
   story=%d" (evs_playcnt, evs_gamesel, evs_gamemode, evs_story_flg - see include/main1x.h for the real names).
2. Write .strata/scratch/modes/<name>.txt for each of these, each ending ~10 s into the game (no more presses):
   classic (1P Classic), story (1P Story, first stage), vscom (1P Vs.Computer), flash (1P Flash), marathon (1P
   Marathon), scoreattack (1P Score Attack), vs2p (2P Vs, port 2 joins), vs2p_flash, vs2p_scoreattack, vs4p (4P Vs,
   all four join), tutorial (if reachable from the menu), records (open the Records screen, then the Classic records
   page), options (open Options and each sub-page once).
3. Write .strata/scratch/modes/README.md: for each script the menu path as mode/sel steps and the "game: start" line
   it produces (or the final menu mode for records/options).
GATE: cd .strata/scratch/modes && for s in classic story vscom flash marathon scoreattack vs2p vs2p_flash vs2p_scoreattack vs4p records options; do [ -s $s.txt ] || { echo missing $s; exit 1; }; done; cd ../../.. && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/modes/story.txt >/dev/null 2>&1 && DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 120 .strata/scratch/modes/r_story >/dev/null; grep -a "game: start" .strata/scratch/modes/r_story/sd/drmario/log.txt | grep -aq "story=1" && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/modes/flash.txt >/dev/null 2>&1 && DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 120 .strata/scratch/modes/r_flash >/dev/null; grep -aq "game: start players=1" .strata/scratch/modes/r_flash/sd/drmario/log.txt && grep -q "story" .strata/scratch/modes/README.md && grep -aq "menu: mode=" .strata/scratch/modes/r_flash/sd/drmario/log.txt
