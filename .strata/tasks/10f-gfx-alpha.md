# Task 10f: translucency does not show
Task 10c added port/gfx/gfx_rm.c, but the title text "PRESS ANY BUTTON" still has an opaque black box around it
(.strata/scratch/m43/framedump_900.png): its texels with alpha 0 are drawn black. Find why: check that gfx_rm_apply runs
after gfx_cc_apply_tex has programmed the TEV (TEV alpha output must be the combiner alpha, written to TEVPREV), that the
blend mode is set from othermode L (FORCE_BL / CVG_X_ALPHA / ALPHA_CVG_SEL / G_AC_THRESHOLD bits: log the othermode L
value of the draw that renders the text and decode it by hand into NOTES.md), and that the EFB pixel format has alpha
where needed (GX_SetPixelFmt(GX_PF_RGBA6_Z24...) only if destination alpha is required; blending with SRCALPHA does not
need it). Alpha-compare cut-out (texels with alpha < threshold discarded) is the usual N64 way for text: implement
G_AC_THRESHOLD and ALPHA_CVG_SEL/CVG_X_ALPHA (coverage from texture alpha) as GX alpha compare GREATER 0.
Check with python: in a frame where the text is visible, the pixels just outside the glyphs inside the old box must
match the background colour, not black.
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t10f >/dev/null && .strata/imgmax.py .strata/scratch/t10f --min-colors 128
