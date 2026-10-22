# Task 18b: keep the off-screen render on the GPU (works with Dolphin's DEFAULT settings and on real hardware)
Ground truth (Claude): Dolphin's default "Store EFB Copies to Texture Only" (EFBToTextureEnable=True) never writes EFB
copies into emulated RAM, so the 18 design (GX_CopyTex -> CPU converts into bg_snap_buf) reads zeros in a default Dolphin.
(With EFBToTextureEnable=False it works - proved.) Players use defaults, so do not depend on RAM readback.
Requires 18a (the redraw path works with a CPU test pattern).
Design (port/gfx/gfx_xform.c gfx_rtt_flush + port/gfx/gfx_tex.c bind):
1. At flush: GX_DrawDone(); GX_SetTexCopySrc(0,0, 2*w, 480); GX_SetTexCopyDst(2*w, 480, GX_TF_RGB565, GX_FALSE);
   GX_CopyTex(alias_buf, GX_FALSE); GX_PixModeSync(); where alias_buf is ONE 32-byte-aligned buffer in MEM2
   (2*328*480*2 bytes ~ 630 KB, allocate once; MEM2 via SYS_GetArena2Lo/SetArena2Lo like the GX FIFO in main.c).
   GX_InitTexObj(&alias_obj, alias_buf, 2*w, 480, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE). Remember the N64 range
   [bg_snap_buf, bg_snap_buf + w*240*2) and w. Keep the RAM write-back only under #ifdef PORT_RTT_RAM.
2. Texture bind: when a texture load (SETTIMG address used by LOADBLOCK/LOADTILE) falls inside an alias range, do not
   decode TMEM. Compute the first row: top = (addr - base) / (2*w). Bind alias_obj and emit texcoords for the texrect in
   ALIAS space: u = s_texel / w, v = (t_texel + top) / 240 (s/t are the N64 tile texel coords the rect samples; the
   alias texture is 2x so normalized coords are the same). Linear filter is fine.
3. Gate on the DEFAULT harness (no test pattern): gameplay frame > 60% non-black.
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t18b >/dev/null; L=.strata/scratch/t18b/sd/drmario/log.txt; ! grep -aq CRASH $L && grep -aq "main_no=5" $L && python3 -c "from PIL import Image; px=Image.open('.strata/scratch/t18b/framedump_3000.png').convert('L').tobytes(); nb=sum(1 for p in px if p>16)/len(px); print('nonblack', nb); assert nb > 0.6" && .strata/imgmax.py .strata/scratch/t18b --min-colors 64
