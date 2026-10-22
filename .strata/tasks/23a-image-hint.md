# Task 23a: pass "whole image" hints through the display list (step 1 of 2, renderer only LOGS them)
Write these files FIRST, then build. Do not change how anything is drawn in this task.
1. New header include/port_img_hint.h (guarded by #ifdef TARGET_WII inside):
     typedef struct PortImgHint { u32 magic; const void *img; const void *tlut; u16 w, h; u8 fmt, siz, tlut_mode, pad; } PortImgHint;
     #define PORT_IMG_MAGIC 0x50494D47u  /* 'PIMG' */
     void port_img_hint(Gfx **gfxP, const void *img, const void *tlut, int w, int h, int fmt, int siz, int tlut_mode);
2. src/main_segment/tex_func.c, under #ifdef TARGET_WII: implement port_img_hint: take the next entry of a static ring
   `static PortImgHint s_hint[128]; static int s_hint_i;`, fill it, and emit gDPNoOpTag((*gfxP)++, (u32)&entry).
   Call it at the START of CopyTexBlock4/8/16, StretchTexBlock4/8/16/4i and StretchTexTile4/8/16/4i with that helper's
   full image (tex/colorTex pointer, tlut pointer or NULL, full width/height arguments, G_IM_FMT_* / G_IM_SIZ_* it
   loads with, G_TT_RGBA16 when a tlut is used else G_TT_NONE). The N64 build must still match (`make` -> ": OK").
3. port/gfx: register a G_NOOP (opcode 0x00) handler: if w1 resolves (gfx_resolve) to memory whose first u32 is
   PORT_IMG_MAGIC, count it; log once "pimg: first w=%d h=%d fmt=%d siz=%d" and every 600 gfx tasks
   "pimg: hints=%u" (cumulative). Keep a pointer to the current hint in gfx state (gfx_st.img_hint), cleared at the end
   of each gfx task - task 23b will use it.
GATE: make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && grep -q PORT_IMG_MAGIC include/port_img_hint.h && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t23a >/dev/null; grep -aq "pimg: first" .strata/scratch/t23a/sd/drmario/log.txt && grep -a "pimg: hints=" .strata/scratch/t23a/sd/drmario/log.txt | tail -1 | grep -aqv "hints=0$" && python3 -c "from PIL import Image, ImageChops, ImageStat as S; d=[S.Stat(ImageChops.difference(Image.open('.strata/scratch/t23a/framedump_%d.png'%n).convert('RGB'), Image.open('.strata/scratch/ref20/framedump_%d.png'%n).convert('RGB'))).mean for n in (180,420)]; print(d); assert all(max(x)<2 for x in d)"
