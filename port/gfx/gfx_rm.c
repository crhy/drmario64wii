/* Dr. Mario 64 -> Wii native port: other-mode render state -> GX (task 10c).

 * gfx_rm_apply() is called before every draw and programs alpha compare,
 * blending and Z mode from gfx_st.otm_l / gfx_st.otm_h.  F3DEX_GBI_2 splits
 * the other mode: G_SETOTHERMODE_L holds bits 0-11 (alpha compare bits 0-1,
 * Z source bit 2, render mode bits 3-11: AA_EN 0x8, Z_CMP 0x10, Z_UPD 0x20,
 * IM_RD 0x40, CLR_ON_CVG 0x80, CVG_DST 0x100, ZMODE 0x400), G_SETOTHERMODE_H
 * holds bits 12-31 right-justified (CVG_X_ALPHA 0x1000, ALPHA_CVG_SEL 0x2000,
 * FORCE_BL 0x4000, blender 0x10000, cycle type bits 20-21).

 * Alpha compare: G_AC_THRESHOLD (1) keeps fragments with alpha >= blend colour
 * alpha (GX_GEQUAL vs blend alpha); G_AC_DITHER (3) approximates the dithered
 * test as alpha >= 0.5 (GX_GEQUAL vs 0x80); G_AC_NONE (0) always passes.
 * When coverage comes from the texture alpha (ALPHA_CVG_SEL and/or CVG_X_ALPHA
 * set in the H word) the N64 cut-out test is "fragment alpha > 0": texels with
 * alpha 0 are discarded so the background shows through instead of a black box.
 * That is the usual way text is drawn, so G_AC_THRESHOLD + coverage-from-alpha
 * becomes GX alpha compare GREATER 0 (the blend colour's alpha is irrelevant
 * there).  Blending: translucent "src*a + mem*(1-a)" when FORCE_BL is set, or
 * when coverage comes from the texture alpha, opaque otherwise; FILL/COPY cycle
 * types never blend (COPY keeps alpha compare, FILL writes the fill colour
 * straight through).  Z compare/update come from the Z_CMP/Z_UPD bits with a
 * LEQUAL function.  Logs "RM %08x %08x" once per distinct (L,H) pair. */

#include <stdint.h>
#include <ogc/gx.h>
#include "gfx.h"
#include "port_log.h"

/* L-word bits (PR/gbi.h F3DEX_GBI_2 render mode). */
#define G_AC_MASK       0x3u
#define G_AC_THRESHOLD  0x1u
#define G_AC_DITHER     0x3u
#define Z_CMP           0x10u
#define Z_UPD           0x20u
/* H-word bits (right-justified 20-bit H field). */
#define CVG_X_ALPHA     (1u << 12)
#define ALPHA_CVG_SEL   (1u << 13)
#define FORCE_BL        (1u << 14)
#define CYC_SHIFT       20
#define CYC_MASK        0x3u

void gfx_rm_apply(void)
{
    uint32_t l = gfx_st.otm_l;
    uint32_t h = gfx_st.otm_h;

    /* Task 12e: skip re-sending alpha compare + blend + Z when the other-mode
     * words and the blend alpha are unchanged since the last applied draw. */
    if (!gfx_state_rm_check(l, h, gfx_st.blend))
        return;

    static uint32_t seen[64][2];
    static int nseen;
    int i, known = 0;
    for (i = 0; i < nseen; i++)
        if (seen[i][0] == l && seen[i][1] == h) { known = 1; break; }
    if (!known && nseen < 64) {
        seen[nseen][0] = l; seen[nseen][1] = h; nseen++;
        PORT_LOG("RM %08x %08x", l, h);
    }

    uint32_t cyc = (h >> CYC_SHIFT) & CYC_MASK;
    uint32_t ac = l & G_AC_MASK;
    uint32_t ref = gfx_st.blend & 0xFFu;
    /* Render-mode bits (FORCE_BL, CVG_X_ALPHA, ALPHA_CVG_SEL) live in othermode HIGH (H word). */
    int cov_alpha = (h & (CVG_X_ALPHA | ALPHA_CVG_SEL)) != 0;
    /* Translucent only when the cycle-1 blender is P*A + MEM*(1-A): A1 (bits 12-13 of H word) = IN_ALPHA (0),
     * M1 (bits 10-11 of H word) = MEM (1). FORCE_BL alone also appears with opaque formulas (e.g. 0x0f0a4000). */
    int xlu = (h & FORCE_BL) && ((h >> 12) & 3u) == 0u && ((h >> 10) & 3u) == 1u;

    if (cyc == 3u) {
        /* FILL: fill colour straight to the framebuffer. */
        GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
        GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
        GX_SetZCompLoc(GX_TRUE);
    } else {
        int cut_out = cov_alpha || (ac == G_AC_THRESHOLD && (ref == 0u || cyc == 2u));
        GX_SetZCompLoc(GX_FALSE);
        if (cut_out) {
            GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_ALWAYS, 0);
            GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
        } else if (ac == G_AC_THRESHOLD) {
            GX_SetAlphaCompare(GX_GEQUAL, (uint8_t)ref, GX_AOP_AND, GX_ALWAYS, 0);
            if (xlu) GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            else     GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
        } else if (ac == G_AC_DITHER) {
            GX_SetAlphaCompare(GX_GEQUAL, 0x80u, GX_AOP_AND, GX_ALWAYS, 0);
            if (xlu) GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            else     GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
        } else {
            GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
            if (xlu && cyc != 2u) GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            else                  GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
        }
    }

    GX_SetZMode((l & Z_CMP) ? 1 : 0, GX_LEQUAL, (l & Z_UPD) ? 1 : 0);
}
