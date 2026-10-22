# Task 04: compile the game C code for PowerPC (objects only)
Add to Makefile.wii a `GAME_SRCS` list: all .c under src/boot, src/main_segment (recursive, incl. audio, graphics,
screen_print; EXCLUDE src/main_segment/unused), src/gzip, src/buffers, src/libnustd, src/libegcs, src/dma_table,
and lib/libmus/src (recursive). Add target `objs` that compiles them to build/wii/obj/... with the Wii CFLAGS plus
`-DTARGET_WII -DVERSION_US -DNON_MATCHING -DAVOID_UB -fno-strict-aliasing -fno-common -w -Iinclude -Iport/include`
plus whatever -I the N64 Makefile uses (grep `IINC` / `-I` in Makefile). `objs` must NOT link.
Make every file compile. Fix errors ONLY by: adding Wii versions of headers under port/include/ (searched first)
or `#ifdef TARGET_WII` blocks in include/ or src/. Typical problems: IDO/MIPS inline asm or `__asm__` in headers,
`GLOBAL_ASM`/`INCLUDE_ASM` macros (make them expand to nothing on Wii), libultra header types clashing with libogc
(do NOT include libogc headers from game code), `#pragma` for IDO. Keep the N64 build matching: after your
changes `make` must still print `build/us/drmario64.us.z64: OK`.
Record in .strata/NOTES.md under "Wii compile" every header/ifdef change you made (one line each).
GATE: make -f Makefile.wii objs -k -j8 && make -j8 2>&1 | tail -1 | grep -q "drmario64.us.z64: OK"
