# Task 24: N64 texture wrap / mirror / clamp + masks (smeared repeating textures, e.g. title grass band and hills)
Ground truth (Claude, 2026-10-21): the title backdrop shows horizontal smears in the grass band and blocky bands on the
hills - repeating textures are drawn clamped/stretched. Found in port/gfx/gfx_tex.c (lines ~448 and ~583):
    t->cms == 0u ? GX_CLAMP : (t->cms == 1u ? GX_REPEAT : GX_MIRROR)
but gbi.h defines the cm field as BIT FLAGS: G_TX_WRAP = 0, G_TX_MIRROR = 1, G_TX_CLAMP = 2 (3 = mirror|clamp).
So wrap is drawn as clamp, mirror as repeat and clamp as mirror. Also POT padding with edge texels breaks repeats.
Implement the RDP rules (per axis s and t, with that axis' cm, mask, shift and tile size w = (lrs-uls)/4+1):
1. Mapping: clamp bit set (cm & 2) -> clamp to the tile size; else if mask != 0 -> repeat with period 2^mask texels,
   mirrored when (cm & 1); else (mask == 0, no clamp bit) -> behaves like clamp.
2. For wrapping axes the GX texture must be exactly 2^mask texels on that axis (decode 2^mask texels from TMEM with
   the tile's line stride; if the tile is smaller than 2^mask, texels repeat inside TMEM like the RDP does) and use
   GX_REPEAT or GX_MIRROR. For clamped axes keep today's path (POT padding with edge texels + GX_CLAMP).
3. shift (shifts/shiftt): coordinate scale before masking: shift 1..10 -> s >> shift, 11..15 -> s << (16-shift).
   Apply it to the texcoords you emit (texrect and triangles).
4. Keep the whole-image hint path (23b) and the 18b alias path: they are always clamped (do not change them).
5. Host test port/tests/test_gfx_wrap.c (plain gcc): a pure helper `void gfx_axis_mode(int cm, int mask, int tile_w,
   int *gx_mode, int *gx_size)` with cases: (cm=0,mask=5,w=32)->REPEAT,32; (cm=1,mask=4,w=16)->MIRROR,16;
   (cm=2,mask=5,w=20)->CLAMP,pot(20)=32; (cm=0,mask=0,w=24)->CLAMP,32; (cm=3,mask=4,w=16)->CLAMP,16. Print "wrap OK".
GATE: gcc -O1 -Iport/gfx -Iport/include -o .strata/scratch/twrap port/tests/test_gfx_wrap.c port/gfx/gfx_wrap.c 2>/dev/null || gcc -O1 -Iport/gfx -Iport/include -o .strata/scratch/twrap port/tests/test_gfx_wrap.c; .strata/scratch/twrap | grep -q "wrap OK" && make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t24 >/dev/null; python3 -c "from PIL import Image, ImageStat as S; g=Image.open('.strata/scratch/t24/framedump_1500.png'); L=g.convert('L'); px=L.tobytes(); nb=sum(1 for p in px if p>16)/len(px); sat=S.Stat(g.convert('HSV').split()[1]).mean[0]; w,h=g.size; rows=sum(1 for y in range(h) if max(L.crop((0,y,w,y+1)).tobytes())<=16); print('nb',nb,'sat',sat,'rows',rows); assert nb>0.6 and sat>60 and rows==0"
