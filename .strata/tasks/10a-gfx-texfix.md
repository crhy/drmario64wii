# Task 10a: fix garbled textures (texture dump tool first)
Claude looked at .strata/scratch/t09c/framedump_60.png: the first textured screen (likely a line of text on black)
renders as a single squashed noisy horizontal strip: texture rows collapse/shear. Typical causes: wrong texture width
or line stride (SETTIMG width vs tile line in 64-bit words), LOADBLOCK dxt handling, the odd-row swap applied wrong,
or texrect t/dtdy (s5.10) and copy-mode 4x dsdx scaling, or s,t not offset by the tile uls/ult (10.2 units).
1. Tool: when built with -DPORT_TEXDUMP, gfx_tex.c writes every NEW decoded texture (first 64) to
   sd:/drmario/tex/NNN_<fmt>_<siz>_<w>x<h>.ppm (binary P6, alpha dropped) plus one line in sd:/drmario/tex/index.txt:
   "NNN addr=%08x fmt siz w h tile line uls ult lrs lrt palette". Also log, for the first 32 texrects, the rect, s, t,
   dsdx, dtdy and cycle type.
2. Use the dump (sizes, and the pixels: e.g. count non-black pixels per row of a PPM with a small python script) to
   find what is wrong; text glyph textures should show distinct rows, not one smeared row. Fix it.
3. Keep -DPORT_TEXDUMP off by default (it stays available for the hi-res texture pipeline).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_TEXDUMP >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t10a >/dev/null && test $(ls .strata/scratch/t10a/sd/drmario/tex/*.ppm | wc -l) -ge 10 && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t10a2 >/dev/null && .strata/imgmax.py .strata/scratch/t10a2 --min-colors 64
