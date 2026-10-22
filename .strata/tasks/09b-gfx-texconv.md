# Task 09b: N64 texel formats -> RGBA8 (pure functions)
Background: port/gfx/gfx.c walks display lists through a per-opcode handler table; gfx_xform.c keeps vertex/matrix/colour state (struct in gfx.h); gfx_gx.c draws untextured tris and fill rects with GX (works: frames show the flat shapes). Full texture design: .strata/scratch/09-gfx-textures-full.md (read once). Opcodes/layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 (grep, never guess).
Write port/gfx/gfx_texconv.c + .h (+WII_SRCS), at most ~300 lines:
void gfx_decode_tile(const u8 *tmem, const GfxTile *t, int w, int h, u8 *out_rgba8 /* w*h*4, row-major RGBA */);
for RGBA16 (5551), RGBA32, IA4, IA8, IA16, I4, I8, CI4 (palette = t->palette, 16 entries), CI8 (256 entries), TLUT type
RGBA16 or IA16 (from othermode H G_MDSFT_TEXTLUT, passed as a parameter). Honour the tile line stride (t->line in 64-bit
words) and the odd-row word swap that LOADBLOCK left in TMEM (rows with odd index read with addr ^ 4 for 32-bit words).
Host test port/tests/test_gfx_texconv.c: one hand-made texel per format decodes to the expected RGBA.
GATE: make -f Makefile.wii >/dev/null && gcc -Iinclude -Iport/include -Iport/gfx -Ilib/ultralib/include -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST -o .strata/scratch/ttc port/tests/test_gfx_texconv.c port/gfx/gfx_texconv.c port/gfx/gfx_tmem.c && .strata/scratch/ttc
