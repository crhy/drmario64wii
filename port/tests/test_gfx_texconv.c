/* Dr. Mario 64 -> Wii native port: gfx_texconv host self-test (task 09b).
 * Compiled standalone with -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST (GATE).
 * Each format gets one hand-made texel in a 4 KB TMEM buffer; the decoded
 * RGBA8 is checked against values hand-derived from the PR/gbi.h bit layouts
 * (RGBA16=5551, IA4=3I+1A, IA8=4+4, IA16=12I+4A, I=opaque, CI=TLUT index).
 * A 2-row RGBA16 tile exercises the 64-bit-word line stride and the LOADBLOCK
 * odd-row 32-bit word swap (recovered by reading addr ^ 4 on odd rows). */

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
#include "gfx_texconv.h"

static uint8_t tmem[4096];
static int g_fail = 0;

/* gfx_tmem.c references gfx_mem() under HOST_TEST; this test builds TMEM by
 * hand and never loads a source image, so the shim just returns NULL. */
const uint8_t *gfx_mem(uint32_t addr) {
    (void)addr;
    return 0;
}

/* put a big-endian 32-bit long at byte off. */
static void put_long(uint32_t off, uint32_t v) {
    tmem[off]     = (uint8_t)(v >> 24);
    tmem[off + 1] = (uint8_t)(v >> 16);
    tmem[off + 2] = (uint8_t)(v >> 8);
    tmem[off + 3] = (uint8_t)v;
}

/* one-texel decode helper: build a tile, decode 1x1, compare RGBA. */
static void one(const char *name, uint8_t fmt, uint8_t siz, uint32_t line,
                uint32_t pal, uint32_t otm_h, uint32_t exp) {
    GfxTile t;
    memset(&t, 0, sizeof t);
    t.fmt = fmt; t.siz = siz; t.line = (uint16_t)line; t.tmem = 0;
    t.palette = (uint16_t)pal;
    uint8_t out[4] = {0, 0, 0, 0};
    gfx_decode_tile(tmem, &t, 1, 1, out, otm_h);
    uint32_t got = ((uint32_t)out[0] << 24) | ((uint32_t)out[1] << 16) |
                   ((uint32_t)out[2] << 8) | out[3];
    if (got == exp) {
        printf("%s ok (0x%08x)\n", name, (unsigned)got);
    } else {
        printf("%s FAIL got=0x%08x exp=0x%08x\n", name, (unsigned)got, (unsigned)exp);
        g_fail++;
    }
}

int main(void)
{
    uint32_t otm_rgba16 = (uint32_t)G_TT_RGBA16;   /* TEXTLUT field = 2 */
    uint32_t otm_ia16   = (uint32_t)G_TT_IA16;     /* TEXTLUT field = 3 */

    /* ---- RGBA16 5551: texel 0xFFFF -> opaque white ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0xFFFF0000u);                      /* texel in high half */
    one("rgba16", G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, otm_rgba16, 0xFFFFFFFFu);

    /* ---- RGBA32: 0x11223344 -> (11,22,33,44) ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0x11223344u);
    one("rgba32", G_IM_FMT_RGBA, G_IM_SIZ_32b, 0, 0, otm_rgba16, 0x11223344u);

    /* ---- IA4: raw 0xA -> I3=5 -> 0xB6, A1=0 -> 0x00 ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0xA0000000u);
    one("ia4", G_IM_FMT_IA, G_IM_SIZ_4b, 0, 0, otm_rgba16, 0xB6B6B600u);

    /* ---- IA8: raw 0x5A -> I4=5->0x55, A4=A->0xAA ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0x5A000000u);
    one("ia8", G_IM_FMT_IA, G_IM_SIZ_8b, 0, 0, otm_rgba16, 0x555555AAu);

    /* ---- IA16: raw 0xABCD -> I8=0xAB, A4=D->0xDD ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0xABCD0000u);
    one("ia16", G_IM_FMT_IA, G_IM_SIZ_16b, 0, 0, otm_rgba16, 0xABABABDDu);

    /* ---- I4: raw 0xA -> 0xAA, opaque ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0xA0000000u);
    one("i4", G_IM_FMT_I, G_IM_SIZ_4b, 0, 0, otm_rgba16, 0xAAAAAAFFu);

    /* ---- I8: raw 0x5A -> 0x5A, opaque ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0x5A000000u);
    one("i8", G_IM_FMT_I, G_IM_SIZ_8b, 0, 0, otm_rgba16, 0x5A5A5AFFu);

    /* ---- CI4: index 0 -> TLUT[0]=0x1234 as RGBA16 -> (10,42,D6,00) ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0x00000000u);                      /* index 0 */
    tmem[0x800] = 0x12; tmem[0x801] = 0x34;        /* TLUT entry 0 */
    one("ci4_rgba16", G_IM_FMT_CI, G_IM_SIZ_4b, 0, 0, otm_rgba16, 0x1042D600u);

    /* ---- CI8: index 1 -> TLUT[1]=0x5A0F as IA16 -> (5A,5A,5A,FF) ---- */
    memset(tmem, 0, sizeof tmem);
    put_long(0, 0x01000000u);                      /* index 1 (high byte) */
    tmem[0x802] = 0x5A; tmem[0x803] = 0x0F;        /* TLUT entry 1 */
    one("ci8_ia16", G_IM_FMT_CI, G_IM_SIZ_8b, 0, 0, otm_ia16, 0x5A5A5AFFu);

    /* ---- 2-row RGBA16, line=1 (8-byte stride), odd-row word swap ---- */
    memset(tmem, 0, sizeof tmem);
    /* row 0 (even, no swap): texels AAAA BBBB CCCC DDDD */
    put_long(0, 0xAAAABBBBu);
    put_long(4, 0xCCCCDDDDu);
    /* row 1 (odd) at byte 8: LOADBLOCK swapped the two longs, so TMEM holds
     * long1=0x33334444 at byte 8 and long0=0x11112222 at byte 12. */
    put_long(8, 0x33334444u);
    put_long(12, 0x11112222u);
    {
        GfxTile t;
        memset(&t, 0, sizeof t);
        t.fmt = G_IM_FMT_RGBA; t.siz = G_IM_SIZ_16b; t.line = 1; t.tmem = 0;
        uint8_t out[4 * 4 * 2];
        gfx_decode_tile(tmem, &t, 4, 2, out, otm_rgba16);
        /* row0 texels -> 5551 expand of AAAA,BBBB,CCCC,DDDD */
        static const uint32_t exp0[4] = {0xAD52AD00u, 0xBD73EFFFu, 0xCE9C3100u, 0xDEBD73FFu};
        /* row1 recovered: 1111,2222,3333,4444 */
        static const uint32_t exp1[4] = {0x102142FFu, 0x21428C00u, 0x3163CEFFu, 0x428C1000u};
        int x;
        for (x = 0; x < 4; x++) {
            uint32_t g0 = ((uint32_t)out[x*4] << 24) | ((uint32_t)out[x*4+1] << 16) |
                          ((uint32_t)out[x*4+2] << 8) | out[x*4+3];
            uint32_t g1 = ((uint32_t)out[16 + x*4] << 24) | ((uint32_t)out[16 + x*4+1] << 16) |
                          ((uint32_t)out[16 + x*4+2] << 8) | out[16 + x*4+3];
            if (g0 != exp0[x]) { printf("swap.r0.%d FAIL got=%08x exp=%08x\n", x, g0, exp0[x]); g_fail++; }
            if (g1 != exp1[x]) { printf("swap.r1.%d FAIL got=%08x exp=%08x\n", x, g1, exp1[x]); g_fail++; }
        }
        printf("swap rows checked\n");
    }

    if (g_fail == 0) {
        printf("GFX_TEXCONV PASS\n");
        return 0;
    }
    printf("GFX_TEXCONV FAIL (%d)\n", g_fail);
    return 1;
}
