/* Dr. Mario 64 -> Wii native port: gfx_tmem host self-test (task 09a).
 * Compiled standalone with -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST (GATE).
 *
 * Exercises the texture-load handlers through the same 256-entry table the
 * walker uses.  Expected values are hand-derived from the F3DEX_GBI_2 bit
 * layouts in PR/gbi.h and the documented LOADBLOCK odd-line word swap. */

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include "gfx_tmem.h"

/* ---- N64 data buffers; gfx_mem() maps a 32-bit key to a host pointer ---- */
static uint8_t tex_buf[64];   /* 16x2 RGBA16: texel i = i */
static uint8_t lut_buf[32];   /* 16 TLUT entries, 16-bit each */
static uint8_t big_tex[64 * 16 * 2]; /* 64x16 RGBA16: texel (x,y) = y*64+x */

const uint8_t *gfx_mem(uint32_t addr) {
    if (addr == 0x1000u) return tex_buf;
    if (addr == 0x2000u) return lut_buf;
    if (addr == 0x3000u) return big_tex;
    return 0;
}

/* ---- per-opcode handler table, filled by gfx_tmem_install ---- */
static void (*table[256])(uint32_t w0, uint32_t w1);

static int g_pass = 1;
static int g_fail = 0;

#define CHECK32(name, got, exp) do { \
    if ((got) == (exp)) { \
        printf("%s ok (0x%x)\n", name, (unsigned)(got)); \
    } else { \
        printf("%s FAIL got=0x%x exp=0x%x\n", name, (unsigned)(got), (unsigned)(exp)); \
        g_pass = 0; g_fail++; \
    } \
} while (0)

/* read a big-endian 32-bit long out of the TMEM model at byte offset off. */
static uint32_t tmem_long(uint32_t off) {
    const uint8_t *m = gfx_tmem();
    return ((uint32_t)m[off] << 24) | ((uint32_t)m[off + 1] << 16) |
           ((uint32_t)m[off + 2] << 8) | m[off + 3];
}

int main(void)
{
    uint32_t w0, w1;
    int i;

    gfx_tmem_install(table);

    /* ---------- build the 16x2 RGBA16 source: texel i = i ---------- */
    memset(tex_buf, 0, sizeof tex_buf);
    for (i = 0; i < 32; i++)
        tex_buf[2 * i + 1] = (uint8_t)i;   /* 16-bit BE value == i */

    /* ---------- G_SETTIMG: fmt RGBA(0), siz 16b(2), width 16 ---------- */
    w0 = ((uint32_t)G_SETTIMG << 24) | (0u << 21) | (2u << 19) | ((16u - 1u) << 0);
    w1 = 0x1000u;
    table[G_SETTIMG & 0xFF](w0, w1);

    /* ---------- G_SETTILE tile 7 (G_TX_LOADTILE): line 0, tmem 0 ---------- */
    w0 = ((uint32_t)G_SETTILE << 24) | (0u << 21) | (2u << 19) | (0u << 9) | (0u << 0);
    w1 = ((uint32_t)7u << 24);
    table[G_SETTILE & 0xFF](w0, w1);
    CHECK32("tile7.tmem", gfx_tile(7)->tmem, 0);
    CHECK32("tile7.siz", gfx_tile(7)->siz, G_IM_SIZ_16b);

    /* ---------- G_LOADBLOCK: uls=0 ult=0 lrs=31, dxt = CALC_DXT = 2048/4 words = 512 (32-byte lines) ---------- */
    w0 = ((uint32_t)G_LOADBLOCK << 24) | (0u << 12) | (0u << 0);
    w1 = ((uint32_t)7u << 24) | (31u << 12) | (512u << 0);
    table[G_LOADBLOCK & 0xFF](w0, w1);

    /* row 0 (even): longs stored as-is. long0 = texels(0,1) = 0x00000001. */
    CHECK32("blk.r0.l0", tmem_long(0), 0x00000001u);
    CHECK32("blk.r0.l1", tmem_long(4), 0x00020003u);
    /* row 1 (odd) at byte 32: source long0=(16,17)=0x00100011, long1=(18,19)=0x00120013.
     * hardware swaps the pair, so TMEM long0 = 0x00120013, long1 = 0x00100011. */
    CHECK32("blk.r1.l0(swapped)", tmem_long(32), 0x00120013u);
    CHECK32("blk.r1.l1(swapped)", tmem_long(36), 0x00100011u);

    /* ---------- G_LOADTLUT: 16 entries land at TMEM 0x800+ ---------- */
    for (i = 0; i < 16; i++) {
        lut_buf[2 * i]     = (uint8_t)(0x10u + (i >> 8));
        lut_buf[2 * i + 1] = (uint8_t)(0x00u + i);   /* entry i = 0x1000 + i */
    }
    /* re-point the image at the LUT and set tile 7 tmem to the upper half. */
    w0 = ((uint32_t)G_SETTIMG << 24) | (0u << 21) | (2u << 19) | ((16u - 1u) << 0);
    w1 = 0x2000u;
    table[G_SETTIMG & 0xFF](w0, w1);
    w0 = ((uint32_t)G_SETTILE << 24) | (0u << 21) | (2u << 19) | (0u << 9) | (0x100u << 0);
    w1 = ((uint32_t)7u << 24);
    table[G_SETTILE & 0xFF](w0, w1);
    w0 = ((uint32_t)G_LOADTLUT << 24);
    w1 = ((uint32_t)7u << 24) | (16u << 14);
    table[G_LOADTLUT & 0xFF](w0, w1);

    const uint8_t *m = gfx_tmem();
    CHECK32("tlut.at_0x800", (m[0x800] << 8) | m[0x801], 0x1000);
    /* TMEM stores each TLUT entry 4x (quadricated): entry i at 0x800 + i*8, replicas at +2,+4,+6 */
    CHECK32("tlut.at_0x802", (m[0x802] << 8) | m[0x803], 0x1000);
    CHECK32("tlut.e1_at_0x808", (m[0x808] << 8) | m[0x809], 0x1001);
    CHECK32("tlut.e15_at_0x878", (m[0x878] << 8) | m[0x879], 0x100f);
    CHECK32("tlut.below_0x800", m[0x7fe], 0);   /* nothing spilled below the bank */

    /* ---------- G_LOADTILE with uls=32 ult=4 (non-zero tile origin) ---------- */
    /* 64x16 RGBA16 source: texel (x,y) = y*64+x */
    for (i = 0; i < 64 * 16; i++) {
        big_tex[2 * i + 0] = (uint8_t)(i >> 8);
        big_tex[2 * i + 1] = (uint8_t)(i & 0xFF);
    }
    w0 = ((uint32_t)G_SETTIMG << 24) | (0u << 21) | (2u << 19) | ((64u - 1u) << 0);
    w1 = 0x3000u;
    table[G_SETTIMG & 0xFF](w0, w1);
    /* tile 0: line=4 (32 bytes/row), tmem=0 */
    w0 = ((uint32_t)G_SETTILE << 24) | (0u << 21) | (2u << 19) | (4u << 9) | (0u << 0);
    w1 = ((uint32_t)0u << 24);
    table[G_SETTILE & 0xFF](w0, w1);
    /* LOADTILE uls=32 ult=4 lrs=47 lrt=7 (10.2 raw: 128,16,188,28) */
    w0 = ((uint32_t)G_LOADTILE << 24) | (128u << 12) | (16u << 0);
    w1 = ((uint32_t)0u << 24) | (188u << 12) | (28u << 0);
    table[G_LOADTILE & 0xFF](w0, w1);
    /* row 0 (even): starts with texel (32,4) = 4*64+32 = 288 = 0x0120 */
    CHECK32("lt.r0.origin", (m[0] << 8) | m[1], 288u);
    /* row 1 (odd, swapped): texel (32,5)=352=0x0160 lands at byte 36 (swap moves it) */
    CHECK32("lt.r1.origin.swap", (m[36] << 8) | m[37], 352u);

    if (g_fail == 0) {
        printf("GFX_TMEM PASS\n");
        return 0;
    }
    printf("GFX_TMEM FAIL (%d)\n", g_fail);
    return 1;
}
