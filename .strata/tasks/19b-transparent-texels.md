# Task 19b: transparent texels are drawn OPAQUE (dark/grey boxes behind sprites)
Ground truth (Claude looked at the frames; 14d input; 824x480 dumps):
- Settings menu (framedump_390.png): the cursor glove is drawn on a solid dark square (dump x 452-480, y 190-206 is
  pure black); the LOW / MED / HI and FEVER/CHILL/CUBE/OFF option plates show black or olive rectangles instead of the
  wooden plates (LOW plate region x 400-490, y 165-215 mean ~(11,15,9)).
- Gameplay (framedump_3000.png): dark rectangles behind the GAME OVER letters and behind parts of the panels.
- The glove (from task 19 trace): CI8 texture (fmt=2 siz=1) with an RGBA16 palette (othermode H TEXTLUT=2 =
  G_TT_RGBA16, gbi.h). Its transparent palette entries have alpha bit 0. The DECODE IS CORRECT (do NOT change the TLUT
  decode in gfx_texconv.c: tlut 2 = RGBA16, 3 = IA16, there is no IA8 TLUT). The problem is that texel alpha 0 is not
  discarded/blended for these draws.
Find for these texrects (debug log gfx_tex.c ~line 587, main_no 6): cycle type, render mode (othermode L), alpha
compare (G_AC_THRESHOLD / G_AC_DITHER, othermode L bits 0-1), CVG_X_ALPHA / ALPHA_CVG_SEL, FORCE_BL and blender mux,
and the combiner alpha. N64 rules to implement: in COPY cycle with G_AC_THRESHOLD the RDP discards texels with alpha 0
(in COPY mode alpha compare uses texel alpha directly); in 1/2-cycle, alpha compare THRESHOLD compares combined alpha
against blend-colour alpha; CVG_X_ALPHA+ALPHA_CVG_SEL makes alpha act as coverage (alpha 0 = no pixel). Map these to
GX_SetAlphaCompare (+ GX_SetZCompLoc(GX_FALSE) when alpha compare is on) and GX blending. Write the cause to NOTES.md.
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t19b >/dev/null; ! grep -aq CRASH .strata/scratch/t19b/sd/drmario/log.txt && python3 -c "from PIL import Image, ImageStat as S; m=Image.open('.strata/scratch/t19b/framedump_390.png').convert('RGB'); low=S.Stat(m.crop((400,165,490,215))).mean; gl=S.Stat(m.crop((452,190,480,206)).convert('L')); t=Image.open('.strata/scratch/t19b/framedump_3000.png'); sat=S.Stat(t.convert('HSV').split()[1]).mean[0]; w,h=t.size; L=t.convert('L'); rows=sum(1 for y in range(h) if max(L.crop((0,y,w,y+1)).tobytes())<=16); box=S.Stat(Image.open('.strata/scratch/t19b/framedump_420.png').convert('RGB').crop((580,128,670,148))).mean; print('low',low,'glove',gl.mean,gl.stddev,'sat',sat,'rows',rows,'box',box); assert sum(low)/3>60 and gl.stddev[0]>8 and sat>60 and rows==0 and min(box)>30"
