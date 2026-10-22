/* Dr. Mario 64 -> Wii native port: GX drawing (task 08c, untextured).

 * Registers the drawing half of the display-list walker: G_TRI1/G_TRI2/G_QUAD
 * draw screen-space triangles with GX (per-vertex shade colour when G_SHADE is
 * set, else the primitive colour), honouring G_CULL_BACK/FRONT.  G_FILLRECT
 * draws a solid quad in the fill colour (RGBA5551 packed twice in G_SETFILLCOLOR
 * w1).  G_SETSCISSOR maps the N64 scissor box (1/4-pixel units) onto the 640x480
 * EFB with GX_SetScissor (scaled x2).  Z compare comes from G_ZBUFFER (LEQUAL).

 * Coordinates: N64 screen is 320x240; an ortho projection maps 0..320 x 0..240
 * onto the full 640x480 EFB (scale 2).  Vertex screen coords (sx,sy) come from
 * gfx_xform.c; screen z is computed here from clip z + viewport z scale/translate.

 * GX vertex format (GX_VTXFMT0): POS = xy f32 (GX_DIRECT), CLR0 = rgba8.
 * Self-contained: gfx.c calls gfx_gx_install() after gfx_xform_install(). */

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <ogc/gx.h>
#include <gccore.h>
#include "gfx.h"
#include "port_log.h"

extern int main_no;

#ifdef PORT_TEXDUMP
static int gfx_menu_overlap(float x0, float y0, float x1, float y1)
{
    if (main_no != 6) return 0;
    return (x1 > 220.0f && x0 < 265.0f && y1 > 60.0f && y0 < 80.0f) ||
           (x1 > 65.0f && x0 < 105.0f && y1 > 115.0f && y0 < 155.0f);
}
#endif

/* Opcode/geometry-bit constants (F3DEX_GBI_2 values from PR/gbi.h).
 * Defined locally so this TU does not pull in ultralib types, which clash
 * with libogc's s32/vu32/vs32/Mtx/Vtx. */
#define G_TRI1          0x05u
#define G_TRI2          0x06u
#define G_QUAD          0x07u
#define G_FILLRECT      0xf6u
#define G_SETSCISSOR    0xedu
#define G_SHADE         0x00000004u
#define G_CULL_FRONT    0x00000200u
#define G_CULL_BACK     0x00000400u

/* Colour combiner -> TEV (task 10b): programs the TEV unit for the current
 * G_SETCOMBINE before each draw, replacing the fixed PASSCLR. */
void gfx_cc_apply(void);
void gfx_cc_apply_tex(int textured);
void gfx_cc_invalidate(void);

/* ---- screen-space ortho projection: 0..320 x 0..240 -> full EFB ---- */
static Mtx44 gfx_ortho;
static Mtx gfx_identity;

/* ---- per-frame GX state (set once at frame start) ---- */
void gfx_frame_start(void)
{
    gfx_state_frame_end();
    gfx_cc_invalidate();
    gfx_state_reset();
    memset(gfx_ortho, 0, sizeof(gfx_ortho));
    gfx_ortho[0][0] = 2.0f / 320.0f;   /* x: 0 -> -1, 320 -> +1 */
    gfx_ortho[0][3] = -1.0f;
    gfx_ortho[1][1] = -2.0f / 240.0f;  /* y: 0 -> +1 (top), 240 -> -1 */
    gfx_ortho[1][3] = 1.0f;
    /* POS is now XYZ (task 10e): vertex z is the ortho depth in [0,1] from gfx_project.
     * GX ortho clip z must lie in [-1, 0]; map z_in [0,1] -> clip z [-1, 0] so nearer
     * (smaller z_in) wins under GX_LEQUAL. */
    gfx_ortho[2][2] = 1.0f;
    gfx_ortho[2][3] = -1.0f;
    gfx_ortho[3][3] = 1.0f;
    GX_LoadProjectionMtx(gfx_ortho, GX_ORTHOGRAPHIC);

    /* POS coords are already screen-space, so the modelview must be identity;
     * GX does not initialise it, and a zero matrix collapses every vertex. */
    memset(gfx_identity, 0, sizeof(gfx_identity));
    gfx_identity[0][0] = gfx_identity[1][1] = gfx_identity[2][2] = gfx_identity[3][3] = 1.0f;
    GX_LoadPosMtxImm(gfx_identity, GX_PNMTX0);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    /* VTXFMT0: untextured (POS_XYZ, CLR0) */
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    /* VTXFMT1: textured triangles (POS_XYZ, CLR0, TEX0) */
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    /* VTXFMT2: texrect quads (POS_XY, CLR0, TEX0) */
    GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetNumChans(1);
    gfx_set_textured(0);
    gfx_cc_apply_tex(0);
    GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0); /* (comp0, ref0, op, comp1, ref1) */
    GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetCullMode(GX_CULL_NONE);
}

/* ---- colour helpers ---- */
/* gfx_st colours are packed RGBA8 (0xRRGGBBAA). */
static inline uint32_t gfx_rgba8_to_r(uint32_t c) { return (c >> 24) & 0xFFu; }
static inline uint32_t gfx_rgba8_to_g(uint32_t c) { return (c >> 16) & 0xFFu; }
static inline uint32_t gfx_rgba8_to_b(uint32_t c) { return (c >> 8) & 0xFFu; }
static inline uint32_t gfx_rgba8_to_a(uint32_t c) { return c & 0xFFu; }

/* G_SETFILLCOLOR w1 holds a 16-bit RGBA5551 value packed twice; decode to RGBA8. */
static void gfx_fill_decode(uint32_t packed, uint32_t *out)
{
    uint32_t c = packed & 0xFFFFu;
    uint32_t r5 = (c >> 11) & 0x1Fu;
    uint32_t g5 = (c >> 6) & 0x1Fu;
    uint32_t b5 = (c >> 1) & 0x1Fu;
    uint32_t a1 = c & 1u;
    *out = ((r5 << 3) | (r5 >> 2)) << 24 |
           ((g5 << 3) | (g5 >> 2)) << 16 |
           ((b5 << 3) | (b5 >> 2)) << 8 |
           (a1 ? 0xFFu : 0x00u);
}

/* ---- screen z from clip z + viewport z scale/translate (1/4-pixel units) ---- */
static inline float gfx_screen_z(int32_t cz)
{
    int64_t sz = ((int64_t)cz * gfx_st.vscale[2] + ((int64_t)gfx_st.vtrans[2] << 16)) >> 18;
    float z = (float)sz / 131072.0f;   /* 0x20000: N64 z range -> 0..1 */
    if (z < 0.0f) z = 0.0f;
    if (z > 1.0f) z = 1.0f;
    return z;
}

/* ---- triangle draw ---- */
unsigned gfx_dbg_tris, gfx_dbg_fills;

/* Textured triangle draw lives in gfx_tex.c (task 09c). */
void gfx_tex_draw_tri(int i0, int i1, int i2);

/* Reset GX state that gfx_tex.c may have left on (TEX0 desc + MODULATE TEV)
 * back to the untextured configuration used by gfx_gx.c.  The textured-mode
 * cache in gfx_state.c makes the re-send a no-op when already untextured. */
static void gfx_reset_untextured(void)
{
    gfx_set_textured(0);
    gfx_cc_apply_tex(0);
}

static void gfx_draw_tri(int i0, int i1, int i2)
{
    gfx_dbg_tris++;
    gfx_rm_apply();
    const gfx_state *s = &gfx_st;
    if (s->tex_on) {
        gfx_tex_draw_tri(i0, i1, i2);
        gfx_reset_untextured();
        return;
    }
    gfx_reset_untextured();
    int shade = (s->geo & G_SHADE) != 0;
    uint32_t prim = s->prim;

    GfxClipVtx in[3], out[4];
    int idx[3] = { i0, i1, i2 };
    int k, all_in = 1, all_out = 1;
    for (k = 0; k < 3; k++) {
        int vi = idx[k];
        if (vi < 0) vi = 0;
        if (vi > 31) vi = 31;
        in[k].cx = s->vtx[vi].cx; in[k].cy = s->vtx[vi].cy;
        in[k].cz = s->vtx[vi].cz; in[k].cw = s->vtx[vi].cw;
        in[k].color = s->vtx[vi].color; in[k].s = s->vtx[vi].s; in[k].t = s->vtx[vi].t;
        if (in[k].cw > W_EPS) all_out = 0; else all_in = 0;
    }
    if (all_out) return;
    int nv;
    if (all_in) { nv = 3; out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; }
    else nv = gfx_clip_tri(in, out);
    if (nv < 3) return;

    float px[4], py[4], pz[4];
    uint32_t pc[4];
    int32_t minx = 0x7FFFFFFF, maxx = -0x7FFFFFFF, miny = 0x7FFFFFFF, maxy = -0x7FFFFFFF;
    for (k = 0; k < nv; k++) {
        int32_t sx, sy; float z;
        gfx_project(out[k].cx, out[k].cy, out[k].cz, out[k].cw, &sx, &sy, &z);
        px[k] = (float)sx; py[k] = (float)sy; pz[k] = z;
        pc[k] = shade ? out[k].color : prim;
        if (sx < minx) minx = sx;
        if (sx > maxx) maxx = sx;
        if (sy < miny) miny = sy;
        if (sy > maxy) maxy = sy;
    }
    if (minx < -4096 || maxx > 4096 || miny < -4096 || maxy > 4096) return;

    gfx_state_count_draw();
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3 * (nv - 2));
    for (k = 1; k + 1 < nv; k++) {
        int a = 0, b = k, c = k + 1;
        GX_Position3f32(px[a], py[a], pz[a]);
        GX_Color4u8((uint8_t)gfx_rgba8_to_r(pc[a]), (uint8_t)gfx_rgba8_to_g(pc[a]),
                    (uint8_t)gfx_rgba8_to_b(pc[a]), (uint8_t)gfx_rgba8_to_a(pc[a]));
        GX_Position3f32(px[b], py[b], pz[b]);
        GX_Color4u8((uint8_t)gfx_rgba8_to_r(pc[b]), (uint8_t)gfx_rgba8_to_g(pc[b]),
                    (uint8_t)gfx_rgba8_to_b(pc[b]), (uint8_t)gfx_rgba8_to_a(pc[b]));
        GX_Position3f32(px[c], py[c], pz[c]);
        GX_Color4u8((uint8_t)gfx_rgba8_to_r(pc[c]), (uint8_t)gfx_rgba8_to_g(pc[c]),
                    (uint8_t)gfx_rgba8_to_b(pc[c]), (uint8_t)gfx_rgba8_to_a(pc[c]));
    }
    GX_End();
}

/* ---- G_TRI1: w0 = op | (v0*2)<<16 | (v1*2)<<8 | (v2*2) ---- */
static void gfx_tri1(u32 w0, u32 w1)
{
    (void)w1;
    int v0 = ((w0 >> 16) & 0xFFu) >> 1;
    int v1 = ((w0 >> 8) & 0xFFu) >> 1;
    int v2 = (w0 & 0xFFu) >> 1;
    gfx_draw_tri(v0, v1, v2);
}

/* ---- G_TRI2 / G_QUAD: two triangles, one encoded in w0, one in w1 ---- */
static void gfx_tri2(u32 w0, u32 w1)
{
    gfx_draw_tri(((w0 >> 16) & 0xFFu) >> 1, ((w0 >> 8) & 0xFFu) >> 1, (w0 & 0xFFu) >> 1);
    gfx_draw_tri(((w1 >> 16) & 0xFFu) >> 1, ((w1 >> 8) & 0xFFu) >> 1, (w1 & 0xFFu) >> 1);
}

/* ---- G_FILLRECT: w0 = op | lrx<<14 | lry<<2 ; w1 = ulx<<14 | uly<<2 (10.2, inclusive) ---- */
static void gfx_fillrect(u32 w0, u32 w1)
{
    gfx_dbg_fills++;
    uint32_t lrx = (w0 >> 14) & 0x3FFu;
    uint32_t lry = (w0 >> 2) & 0x3FFu;
    uint32_t ulx = (w1 >> 14) & 0x3FFu;
    uint32_t uly = (w1 >> 2) & 0x3FFu;

    uint32_t col;
    gfx_fill_decode(gfx_st.fill, &col);
    uint8_t r = (uint8_t)gfx_rgba8_to_r(col);
    uint8_t g = (uint8_t)gfx_rgba8_to_g(col);
    uint8_t b = (uint8_t)gfx_rgba8_to_b(col);
    uint8_t a = (uint8_t)gfx_rgba8_to_a(col);

    /* fill mode is inclusive: cover pixels [ulx/4 .. lrx/4] -> quad right edge (lrx+1)/4 */
    /* gDPFillRectangle packs whole pixels at bits 14 and 2 (10.2 fields with zero fraction), so
     * the extracted values are already pixels. FILL mode is inclusive of the lower-right pixel. */
    float x0 = (float)ulx;
    float y0 = (float)uly;
    float x1 = (float)(lrx + 1u);
    float y1 = (float)(lry + 1u);
    float z = 0.5f;

    gfx_state_count_draw();
    /* The quad below sends POS+CLR0 only: the vertex descriptor must not still have TEX0 from a
     * previous textured draw, or the GP reads texcoords that never come and hangs (seen as a
     * GX_DrawDone that never returns when the title fade-out drew its first fill rect). */
    gfx_set_textured(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    /* N64 FILL mode ignores the combiner and Z: write the fill colour straight
     * to the framebuffer with a pass-colour TEV and Z compare off. */
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    /* N64 FILL mode respects alpha compare: test the fill alpha against the mode.
     * CVG_X_ALPHA (bit 12 of H word) and ALPHA_CVG_SEL (bit 13 of H word) mean
     * the cut-out test is "alpha > 0"; otherwise THRESHOLD tests alpha >= blend alpha. */
    uint32_t ac = gfx_st.otm_l & 3u;
    uint32_t ref = gfx_st.blend & 0xFFu;
    int cov_alpha = (gfx_st.otm_h & ((1u << 12) | (1u << 13))) != 0u;
    int pass = 1;
    if (ac == 1u) { /* THRESHOLD */
        if (cov_alpha || ref == 0u) pass = (a > 0u);
        else pass = ((uint32_t)a >= ref);
    } else if (ac == 3u) { /* DITHER */
        pass = (a >= 0x80u);
    }
    if (!pass) { gfx_state_reset(); gfx_cc_invalidate(); return; }
    GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
    GX_SetZMode(GX_DISABLE, GX_LEQUAL, GX_TRUE);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(x0, y0, z); GX_Color4u8(r, g, b, a);
    GX_Position3f32(x1, y0, z); GX_Color4u8(r, g, b, a);
    GX_Position3f32(x1, y1, z); GX_Color4u8(r, g, b, a);
    GX_Position3f32(x0, y1, z); GX_Color4u8(r, g, b, a);
    GX_End();
    /* COPY/pass-colour drew with hand-set state: invalidate so the next draw reprograms. */
    gfx_state_reset();
    gfx_cc_invalidate();
#ifdef PORT_TEXDUMP
    { static int nfill = 0; if (gfx_menu_overlap(x0, y0, x1, y1) && nfill < 64) { nfill++; PORT_LOG("menu fillrect %d: col=%08x rect=(%.1f,%.1f,%.1f,%.1f) otm_l=%08x otm_h=%08x fill=%08x\n", nfill, col, x0, y0, x1, y1, gfx_st.otm_l, gfx_st.otm_h, gfx_st.fill); } }
#endif
}

/* ---- G_SETSCISSOR: N64 box in 1/4-pixel units -> GX pixels on the 640x480 EFB (x2) ---- */
static void gfx_scissor(u32 w0, u32 w1)
{
    uint32_t ulx = (w0 >> 12) & 0xFFFu;
    uint32_t uly = w0 & 0xFFFu;
    uint32_t lrx = (w1 >> 12) & 0xFFFu;
    uint32_t lry = w1 & 0xFFFu;

    gfx_st.scissor[0] = (int)ulx;
    gfx_st.scissor[1] = (int)uly;
    gfx_st.scissor[2] = (int)lrx;
    gfx_st.scissor[3] = (int)lry;

    /* N64 scissor is inclusive [ulx/4 .. lrx/4]; GX_SetScissor is exclusive [x, x+w).
     * Width in N64 pixels = lrx/4 - ulx/4 + 1; in EFB (x2) = ((lrx-ulx)/4 + 1)*2. */
    uint32_t x = ulx / 2u;
    uint32_t y = uly / 2u;
    uint32_t w = (lrx > ulx ? ((lrx - ulx) / 4u + 1u) * 2u : 2u);
    uint32_t h = (lry > uly ? ((lry - uly) / 4u + 1u) * 2u : 2u);
    GX_SetScissor((u16)x, (u16)y, (u16)w, (u16)h);
}

/* ---- install drawing handlers (run after gfx_xform_install) ---- */
void gfx_gx_install(void (*table[256])(u32 w0, u32 w1))
{
    table[G_TRI1 & 0xFF] = gfx_tri1;
    table[G_TRI2 & 0xFF] = gfx_tri2;
    table[G_QUAD & 0xFF] = gfx_tri2;
    table[G_FILLRECT & 0xFF] = gfx_fillrect;
    table[G_SETSCISSOR & 0xFF] = gfx_scissor;
}

