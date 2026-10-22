# Task 12g: keep the N64 build matching, safer Wii compile flags
1. src/main_segment/audio/music_driver.c line ~265 has `PORT_LOG("drpre2");` outside any `#ifdef TARGET_WII`
   (and check its `#include "port_log.h"`): that breaks the original N64 build. Guard every port-only line in src/,
   lib/ and include/ with #ifdef TARGET_WII (`git diff src lib include` shows them all).
2. Makefile.wii CFLAGS: add -fno-strict-aliasing (decompiled IDO-era code type-puns freely; GCC -O2 strict aliasing
   can silently miscompile it) and -fno-common.
3. Verify the N64 ROM still builds byte-identical: `make` must print the "OK" checksum line for
   build/us/drmario64.us.z64.
GATE: make 2>&1 | tail -3 | grep -q ": OK" && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12g >/dev/null && .strata/imgmax.py .strata/scratch/t12g --min-colors 128
