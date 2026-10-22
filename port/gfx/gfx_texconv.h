#ifndef GFX_TEXCONV_H
#define GFX_TEXCONV_H

/* N64 texel formats -> RGBA8 (task 09b, pure functions).
 * gfx_texconv.c decodes a tile that already lives in the TMEM model (task 09a)
 * into a row-major RGBA8 buffer for GX upload.  Formats: RGBA16 (5551), RGBA32,
 * IA4, IA8, IA16, I4, I8, CI4, CI8, and TLUT (16-bit palette entries read as
 * RGBA16 or IA16 per the other-mode-H TEXTLUT field).  The tile line stride is
 * in 64-bit words and the LOADBLOCK odd-row 32-bit word swap is undone on read. */

#include <stdint.h>
#include "gfx_tmem.h"

/* Decode tile t (w x h texels) from the TMEM byte array tmem into out_rgba8
 * (w*h*4 bytes, row-major R,G,B,A).  otm_h is the other-mode-H word; its
 * G_MDSFT_TEXTLUT field selects RGBA16 vs IA16 for TLUT / CI palette entries. */
void gfx_decode_tile(const uint8_t *tmem, const GfxTile *t, int w, int h,
                     uint8_t *out_rgba8, uint32_t otm_h);

#endif
