# Task 07ca: C versions of calcsub.s functions, part a of 4
Functions this task: waitEQ4 sqrtS sqrtL get_angle sinL cosL
They are in src/main_segment/calcsub.s lines 30-674 ONLY (MIPS assembly; read just that range, in two reads of at
most 260 lines). C prototypes: include/calcsub.h. Types: include/libultra.h / lib/ultralib/include/PR/gbi.h (Mtx).
1. Append exact C equivalents (same names, signatures, fixed-point formats and rounding) to port/game/calcsub.c
   (create it with the includes if it does not exist; it is in WII_SRCS, add it if missing). Do not touch functions
   other parts own.
2. Delete the stub versions of exactly these functions from port/ultra/os_stub.c.
3. Write port/tests/test_calcsub_a.c (built with the host gcc) that calls each scalar function on 3 inputs and
   checks results you derive by hand from the assembly (comment the derivation). For functions that fill an N64 Mtx
   (packed s16 integer/fraction halves; the host is little-endian, the Wii big-endian), test the float/fixed values via
   the same struct fields the C code writes, not raw bytes.
GATE: make -f Makefile.wii >/dev/null && gcc -Iinclude -Iport/include -Ilib/ultralib/include -DTARGET_WII -DHOST_TEST -o .strata/scratch/tca port/tests/test_calcsub_a.c port/game/calcsub.c -lm && .strata/scratch/tca
