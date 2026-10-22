/* Dr. Mario 64 -> Wii native port: gfx_xform host self-test (task 08b).
 * Compiled standalone with -DTARGET_WII -DF3DEX_GBI_2 -DHOST_TEST (GATE).
 *
 * Exercises gfx_xform.c's handlers through the same 256-entry table the
 * walker uses.  Every expected value is hand-derived from the F3DEX_GBI_2
 * bit layouts in PR/gbi.h and the s15.16 Mtx / 1/4-pixel Vp formats.
 *
 * gfx_xform.c reads N64 data big-endian and receives a 32-bit RAM address.
 * On the Wii that address is a real pointer; on the 64-bit host it is not,
 * so we supply a gfx_mem() shim that maps a small 32-bit key to a host
 * buffer.  The handlers themselves are byte-for-byte the Wii code.
 */

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include "gfx.h"

/* gfx_xform.c owns the single global state instance. */
extern gfx_state gfx_st;

/* ---- big-endian writers to build N64 Mtx/Vtx/Vp byte arrays ---- */
static void put_be16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFFu);
}
/* s15.16 value -> Mtx slot k (integer part then fraction). */
static void put_mtx(uint8_t *m, int k, int32_t val) {
    put_be16(m + 2 * k, (uint16_t)(val >> 16));
    put_be16(m + 32 + 2 * k, (uint16_t)(val & 0xFFFF));
}

/* ---- N64 data buffers.  The handlers only ever see a 32-bit key; gfx_mem()
 * maps that key to a host pointer, so the buffers may live anywhere. ---- */
static uint8_t mtx_buf[64], vp_buf[16], vtx_buf[16];

const uint8_t *gfx_mem(u32 addr) {
    if (addr == 0x1000u) return mtx_buf;
    if (addr == 0x2000u) return vp_buf;
    if (addr == 0x3000u) return vtx_buf;
    return 0;
}

/* ---- per-opcode handler table, filled by gfx_xform_install ---- */
static void (*table[256])(u32 w0, u32 w1);

static int g_pass = 1;
static int g_fail = 0;

#define CHECK32(name, got, exp) do { \
    if ((got) == (exp)) { \
        printf("%s ok (%d)\n", name, (int)(got)); \
    } else { \
        printf("%s FAIL got=%d exp=%d\n", name, (int)(got), (int)(exp)); \
        g_pass = 0; g_fail++; \
    } \
} while (0)

/* G_MTX w0: op | (((64-1)/8)<<19) | (ofs/8<<8) | idx, idx = p ^ G_MTX_PUSH. */
static uint32_t mtx_w0(uint32_t p) {
    uint32_t idx = p ^ G_MTX_PUSH;
    return ((uint32_t)G_MTX << 24) | (7u << 19) | idx;
}
/* G_MOVEMEM viewport w0: op | (((16-1)/8)<<19) | (0<<8) | G_MV_VIEWPORT. */
static uint32_t vp_w0(void) {
    return ((uint32_t)G_MOVEMEM << 24) | (1u << 19) | G_MV_VIEWPORT;
}
/* G_VTX w0: op | (n<<12) | ((v0+n)<<1). */
static uint32_t vtx_w0(int n, int v0) {
    return ((uint32_t)G_VTX << 24) | ((uint32_t)n << 12) | ((uint32_t)(v0 + n) << 1);
}

static void build_identity(uint8_t *m) {
    int i;
    memset(m, 0, 64);
    for (i = 0; i < 4; i++)
        put_mtx(m, i * 4 + i, 0x10000);
}

/* identity with translation (tx,ty,tz) in the last column. */
static void build_translation(uint8_t *m, int tx, int ty, int tz) {
    build_identity(m);
    put_mtx(m, 0 * 4 + 3, (int32_t)tx << 16);
    put_mtx(m, 1 * 4 + 3, (int32_t)ty << 16);
    put_mtx(m, 2 * 4 + 3, (int32_t)tz << 16);
}

int main(void)
{
    u32 w0, w1;
    int i, j;

    gfx_xform_install(table);

    /* ---------- Mtx decode: identity, LOAD MODELVIEW NOPUSH ---------- */
    build_identity(mtx_buf);
    w0 = mtx_w0(G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    w1 = 0x1000u;
    table[G_MTX & 0xFF](w0, w1);
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            int32_t exp = (i == j) ? 0x10000 : 0;
            char nm[32];
            snprintf(nm, sizeof nm, "id.mv[%d][%d]", i, j);
            CHECK32(nm, gfx_st.mv[i][j], exp);
        }

    /* ---------- Mtx decode: translation (1,2,3), LOAD MODELVIEW PUSH ---------- */
    build_translation(mtx_buf, 1, 2, 3);
    w0 = mtx_w0(G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    w1 = 0x1000u;
    table[G_MTX & 0xFF](w0, w1);
    CHECK32("tr.mv[0][3]", gfx_st.mv[0][3], 1 << 16);
    CHECK32("tr.mv[1][3]", gfx_st.mv[1][3], 2 << 16);
    CHECK32("tr.mv[2][3]", gfx_st.mv[2][3], 3 << 16);
    CHECK32("tr.mv[0][0]", gfx_st.mv[0][0], 0x10000);
    CHECK32("tr.msp", gfx_st.msp, 1);   /* PUSH saved the previous (identity) mv */

    /* ---------- viewport: 320x240 -> vscale/vtrans = 640,480 (1/4 px) ---------- */
    memset(vp_buf, 0, 16);
    put_be16(vp_buf + 0, 640);   /* vscale x */
    put_be16(vp_buf + 2, 480);   /* vscale y */
    put_be16(vp_buf + 8, 640);   /* vtrans x */
    put_be16(vp_buf + 10, 480);  /* vtrans y */
    table[G_MOVEMEM & 0xFF](vp_w0(), 0x2000u);
    CHECK32("vp.vscale[0]", gfx_st.vscale[0], 640);
    CHECK32("vp.vscale[1]", gfx_st.vscale[1], 480);
    CHECK32("vp.vtrans[0]", gfx_st.vtrans[0], 640);
    CHECK32("vp.vtrans[1]", gfx_st.vtrans[1], 480);

    /* ---------- reset mv and load identity pr so the MVP is identity ---------- */
    build_identity(mtx_buf);
    w0 = mtx_w0(G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    w1 = 0x1000u;
    table[G_MTX & 0xFF](w0, w1);
    w0 = mtx_w0(G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    w1 = 0x1000u;
    table[G_MTX & 0xFF](w0, w1);

    /* ---------- one vertex: mv=identity, pr=identity, pos=(1,0,0) ---------- */
    /* clip = (1,0,0) in s15.16; sx=(0x10000*640 + 640<<16)>>18 = 320, sy=120. */
    memset(vtx_buf, 0, 16);
    put_be16(vtx_buf + 0, 1);    /* x = 1 */
    put_be16(vtx_buf + 2, 0);    /* y = 0 */
    put_be16(vtx_buf + 4, 0);    /* z = 0 */
    vtx_buf[12] = 0x11;          /* r */
    vtx_buf[13] = 0x22;          /* g */
    vtx_buf[14] = 0x33;          /* b */
    vtx_buf[15] = 0x44;          /* a */
    table[G_VTX & 0xFF](vtx_w0(1, 0), 0x3000u);
    CHECK32("v0.cx", gfx_st.vtx[0].cx, 1 << 16);
    CHECK32("v0.cy", gfx_st.vtx[0].cy, 0);
    CHECK32("v0.sx", gfx_st.vtx[0].sx, 320);
    CHECK32("v0.sy", gfx_st.vtx[0].sy, 120);
    CHECK32("v0.color", (int)gfx_st.vtx[0].color, 0x11223344);
    CHECK32("nvtx", gfx_st.nvtx, 1);

    if (g_fail == 0) {
        printf("GFX_XFORM PASS\n");
        return 0;
    }
    printf("GFX_XFORM FAIL (%d)\n", g_fail);
    return 1;
}
