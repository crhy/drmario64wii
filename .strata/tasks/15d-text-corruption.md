# Task 15d: find and fix what overwrites game CODE during the menu fade (renderer freezes, game logic continues)
Ground truth (Claude, 2026-10-21, build = state after 15c4):
- From the 1P settings menu fade-in on, the picture never updates again (last frame = washed-out menu) while game logic,
  input and audio continue into main_no=5. Same frozen frame was already the last frame in the 14d run.
- dolphin.log: ~10000x "IntCPU: Unknown instruction 009fe001 at PC = 8000951c ... LR = 80007a70" (also 00000000 at
  80009518). addr2line: 0x80009518 = tm_loadtile (port/gfx/gfx_tmem.c), LR = gfx_run_task. So the CODE of tm_loadtile
  in .text has been OVERWRITTEN with data; the RSP LWP (sole GX owner) then runs garbage and never presents again.
- Likely mechanism: a write through a near-NULL / PHYSICAL address. N64 code converts pointers with K0_TO_PHYS-style
  macros (addr & 0x1FFFFFFF), so the renderer / naudio receive 0x00xxxxxx physical addresses. Dolphin (no MMU
  emulation) maps 0x00xxxxxx onto the same RAM as 0x80xxxxxx, so a bad physical write lands in our .text (0x80004000+).
Do this:
1. Canary (under #ifdef PORT_TEXT_CANARY, off by default): at boot compute a 32-bit checksum of the .text range
   (linker symbols: use `extern char __text_start[], __text_end[];` or `_stext`/`_etext` - check with powerpc-eabi-nm
   which ones the devkitPPC link script defines) and store it. After EVERY gfx task (os_rsp.c, after gfx_run_task) and
   EVERY audio task (port_audio_task_run, after naudio_run) and after every osPiStartDma/osEPiStartDma copy
   (os_stub.c), re-check only the 4 KB page around 0x80009000 cheaply (keep a copy of that page and memcmp) ; on the first
   mismatch log ONCE "canary: text changed after <gfx task N | audio task N | pi dma dev=%x dram=%p size=%x>" plus the
   first differing offset, then dump 8 words of the new contents. Build with -DPORT_TEXT_CANARY and run the 14d input.
2. From that log find the writer and fix it at the source. Expected kind of fix: every place that turns a game address
   into a pointer (gfx_resolve in port/gfx/gfx.c, raddr/resolve in port/audio/naudio.c, osPiStartDma/osEPiStartDma
   dramAddr in os_stub.c, any texture/TLUT/framebuffer address in port/gfx/*) must map a physical address
   0x00000000-0x017FFFFF to 0x80000000|addr (K0), keep 0x80xxxxxx/0x90xxxxxx as is, resolve segmented 0x0Sxxxxxx
   through the segment table, and REJECT (log once, skip the write) anything that would land below the game's own data,
   i.e. inside [__text_start, __text_end). Write down in NOTES.md which call was the culprit.
3. Leave the canary code in (off by default).
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t15d >/dev/null; L=.strata/scratch/t15d/sd/drmario/log.txt; ! grep -aq CRASH $L && ! grep -q "Unknown instruction\|ISI exception\|DSI exception" .strata/scratch/t15d/dolphin.log && grep -aq "main_no=5" $L && [ $(ls .strata/scratch/t15d/framedump_*.png | wc -l) -ge 120 ] && .strata/imgmax.py .strata/scratch/t15d --min-colors 64
