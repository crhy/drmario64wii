# Task 10c: render modes: alpha compare, blending, depth
Background: port/gfx: gfx.c (walker + per-opcode handler table), gfx_xform.c (state struct in gfx.h), gfx_gx.c (untextured GX draws), gfx_tmem.c, gfx_texconv.c, gfx_tex.c (textured tris/texrects, TEV MODULATE/REPLACE). Opcodes/layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 and PR/gs2dex.h (grep, never guess). Keep each new file under ~300 lines; read other files only with grep + small ranges (your context is 64k).
New file port/gfx/gfx_rm.c (+WII_SRCS) applied before every draw from G_SETOTHERMODE_L/H state:
alpha compare (G_AC_THRESHOLD: alpha >= blend colour alpha; G_AC_DITHER: >= 0.5; none: always) via GX_SetAlphaCompare
(argument order is (comp0, ref0, op, comp1, ref1)!), translucent blending when FORCE_BL is set with the common
"src*a + mem*(1-a)" blender (GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR)), opaque
otherwise, Z compare/update from Z_CMP/Z_UPD bits (GX_SetZMode, LEQUAL), FILL/COPY cycle types: no blending, copy mode
keeps alpha compare. Log once per distinct othermode L value: "RM %08x".
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t10c >/dev/null && grep -aq "^RM " .strata/scratch/t10c/sd/drmario/log.txt && .strata/imgmax.py .strata/scratch/t10c --min-colors 64
