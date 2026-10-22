# Task 09a: TMEM emulation and texture load commands (no drawing)
Background: port/gfx/gfx.c walks display lists through a per-opcode handler table; gfx_xform.c keeps vertex/matrix/colour state (struct in gfx.h); gfx_gx.c draws untextured tris and fill rects with GX (works: frames show the flat shapes). Full texture design: .strata/scratch/09-gfx-textures-full.md (read once). Opcodes/layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 (grep, never guess).
Write port/gfx/gfx_tmem.c + gfx_tmem.h (+WII_SRCS), at most ~300 lines: 4 KB TMEM byte array, 8 tile descriptors
(fmt, siz, line, tmem, palette, cmt/maskt/shiftt, cms/masks/shifts, uls/ult/lrs/lrt), and handlers registered in the
walker's table for G_SETTIMG (fmt, siz, width, resolved address), G_SETTILE, G_SETTILESIZE, G_LOADBLOCK (with dxt and the
odd-line 32-bit word swap), G_LOADTILE, G_LOADTLUT (palette entries into the upper half of TMEM, 16-bit each).
Expose: const u8 *gfx_tmem(void); const GfxTile *gfx_tile(int i).
Host test port/tests/test_gfx_tmem.c (gcc, -DHOST_TEST): a LOADBLOCK of a 16x2 RGBA16 texture with dxt for 32-byte lines
places row 1 swapped as the hardware does; a LOADTLUT of 16 entries lands at TMEM 0x800+.
GATE: make -f Makefile.wii >/dev/null && gcc -Iinclude -Iport/include -Iport/gfx -Ilib/ultralib/include -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST -o .strata/scratch/ttm port/tests/test_gfx_tmem.c port/gfx/gfx_tmem.c && .strata/scratch/ttm
