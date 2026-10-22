/* Dr. Mario 64 -> Wii native port: GX state cache (task 12e).

 * gfx_set_textured(on) re-sends the vertex-desc / texgen / tev-order state
 * only when the textured mode actually changes; gfx_rm_apply consults
 * gfx_state_rm_check so alpha compare + blend + Z mode are re-sent only
 * when (otm_l, otm_h, blend alpha) differ from the last applied values.
 * gfx_state_reset() invalidates both caches (frame start, COPY-mode texrect
 * code, FILL pass-colour draws).  Counters log "gx state changes/frame"
 * and "draws/frame" every 30 frames. */

#include <stdint.h>
#include <ogc/gx.h>
#include "gfx.h"
#include "port_log.h"

/* textured-mode cache: -1 = unknown (re-send on next call) */
static int st_tex = -1;

/* rm cache */
static uint32_t rm_l, rm_h, rm_blend;
static int rm_valid;

/* counters */
static uint32_t cnt_changes, cnt_draws, cnt_frames;

void gfx_state_reset(void)
{
    st_tex = -1;
    rm_valid = 0;
}

void gfx_set_textured(int on)
{
    on = on ? 1 : 0;
    if (on == st_tex)
        return;
    st_tex = on;
    cnt_changes++;
    if (on) {
        GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GX_SetNumTexGens(1);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    } else {
        GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
        GX_SetNumTexGens(0);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    }
}

/* Returns 1 when the rm state changed (cache updated), 0 when identical. */
int gfx_state_rm_check(uint32_t l, uint32_t h, uint32_t blend)
{
    if (rm_valid && l == rm_l && h == rm_h && blend == rm_blend)
        return 0;
    rm_l = l; rm_h = h; rm_blend = blend;
    rm_valid = 1;
    cnt_changes++;
    return 1;
}

void gfx_state_count_draw(void)
{
    cnt_draws++;
}

void gfx_state_frame_end(void)
{
    cnt_frames++;
    if ((cnt_frames % 30u) == 0u) {
        PORT_LOG("gx state changes/frame=%u draws/frame=%u",
                 (unsigned)(cnt_changes / 30u), (unsigned)(cnt_draws / 30u));
        cnt_changes = 0;
        cnt_draws = 0;
    }
}
