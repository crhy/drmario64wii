# Task 20: tiled images are assembled from misplaced pieces (magnifier, score panel labels, title trees)
Ground truth (Claude, compared with screenshots of the original rhY supplied, 2026-10-21):
- The 1P magnifier (bottom-left) must be a round glass lens with a silver rim and handle; ours is horizontal bands of the
  texture shifted against each other with black/white stripes. The score panel is missing its "SCORE" label and clips
  "TIME". The title trees/hills have blocky stair-stepped edges. All are drawn with tiStretchTexTile (tex_func.c):
  gDPLoadTextureTile(img, fmt, siz, width=colorTexW, uls=lx, ult=top, lrs, lrt) -> SETTIMG, SETTILE, LOADTILE(uls,ult,
  lrs,lrt), SETTILESIZE(render tile, uls<<2, ult<<2, lrs<<2, lrt<<2), then gSPScisTextureRectangle with s,t in ABSOLUTE
  texture coordinates (S10.5).
Bugs to fix (all three, nothing else):
1. port/gfx/gfx_tex.c texrect (~line 555): `s0 = tr_s/32` is used as-is. The RDP samples texel (s - tile.uls/4,
   t - tile.ult/4) of what is in TMEM. Subtract the render tile's origin: s0 = tr_s/32 - t->uls/4.0, t0 = tr_t/32 -
   t->ult/4.0 (tile uls/ult are 10.2 fixed point). Same for triangles (~line 438): `uls = (float)t->uls` must be
   t->uls/4.0f (and ult). Keep the alias (bind_alias) branch working.
2. port/gfx/gfx_tex.c (~line 562) `x1 += 1; y1 += 1` is applied for ALL cycle types. On the N64 only COPY and FILL
   cycle rects are inclusive; 1-cycle and 2-cycle texrects are exclusive. Apply +1 only when cycle is COPY (2) or FILL (3).
   (Task 19's seam fix must stay: the background CopyTexBlock16 strips are COPY cycle.)
3. port/tests/test_gfx_tmem.c (host test of port/gfx/gfx_tmem.c) has regressed to "GFX_TMEM FAIL (4)":
   blk.r1.l0/l1(swapped) (LOADBLOCK odd-row 32-bit word swap) and tlut.at_0x802 / tlut.at_0x81e. Read the test's
   expectations (they were passing in task 09a) and fix gfx_tmem.c, NOT the test.
Add a host test case to test_gfx_tmem.c for LOADTILE with uls/ult != 0 (64x16 RGBA16 source with texel = y*64+x,
LOADTILE uls=32 ult=4 lrs=47 lrt=7, check the TMEM row 0 starts with texel (32,4) and row 1 with (32,5)).
GATE: gcc -Iinclude -Iport/include -Iport/gfx -Ilib/ultralib/include -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST -o .strata/scratch/ttm port/tests/test_gfx_tmem.c port/gfx/gfx_tmem.c && .strata/scratch/ttm | tail -1 | grep -q "GFX_TMEM PASS" && grep -q "uls=32\|uls 32\|0x20" port/tests/test_gfx_tmem.c && make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t20 >/dev/null; ! grep -aq CRASH .strata/scratch/t20/sd/drmario/log.txt && python3 -c "from PIL import Image, ImageStat as S; t=Image.open('.strata/scratch/t20/framedump_3000.png'); sat=S.Stat(t.convert('HSV').split()[1]).mean[0]; px=t.convert('L').tobytes(); nb=sum(1 for p in px if p>16)/len(px); w,h=t.size; L=t.convert('L'); rows=sum(1 for y in range(h) if max(L.crop((0,y,w,y+1)).tobytes())<=16); print('sat',sat,'nonblack',nb,'rows',rows); assert sat>60 and nb>0.6 and rows==0"
