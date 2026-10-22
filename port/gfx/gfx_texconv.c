/* Dr. Mario 64 -> Wii native port: N64 texel formats -> RGBA8.
 * TMEM model shared with gfx_tmem.c: tile.tmem/line in 64-bit words, row y at tmem*8 + y*line*8, odd rows read
 * with byte offset XOR 4, TLUT entries quadrupled (entry i of the palette area at 0x800 + i*8).
 * fmt: RGBA=0 YUV=1 CI=2 IA=3 I=4; siz: 4b=0 8b=1 16b=2 32b=3.
 * RGBA16 = 5551, RGBA32 = 8888, IA4 = 3-bit I + 1-bit A, IA8 = 4+4, IA16 = 8-bit I + 8-bit A, I4/I8 = intensity
 * used for colour and alpha, CI4/CI8 = palette index, entries RGBA16 or IA16 per other-mode-H TEXTLUT. */

#include <stddef.h>
#include <stdint.h>
#include <PR/gbi.h>
#include "gfx_texconv.h"

static uint8_t exp5(uint32_t v) { return (uint8_t)((v << 3) | (v >> 2)); }
static uint8_t exp3(uint32_t v) { return (uint8_t)((v << 5) | (v << 2) | (v >> 1)); }
static uint8_t exp4(uint32_t v) { return (uint8_t)(v * 0x11u); }

static void put(uint8_t *o, uint8_t r, uint8_t g, uint8_t b, uint8_t a) { o[0] = r; o[1] = g; o[2] = b; o[3] = a; }

static void put_rgba16(uint8_t *o, uint32_t e)
{
    put(o, exp5((e >> 11) & 0x1F), exp5((e >> 6) & 0x1F), exp5((e >> 1) & 0x1F), (e & 1) ? 0xFF : 0x00);
}

/* 16-bit palette entry: TEXTLUT field (othermode H >> 14): 2 = G_TT_RGBA16, 3 = G_TT_IA16 (gbi.h).
 * There is no IA8 TLUT mode. */
static void put_entry16(uint8_t *o, uint32_t e, int tlut)
{
    if (tlut == 3) put(o, (uint8_t)(e >> 8), (uint8_t)(e >> 8), (uint8_t)(e >> 8), (uint8_t)e);
    else put_rgba16(o, e);
}

void gfx_decode_tile(const uint8_t *tmem, const GfxTile *t, int w, int h, uint8_t *out_rgba8, uint32_t otm_h)
{
    uint32_t base = (uint32_t)t->tmem * 8u, stride = (uint32_t)t->line * 8u;
    int tlut = (int)((otm_h >> G_MDSFT_TEXTLUT) & 3u);
    int x, y;
    if (stride == 0) stride = 8;
    for (y = 0; y < h; y++) {
        uint32_t xr = (y & 1) ? 4u : 0u, row = base + (uint32_t)y * stride;
        for (x = 0; x < w; x++) {
            uint8_t *o = out_rgba8 + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            uint32_t v;
            switch (t->siz) {
            case 0: { /* 4-bit: high nibble is the left texel */
                uint8_t b = tmem[((row + (uint32_t)x / 2u) ^ xr) & 0xFFFu];
                v = (x & 1) ? (b & 0xFu) : (b >> 4);
                if (t->fmt == 2) {
                    uint32_t idx = ((uint32_t)t->palette << 4) | v, a = (0x800u + idx * 8u) & 0xFFFu;
                    put_entry16(o, ((uint32_t)tmem[a] << 8) | tmem[(a + 1) & 0xFFFu], tlut);
                } else if (t->fmt == 3) { uint8_t i = exp3(v >> 1); put(o, i, i, i, (v & 1) ? 0xFF : 0); }
                else { uint8_t i = exp4(v); put(o, i, i, i, i); }
                break;
            }
            case 1: { /* 8-bit */
                v = tmem[((row + (uint32_t)x) ^ xr) & 0xFFFu];
                if (t->fmt == 2) {
                    uint32_t a = (0x800u + v * 8u) & 0xFFFu;
                    put_entry16(o, ((uint32_t)tmem[a] << 8) | tmem[(a + 1) & 0xFFFu], tlut);
                } else if (t->fmt == 3) put(o, exp4(v >> 4), exp4(v >> 4), exp4(v >> 4), exp4(v & 0xF));
                else put(o, (uint8_t)v, (uint8_t)v, (uint8_t)v, (uint8_t)v);
                break;
            }
            case 2: { /* 16-bit */
                uint32_t a = ((row + (uint32_t)x * 2u) ^ xr) & 0xFFFu;
                v = ((uint32_t)tmem[a] << 8) | tmem[(a + 1) & 0xFFFu];
                if (t->fmt == 3) put(o, (uint8_t)(v >> 8), (uint8_t)(v >> 8), (uint8_t)(v >> 8), (uint8_t)v);
                else put_rgba16(o, v);
                break;
            }
            default: { /* 32-bit RGBA: stored contiguously here (rare in this game) */
                uint32_t a = ((row + (uint32_t)x * 4u) ^ xr) & 0xFFFu;
                put(o, tmem[a], tmem[(a + 1) & 0xFFFu], tmem[(a + 2) & 0xFFFu], tmem[(a + 3) & 0xFFFu]);
                break;
            }
            }
        }
    }
}
