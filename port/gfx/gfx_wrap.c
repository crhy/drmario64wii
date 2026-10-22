/* Dr. Mario 64 -> Wii native port: N64 RDP per-axis wrap/mirror/clamp mapping (task 24).
 * Pure C: compiled both into the Wii DOL and standalone for the host test.
 *
 * RDP rules per axis (cm = GBI bit flags: bit0 mirror, bit1 clamp; mask; tile size w):
 *  - clamp bit set (cm & 2): clamp to the tile size -> GX_CLAMP, POT-padded texture.
 *  - else mask != 0: repeat with period 2^mask texels, mirrored when (cm & 1).
 *  - else (mask == 0, no clamp bit): behaves like clamp.
 */

#include "gfx_wrap.h"

static int gfx_next_pot(int v)
{
    int p = 4;
    while (p < v) p <<= 1;
    return p;
}

void gfx_axis_mode(int cm, int mask, int tile_w, int *gx_mode, int *gx_size)
{
    if (tile_w < 1) tile_w = 1;
    if ((cm & 2) != 0 || mask == 0) {
        *gx_mode = 0; /* GX_CLAMP */
        *gx_size = gfx_next_pot(tile_w);
    } else if ((cm & 1) != 0) {
        *gx_mode = 2; /* GX_MIRROR */
        *gx_size = 1 << mask;
    } else {
        *gx_mode = 1; /* GX_REPEAT */
        *gx_size = 1 << mask;
    }
}
