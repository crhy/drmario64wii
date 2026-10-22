/* Dr. Mario 64 -> Wii native port: display-list state handlers (task 08b).
 *
 * Registers per-opcode handlers that mutate gfx_st (matrices, viewport,
 * transformed vertices, mode/colour state).  No drawing yet.  Self-contained
 * for the host test: it does not call PORT_LOG or reference gfx.c.
 *
 * All N64 data (Mtx/Vtx/Vp) is big-endian in RAM; we read it byte-wise so the
 * code is endianness-independent (Wii is BE, the host test is LE).
 *
 * Bit layouts (F3DEX_GBI_2, from PR/gbi.h):
 *  G_MTX      w0 = op | (((64-1)/8)<<19) | (idx) ; idx = params ^ G_MTX_PUSH
 *             w1 = Mtx address.  Mtx = 16 s16 ints then 16 u16 fracs (s15.16).
 *  G_VTX      w0 = op | (n<<12) | ((v0+n)<<1) ; w1 = Vtx array address.
 *  G_MOVEMEM  w0 = op | (1<<19) | (ofs/8<<8) | idx ; idx==G_MV_VIEWPORT -> Vp.
 *  G_TEXTURE  w1 = (s<<16)|t ; w0: level=(>>11)&7 tile=(>>8)&7 on=(>>1)&0x7f.
 *  G_SETSCISSOR w0: ulx=(>>12)&0xfff uly=&0xfff ; w1: lrx=(>>12)&0xfff lry=&0xfff
 *             (coords in 1/4 pixel units).
 *  colour ops: w1 = (r<<24)|(g<<16)|(b<<8)|a (G_SETPRIM/ENV/BLEND); G_SETFILL w1
 *             is the packed colour word.
 */

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#include <stddef.h>
#include <stdint.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include <string.h>
#define GFX_NEED_SPTASK
#include "gfx.h"
#ifdef TARGET_WII
#include "port_log.h"
/* libogc cache op (ogc/cache.h declares it as an alias of PPCDCacheInvalidate). */
void DCInvalidateRange(void *startaddress, unsigned int len) __asm__("PPCDCacheInvalidate");
/* GX + DCFlushRange prototypes declared manually: including <ogc/gx.h> here
 * clashes with PR/gbi.h (Mtx/Vtx) and PR/ultratypes.h (vu32/vs32). */
extern void GX_DrawDone(void);
extern void GX_SetTexCopySrc(unsigned short left, unsigned short top, unsigned short w, unsigned short h); /* u16 in libogc: NOT float */
extern void GX_SetTexCopyDst(unsigned short w, unsigned short h, unsigned char tfmt, unsigned char clamp);
extern void GX_CopyTex(void *dst, unsigned char clear);
extern void GX_PixModeSync(void);
/* GXTexObj layout from <ogc/gx.h> (gx.h itself clashes with PR/gbi.h Mtx/Vtx).
 * Must match libogc exactly: GX_InitTexObj writes it, GX_LoadTexObj reads it. */
typedef struct _gx_texobj { uint32_t val[8]; } GXTexObj;
/* PPC cache flush: libogc's DCFlushRange is an inline in <ogc/cache.h>, but that
 * header clashes with PR/ultratypes.h, so provide the same dcbf+sync here. */
static inline void gfx_dc_flush(void *start, unsigned int len)
{
    unsigned char *p = (unsigned char *)start;
    unsigned int i;
    for (i = 0; i < len; i += 32u)
        __asm__ __volatile__("dcbf 0, %0" :: "r"(p + i));
    __asm__ __volatile__("sync" ::: "memory");
}
#define GFX_GX_TF_RGBA8 6u
#define GFX_GX_TF_RGB565 4u
#define GFX_GX_TRUE     1u
#define GFX_GX_FALSE    0u
#define GFX_GX_CLAMP    0u
/* MEM_K0_TO_K1 + arena2 from <ogc/system.h> (clashes with PR/ultratypes.h). */
#define GFX_MEM_K0_TO_K1(x) ((uint8_t *)((uint32_t)(x) + 0x20000000u))
extern void *SYS_GetArena2Lo(void);
extern void SYS_SetArena2Lo(void *newLo);
extern void GX_InitTexObj(GXTexObj *obj, void *img_ptr, unsigned short wd,
                         unsigned short ht, unsigned char fmt,
                         unsigned char wrap_s, unsigned char wrap_t,
                         unsigned char mipmap);
#endif

/* F3DEX_GBI_2's gbi.h block omits the geometry-mode opcodes; supply them. */
#ifdef F3DEX_GBI_2
#ifndef G_SETGEOMETRYMODE
#define G_SETGEOMETRYMODE 0xB7
#endif
#ifndef G_CLEARGEOMETRYMODE
#define G_CLEARGEOMETRYMODE 0xB6
#endif
#endif

gfx_state gfx_st;

/* N64 RAM is 32-bit addressable on the Wii, so handlers receive a uint32_t
 * address and read it directly.  On the 64-bit host test the real buffers do
 * not fit in 32 bits, so the test supplies a gfx_mem() shim that maps a 32-bit
 * key to a host pointer.  On the Wii this is exactly the raw cast. */
#ifdef HOST_TEST
extern const uint8_t *gfx_mem(u32 addr);
#else
extern void *gfx_resolve(u32 addr);
/* Segmented addresses (e.g. 0x05xxxxxx asset data) must go through the segment table. */
#define gfx_mem(a) ((const uint8_t *)gfx_resolve(a))
#endif

/* ---- big-endian readers (endianness-independent) ---- */
static int16_t be16(const uint8_t *p) {
    return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static uint16_t beu16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

/* ---- matrix helpers (s15.16, row-major) ---- */
static void mtx_decode(const uint8_t *d, int32_t out[4][4]) {
    int i, j, k = 0;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++, k++) {
            int16_t ip = be16(d + 2 * k);
            uint16_t fp = beu16(d + 32 + 2 * k);
            out[i][j] = ((int32_t)ip << 16) | (int32_t)fp;
        }
}

static void mtx_mul(const int32_t a[4][4], const int32_t b[4][4],
                    int32_t out[4][4]) {
    int i, j, k;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            int64_t acc = 0;
            for (k = 0; k < 4; k++)
                acc += (int64_t)a[i][k] * b[k][j];
            out[i][j] = (int32_t)(acc >> 16);
        }
}

static void mtx_copy(const int32_t src[4][4], int32_t dst[4][4]) {
    memcpy(dst, src, sizeof(int32_t) * 16);
}

/* ---- G_MTX ---- */
static void xf_mtx(u32 w0, u32 w1) {
    uint32_t p = (w0 & 0xFFu) ^ G_MTX_PUSH;
    int is_proj = (p & G_MTX_PROJECTION) != 0;
    int load = (p & G_MTX_LOAD) != 0;
    int push = (p & G_MTX_PUSH) != 0;
    int32_t m[4][4];
    const uint8_t *d = gfx_mem(w1);
    mtx_decode(d, m);
    if (is_proj) {
        if (load)
            mtx_copy(m, gfx_st.pr);
        else {  /* row vectors: new = M * current; mtx_mul must not alias its output */
            int32_t t[4][4];
            mtx_mul(m, gfx_st.pr, t);
            mtx_copy(t, gfx_st.pr);
        }
    } else {
        if (push && gfx_st.msp < 8) {
            mtx_copy(gfx_st.mv, gfx_st.mstack[gfx_st.msp]);
            gfx_st.msp++;
        }
        if (load)
            mtx_copy(m, gfx_st.mv);
        else {
            int32_t t[4][4];
            mtx_mul(m, gfx_st.mv, t);
            mtx_copy(t, gfx_st.mv);
        }
    }
}

static void xf_popmtx(u32 w0, u32 w1) {
    (void)w0;
    (void)w1;
    if (gfx_st.msp > 0) {
        gfx_st.msp--;
        mtx_copy(gfx_st.mstack[gfx_st.msp], gfx_st.mv);
    }
}

/* ---- G_MOVEMEM viewport ---- */
static void xf_movemem(u32 w0, u32 w1) {
    int idx = w0 & 0xFF;
    if (idx != G_MV_VIEWPORT)
        return;
    const uint8_t *d = gfx_mem(w1);
    int i;
    for (i = 0; i < 4; i++) {
        gfx_st.vscale[i] = be16(d + 2 * i);
        gfx_st.vtrans[i] = be16(d + 8 + 2 * i);
    }
}

/* ---- G_VTX: transform by modelview*projection, store clip + screen ---- */
static void xf_vtx(u32 w0, u32 w1) {
    int n = (w0 >> 12) & 0xFF;
    int v0 = (int)(((w0 >> 1) & 0x7F)) - n;
    const uint8_t *d = gfx_mem(w1);
    int32_t mvp[4][4];
    int i;
    if (d == NULL) {
        static int logged = 0;
#ifdef TARGET_WII
        if (!logged) { logged = 1; PORT_LOG("gfx: vtx d NULL w1=%08x", (unsigned)w1); }
#else
        (void)logged;
#endif
        return;
    }
    mtx_mul(gfx_st.mv, gfx_st.pr, mvp);
    for (i = 0; i < n && (v0 + i) >= 0 && (v0 + i) < 32; i++) {
        const uint8_t *v = d + 16 * i;
        int16_t x = be16(v + 0), y = be16(v + 2), z = be16(v + 4);
        int16_t s = be16(v + 8), t = be16(v + 10);
        uint32_t col = ((uint32_t)v[12] << 24) | ((uint32_t)v[13] << 16) |
                       ((uint32_t)v[14] << 8) | v[15];
        /* N64 vertices are row vectors: clip = v * (MV * P); translation lives in row 3. */
        int64_t px = x, py = y, pz = z;
        int32_t c4[4];
        int j;
        for (j = 0; j < 4; j++)
            c4[j] = (int32_t)(px * mvp[0][j] + py * mvp[1][j] + pz * mvp[2][j] + mvp[3][j]);  /* 16.16 */
        int32_t cx = c4[0], cy = c4[1], cz = c4[2], cw = c4[3];
        if (cw == 0) cw = 1;
        gfx_st.vtx[v0 + i].cx = cx;
        gfx_st.vtx[v0 + i].cy = cy;
        gfx_st.vtx[v0 + i].cz = cz;
        gfx_st.vtx[v0 + i].cw = cw;
        int32_t sx, sy;
        float zscr;
        gfx_project(cx, cy, cz, cw, &sx, &sy, &zscr);
        gfx_st.vtx[v0 + i].sx = sx;
        gfx_st.vtx[v0 + i].sy = sy;
        gfx_st.vtx[v0 + i].color = col;
        gfx_st.vtx[v0 + i].s = s;
        gfx_st.vtx[v0 + i].t = t;
    }
    {
        int end = v0 + n;
        if (end > 32) end = 32;
        if (end > gfx_st.nvtx) gfx_st.nvtx = end;
    }
}

/* ---- mode / colour / image handlers ---- */
/* F3DEX2 has ONE geometry-mode opcode (0xD9): w0 low 24 bits = AND mask (bits to keep), w1 = bits to set. */
static void xf_geo(u32 w0, u32 w1) {
    gfx_st.geo = (gfx_st.geo & (w0 | 0xFF000000u)) | w1;
}
/* G_SETOTHERMODE_H/L update ONE field: w0 = op | (32 - shift - len) << 8 | (len - 1), w1 = field bits in place. */
static uint32_t othm_mask(u32 w0) {
    uint32_t len = (w0 & 0xFFu) + 1u, sft = 32u - ((w0 >> 8) & 0xFFu) - len;
    return (len >= 32u) ? 0xFFFFFFFFu : (((1u << len) - 1u) << sft);
}
static void xf_othm_h(u32 w0, u32 w1) {
    uint32_t m = othm_mask(w0);
    gfx_st.otm_h = (gfx_st.otm_h & ~m) | (w1 & m);
}
static void xf_othm_l(u32 w0, u32 w1) {
    uint32_t m = othm_mask(w0);
    gfx_st.otm_l = (gfx_st.otm_l & ~m) | (w1 & m);
}
/* G_RDPSETOTHERMODE (0xEF, gDPSetOtherMode): w0 low 24 bits = whole H word, w1 = whole L word. */
static void xf_othm_both(u32 w0, u32 w1) {
    gfx_st.otm_h = w0 & 0x00FFFFFFu;
    gfx_st.otm_l = w1;
}
static void xf_texture(u32 w0, u32 w1) {
    gfx_st.tex_s = (w1 >> 16) & 0xFFFFu;
    gfx_st.tex_t = w1 & 0xFFFFu;
    gfx_st.tex_level = (w0 >> 11) & 7u;
    gfx_st.tex_on = (w0 >> 1) & 0x7Fu;
}
static void xf_prim(u32 w0, u32 w1) {
    (void)w0;
    gfx_st.prim = w1;
}
static void xf_env(u32 w0, u32 w1) {
    (void)w0;
    gfx_st.env = w1;
}
static void xf_fill(u32 w0, u32 w1) {
    (void)w0;
    gfx_st.fill = w1;
}
static void xf_blend(u32 w0, u32 w1) {
    (void)w0;
    gfx_st.blend = w1;
}
static void xf_scissor(u32 w0, u32 w1) {
    gfx_st.scissor[0] = (int)((w0 >> 12) & 0xFFFu);
    gfx_st.scissor[1] = (int)(w0 & 0xFFFu);
    gfx_st.scissor[2] = (int)((w1 >> 12) & 0xFFFu);
    gfx_st.scissor[3] = (int)(w1 & 0xFFFu);
}
/* ---- render-to-RAM (task 18): off-screen G_SETCIMG targets ----
 * The game draws the stage background into a RAM buffer (bg_snap_buf, RGBA5551,
 * width = SCREEN_WIDTH+8) by pointing G_SETCIMG at it.  The Wii port always
 * draws into the EFB, so we capture the EFB region and convert it back into the
 * game's RGBA5551 buffer at the end of the task.  Only active while cimg points
 * outside the framebuffer (gFramebuffers). */
#ifdef TARGET_WII
extern u16 gFramebuffers[];
#define GFX_FB_BYTES (2u * 240u * 320u * 2u)

static uint32_t rtt_n64;     /* N64 cimg address of the active off-screen target */
static uint32_t rtt_w;       /* target width in texels */
static int rtt_active;

/* EFB is 640x480; we read the full screen region (0,0,640,480) -> 320x240 RGBA8. */
#define GFX_RTT_W 320
#define GFX_RTT_H 240
static uint8_t rtt_stage[GFX_RTT_W * GFX_RTT_H * 4u] __attribute__((aligned(32)));

/* ---- GPU-resident off-screen render (task 18b) ----
 * Instead of copying the EFB into RAM (which Dolphin's default "store EFB
 * copies to texture only" never does), GX_CopyTex writes the EFB region
 * straight into a MEM2 alias texture (RGB565, 2x the N64 snap buffer).  A
 * later texture load whose SETTIMG address falls inside the snap-buffer range
 * binds this alias texture instead of decoding TMEM.  alias_buf is allocated
 * once from the bottom of the MEM2 arena (like the GX FIFO in main.c). */
#define GFX_RTT_ALIAS_MAXW 656u          /* 2 * (SCREEN_WIDTH+8) = 2*328 */
#define GFX_RTT_ALIAS_H    (GFX_RTT_H * 2u)   /* 480 */
#define GFX_RTT_ALIAS_SIZE (GFX_RTT_ALIAS_MAXW * GFX_RTT_ALIAS_H * 2u) /* ~630 KB */
static uint8_t *alias_buf = NULL;        /* uncached (K1) MEM2 buffer */
static GXTexObj alias_obj;
static uint32_t alias_base = 0;          /* host addr of snap-buffer row 0 */
static uint32_t alias_w = 0;             /* snap-buffer width in texels */
static int alias_active = 0;

void gfx_tex_invalidate_range(uint32_t addr, uint32_t size);

static void gfx_rtt_flush(void)
{
    uint32_t host = (uint32_t)(uintptr_t)gfx_resolve(rtt_n64);
    if (host == 0) { rtt_active = 0; return; }

#ifdef PORT_RTT_TESTPATTERN
    /* Test pattern (off by default): skip the EFB read entirely and stamp a
     * CPU pattern straight into the game's snap buffer so the redraw path
     * (CopyTexBlock16 / COPY cycle) can be exercised independently of the
     * GPU->RAM copy.  RGBA5551 big-endian, opaque, colour = ((x/8)&1) ? red
     * : green, with a 2-pixel white border. */
    {
        uint32_t x, y, pitch = rtt_w;
        uint8_t *dst = (uint8_t *)(uintptr_t)host;
        for (y = 0; y < GFX_RTT_H; y++) {
            for (x = 0; x < rtt_w; x++) {
                uint32_t r, g, b;
                if (x < 2u || y < 2u || x + 2u >= rtt_w || y + 2u >= GFX_RTT_H) {
                    r = 31u; g = 31u; b = 31u;            /* white border */
                } else if ((x >> 3) & 1u) {
                    r = 31u; g = 0u;  b = 0u;             /* red */
                } else {
                    r = 0u;  g = 31u; b = 0u;             /* green */
                }
                uint16_t v = (uint16_t)((r << 11) | (g << 6) | (b << 1) | 1u);
                uint32_t o = (y * pitch + x) * 2u;
                dst[o] = (uint8_t)(v >> 8);
                dst[o + 1u] = (uint8_t)(v & 0xFFu);
            }
        }
        gfx_dc_flush((void *)(uintptr_t)host, rtt_w * GFX_RTT_H * 2u);
        gfx_tex_invalidate_range(host, rtt_w * GFX_RTT_H * 2u);
        PORT_LOG("rtt: testpattern %dx%d", (int)rtt_w, GFX_RTT_H);
        rtt_active = 0;
        return;
    }
#endif

    /* Wait for all queued draws so the EFB holds the finished background. */
    GX_DrawDone();

#ifdef PORT_RTT_RAM
    /* Legacy RAM read-back (only with -DPORT_RTT_RAM): Dolphin's default
     * "store EFB copies to texture only" never fills emulated RAM, so this
     * path is kept only for the EFBToTextureEnable=False harness. */
    GX_SetTexCopySrc(0, 0, GFX_RTT_W * 2, GFX_RTT_H * 2);
    GX_SetTexCopyDst(GFX_RTT_W, GFX_RTT_H, GFX_GX_TF_RGBA8, GFX_GX_TRUE);
    GX_CopyTex(rtt_stage, GFX_GX_FALSE);
    GX_PixModeSync();
    {
        uint32_t nz = 0, q;
        for (q = 0; q < GFX_RTT_W * GFX_RTT_H * 4u; q++) if (rtt_stage[q]) nz++;
        PORT_LOG("rtt: stage nz=%d", (int)nz);
    }
    GX_DrawDone();                      /* the copy itself must have landed in RAM */
    DCInvalidateRange(rtt_stage, sizeof rtt_stage);
    {
        uint32_t x, y;
        uint8_t *dst = (uint8_t *)(uintptr_t)host;
        uint32_t pitch = rtt_w;
        for (y = 0; y < GFX_RTT_H; y++) {
            uint32_t by = y >> 2;
            for (x = 0; x < GFX_RTT_W; x++) {
                uint32_t bx = x >> 2;
                uint32_t blk = (by * (GFX_RTT_W / 4u) + bx) * 64u;
                uint32_t i = ((y & 3u) << 2) | (x & 3u);
                uint8_t a = rtt_stage[blk + i * 2u];
                uint8_t r = rtt_stage[blk + i * 2u + 1u];
                uint8_t g = rtt_stage[blk + 32u + i * 2u];
                uint8_t b = rtt_stage[blk + 32u + i * 2u + 1u];
                uint16_t v = (uint16_t)(((uint32_t)(r >> 3) << 11) |
                                        ((uint32_t)(g >> 3) << 6) |
                                        ((uint32_t)(b >> 3) << 1) |
                                        ((a >= 128u) ? 1u : 0u));
                uint32_t o = (y * pitch + x) * 2u;
                dst[o] = (uint8_t)(v >> 8);
                dst[o + 1u] = (uint8_t)(v & 0xFFu);
            }
        }
    }
    gfx_dc_flush((void *)(uintptr_t)host, rtt_w * GFX_RTT_H * 2u);
    gfx_tex_invalidate_range(host, rtt_w * GFX_RTT_H * 2u);
    PORT_LOG("rtt: flushed %dx%d", (int)rtt_w, GFX_RTT_H);
    rtt_active = 0;
#else
    /* GPU-resident alias (task 18b): GX_CopyTex writes the EFB region straight
     * into a MEM2 RGB565 texture (2x the N64 snap buffer).  No CPU readback, so
     * this works with Dolphin's default "store EFB copies to texture only". */
    if (alias_buf == NULL) {
        uint8_t *lo = (uint8_t *)(((u32)SYS_GetArena2Lo() + 31u) & ~31u);
        SYS_SetArena2Lo(lo + GFX_RTT_ALIAS_SIZE);
        alias_buf = GFX_MEM_K0_TO_K1(lo);
        memset(alias_buf, 0, GFX_RTT_ALIAS_SIZE);
    }
    GX_SetTexCopySrc(0, 0, (unsigned short)(rtt_w * 2u), (unsigned short)(GFX_RTT_H * 2u));
    GX_SetTexCopyDst((unsigned short)(rtt_w * 2u), (unsigned short)(GFX_RTT_H * 2u), GFX_GX_TF_RGB565, GFX_GX_FALSE);
    GX_CopyTex(alias_buf, GFX_GX_FALSE);
    GX_PixModeSync();
    alias_base = host;
    alias_w = rtt_w;
    alias_active = 1;
    GX_InitTexObj(&alias_obj, alias_buf, (unsigned short)(rtt_w * 2u), (unsigned short)(GFX_RTT_H * 2u),
                  GFX_GX_TF_RGB565, GFX_GX_CLAMP, GFX_GX_CLAMP, GFX_GX_FALSE);
    PORT_LOG("rtt: gpu-alias %dx%d base=%08x", (int)rtt_w, GFX_RTT_H, (unsigned)host);
    rtt_active = 0;
#endif
}

void gfx_rtt_flush_if_active(void)
{
    if (rtt_active)
        gfx_rtt_flush();
}

/* Returns 1 if addr falls inside the active snap-buffer range, setting *top
 * (first N64 row) and *w (snap width).  top = (addr-base)/(2*w). */
int gfx_rtt_alias_lookup(uint32_t addr, uint32_t *top, uint32_t *w)
{
    if (!alias_active || alias_buf == NULL)
        return 0;
    uint32_t span = alias_w * GFX_RTT_H * 2u;
    if (addr < alias_base || addr >= alias_base + span)
        return 0;
    *top = (addr - alias_base) / (alias_w * 2u);
    *w = alias_w;
    return 1;
}

GXTexObj *gfx_rtt_alias_obj(void)
{
    return &alias_obj;
}
#endif

static void xf_cimg(u32 w0, u32 w1) {
    gfx_st.cimg = w1;
#ifdef TARGET_WII
    uint32_t width = (w0 & 0xFFFu) + 1u;
    const uint8_t *host = gfx_mem(w1);
    static int dbg_n = 0;
    if (dbg_n < 6000) { dbg_n++; PORT_LOG("cimg all n=%d w0=%08x w1=%08x w=%d host=%p", dbg_n, (unsigned)w0, (unsigned)w1, (int)width, (void *)host); }
    if (host != NULL) {
        uint32_t h = (uint32_t)(uintptr_t)host;
        uint32_t lo = (uint32_t)(uintptr_t)gFramebuffers;
        int in_fb = (h >= lo && h < lo + GFX_FB_BYTES);
        if (!in_fb && width >= 320u && width <= 512u) {
            rtt_n64 = w1;
            rtt_w = width;
            rtt_active = 1;
            PORT_LOG("rtt: offscreen cimg %p w=%d", (void *)(uintptr_t)host, (int)width);
        } else {
            if (rtt_active)
                gfx_rtt_flush();
            rtt_active = 0;
        }
    }
#endif
}
static void xf_zimg(u32 w0, u32 w1) {
    (void)w0;
    gfx_st.zimg = w1;
}
static void xf_nop(u32 w0, u32 w1) {
    (void)w0;
    (void)w1;
}

void gfx_xform_install(void (*table[256])(u32 w0, u32 w1)) {
    table[G_MTX & 0xFF] = xf_mtx;
    table[G_POPMTX & 0xFF] = xf_popmtx;
    table[G_MOVEMEM & 0xFF] = xf_movemem;
    table[G_VTX & 0xFF] = xf_vtx;
    table[0xD9] = xf_geo;           /* G_GEOMETRYMODE (F3DEX2) */
    table[0xEF] = xf_othm_both;     /* G_RDPSETOTHERMODE */
    table[G_SETOTHERMODE_H & 0xFF] = xf_othm_h;
    table[G_SETOTHERMODE_L & 0xFF] = xf_othm_l;
    table[G_TEXTURE & 0xFF] = xf_texture;
    table[G_SETPRIMCOLOR & 0xFF] = xf_prim;
    table[G_SETENVCOLOR & 0xFF] = xf_env;
    table[G_SETFILLCOLOR & 0xFF] = xf_fill;
    table[G_SETBLENDCOLOR & 0xFF] = xf_blend;
    table[G_SETSCISSOR & 0xFF] = xf_scissor;
    table[G_SETCIMG & 0xFF] = xf_cimg;
    table[G_SETZIMG & 0xFF] = xf_zimg;
    table[G_RDPFULLSYNC & 0xFF] = xf_nop;
    table[G_RDPTILESYNC & 0xFF] = xf_nop;
    table[G_RDPPIPESYNC & 0xFF] = xf_nop;
    table[G_RDPLOADSYNC & 0xFF] = xf_nop;
}
