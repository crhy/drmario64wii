#ifndef GFX_TMEM_H
#define GFX_TMEM_H

/* TMEM emulation + texture load commands (task 09a, no drawing).
 * gfx_tmem.c owns a 4 KB TMEM byte array and 8 tile descriptors, and
 * registers handlers for G_SETTIMG / G_SETTILE / G_SETTILESIZE /
 * G_LOADBLOCK / G_LOADTILE / G_LOADTLUT into the walker's opcode table
 * via gfx_tmem_install().  Bit layouts come from PR/gbi.h (F3DEX_GBI_2). */

typedef struct {
    uint8_t  fmt, siz;       /* image format + texel size code (G_IM_SIZ_*) */
    uint16_t line;           /* line length in longs (32-bit words) */
    uint16_t tmem;           /* TMEM start: bit8 = bank, bits0-7 = long idx */
    uint16_t palette;        /* TLUT entry offset */
    uint8_t  cms, cmt, masks, maskt, shifts, shiftt;
    int16_t  uls, ult, lrs, lrt; /* tile size in texels (10.5) */
} GfxTile;

/* The 4 KB TMEM byte array (byte-addressed; long 0 = bytes 0..3). */
const uint8_t *gfx_tmem(void);

/* Tile descriptor i (0..7); NULL if out of range. */
const GfxTile *gfx_tile(int i);

/* Fill a 256-entry per-opcode handler table with the load handlers. */
void gfx_tmem_install(void (*table[256])(uint32_t w0, uint32_t w1));

/* Resolved source address of the most recent G_SETTIMG (0 if none). */
uint32_t gfx_timg_addr(void);

/* Generation counter: incremented by every LOADBLOCK/LOADTILE/LOADTLUT. */
uint32_t gfx_tmem_gen(void);

#endif
