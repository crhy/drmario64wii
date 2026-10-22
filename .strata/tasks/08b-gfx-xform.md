# Task 08b: matrices, vertices, viewport (state only, no drawing)
Background: the game runs (music synthesis off via -DPORT_NO_SYNTH, leave it). osSpTaskStartGo in port/ultra/os_sp.c gets ~60 M_GFXTASK/s. Presentation is port_present() in port/wii/main.c (copies EFB -> the port's own XFBs; NEVER write the game's fb). Full design of the renderer: .strata/scratch/08-gfx-core-full.md (read it once). Opcode values/bit layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 (grep, never guess).
port/gfx/gfx.c (task 08a) walks display lists with a per-opcode handler table. Add port/gfx/gfx_xform.c (+WII_SRCS),
at most ~350 lines, registering handlers for: G_MTX (push/load/mul, projection vs modelview; N64 Mtx = 16 s16 integer
parts then 16 u16 fractions, row-major s15.16), G_POPMTX, G_MOVEMEM viewport (Vp: vscale/vtrans in 1/4 pixel units),
G_VTX (F3DEX2: n and v0 from w0, up to 32 vertices; Vtx = s16 x,y,z, u16 flag, s16 s,t, u8 r,g,b,a; transform by
modelview*projection at load time, store clip coords + screen x,y (N64 320x240 space after viewport) + colour + s,t),
G_GEOMETRYMODE (clear/set masks), G_TEXTURE (scale s/t, tile, on/off), G_SETOTHERMODE_H/L (store full words),
G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETFILLCOLOR, G_SETBLENDCOLOR, G_SETSCISSOR, G_SETCIMG, G_SETZIMG, and no-op handlers
for G_RDPPIPESYNC/TILESYNC/LOADSYNC/FULLSYNC. Keep the state in a struct in gfx.h.
Add a host test port/tests/test_gfx_xform.c (system gcc, -DHOST_TEST) for the Mtx decode (identity, a translation)
and the viewport transform of one vertex.
GATE: make -f Makefile.wii >/dev/null && gcc -Iinclude -Iport/include -Iport/gfx -Ilib/ultralib/include -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST -o .strata/scratch/tgx port/tests/test_gfx_xform.c port/gfx/gfx_xform.c -lm && .strata/scratch/tgx
