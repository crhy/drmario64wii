/* Host test for near-plane clipping (task 10e).
 * Links gfx_clip.c directly; gfx_st is defined here so gfx_project (which reads
 * gfx_st.vscale/vtrans) resolves even though this test only exercises gfx_clip_tri. */

#include <stdio.h>
#include <stdint.h>
#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include <PR/ultratypes.h>
#include "gfx.h"

gfx_state gfx_st;

static int lerp_i(int32_t a, int32_t b, int64_t tn, int64_t td)
{
    if (td == 0) return a;
    return (int32_t)(a + (int64_t)(b - a) * tn / td);
}

int main(void)
{
    GfxClipVtx in[3], out[4];

    /* Triangle crossing the plane: two vertices in front (w = 1.0), one behind. */
    in[0].cx = 0;      in[0].cy = 0;      in[0].cz = 0; in[0].cw = 65536;
    in[0].color = 0xFF0000FFu; in[0].s = 0;      in[0].t = 0;
    in[1].cx = 65536;  in[1].cy = 0;      in[1].cz = 0; in[1].cw = 65536;
    in[1].color = 0x00FF00FFu; in[1].s = 65536;  in[1].t = 0;
    in[2].cx = 0;      in[2].cy = 65536;  in[2].cz = 0; in[2].cw = 0;
    in[2].color = 0x0000FFFFu; in[2].s = 0;      in[2].t = 65536;

    int nv = gfx_clip_tri(in, out);
    if (nv != 4) { printf("FAIL: crossing tri gave %d verts (want 4)\n", nv); return 1; }

    int k;
    for (k = 0; k < nv; k++) {
        if (out[k].cw < W_EPS) {
            printf("FAIL: out[%d].cw=%d < W_EPS=%d\n", k, out[k].cw, W_EPS);
            return 1;
        }
    }

    /* out[1] is the intersection of edge in[1]->in[2]; check interpolated s,t. */
    int64_t tn = (int64_t)W_EPS - in[1].cw;   /* 256 - 65536 */
    int64_t td = (int64_t)in[2].cw - in[1].cw; /* 0 - 65536 */
    int32_t exp_s = lerp_i(in[1].s, in[2].s, tn, td);
    int32_t exp_t = lerp_i(in[1].t, in[2].t, tn, td);
    if (out[1].s != exp_s || out[1].t != exp_t) {
        printf("FAIL: interp s,t got %d,%d want %d,%d\n", out[1].s, out[1].t, exp_s, exp_t);
        return 1;
    }
    /* colour channel g interpolates 255 -> 0 at t = tn/td. */
    int32_t exp_g = lerp_i(0xFF, 0x00, tn, td);
    if (((out[1].color >> 16) & 0xFF) != (uint32_t)(exp_g & 0xFF)) {
        printf("FAIL: interp colour g got %d want %d\n",
               (int)((out[1].color >> 16) & 0xFF), exp_g & 0xFF);
        return 1;
    }

    /* Fully behind the plane: all w <= W_EPS -> 0 output vertices. */
    in[0].cw = 0; in[1].cw = 0; in[2].cw = 0;
    int nv2 = gfx_clip_tri(in, out);
    if (nv2 != 0) { printf("FAIL: behind tri gave %d verts (want 0)\n", nv2); return 1; }

    printf("PASS: gfx_clip\n");
    return 0;
}
