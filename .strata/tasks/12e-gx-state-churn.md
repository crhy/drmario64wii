# Task 12e: stop re-sending GX state on every triangle (port/gfx/gfx_gx.c, gfx_tex.c, gfx_rm.c, gfx_cc.c)
Today every triangle calls gfx_rm_apply (alpha compare + blend mode, plus a 64-entry "seen" scan), gfx_reset_untextured
(vtx desc, texgens, tev order, cc apply), and textured draws call GX_SetVtxAttrFmt 3x and GX_SetVtxDesc each time.
VAT/vtx-desc changes force GX state flushes and fill the FIFO.
1. Vertex formats: set up once in gfx_frame_start: GX_VTXFMT0 = POS XYZ f32 + CLR0 rgba8 (untextured),
   GX_VTXFMT1 = POS XYZ f32 + CLR0 + TEX0 ST f32 (textured tris), GX_VTXFMT2 = POS XY f32 + CLR0 + TEX0 (texrects).
   Remove all GX_SetVtxAttrFmt calls from the per-draw paths.
2. Add a tiny state cache in a new port/gfx/gfx_state.c (+ decls in gfx.h): `gfx_set_textured(int on)` that only
   calls GX_SetVtxDesc(GX_VA_TEX0, ...), GX_SetNumTexGens, GX_SetTevOrder when the mode actually changes; and make
   gfx_rm_apply return early when (otm_l, otm_h, blend alpha) equal the last applied values (keep the RM log, but
   only on the not-equal path). gfx_frame_start and gfx_cc_invalidate must reset these caches (GX state may have
   been changed by COPY-mode texrect code).
3. gfx_cc_apply_tex: only call GX_SetTevKColor / GX_SetTevColor when prim / env changed since the last call.
4. G_FILLRECT: N64 FILL mode ignores the combiner and Z. When the cycle type is FILL (otm_h bits 20-21 == 3), draw
   with a pass-colour TEV (GX_PASSCLR) and Z compare off, then invalidate the caches so the next draw reprograms.
   Coordinates stay inclusive only in FILL/COPY mode; in 1/2-cycle mode lrx/lry are exclusive (no +1).
Count GX state changes per frame (PORT_LOG every 600 frames "gx state changes/frame=%u draws/frame=%u").
GATE: make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12e >/dev/null && .strata/imgmax.py .strata/scratch/t12e --min-colors 128 && grep -aq "gx state changes/frame" .strata/scratch/t12e/sd/drmario/log.txt && ! grep -n "GX_SetVtxAttrFmt" port/gfx/gfx_tex.c
