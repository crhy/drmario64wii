# Task 10e: near-plane clipping (and depth) for triangles
State: port/gfx/gfx_xform.c xf_vtx stores per vertex cx,cy,cz (clip, 16.16) and screen sx,sy computed with a perspective
divide (Claude fixed the transform: N64 row vectors, clip = v*(MV*P)). Triangles with a vertex at w <= 0 project to huge
screen coordinates and smear across the frame (.strata/scratch/m45/framedump_660.png).
1. Store cw too (gfx.h vertex struct). Move the viewport/projection step into one helper gfx_project(clip -> sx, sy, z).
2. In gfx_draw_tri (gfx_gx.c) and gfx_tex_draw_tri (gfx_tex.c): if all three w > W_EPS (W_EPS = 1/256 in 16.16 -> 256),
   draw as now. If all are <= W_EPS, skip. Otherwise clip the triangle against the plane w = W_EPS (Sutherland-Hodgman,
   one plane): new vertices interpolate clip coords, colour (each channel) and s,t linearly by
   t = (W_EPS - w0) / (w1 - w0); project the resulting 3 or 4 vertices and draw a triangle fan.
3. Depth: z_screen = cz / cw mapped from [-1,1] to the GX ortho depth range the renderer uses; only when G_ZBUFFER is set
   use GX_SetZMode(GX_TRUE, GX_LEQUAL, update from othermode); otherwise depth test off. Use POS XYZ instead of XY.
4. Also drop triangles whose projected bbox exceeds +-4096 pixels (safety net).
Put the clipping in port/gfx/gfx_clip.c (+WII_SRCS), pure C (no GX) so it can be host-tested. Host test port/tests/test_gfx_clip.c: one triangle crossing the plane gives 4 output vertices with w >= W_EPS and the
interpolated colour/st are correct; a triangle fully behind gives 0.
GATE: make -f Makefile.wii >/dev/null && gcc -Iinclude -Iport/include -Iport/gfx -Ilib/ultralib/include -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST -o .strata/scratch/tclip port/tests/test_gfx_clip.c port/gfx/gfx_clip.c -lm && .strata/scratch/tclip && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t10e >/dev/null && .strata/imgmax.py .strata/scratch/t10e --min-colors 256
