# Task 10a3: colour combiner -> TEV (second attempt)
A first attempt (gfx_cc.c may already exist) made every frame a single colour. Check: GX_SetNumTevStages matches the
stages used, GX_SetTevKColorSel/KAlphaSel for prim/env, and that untextured draws do not sample a texture.
Exact mapping of one N64 cycle (A-B)*C+D onto GX TEV (GX computes d OP ((1-c)*a + c*b)):
  stage n  : a=ZERO, b=A, c=C, d=ZERO, op ADD, clamp OFF  -> C*A
  stage n+1: a=ZERO, b=B, c=C, d=CPREV, op SUB, clamp OFF -> C*A - C*B
  stage n+2: a=ZERO, b=D, c=ONE, d=CPREV, op ADD, clamp ON -> (A-B)*C + D
Do the same for alpha with GX_SetTevAlphaIn/Op. Simplify when B=ZERO or C=ONE etc. if you like, but correctness first.
2-cycle: the second cycle's COMBINED input = CPREV/APREV of the first cycle's last stage.
Textures were also broken until task 10a2 (address 0 bug), so judge with the frames of THIS build only.
Background: port/gfx: gfx.c (walker + per-opcode handler table), gfx_xform.c (state struct in gfx.h), gfx_gx.c (untextured GX draws), gfx_tmem.c, gfx_texconv.c, gfx_tex.c (textured tris/texrects, TEV MODULATE/REPLACE). Opcodes/layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 and PR/gs2dex.h (grep, never guess). Keep each new file under ~300 lines; read other files only with grep + small ranges (your context is 64k).
New file port/gfx/gfx_cc.c (+WII_SRCS): decode G_SETCOMBINE (a,b,c,d for colour and alpha, cycle 1 and 2) and set GX TEV
stages computing (a-b)*c+d. Inputs: TEXEL0, TEXEL1 (= TEXEL0 for now), SHADE (vertex colour), PRIMITIVE, ENVIRONMENT
(GX konst colours GX_KCOLOR0/1 via GX_SetTevKColor), COMBINED (previous stage), 1, 0; PRIM_LOD_FRAC and K4/K5 = 0.
2-cycle mode chains two stage pairs. Cache the TEV setup per combine word pair. gfx_tex.c / gfx_gx.c call gfx_cc_apply()
before drawing instead of their fixed MODULATE/REPLACE/PASSCLR (keep COPY cycle type = texture only).
PORT_LOG once per new combine word: "CC %08x %08x".
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t10a3 >/dev/null && grep -aq "^CC " .strata/scratch/t10a3/sd/drmario/log.txt && .strata/imgmax.py .strata/scratch/t10a3 --min-colors 64
