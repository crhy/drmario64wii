# Task 19: three rendering defects (Claude looked at the frames; numbers below are from the 14d input run)
A. Black seams between background strips: the in-game background (CopyTexBlock16, COPY cycle, 6-row strips) has a
   1-pixel black line under every strip (20 fully black rows in framedump_3000.png). On the N64, in COPY and FILL cycle
   the texrect/fillrect lower-right edge is INCLUSIVE (the RDP draws rows yh..yl inclusive). Fix the rect extents for
   COPY and FILL cycles in port/gfx/gfx_tex.c (texrect) and port/gfx/gfx_gx.c (fillrect); 1-cycle/2-cycle stay exclusive.
B. Black box in the 1P settings menu: the Virus-level gauge (top row, EASY..HARD bar) has a solid black rectangle on its
   right part (dump pixels x 580-670, y 128-148 in the 824x480 framedump_420.png; that is N64 x ~225-260, y ~64-74).
   The real game shows the empty part of the gauge there.
C. Grey square behind the cursor glove in the same menu (dump x ~178-258, y ~236-298; N64 x ~69-100, y ~118-149): the
   glove sprite's transparent texels are drawn as flat grey (172,172,172) instead of being transparent.
How: build with the texrect/fillrect debug log (port/gfx/gfx_tex.c ~line 512, and add the same for fillrect) limited to
main_no==6 and to rects overlapping those N64 areas; log fmt/siz/TLUT mode (othermode H G_MDSA_TEXTLUT bits), combiner,
render mode, alpha compare, cycle type, prim/env colours. Typical causes: CI/IA texture alpha lost in conversion,
TLUT not applied (G_TT_RGBA16 vs G_TT_IA16), alpha compare / cvg-x-alpha not mapped, combiner using a colour source that
is 0. Fix the cause in the renderer (not by special-casing these sprites). Write causes to NOTES.md.
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t19 >/dev/null; ! grep -aq CRASH .strata/scratch/t19/sd/drmario/log.txt && python3 -c "from PIL import Image, ImageStat as S; m=Image.open('.strata/scratch/t19/framedump_420.png').convert('RGB'); box=S.Stat(m.crop((580,128,670,148))).mean; g=S.Stat(m.crop((182,240,198,254)).convert('L')); t=Image.open('.strata/scratch/t19/framedump_3000.png').convert('L'); w,h=t.size; rows=sum(1 for y in range(h) if max(t.crop((0,y,w,y+1)).tobytes())<=16); print('box',box,'glove',g.mean,g.stddev,'blackrows',rows); assert min(box)>30 and not (abs(g.mean[0]-172)<2 and g.stddev[0]<1) and rows==0"
