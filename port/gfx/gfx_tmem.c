/* Dr. Mario 64 -> Wii native port: TMEM + texture load commands (task 09a).
 * No drawing: this file only models TMEM, the 8 tile descriptors, and the
 * G_SETTIMG / G_SETTILE / G_SETTILESIZE / G_LOADBLOCK / G_LOADTILE /
 * G_LOADTLUT commands.  Bit layouts are from PR/gbi.h (F3DEX_GBI_2):
 *   G_SETTIMG     w0 = op|fmt<<21|siz<<19|(w-1)<<0 ; w1 = address
 *   G_SETTILE     w0 = op|fmt<<21|siz<<19|line<<9|tmem<<0
 *                 w1 = tile<<24|palette<<20|cmt<<18|maskt<<14|shiftt<<10|
 *                      cms<<8|masks<<4|shifts<<0
 *   G_SETTILESIZE w0 = op|uls<<12|ult<<0 ; w1 = tile<<24|lrs<<12|lrt<<0
 *   G_LOADBLOCK   w0 = op|uls<<12|ult<<0 ; w1 = tile<<24|lrs<<12|dxt<<0
 *   G_LOADTILE    w0 = op|uls<<12|ult<<0 ; w1 = tile<<24|lrs<<12|lrt<<0
 *   G_LOADTLUT    w0 = op ; w1 = tile<<24|count<<14
 * LOADBLOCK reads (lrs-uls+1) texels linearly from the source image (raster
 * order from (uls,ult)) and writes them to TMEM in rows of dxt*8 bytes; on
 * odd rows the two 32-bit words of each 8-byte group are swapped, as the
 * hardware does.  LOADTLUT writes 16-bit palette entries into the upper
 * half of TMEM. */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include "gfx_tmem.h"

/* Source-RAM access: on the Wii a G_SETTIMG address is resolved with the
 * walker's segment resolver (direct if >= 0x80000000, else segtable[seg] +
 * offset); on the 64-bit host test the buffers do not fit in 32 bits, so the
 * test supplies a gfx_mem() shim mapping a 32-bit key to a host pointer. */
#ifdef HOST_TEST
extern const uint8_t *gfx_mem(uint32_t addr);
#else
void *gfx_resolve(u32 addr);
#endif

/* ---- the model ---- */
static uint8_t  tmem_bytes[4096] __attribute__((aligned(32)));
static GfxTile  tiles[8];
static uint32_t tmem_gen = 0;

/* texture image state (G_SETTIMG) */
static const uint8_t *src_img;
static uint32_t src_w;     /* image width in texels */
static uint32_t src_siz;   /* texel size code */
static uint32_t src_addr;  /* resolved source address (for the dump/cache key) */

/* ---- helpers ---- */
static int siz_bits(uint32_t siz) {
    switch (siz) {
        case G_IM_SIZ_4b:  return 4;
        case G_IM_SIZ_8b:  return 8;
        case G_IM_SIZ_16b: return 16;
        case G_IM_SIZ_32b: return 32;
        default:           return 16;
    }
}

/* byte address of a 9-bit TMEM pointer: bit8 = bank, bits0-7 = long index. */
static uint32_t tmem_byte(uint32_t tmem) {
    return ((tmem & 0x100u) ? 2048u : 0u) + ((tmem & 0xFFu) * 4u);
}

/* big-endian texel read at linear texel index idx from the source image. */
static uint32_t read_texel(long idx, uint32_t siz) {
    int bits = siz_bits(siz);
    if (bits == 32) {
        const uint8_t *p = src_img + (long)idx * 4;
        return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
               ((uint32_t)p[2] << 8) | p[3];
    }
    if (bits == 16) {
        const uint8_t *p = src_img + (long)idx * 2;
        return ((uint32_t)p[0] << 8) | p[1];
    }
    if (bits == 8) {
        return src_img[idx];
    }
    /* 4-bit: first texel is the high nibble. */
    uint8_t b = src_img[idx >> 1];
    return (idx & 1) ? (b & 0xFu) : (b >> 4);
}

/* pack `count` texel values (each `bits` wide) MSB-first into one 32-bit long. */
static uint32_t pack_long(const uint32_t *vals, int bits, int count) {
    uint32_t out = 0;
    int j;
    for (j = 0; j < count; j++)
        out |= (vals[j] << (bits * (count - 1 - j)));
    return out;
}

/* write one 32-bit long (big-endian) into TMEM at byte offset off. */
static void put_long(uint32_t off, uint32_t v) {
    if (off + 4u <= 4096u) {
        tmem_bytes[off + 0] = (uint8_t)(v >> 24);
        tmem_bytes[off + 1] = (uint8_t)(v >> 16);
        tmem_bytes[off + 2] = (uint8_t)(v >> 8);
        tmem_bytes[off + 3] = (uint8_t)v;
    }
}

/* ---- G_SETTIMG ---- */
static void tm_settimg(uint32_t w0, uint32_t w1) {
    src_siz = (w0 >> 19) & 3u;
    src_w   = ((w0 >> 0) & 0xFFFu) + 1u;
#ifdef HOST_TEST
    src_img  = gfx_mem(w1);
    src_addr = w1;
#else
    void *p = gfx_resolve(w1);
    if (!p)
        p = (void *)(uintptr_t)(w1 >= 0x80000000u ? w1 : (0x80000000u | (w1 & 0xFFFFFFu)));
    src_img  = (const uint8_t *)p;
    src_addr = (uint32_t)(uintptr_t)p;
#endif
}

/* ---- G_SETTILE ---- */
static void tm_settile(uint32_t w0, uint32_t w1) {
    int i = (w1 >> 24) & 7;
    GfxTile *t = &tiles[i];
    t->fmt     = (w0 >> 21) & 7u;
    t->siz     = (w0 >> 19) & 3u;
    t->line    = (w0 >> 9)  & 0x1FFu;
    t->tmem    = (w0 >> 0)  & 0x1FFu;
    t->palette = (w1 >> 20) & 0xFu;
    t->cmt     = (w1 >> 18) & 3u;
    t->maskt   = (w1 >> 14) & 0xFu;
    t->shiftt  = (w1 >> 10) & 0xFu;
    t->cms     = (w1 >> 8)  & 3u;
    t->masks   = (w1 >> 4)  & 0xFu;
    t->shifts  = (w1 >> 0)  & 0xFu;
}

/* ---- G_SETTILESIZE ---- */
static void tm_settilesize(uint32_t w0, uint32_t w1) {
    int i = (w1 >> 24) & 7;
    GfxTile *t = &tiles[i];
    t->uls = (int16_t)((w0 >> 12) & 0xFFFu);
    t->ult = (int16_t)((w0 >> 0)  & 0xFFFu);
    t->lrs = (int16_t)((w1 >> 12) & 0xFFFu);
    t->lrt = (int16_t)((w1 >> 0)  & 0xFFFu);
}

/* TMEM model (consistent with gfx_texconv.c):
 * - tile.tmem and tile.line are in 64-bit words; TMEM byte address = tmem*8, row y starts at tmem*8 + y*line*8.
 * - Odd rows hold their 32-bit words swapped within each 64-bit word: byte offsets on odd rows are XOR 4.
 *   LOADBLOCK produces the swap through dxt (the row counter advances by dxt per 64-bit word, bit 11 = odd row);
 *   LOADTILE swaps on odd destination rows.
 * - TLUT entries are stored quadrupled (8 bytes per 16-bit entry) like the hardware, so a palette loaded at tmem
 *   256 + n*16 lands at byte 0x800 + n*128 and entry i of the 256-entry palette is at 0x800 + i*8. */
static int bytes_per_texel(uint32_t siz) { return siz == 0 ? 0 : (1 << (siz - 1)); } /* 4b handled by callers */

/* ---- G_LOADBLOCK ---- */
static void tm_loadblock(uint32_t w0, uint32_t w1) {
    int ti  = (w1 >> 24) & 7;
    int uls = (int)((w0 >> 12) & 0xFFFu);
    int ult = (int)(w0 & 0xFFFu);
    int lrs = (int)((w1 >> 12) & 0xFFFu);
    int dxt = (int)(w1 & 0xFFFu);
    GfxTile *t = &tiles[ti];
    int bpt = src_siz == 0 ? 1 : bytes_per_texel(src_siz);   /* 4b blocks are loaded as 16b by the macros */
    uint32_t nbytes = (uint32_t)(lrs - uls + 1) * (uint32_t)bpt;
    if (src_siz == 0) nbytes = (uint32_t)(lrs - uls + 1 + 1) / 2;
    const uint8_t *src = src_img + ((uint32_t)ult * src_w + (uint32_t)uls) * (uint32_t)(bpt ? bpt : 1);
    uint32_t base = (uint32_t)t->tmem * 8u, words = (nbytes + 7u) / 8u, i, k, line_ctr = 0;
    if (!src_img) return;
    for (i = 0; i < words; i++) {
        int odd = (line_ctr >> 11) & 1;
        for (k = 0; k < 8; k++) {
            uint32_t sb = i * 8u + k;
            uint32_t db = (base + i * 8u + (odd ? (k ^ 4u) : k)) & 0xFFFu;
            tmem_bytes[db] = sb < nbytes ? src[sb] : 0;
        }
        line_ctr += (uint32_t)dxt;
    }
    tmem_gen++;
}

/* ---- G_LOADTILE ---- */
static void tm_loadtile(uint32_t w0, uint32_t w1) {
    int ti  = (w1 >> 24) & 7;
    int uls = (int)((w0 >> 12) & 0xFFFu) >> 2, ult = (int)(w0 & 0xFFFu) >> 2;
    int lrs = (int)((w1 >> 12) & 0xFFFu) >> 2, lrt = (int)(w1 & 0xFFFu) >> 2;
    GfxTile *t = &tiles[ti];
    int bpt = src_siz == 0 ? 0 : bytes_per_texel(src_siz);
    uint32_t base = (uint32_t)t->tmem * 8u, stride = (uint32_t)t->line * 8u, y, k;
    if (!src_img) return;
    for (y = 0; y <= (uint32_t)(lrt - ult); y++) {
        uint32_t row_bytes = bpt ? (uint32_t)(lrs - uls + 1) * (uint32_t)bpt : (uint32_t)(lrs - uls + 2) / 2;
        const uint8_t *src = src_img + ((uint32_t)(ult + (int)y) * src_w + (uint32_t)uls) * (uint32_t)(bpt ? bpt : 1) / (bpt ? 1 : 2);
        for (k = 0; k < row_bytes; k++) {
            uint32_t off = y * stride + k;
            if (y & 1) off ^= 4u;
            tmem_bytes[(base + off) & 0xFFFu] = src[k];
        }
    }
    tmem_gen++;
}

/* ---- G_LOADTLUT: count = (lrs >> 2) + 1 entries of 16 bits, stored quadrupled ---- */
static void tm_loadtlut(uint32_t w0, uint32_t w1) {
    int ti = (w1 >> 24) & 7;
    int uls = (int)((w0 >> 12) & 0xFFFu) >> 2, lrs = (int)((w1 >> 12) & 0xFFFu) >> 2;
    GfxTile *t = &tiles[ti];
    uint32_t base = (uint32_t)t->tmem * 8u, i, k;
    if (!src_img) return;
    for (i = 0; i <= (uint32_t)(lrs - uls); i++)
        for (k = 0; k < 4; k++) {
            uint32_t db = (base + i * 8u + k * 2u) & 0xFFFu;
            tmem_bytes[db] = src_img[(uls + (int)i) * 2];
            tmem_bytes[(db + 1u) & 0xFFFu] = src_img[(uls + (int)i) * 2 + 1];
        }
    tmem_gen++;
}

/* ---- install ---- */
void gfx_tmem_install(void (*table[256])(uint32_t w0, uint32_t w1)) {
    table[G_SETTIMG     & 0xFF] = tm_settimg;
    table[G_SETTILE     & 0xFF] = tm_settile;
    table[G_SETTILESIZE & 0xFF] = tm_settilesize;
    table[G_LOADBLOCK   & 0xFF] = tm_loadblock;
    table[G_LOADTILE    & 0xFF] = tm_loadtile;
    table[G_LOADTLUT    & 0xFF] = tm_loadtlut;
}

const uint8_t *gfx_tmem(void) { return tmem_bytes; }
const GfxTile *gfx_tile(int i) { return (i >= 0 && i < 8) ? &tiles[i] : NULL; }
uint32_t gfx_timg_addr(void) { return src_addr; }
uint32_t gfx_tmem_gen(void) { return tmem_gen; }
