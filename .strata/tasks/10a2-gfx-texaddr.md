# Task 10a2: textures decode as noise: fix the texture source address and line units
Facts (Claude checked the 10a dump in .strata/scratch/t10a/sd/drmario/tex/):
- index.txt shows addr=00000000 for EVERY texture: G_SETTIMG's address is not resolved/stored (or the dump prints the
  wrong field). The decoded 80x50 CI4 texture (001_2_0_80x50.ppm) is random-looking noise, so the load reads the wrong
  memory.
- NOTES.md: gfx_texconv uses tile line in 64-bit words (stride line*8) but gfx_tmem.c LOADTILE uses line*4. In the RDP,
  tile `line` is in 64-bit words everywhere; make both use 8-byte units.
Fix: G_SETTIMG must resolve its address with the walker's segment resolver (direct if >= 0x80000000, else segment base +
offset) and keep it in the state; LOADBLOCK/LOADTILE/LOADTLUT must copy from that resolved DRAM address; the dump index
must print it. Re-check LOADBLOCK texel counting (lrs = number of texels - 1, in the load's siz units; 4-bit textures
are loaded as 16-bit by the gDPLoadTextureBlock_4b macro: see gbi.h) and CI palette lookup.
Check with .strata/texcheck.py (share of the most common colour; real art >= 30 %, noise < 15 %).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_TEXDUMP >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t10a2 >/dev/null && ! grep -q "addr=00000000" .strata/scratch/t10a2/sd/drmario/tex/index.txt && .strata/texcheck.py .strata/scratch/t10a2/sd/drmario/tex && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null
