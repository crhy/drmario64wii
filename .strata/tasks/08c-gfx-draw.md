# Task 08c: draw with GX (untextured)
Background: the game runs (music synthesis off via -DPORT_NO_SYNTH, leave it). osSpTaskStartGo in port/ultra/os_sp.c gets ~60 M_GFXTASK/s. Presentation is port_present() in port/wii/main.c (copies EFB -> the port's own XFBs; NEVER write the game's fb). Full design of the renderer: .strata/scratch/08-gfx-core-full.md (read it once). Opcode values/bit layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 (grep, never guess).
port/gfx/gfx.c walks lists, port/gfx/gfx_xform.c keeps vertex/matrix/colour state. Add port/gfx/gfx_gx.c (+WII_SRCS),
at most ~350 lines:
- G_TRI1, G_TRI2, G_QUAD: draw with GX in screen space: ortho projection 0..320 x 0..240 into the 640x480 EFB,
  per-vertex colour (shade) when G_SHADE is set, else prim colour. Honour G_CULL_BACK/FRONT. No textures yet.
- G_FILLRECT (cycle type FILL: colour from G_SETFILLCOLOR, RGBA5551 packed twice; coords are 10.2 fixed, inclusive in
  fill mode) and G_SETSCISSOR -> GX_SetScissor scaled x2.
- Z: GX_SetZMode from geometry mode G_ZBUFFER with compare LEQUAL; clear Z at frame start.
- Remove the colour-cycling clear from port_present() in port/wii/main.c (keep the EFB->XFB copy there).
Use $DEVKITPRO/examples/wii/graphics/gx/triangle/source/triangle.c for GX vertex-format setup (GX_VTXFMT0 with
POS xy f32, CLR0 rgba8).
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 45 .strata/scratch/t08c && .strata/imgstat.py .strata/scratch/t08c/last.png --min-colors 3 --min-nonblack 5
