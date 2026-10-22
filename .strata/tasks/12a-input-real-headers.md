# Task 12a: controller code must use libogc's real button constants
port/ultra/os_cont.c re-declares libogc's WPAD/PAD API and button bits by hand, and several are WRONG (checked against
~/devkitpro/libogc/include/wiiuse/wpad.h): WPAD_BUTTON_LEFT is 0x0100, RIGHT 0x0200, DOWN 0x0400, UP 0x0800, PLUS 0x1000
(os_cont.c has UP=0x0100, DOWN=0x0200, LEFT=0x0400, RIGHT=0x0800, PLUS=0x0020), CLASSIC UP is 0x0001 (os_cont.c says
0x1000, the same as MINUS) and CLASSIC FULL_R (0x0200) is missing. Hand-copied structs for WPAD_Expansion are also an
ABI risk. Fix it structurally:
1. New file port/wii/pad_wii.c that includes <gccore.h> and <wiiuse/wpad.h> (real headers, real constants, real
   expansion_t) and exports a plain-C interface declared in new port/include/pad_wii.h, e.g.
   `typedef struct { int present; unsigned short n64_buttons; signed char stick_x, stick_y; } PortPad;`
   `void port_pad_scan(PortPad out[4]);` — it does WPAD_ScanPads/PAD_ScanPads and ALL the mapping to N64 bits
   (sideways Wiimote, Classic, GameCube, exactly as described in .strata/tasks/11-wii-input.md). Use the N64 bit values
   from include/PR/os_cont.h, copied as #defines with a comment (pad_wii.c must not include ultra headers).
2. os_cont.c: delete every hand-declared WPAD/PAD prototype, enum and struct; call port_pad_scan from port_cont_scan.
   Keep the input-script code. Add pad_wii.c to Makefile.wii.
3. Add port/tests/test_pad_map.c (host test, built with plain gcc, with a stub that feeds fake held-button words to the
   mapping function — split the pure mapping into `unsigned short port_map_wiimote_sideways(unsigned held)`,
   `port_map_classic(unsigned btns)`, `port_map_gc(unsigned btns)` in pad_wii.c behind #ifndef PAD_HOST_TEST for the
   libogc parts) that checks (sideways Wiimote, real bits: Wii Up -> N64 D-Left, Down -> D-Right,
   Left -> D-Down, Right -> D-Up), 2->A, 1->B, PLUS(0x1000)->START, MINUS->Z; Classic UP(0x0001)->D-up,
   FULL_R(0x0200)->R, PLUS(0x0400)->START; GC START(0x1000)->START. Print "pad map OK".
GATE: gcc -DPAD_HOST_TEST -Iport/include port/tests/test_pad_map.c port/wii/pad_wii.c -o .strata/scratch/t12a_host && .strata/scratch/t12a_host | grep -q "pad map OK" && ! grep -nE "WPAD_BUTTON_UP *= *0x|enum *\{ *WPAD" port/ultra/os_cont.c && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12a >/dev/null && .strata/imgmax.py .strata/scratch/t12a --min-colors 128
