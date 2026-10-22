# Task 09c: textured drawing with GX
Background: port/gfx/gfx.c walks display lists through a per-opcode handler table; gfx_xform.c keeps vertex/matrix/colour state (struct in gfx.h); gfx_gx.c draws untextured tris and fill rects with GX (works: frames show the flat shapes). Full texture design: .strata/scratch/09-gfx-textures-full.md (read once). Opcodes/layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 (grep, never guess).
gfx_tmem.c (TMEM + loads) and gfx_texconv.c (decode to RGBA8) exist. Write port/gfx/gfx_tex.c (+WII_SRCS), ~350 lines:
- gfx_tex_bind(int tile): decode the tile with gfx_decode_tile, convert RGBA8 row-major to GX_TF_RGBA8 4x4-tiled layout
  (AR plane then GB plane per 4x4 block), DCFlushRange, GX_InitTexObj with wrap/mirror/clamp from the tile and point
  filtering (bilinear if othermode H TEXTFILT is G_TF_BILERP), GX_LoadTexObj to GX_TEXMAP0. Cache by (address, fmt,
  siz, w, h, CRC32 of texels + palette) in a hash table so identical N64 textures always give byte-identical GX data
  (Dolphin hi-res packs rely on it). LRU-evict above 8 MB (memalign'd buffers).
- Textured triangles (G_TEXTURE on): vertex format adds GX_VA_TEX0 f32 s,t; s,t = vertex s,t * G_TEXTURE scale / 32 /
  65536 minus tile uls/ult, divided by texture width/height; TEV: GX_MODULATE (texture * vertex colour).
- G_TEXRECT and G_TEXRECTFLIP (rect in 10.2, then two G_RDPHALF words: s,t s10.5 and dsdx,dtdy s5.10; COPY cycle type
  means dsdx is 4x): draw a textured quad, TEV GX_REPLACE in COPY mode, GX_MODULATE with prim colour otherwise.
- Switch the vertex format/TEV back for untextured draws in gfx_gx.c.
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t09c >/dev/null && .strata/imgmax.py .strata/scratch/t09c --min-colors 64
