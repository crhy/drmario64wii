# Task 10a4: textures are recognisable but rows interleave/shear
After 10a2 the dumped textures show real shapes (Claude viewed 001_2_0_80x50.ppm: CI4 80x50, a drawing whose rows are
interleaved/sheared, like alternate rows coming from the wrong place). Check, in this order, against gbi.h and the RDP rules:
1. Odd-row swap: only data loaded with G_LOADBLOCK is swapped, by 32-bit words within each 64-bit TMEM word, on odd
   ROWS of the TILE as the texture is read (row = t, using the tile's line), not on odd 64-bit words. 4-bit textures
   loaded with gDPLoadTextureBlock_4b are loaded as 16-bit texels: the row length in TMEM is line*8 bytes.
2. Tile line stride: line is in 64-bit words; a CI4 80-wide row is 40 bytes -> line 5 (40/8). Row y starts at
   tmem + y*line*8.
3. CI4 nibble order: high nibble is the first (left) texel. Palette index = (tile palette << 4) | nibble; palette entry is
   16-bit big-endian at TMEM 0x800 + index*8? (check: TLUT entries are quadrupled in real TMEM; your LOADTLUT layout and
   lookup must agree with each other).
Use .strata/texrows.py (vertical coherence; real art >= 0.6 median) and .strata/texcheck.py on a -DPORT_TEXDUMP run.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_TEXDUMP >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t10a4 >/dev/null && .strata/texcheck.py .strata/scratch/t10a4/sd/drmario/tex >/dev/null && .strata/texrows.py .strata/scratch/t10a4/sd/drmario/tex --min 0.6 && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null
