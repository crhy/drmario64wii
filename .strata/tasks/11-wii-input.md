# Task 11: controls (sideways Wiimote, Classic Controller, GameCube pad)
The game reads controllers through libultra osContInit / osContStartQuery / osContGetQuery / osContStartReadData /
osContGetReadData (see src/main_segment/joy.c and NOTES.md). Implement them in port/ultra/os_cont.c (remove the stubs from
os_stub.c): report 4 ports with the controller present when a Wiimote/GC pad is connected (port 1 always present),
send the SI completion message to the queue passed in (as the stubs do now), and fill OSContPad {button, stick_x, stick_y}.
Call WPAD_ScanPads / PAD_ScanPads once per VI retrace in the event pump (not from game threads), cache the state.
Mapping (N64 bits from include/PR/os_cont.h):
- Wiimote held SIDEWAYS (D-pad on the left): Wii D-pad Up -> N64 D-Left, Down -> D-Right, Left -> D-Down, Right -> D-Up;
  also drive stick_x/stick_y = +-80 from the same direction (menus read the stick). 2 -> A, 1 -> B, + -> START,
  - -> Z, A -> L, B -> R.
- Classic Controller: D-pad/left stick -> D-pad/stick, a -> A, b -> B, + -> START, ZL -> Z, L -> L, R -> R, y -> C-left,
  x -> C-up.
- GameCube pad: A -> A, B -> B, START -> START, Z -> Z, L/R -> L/R, stick -> stick, D-pad -> D-pad, C-stick -> C buttons.
Add a debug input script: when built with -DPORT_INPUT_SCRIPT, port 1 replays sd:/drmario/input.txt lines
"<frame> <hex buttons> <stick_x> <stick_y>" (held until the next line) so tests can press START without a real pad.
Test: an input script pressing START at frames 300 and 420 must make the game leave the title screen (log "main_no=" changes
via the existing mp loop log, or the frame at 600 differs strongly from the title frame).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT >/dev/null && mkdir -p .strata/scratch/t11in && printf '300 1000 0 0\n310 0 0 0\n420 1000 0 0\n430 0 0 0\n' > .strata/scratch/t11in/input.txt && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t11 >/dev/null; grep -a "input script" .strata/scratch/t11/sd/drmario/log.txt
NOTE (Claude): the 11 GATE needs the SD card to contain input.txt; dolphin-run.sh copies nothing in. Implement reading the script from the DOL's embedded data instead if SD seeding is not possible: build-time embed .strata/scratch/t11in/input.txt via Makefile.wii when PORT_INPUT_SCRIPT=path is given. Log 'input script loaded N lines' and log every press.
