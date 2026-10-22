# Task 18a: make the snapped-background REDRAW work (CopyTexBlock16 of a 328x240 RGBA16 RAM buffer draws nothing)
Ground truth (Claude, 2026-10-21): every gameplay frame calls dm_game_draw_snapped_bg (dm_game_main.c:7848) ->
CopyTexBlock16(bg_snap_buf, 0,0, 328, 240) (tex_func.c:270): copy_texture_init_dl (COPY cycle), then per strip of
step = 0x800/328 = 6 rows: gDPLoadTextureBlock(RGBA 16b, width 328, height 6) + gSPScisTextureRectangle(x=0,
y=top..bottom-1, tile 0, s=0, t=0, dsdx=4<<10, dtdy=1<<10). Even with bg_snap_buf FULL of picture data (proved with
Dolphin EFB-to-RAM), the gameplay frame stays black behind the sprites: this draw path is broken.
Known bug: port/gfx/gfx_tex.c:497 `if (copy) { dsdx /= 4.0f; dtdy /= 4.0f; }` - in COPY mode ONLY dsdx is in units of
4 (4 pixels per clock); dtdy is a normal 1.0. Fix to divide dsdx only. There may be more (width 328 not a power of two,
6-row LOADBLOCK, gSPScisTextureRectangle clipping of negative coords, texture cache key, COPY-mode alpha).
Do this:
1. Test pattern (under #ifdef PORT_RTT_TESTPATTERN, off by default): in port/gfx/gfx_xform.c gfx_rtt_flush, instead of
   reading the EFB, fill the game buffer with a CPU pattern: RGBA5551 big-endian, opaque, colour = ((x/8)&1) ? red :
   green, with a 2-pixel white border; DCFlushRange it and invalidate the texture cache range (as today).
2. Build with -DPORT_RTT_TESTPATTERN and the 14d input; fix the redraw path until the gameplay frames show the pattern
   behind the sprites (check with PIL: framedump_3000.png must be > 60% non-black, and pixel (12, 470) of the 824x480
   dump must not be black). Use the texrect debug log at gfx_tex.c:512 (first strips only) to see what the renderer does.
3. Write the cause(s) to NOTES.md. Keep the test pattern code (off by default).
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS="-DPORT_INPUT_SCRIPT -DPORT_RTT_TESTPATTERN" PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t18a >/dev/null; L=.strata/scratch/t18a/sd/drmario/log.txt; ! grep -aq CRASH $L && grep -aq "main_no=5" $L && python3 -c "from PIL import Image; im=Image.open('.strata/scratch/t18a/framedump_3000.png').convert('L'); px=im.tobytes(); nb=sum(1 for p in px if p>16)/len(px); print('nonblack', nb, 'corner', im.getpixel((12,470))); assert nb > 0.6 and im.getpixel((12,470)) > 16"
