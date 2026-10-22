# Task 21: four controllers -> four N64 ports (pad scan bug) + multi-port input script
Ground truth (Claude, 2026-10-21):
- port/wii/pad_wii.c port_pad_scan treats `pads = PAD_ScanPads()` as a COUNT and loops `i < pads`; libogc returns a
  BITMASK of connected GameCube pads (bit i = port i; see ~/devkitpro/libogc/include/ogc/pad.h). Wiimotes are assumed
  to be channels 0..wpads-1 instead of probed per channel. And a GC pad overwrites a Wiimote on the same port.
Do this:
1. port_pad_scan: for each channel i in 0..3: wiimote present = (WPAD_Probe(i, &type) == WPAD_ERR_NONE);
   gc present = (pad_mask >> i) & 1 where pad_mask = PAD_ScanPads(). If both are present on one port, OR the N64
   buttons and use the stick with the larger magnitude. present = wiimote || gc. Keep the existing mapping helpers.
   Copy exact libogc prototypes/constants from the headers (never guess types).
2. Host test (port/tests/test_pad_map.c, -DPAD_HOST_TEST): add a test of the presence logic, factored into a pure helper
   `int port_pad_merge(int wii_present, unsigned wii_btn, int gc_present, unsigned gc_btn, signed char gcx,
   signed char gcy, PortPad *out)`: mask 0b0110 -> ports 1,2 present, 0,3 absent; both on one port -> buttons ORed.
3. Input script (port/ultra/os_cont.c script_parse): optional 5th column = port number 1..4 (default 1), e.g.
   "300 1000 0 0 2" presses START on controller 2 at frame 300. Each port keeps its own "current line"; a port that
   appears in the script is reported present. Lines for different ports may be interleaved in frame order.
   Keep old 4-column scripts working unchanged (t14d_in/input.txt must still reach main_no=5).
4. Log once at boot of the script: "input script loaded N lines ports=%x" (bit per port used).
GATE: gcc -O1 -DPAD_HOST_TEST -Iport/include -o .strata/scratch/tpad port/tests/test_pad_map.c port/wii/pad_wii.c && .strata/scratch/tpad | grep -q "pad map OK" && grep -q "port_pad_merge" port/tests/test_pad_map.c && make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && printf '300 1000 0 0 1\n306 0 0 0 1\n310 1000 0 0 3\n316 0 0 0 3\n' > .strata/scratch/t21in.txt && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t21in.txt >/dev/null 2>&1 && DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t21 >/dev/null; grep -a "input script loaded" .strata/scratch/t21/sd/drmario/log.txt | grep -aq "ports=5" && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t21b >/dev/null; grep -aq "main_no=5" .strata/scratch/t21b/sd/drmario/log.txt
