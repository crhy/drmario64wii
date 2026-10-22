/* Dr. Mario 64 -> Wii native port: near-plane clipping + projection (task 10e).

 * Pure C (no GX) so it can be host-tested.  gfx_project turns a clip-space
 * vertex into N64 screen x,y plus a GX ortho depth in [0,1]; gfx_clip_tri runs
 * Sutherland-Hodgman against the single plane w = W_EPS, keeping the w >= W_EPS
 * side, and interpolates clip coords, colour (per channel) and s,t linearly. */

#include <stdint.h>
#include <math.h>
#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include <PR/ultratypes.h>
#include "gfx.h"

/* 1/256 in s15.16: the near-plane w threshold (defined in gfx.h). */

/* ---- project one clip vertex to screen + ortho depth ---- */
void gfx_project(int32_t cx, int32_t cy, int32_t cz, int32_t cw,
                 int32_t *sx, int32_t *sy, float *z)
{
    if (cw == 0) cw = 1;
    int64_t nx = ((int64_t)cx << 16) / cw, ny = ((int64_t)cy << 16) / cw;   /* ndc 16.16 */
    *sx = (int32_t)((nx * gfx_st.vscale[0] + ((int64_t)gfx_st.vtrans[0] << 16)) >> 18);
    *sy = (int32_t)((-ny * gfx_st.vscale[1] + ((int64_t)gfx_st.vtrans[1] << 16)) >> 18);
    /* z = cz/cw in [-1,1] -> [0,1] (GX ortho depth). */
    int64_t nz = ((int64_t)cz << 16) / cw;
    float zf = (float)nz / 65536.0f;
    float z01 = (zf + 1.0f) * 0.5f;
    if (z01 < 0.0f) z01 = 0.0f;
    if (z01 > 1.0f) z01 = 1.0f;
    *z = z01;
}

/* ---- linear interpolation helpers (t = tn/td, result = a + (b-a)*t) ---- */
static int32_t lerp_i(int32_t a, int32_t b, int64_t tn, int64_t td)
{
    if (td == 0) return a;
    return (int32_t)(a + (int64_t)(b - a) * tn / td);
}

static uint32_t lerp_color(uint32_t ca, uint32_t cb, int64_t tn, int64_t td)
{
    if (td == 0) return ca;
    uint32_t r = (uint32_t)lerp_i((int32_t)((ca >> 24) & 0xFFu), (int32_t)((cb >> 24) & 0xFFu), tn, td);
    uint32_t g = (uint32_t)lerp_i((int32_t)((ca >> 16) & 0xFFu), (int32_t)((cb >> 16) & 0xFFu), tn, td);
    uint32_t b = (uint32_t)lerp_i((int32_t)((ca >> 8) & 0xFFu), (int32_t)((cb >> 8) & 0xFFu), tn, td);
    uint32_t a = (uint32_t)lerp_i((int32_t)(ca & 0xFFu), (int32_t)(cb & 0xFFu), tn, td);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

/* Intersection of edge a->b with the plane w = W_EPS. */
static void clip_intersect(GfxClipVtx *out, const GfxClipVtx *a, const GfxClipVtx *b)
{
    int64_t tn = (int64_t)W_EPS - a->cw;
    int64_t td = (int64_t)b->cw - a->cw;
    out->cx = lerp_i(a->cx, b->cx, tn, td);
    out->cy = lerp_i(a->cy, b->cy, tn, td);
    out->cz = lerp_i(a->cz, b->cz, tn, td);
    out->cw = W_EPS;
    out->color = lerp_color(a->color, b->color, tn, td);
    out->s = lerp_i(a->s, b->s, tn, td);
    out->t = lerp_i(a->t, b->t, tn, td);
}

/* ---- Sutherland-Hodgman against w >= W_EPS ---- */
int gfx_clip_tri(const GfxClipVtx in[3], GfxClipVtx out[4])
{
    int n = 0;
    int i;
    for (i = 0; i < 3; i++) {
        const GfxClipVtx *S = &in[i];
        const GfxClipVtx *E = &in[(i + 1) % 3];
        int s_in = S->cw >= W_EPS;
        int e_in = E->cw >= W_EPS;
        if (e_in) {
            if (!s_in && n < 4)
                clip_intersect(&out[n++], S, E);
            if (n < 4)
                out[n++] = *E;
        } else if (s_in && n < 4) {
            clip_intersect(&out[n++], S, E);
        }
    }
    return n;
}
