/* Dr. Mario 64 -> Wii native port: per-axis texture wrap mode (task 24). */
#ifndef GFX_WRAP_H
#define GFX_WRAP_H
/* cm uses the GBI bit flags (G_TX_WRAP=0, G_TX_MIRROR=1, G_TX_CLAMP=2).
 * gx_mode: 0=clamp, 1=repeat, 2=mirror (GX_CLAMP/GX_REPEAT/GX_MIRROR values).
 * gx_size: texels the GX texture must be on this axis (POT for clamp, 2^mask for wrap). */
void gfx_axis_mode(int cm, int mask, int tile_w, int *gx_mode, int *gx_size);
#endif
