/* Dr. Mario 64 -> Wii native port: textured drawing (task 09c).

 * Registers the textured half of the display-list walker: G_TEXRECT/G_TEXRECTFLIP
 * draw a textured quad; G_RDPHALF_1/2 supply the s,t and dsdx,dtdy words that
 * follow a G_TEXRECT.  Textured triangles (G_TEXTURE on) use GX_VTXFMT1 (POS +
 * CLR0 + TEX0) with GX_MODULATE; untextured draws in gfx_gx.c use GX_VTXFMT0.

 * Texture cache: keyed by (address, fmt, siz, w, h, CRC32 of texels + palette).
 * LRU-evict above 8 MB.  Byte-identical GX data for identical tiles (Dolphin
 * hi-res packs rely on it). */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogc/gx.h>
#include <gccore.h>
#include "gfx.h"
#include "gfx_tmem.h"
#include "gfx_texconv.h"
#include "port_log.h"
typedef struct Gfx Gfx;
#include "port_img_hint.h"
#include "gfx_wrap.h"

/* Colour combiner -> TEV (task 10b): programs the TEV unit for the current
 * G_SETCOMBINE before each textured draw, replacing the fixed MODULATE. */
void gfx_cc_apply(void);
void gfx_cc_apply_tex(int textured);
void gfx_cc_invalidate(void);

extern int main_no;

#ifdef PORT_TEXDUMP
static int gfx_menu_overlap(float x0, float y0, float x1, float y1)
{
    if (main_no != 6) return 0;
    return 1;
}
#endif

/* GPU-resident off-screen render (task 18b): when a texture load's SETTIMG
 * address falls inside the snap-buffer alias range, bind the alias texture
 * instead of decoding TMEM. */
#ifdef TARGET_WII
extern int gfx_rtt_alias_lookup(uint32_t addr, uint32_t *top, uint32_t *w);
extern GXTexObj *gfx_rtt_alias_obj(void);
static int bind_alias = 0;
static uint32_t bind_alias_top = 0, bind_alias_w = 0;
#endif

/* ---- texture dump tool (task 10a, -DPORT_TEXDUMP only) ---- */
#ifdef PORT_TEXDUMP
#include <stdio.h>
#include <sys/stat.h>
static int tex_dump_n = 0;
static int tex_dump_dir = 0;
static int texrect_log_n = 0;
static int tri_log_n = 0;

static void tex_dump_ppm(int w, int h, const uint8_t *rgba8,
                         const GfxTile *t, uint32_t addr)
{
    if (tex_dump_n >= 64)
        return;
    if (!tex_dump_dir) {
        mkdir("sd:/drmario/tex", 0777);
        tex_dump_dir = 1;
    }
    char path[160];
    snprintf(path, sizeof(path), "sd:/drmario/tex/%03d_%d_%d_%dx%d.ppm",
             tex_dump_n, (int)t->fmt, (int)t->siz, w, h);
    FILE *f = fopen(path, "wb");
    if (f) {
        int x, y;
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (y = 0; y < h; y++)
            for (x = 0; x < w; x++) {
                const uint8_t *p = rgba8 + ((size_t)y * (size_t)w + (size_t)x) * 4u;
                fwrite(p, 1, 3, f);
            }
        fclose(f);
    }
    FILE *ix = fopen("sd:/drmario/tex/index.txt", "a");
    if (ix) {
        fprintf(ix, "%03d addr=%08x fmt %d siz %d w %d h %d tile %d line %d "
                    "uls %d ult %d lrs %d lrt %d palette %d\n",
                tex_dump_n, addr, (int)t->fmt, (int)t->siz, w, h,
                (int)t->tmem, (int)t->line, (int)t->uls, (int)t->ult,
                (int)t->lrs, (int)t->lrt, (int)t->palette);
        fclose(ix);
    }
    tex_dump_n++;
}
#endif

/* ---- opcode constants (F3DEX_GBI_2) ---- */
#define G_TEXRECT       0xe4u
#define G_TEXRECTFLIP   0xe5u
#define G_RDPHALF_1     0xe1u
#define G_RDPHALF_2     0xf1u
#define G_SETCOMBINE    0xfcu

/* ---- GX constants ---- */
#define GX_TEXCOORD0    0x0u
#define GX_TEXMAP0      0x0u
#define GX_TF_RGBA8     0x6u
#define GX_MODULATE     0u
#define GX_REPLACE      3u
#define GX_CLAMP        0u
#define GX_REPEAT       1u
#define GX_MIRROR       2u

/* ---- texture cache: 4-way set associative, 64 sets x 4 ways ---- */
#define GFX_TEX_CACHE_MAX  (8u * 1024u * 1024u)
#define GFX_TEX_SETS       64u
#define GFX_TEX_WAYS       4u

typedef struct {
    uint32_t addr, fmt, siz, w, h, crc, pal;
    uint32_t gx_w, gx_h;
    uint8_t *gx_data;
    uint32_t gx_size;
    uint32_t lru;
} TexCacheEntry;

static TexCacheEntry tex_cache[GFX_TEX_SETS][GFX_TEX_WAYS];
static uint32_t tex_cache_total = 0;
static uint32_t tex_lru_clock = 0;

/* pending-free list: entries evicted while the GX FIFO may still reference them */
#define GFX_TEX_PENDING_MAX 512u
static uint8_t *pending_free[GFX_TEX_PENDING_MAX];
static uint32_t pending_n = 0;

/* counters */
static uint32_t cnt_binds = 0, cnt_skips = 0, cnt_hits = 0, cnt_misses = 0;
static uint32_t cnt_hint = 0, cnt_hint_hit = 0;
static uint32_t frame_count = 0;

/* last-bind shortcut state */
static uint32_t lb_gen = 0, lb_otm_h = 0xFFFFFFFFu, lb_set = 0, lb_way = 0;
static GfxTile  lb_tile_desc;
static TexCacheEntry *lb_entry = NULL;
static int lb_valid = 0;

/* whole-image hint state (task 23b) */
static const PortImgHint *bind_hint = NULL;
static uint32_t bind_hint_row0 = 0;
static const PortImgHint *lb_hint = NULL;
static uint32_t lb_row0 = 0;
/* cached hash per hint pointer (avoid re-hashing full image per tile) */
static uint32_t hint_hh_base = 0;
static uint32_t hint_hh_crc = 0, hint_hh_pal = 0;

/* static RGBA8 scratch buffer: worst case 8192 texels * 4 bytes = 32768 */
static uint8_t rgba8_buf[8192u * 4u] __attribute__((aligned(32)));
static uint8_t tiled_buf[8192u * 4u] __attribute__((aligned(32)));

/* GX textures must be power-of-two: the redraw strips are 328 wide (CopyTexBlock16
 * of the 328x240 snap buffer), so gx_w is padded up and the texrect texcoords are
 * divided by the padded size.  These hold the padded size of the last bound texture. */
static uint32_t bind_gx_w = 0, bind_gx_h = 0;

/* Per-axis wrap state for the last bound texture (task 24).  When an axis
 * repeats/mirrors, the GX texture is exactly 2^mask texels on that axis and
 * texcoords are divided by the period (not the padded tile size), with no
 * uls/ult offset.  shift_* holds the G_TEXSHIFTS/SHIFTT value to apply to
 * raw 10.5 texel coords before masking.  0 means "clamp axis, use tile path". */
static uint32_t bind_wrap_s = 0, bind_wrap_t = 0;
static uint32_t bind_period_s = 0, bind_period_t = 0;
static uint32_t bind_shift_s = 0, bind_shift_t = 0;

static uint32_t gfx_next_pot(uint32_t v)
{
    uint32_t p = 4u;
    while (p < v) p <<= 1;
    return p;
}

/* Apply the N64 G_TEXSHIFTS/SHIFTT scale to a 10.5 texel coordinate (task 24).
 * shift 1..10: s >> shift;  shift 11..15: s << (16 - shift); else identity. */
static int32_t gfx_tex_shift10(int32_t s10, uint32_t shift)
{
    if (shift >= 1u && shift <= 10u) return s10 >> shift;
    if (shift >= 11u && shift <= 15u) return s10 << (16 - (int)shift);
    return s10;
}

/* ---- FNV-1a word hash (replaces slow bitwise CRC32) ---- */
static uint32_t gfx_hash32(const uint8_t *data, uint32_t len)
{
    uint32_t h = 2166136261u;
    uint32_t i;
    for (i = 0; i + 4u <= len; i += 4u) {
        uint32_t w = ((uint32_t)data[i] << 24) | ((uint32_t)data[i + 1u] << 16) |
                     ((uint32_t)data[i + 2u] << 8) | data[i + 3u];
        h = (h ^ w) * 16777619u;
    }
    for (; i < len; i++)
        h = (h ^ data[i]) * 16777619u;
    return h;
}

/* ---- set index ---- */
static uint32_t gfx_tex_set(uint32_t addr, uint32_t fmt, uint32_t siz,
                            uint32_t w, uint32_t h, uint32_t crc, uint32_t pal)
{
    uint32_t h2 = addr * 2654435761u ^ fmt * 40503u ^ siz * 143u
                  ^ w * 1000003u ^ h * 10007u ^ crc * 31u ^ pal * 7u;
    return (h2 >> 16) & (GFX_TEX_SETS - 1u);
}

/* ---- pending-free helpers ---- */
static void gfx_tex_pending_add(uint8_t *p)
{
    if (pending_n < GFX_TEX_PENDING_MAX)
        pending_free[pending_n++] = p;
    else
        free(p);
}

/* ---- global evict when over 8 MB: scan all sets for global LRU ---- */
static void gfx_tex_evict_global(void)
{
    while (tex_cache_total > GFX_TEX_CACHE_MAX) {
        uint32_t oldest = 0xFFFFFFFFu;
        uint32_t os = 0, oo = 0;
        uint32_t s, k;
        for (s = 0; s < GFX_TEX_SETS; s++)
            for (k = 0; k < GFX_TEX_WAYS; k++)
                if (tex_cache[s][k].gx_data && tex_cache[s][k].lru < oldest) {
                    oldest = tex_cache[s][k].lru;
                    os = s; oo = k;
                }
        if (oldest == 0xFFFFFFFFu) break;
        tex_cache_total -= tex_cache[os][oo].gx_size;
        gfx_tex_pending_add(tex_cache[os][oo].gx_data);
        tex_cache[os][oo].gx_data = NULL;
    }
}

/* ---- invalidate cache entries whose source addr overlaps [addr, addr+size) ---- */
void gfx_tex_invalidate_range(uint32_t addr, uint32_t size)
{
    uint32_t lo = addr, hi = addr + size;
    uint32_t s, k;
    for (s = 0; s < GFX_TEX_SETS; s++)
        for (k = 0; k < GFX_TEX_WAYS; k++) {
            TexCacheEntry *c = &tex_cache[s][k];
            if (!c->gx_data) continue;
            if (c->addr >= lo && c->addr < hi) {
                tex_cache_total -= c->gx_size;
                gfx_tex_pending_add(c->gx_data);
                c->gx_data = NULL;
                if (lb_entry == c) lb_valid = 0;
            }
        }
    if (lb_entry && lb_entry->addr >= lo && lb_entry->addr < hi)
        lb_valid = 0;
}

/* ---- RGBA8 row-major -> GX_TF_RGBA8 4x4-tiled (dst padded to dw x dh POT,
 *      padding columns/rows filled with edge texels) ---- */
static void gfx_rgba8_to_gx(const uint8_t *src, uint32_t w, uint32_t h,
                             uint32_t dw, uint32_t dh, uint8_t *dst)
{
    uint32_t bx, by, lx, ly, i;
    uint32_t bw = dw / 4u, bh = dh / 4u;
    for (by = 0; by < bh; by++) {
        for (bx = 0; bx < bw; bx++) {
            uint32_t blk = (by * bw + bx) * 64u;
            for (ly = 0; ly < 4u; ly++) {
                for (lx = 0; lx < 4u; lx++) {
                    i = ly * 4u + lx;
                    uint32_t sx = bx * 4u + lx, sy = by * 4u + ly;
                    if (sx >= w) sx = w - 1u;
                    if (sy >= h) sy = h - 1u;
                    uint32_t si = (sy * w + sx) * 4u;
                    dst[blk + i * 2u]      = src[si + 3u]; /* A */
                    dst[blk + i * 2u + 1u] = src[si + 0u]; /* R */
                    dst[blk + 32u + i * 2u]      = src[si + 1u]; /* G */
                    dst[blk + 32u + i * 2u + 1u] = src[si + 2u]; /* B */
                }
            }
        }
    }
}

/* ---- Build a dw x dh RGBA8 image from a w x h source, repeating (tiling) on
 *      wrap axes and edge-padding on clamp axes, so the GX texture can use
 *      REPEAT/MIRROR with period == dw (or dh). ---- */
static void gfx_tile_rgba8(const uint8_t *src, uint32_t w, uint32_t h,
                           uint32_t dw, uint32_t dh, int wrap_s, int wrap_t,
                           uint8_t *dst)
{
    uint32_t x, y;
    for (y = 0; y < dh; y++) {
        uint32_t sy = wrap_t ? (y % h) : (y >= h ? h - 1u : y);
        for (x = 0; x < dw; x++) {
            uint32_t sx = wrap_s ? (x % w) : (x >= w ? w - 1u : x);
            uint32_t si = (sy * w + sx) * 4u, di = (y * dw + x) * 4u;
            dst[di + 0u] = src[si + 0u];
            dst[di + 1u] = src[si + 1u];
            dst[di + 2u] = src[si + 2u];
            dst[di + 3u] = src[si + 3u];
        }
    }
}

/* ---- linear-RAM whole-image decode (task 23b) ---- */
static uint8_t exp5(uint32_t v) { return (uint8_t)((v << 3) | (v >> 2)); }
static uint8_t exp4(uint32_t v) { return (uint8_t)(v * 0x11u); }
static void put(uint8_t *o, uint8_t r, uint8_t g, uint8_t b, uint8_t a) { o[0]=r; o[1]=g; o[2]=b; o[3]=a; }

static void gfx_decode_img(const uint8_t *img, const uint8_t *tlut,
                           int w, int h, int fmt, int siz, uint8_t *out)
{
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            uint8_t *o = out + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            uint32_t v;
            switch (siz) {
            case 0: { /* 4-bit: high nibble = left texel */
                uint8_t b = img[(y * w + x) / 2];
                v = (x & 1) ? (b & 0xFu) : (b >> 4);
                if (fmt == 2 && tlut) {
                    uint32_t a = (uint32_t)v * 2u;
                    uint32_t e = ((uint32_t)tlut[a] << 8) | tlut[a + 1u];
                    o[0] = exp5((e >> 11) & 0x1Fu);
                    o[1] = exp5((e >> 6) & 0x1Fu);
                    o[2] = exp5((e >> 1) & 0x1Fu);
                    o[3] = (e & 1u) ? 0xFFu : 0u;
                } else if (fmt == 3) {
                    uint8_t i = exp4(v); put(o, i, i, i, i);
                } else {
                    uint8_t i = exp4(v); put(o, i, i, i, i);
                }
                break;
            }
            case 1: { /* 8-bit */
                v = img[y * w + x];
                if (fmt == 2 && tlut) {
                    uint32_t a = (uint32_t)v * 2u;
                    uint32_t e = ((uint32_t)tlut[a] << 8) | tlut[a + 1u];
                    o[0] = exp5((e >> 11) & 0x1Fu);
                    o[1] = exp5((e >> 6) & 0x1Fu);
                    o[2] = exp5((e >> 1) & 0x1Fu);
                    o[3] = (e & 1u) ? 0xFFu : 0u;
                } else if (fmt == 3) {
                    put(o, exp4(v >> 4), exp4(v >> 4), exp4(v >> 4), exp4(v & 0xFu));
                } else {
                    put(o, (uint8_t)v, (uint8_t)v, (uint8_t)v, (uint8_t)v);
                }
                break;
            }
            case 2: { /* 16-bit big-endian */
                uint32_t a = (uint32_t)(y * w + x) * 2u;
                v = ((uint32_t)img[a] << 8) | img[a + 1u];
                if (fmt == 3) put(o, (uint8_t)(v >> 8), (uint8_t)(v >> 8), (uint8_t)(v >> 8), (uint8_t)v);
                else {
                    o[0] = exp5((v >> 11) & 0x1Fu);
                    o[1] = exp5((v >> 6) & 0x1Fu);
                    o[2] = exp5((v >> 1) & 0x1Fu);
                    o[3] = (v & 1u) ? 0xFFu : 0u;
                }
                break;
            }
            default: { /* 32-bit RGBA */
                uint32_t a = (uint32_t)(y * w + x) * 4u;
                put(o, img[a], img[a + 1u], img[a + 2u], img[a + 3u]);
                break;
            }
            }
        }
    }
}

/* ---- gfx_tex_bind: decode tile, cache, upload to GX_TEXMAP0 ---- */
static void gfx_tex_bind(int tile)
{
    cnt_binds++;
    const GfxTile *t = gfx_tile(tile);
    if (!t) return;

#ifdef TARGET_WII
    bind_alias = 0;
    uint32_t aaddr = gfx_timg_addr();
    uint32_t atop, aw;
    if (gfx_rtt_alias_lookup(aaddr, &atop, &aw)) {
        GX_LoadTexObj(gfx_rtt_alias_obj(), GX_TEXMAP0);
        bind_alias = 1;
        bind_alias_top = atop;
        bind_alias_w = aw;
        bind_gx_w = aw * 2u;
        bind_gx_h = 480u;
        bind_wrap_s = 0; bind_wrap_t = 0;
        bind_period_s = 0; bind_period_t = 0;
        bind_shift_s = 0; bind_shift_t = 0;
        lb_valid = 0;
        return;
    }

    /* whole-image hint path (task 23b): when SETTIMG falls inside a hinted image,
     * decode the entire image once and bind it; texcoords use the unified rule. */
    const PortImgHint *hint = gfx_st.img_hint;
    if (hint && hint->img) {
        uint32_t row_bytes = (hint->siz == 0u) ? (hint->w / 2u) :
                             (hint->siz == 1u) ? hint->w :
                             (hint->siz == 2u) ? (hint->w * 2u) : (hint->w * 4u);
        uint32_t img_bytes = (hint->siz == 0u) ? ((hint->w * hint->h + 1u) / 2u) :
                             (hint->w * hint->h * (1u << (hint->siz - 1u)));
        uint32_t hbase = (uint32_t)(uintptr_t)hint->img;
        if (aaddr >= hbase && aaddr < hbase + img_bytes &&
            (uint32_t)t->fmt == (uint32_t)hint->fmt && (uint32_t)t->siz == (uint32_t)hint->siz &&
            !(t->fmt == 2u && hint->tlut == NULL)) {
            uint32_t row0 = (aaddr - hbase) / row_bytes;
            uint32_t iw = hint->w, ih = hint->h;
            cnt_hint++;
            uint32_t gx_w = gfx_next_pot(iw);
            uint32_t gx_h = gfx_next_pot(ih);
            uint32_t gx_size = gx_w * gx_h * 4u;
            if (hbase != hint_hh_base) {
                hint_hh_crc = gfx_hash32((const uint8_t *)hint->img, img_bytes);
                hint_hh_pal = (uint32_t)(uintptr_t)hint->tlut;
                if (hint->tlut)   /* CI4: 16 entries, CI8: 256 entries, 2 bytes each */
                    hint_hh_crc ^= gfx_hash32((const uint8_t *)hint->tlut,
                                              (hint->siz == 0u) ? 32u : 512u);
                hint_hh_base = hbase;
            }
            uint32_t crc = hint_hh_crc;
            uint32_t pal = hint_hh_pal;
            uint32_t set = gfx_tex_set(hbase, (uint32_t)hint->fmt, (uint32_t)hint->siz,
                                       iw, ih, crc, pal);
            TexCacheEntry *e = NULL;
            uint32_t k;
            for (k = 0; k < GFX_TEX_WAYS; k++) {
                TexCacheEntry *c = &tex_cache[set][k];
                if (c->gx_data && c->addr == hbase && c->fmt == (uint32_t)hint->fmt &&
                    c->siz == (uint32_t)hint->siz && c->w == iw && c->h == ih &&
                    c->crc == crc && c->pal == pal) {
                    e = c; break;
                }
            }
            if (e) {
                cnt_hits++;
                cnt_hint_hit++;
                e->lru = ++tex_lru_clock;
            } else {
                cnt_misses++;
                uint32_t way = 0;
                for (k = 0; k < GFX_TEX_WAYS; k++)
                    if (!tex_cache[set][k].gx_data) { way = k; break; }
                if (k == GFX_TEX_WAYS) {
                    uint32_t oldest = 0xFFFFFFFFu;
                    for (k = 0; k < GFX_TEX_WAYS; k++)
                        if (tex_cache[set][k].lru < oldest) {
                            oldest = tex_cache[set][k].lru; way = k;
                        }
                    if (lb_valid && set == lb_set && way == lb_way) lb_valid = 0;
                    gfx_tex_pending_add(tex_cache[set][way].gx_data);
                    tex_cache_total -= tex_cache[set][way].gx_size;
                }
                e = &tex_cache[set][way];
                uint8_t *scratch = (uint8_t *)malloc((size_t)iw * (size_t)ih * 4u);
                if (!scratch) return;
                gfx_decode_img((const uint8_t *)hint->img,
                               (const uint8_t *)hint->tlut,
                               (int)iw, (int)ih, (int)hint->fmt, (int)hint->siz, scratch);
                uint8_t *gx_data = (uint8_t *)memalign(32, gx_size);
                if (!gx_data) { free(scratch); return; }
                gfx_rgba8_to_gx(scratch, iw, ih, gx_w, gx_h, gx_data);
                free(scratch);
                e->addr = hbase; e->fmt = (uint32_t)hint->fmt; e->siz = (uint32_t)hint->siz;
                e->w = iw; e->h = ih; e->crc = crc; e->pal = pal;
                e->gx_w = gx_w; e->gx_h = gx_h;
                e->gx_data = gx_data; e->gx_size = gx_size;
                e->lru = ++tex_lru_clock;
                tex_cache_total += gx_size;
                gfx_tex_evict_global();
                DCFlushRange(gx_data, gx_size);
            }
            uint32_t filt = (gfx_st.otm_h >> 12) & 3u;
            GXTexObj tex;
            GX_InitTexObj(&tex, e->gx_data, e->gx_w, e->gx_h, GX_TF_RGBA8,
                          GX_CLAMP, GX_CLAMP, GX_FALSE);
            GX_InitTexObjFilterMode(&tex,
                filt == 2u ? GX_LINEAR : GX_NEAR,
                filt == 2u ? GX_LINEAR : GX_NEAR);
            GX_InitTexObjWrapMode(&tex, GX_CLAMP, GX_CLAMP);
            GX_LoadTexObj(&tex, GX_TEXMAP0);
            bind_gx_w = e->gx_w;
            bind_gx_h = e->gx_h;
            bind_wrap_s = 0; bind_wrap_t = 0;
            bind_period_s = 0; bind_period_t = 0;
            bind_shift_s = 0; bind_shift_t = 0;
            bind_hint = hint;
            bind_hint_row0 = row0;
            lb_gen = gfx_tmem_gen();
            lb_otm_h = filt;
            lb_tile_desc = *t;
            lb_entry = e;
            lb_set = set;
            lb_way = (uint32_t)(e - &tex_cache[set][0]);
            lb_hint = hint;
            lb_row0 = row0;
            lb_valid = 1;
            return;
        }
    }
#endif

    bind_hint = NULL;
    uint32_t gen = gfx_tmem_gen();
    uint32_t filt = (gfx_st.otm_h >> 12) & 3u;

    /* last-bind shortcut: identical consecutive bind -> skip hashing + upload */
    if (lb_valid && gen == lb_gen && filt == lb_otm_h &&
        memcmp((const void *)t, (const void *)&lb_tile_desc, sizeof(GfxTile)) == 0 &&
        lb_entry && lb_entry->gx_data &&
        gfx_st.img_hint == lb_hint) {
        cnt_skips++;
        bind_gx_w = lb_entry->gx_w;
        bind_gx_h = lb_entry->gx_h;
        {
            int sw = ((int)t->lrs - (int)t->uls + 4) >> 2;
            int sh = ((int)t->lrt - (int)t->ult + 4) >> 2;
            if (sw <= 0) sw = 1;
            if (sh <= 0) sh = 1;
            int ms, mt, szs, szt;
            gfx_axis_mode((int)t->cms, (int)t->masks, sw, &ms, &szs);
            gfx_axis_mode((int)t->cmt, (int)t->maskt, sh, &mt, &szt);
            bind_wrap_s = (ms != 0); bind_wrap_t = (mt != 0);
            bind_period_s = (uint32_t)szs; bind_period_t = (uint32_t)szt;
            bind_shift_s = t->shifts; bind_shift_t = t->shiftt;
        }
        return;
    }

    int w = ((int)t->lrs - (int)t->uls + 4) >> 2;
    int h = ((int)t->lrt - (int)t->ult + 4) >> 2;
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    /* per-axis RDP wrap/mirror/clamp mapping (task 24) */
    int mode_s, mode_t, size_s, size_t;
    gfx_axis_mode((int)t->cms, (int)t->masks, w, &mode_s, &size_s);
    gfx_axis_mode((int)t->cmt, (int)t->maskt, h, &mode_t, &size_t);
    int wrap_s = (mode_s != 0);
    int wrap_t = (mode_t != 0);

    const uint8_t *tmem = gfx_tmem();
    uint32_t base = (uint32_t)t->tmem * 8u;
    if (base > 4096u) base = 4096u;
    uint32_t len = (uint32_t)t->line * 8u * (uint32_t)h;
    if (len > 4096u - base) len = 4096u - base;
    const uint8_t *tmem_src = tmem + base;

    uint32_t crc = gfx_hash32(tmem_src, len);
    /* wrap/mirror/clamp + mask change the GX texture, so fold them into the key */
    crc ^= ((uint32_t)t->cms | ((uint32_t)t->cmt << 8) |
            ((uint32_t)t->masks << 16) | ((uint32_t)t->maskt << 24)) * 0x01000193u;
    uint32_t pal = t->palette;
    uint32_t addr = gfx_timg_addr();
    uint32_t fmt = t->fmt, siz = t->siz;

    /* CI formats: fold the TLUT bytes actually used into the key (fix #2).
     * The TEXTLUT mode itself changes the decode (IA8/IA16 vs RGBA16), so
     * fold it into the key too or the cache would reuse a wrong conversion. */
    if (fmt == 2) {
        uint32_t tlut = (gfx_st.otm_h >> 14) & 3u;
        crc ^= (tlut + 1u) * 0x01000193u;
        if (siz == 0) { /* CI4: 16 entries at palette*16 -> 128 bytes */
            uint32_t pbase = 0x800u + ((uint32_t)pal << 4) * 8u;
            crc ^= gfx_hash32(tmem + pbase, 128u);
        } else { /* CI8 (and other CI sizes): all 256 entries -> 2048 bytes */
            crc ^= gfx_hash32(tmem + 0x800u, 2048u);
        }
    }

    uint32_t set = gfx_tex_set(addr, fmt, siz, (uint32_t)w, (uint32_t)h, crc, pal);
    TexCacheEntry *e = NULL;
    uint32_t k;
    for (k = 0; k < GFX_TEX_WAYS; k++) {
        TexCacheEntry *c = &tex_cache[set][k];
        if (c->gx_data && c->addr == addr && c->fmt == fmt && c->siz == siz &&
            c->w == (uint32_t)w && c->h == (uint32_t)h && c->crc == crc && c->pal == pal) {
            e = c;
            break;
        }
    }

    if (e) {
        cnt_hits++;
        e->lru = ++tex_lru_clock;
    } else {
        cnt_misses++;
        /* pick a way: first empty, else LRU within the set */
        uint32_t way = 0;
        for (k = 0; k < GFX_TEX_WAYS; k++)
            if (!tex_cache[set][k].gx_data) { way = k; break; }
        if (k == GFX_TEX_WAYS) {
            uint32_t oldest = 0xFFFFFFFFu;
            for (k = 0; k < GFX_TEX_WAYS; k++)
                if (tex_cache[set][k].lru < oldest) { oldest = tex_cache[set][k].lru; way = k; }
            /* evicting the last-bind entry invalidates the shortcut */
            if (lb_valid && set == lb_set && way == lb_way) lb_valid = 0;
            gfx_tex_pending_add(tex_cache[set][way].gx_data);
            tex_cache_total -= tex_cache[set][way].gx_size;
        }
        e = &tex_cache[set][way];

        uint32_t gx_w = (uint32_t)size_s;
        uint32_t gx_h = (uint32_t)size_t;
        uint32_t gx_size = gx_w * gx_h * 4u;

        if ((uint32_t)w * (uint32_t)h > 8192u || gx_w * gx_h > 8192u) {
#ifdef PORT_LOG_VERBOSE
            PORT_LOG("tex skip: tile %d w*h %d gx %d > 8192\n", tile, (int)(w * h), (int)(gx_w * gx_h));
#endif
            return;
        }
        gfx_decode_tile(tmem, t, w, h, rgba8_buf, gfx_st.otm_h);
#ifdef PORT_TEXDUMP
        tex_dump_ppm(w, h, rgba8_buf, t, addr);
#endif
        uint8_t *gx_data = (uint8_t *)memalign(32, gx_size);
        if (!gx_data) return;
        if (wrap_s || wrap_t) {
            gfx_tile_rgba8(rgba8_buf, (uint32_t)w, (uint32_t)h, gx_w, gx_h, wrap_s, wrap_t, tiled_buf);
            gfx_rgba8_to_gx(tiled_buf, gx_w, gx_h, gx_w, gx_h, gx_data);
        } else {
            gfx_rgba8_to_gx(rgba8_buf, (uint32_t)w, (uint32_t)h, gx_w, gx_h, gx_data);
        }

        e->addr = addr; e->fmt = fmt; e->siz = siz;
        e->w = (uint32_t)w; e->h = (uint32_t)h; e->crc = crc; e->pal = pal;
        e->gx_w = gx_w; e->gx_h = gx_h;
        e->gx_data = gx_data; e->gx_size = gx_size;
        e->lru = ++tex_lru_clock;
        tex_cache_total += gx_size;
        gfx_tex_evict_global();
        /* DCFlushRange once, right after conversion (fix #5) */
        DCFlushRange(gx_data, gx_size);
    }

    GXTexObj tex;
    GX_InitTexObj(&tex, e->gx_data, e->gx_w, e->gx_h, GX_TF_RGBA8,
                  GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjFilterMode(&tex,
        filt == 2u ? GX_LINEAR : GX_NEAR,
        filt == 2u ? GX_LINEAR : GX_NEAR);
    GX_InitTexObjWrapMode(&tex,
        mode_s == 0 ? GX_CLAMP : (mode_s == 1 ? GX_REPEAT : GX_MIRROR),
        mode_t == 0 ? GX_CLAMP : (mode_t == 1 ? GX_REPEAT : GX_MIRROR));
    GX_LoadTexObj(&tex, GX_TEXMAP0);
    bind_gx_w = e->gx_w;
    bind_gx_h = e->gx_h;
    bind_wrap_s = (uint32_t)wrap_s; bind_wrap_t = (uint32_t)wrap_t;
    bind_period_s = (uint32_t)size_s; bind_period_t = (uint32_t)size_t;
    bind_shift_s = t->shifts; bind_shift_t = t->shiftt;

    /* record last-bind state for the shortcut */
    lb_gen = gen;
    lb_otm_h = filt;
    lb_tile_desc = *t;
    lb_entry = e;
    lb_set = set;
    lb_way = (uint32_t)(e - &tex_cache[set][0]);
    lb_hint = NULL;
    lb_valid = 1;
}

/* ---- drain pending-free list + log counters (called after GX_DrawDone) ---- */
/* Called at the start of every gfx task: the whole-image hash memo (hint_hh_*) is only valid within one task. A new
 * scene often loads a different image at the SAME heap address; keeping the memo across tasks returned the previous
 * scene's texture (menu art drawn as the game background, layout dependent). */
void gfx_tex_task_begin(void)
{
    hint_hh_base = 0;
}

void gfx_tex_frame_end(void)
{
    uint32_t i;
    for (i = 0; i < pending_n; i++) {
        free(pending_free[i]);
        pending_free[i] = NULL;
    }
    pending_n = 0;

    frame_count++;
    if ((frame_count % 60u) == 0u)
        PORT_LOG("tex stats: binds %u skips %u hits %u misses %u hint %u hint_hit %u\n",
                 cnt_binds, cnt_skips, cnt_hits, cnt_misses, cnt_hint, cnt_hint_hit);
}

/* ---- textured triangle draw ---- */
void gfx_tex_draw_tri(int i0, int i1, int i2)
{
    const gfx_state *s = &gfx_st;
    int tile = (int)(s->geo >> 24) & 7;
    gfx_tex_bind(tile);

    const GfxTile *t = gfx_tile(tile);
    float tw = 1.0f, th = 1.0f;
    float uls = 0.0f, ult = 0.0f;
    if (t) {
        tw = (float)((((int)t->lrs - (int)t->uls) >> 2) + 1); /* tile size is 10.2 */
        th = (float)((((int)t->lrt - (int)t->ult) >> 2) + 1);
        uls = (float)t->uls / 4.0f;
        ult = (float)t->ult / 4.0f;
    }

#ifdef PORT_TEXDUMP
    if (tw == 128.0f && th == 14.0f && tri_log_n < 64) {
        PORT_LOG("tri %d: tile=%d fmt=%d siz=%d w=%d h=%d cc0=%08x cc1=%08x otm_l=%08x otm_h=%08x prim=%08x geo=%08x cyc=%d\n",
                 tri_log_n, tile, (int)t->fmt, (int)t->siz, (int)tw, (int)th,
                 gfx_cc_get_w0(), gfx_cc_get_w1(), s->otm_l, s->otm_h, s->prim, s->geo,
                 (int)((s->otm_h >> 20) & 3u));
        tri_log_n++;
    }
#endif

    gfx_state_count_draw();
    gfx_set_textured(1);
    gfx_cc_apply_tex(1);
    gfx_rm_apply();

    GfxClipVtx in[3], out[4];
    int idx[3] = { i0, i1, i2 };
    int k, all_in = 1, all_out = 1;
    for (k = 0; k < 3; k++) {
        int vi = idx[k];
        if (vi < 0) vi = 0;
        if (vi > 31) vi = 31;
        in[k].cx = s->vtx[vi].cx; in[k].cy = s->vtx[vi].cy;
        in[k].cz = s->vtx[vi].cz; in[k].cw = s->vtx[vi].cw;
        in[k].color = s->vtx[vi].color; in[k].s = s->vtx[vi].s; in[k].t = s->vtx[vi].t;
        if (in[k].cw > W_EPS) all_out = 0; else all_in = 0;
    }
    if (all_out) return;
    int nv;
    if (all_in) { nv = 3; out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; }
    else nv = gfx_clip_tri(in, out);
    if (nv < 3) return;

    float px[4], py[4], pz[4], tx[4], ty[4];
    uint32_t pc[4];
    int32_t minx = 0x7FFFFFFF, maxx = -0x7FFFFFFF, miny = 0x7FFFFFFF, maxy = -0x7FFFFFFF;
    for (k = 0; k < nv; k++) {
        int32_t sx, sy; float z;
        gfx_project(out[k].cx, out[k].cy, out[k].cz, out[k].cw, &sx, &sy, &z);
        px[k] = (float)sx; py[k] = (float)sy; pz[k] = z;
        pc[k] = (s->geo & 0x4u) ? out[k].color : s->prim;
        if (bind_wrap_s || bind_shift_s) {
            int32_t s10 = (int32_t)((float)out[k].s * (float)s->tex_s / 65536.0f);
            float stexel = (float)gfx_tex_shift10(s10, bind_shift_s) / 32.0f;
            tx[k] = bind_wrap_s ? (stexel / (float)bind_period_s)
                                : ((stexel - uls) / tw);
        } else {
            tx[k] = ((float)out[k].s * (float)s->tex_s / 32.0f / 65536.0f - uls) / tw;
        }
        if (bind_wrap_t || bind_shift_t) {
            int32_t t10 = (int32_t)((float)out[k].t * (float)s->tex_t / 65536.0f);
            float ttexel = (float)gfx_tex_shift10(t10, bind_shift_t) / 32.0f;
            ty[k] = bind_wrap_t ? (ttexel / (float)bind_period_t)
                                : ((ttexel - ult) / th);
        } else {
            ty[k] = ((float)out[k].t * (float)s->tex_t / 32.0f / 65536.0f - ult) / th;
        }
        if (bind_hint) {
            tx[k] = ((float)out[k].s * (float)s->tex_s / 32.0f / 65536.0f) / (float)bind_gx_w;
            ty[k] = ((float)out[k].t * (float)s->tex_t / 32.0f / 65536.0f + (float)bind_hint_row0) / (float)bind_gx_h;
        }
        if (sx < minx) minx = sx;
        if (sx > maxx) maxx = sx;
        if (sy < miny) miny = sy;
        if (sy > maxy) maxy = sy;
    }
    if (minx < -4096 || maxx > 4096 || miny < -4096 || maxy > 4096) return;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT1, 3 * (nv - 2));
    for (k = 1; k + 1 < nv; k++) {
        int a = 0, b = k, c = k + 1;
        GX_Position3f32(px[a], py[a], pz[a]);
        GX_Color4u8((uint8_t)((pc[a] >> 24) & 0xFFu), (uint8_t)((pc[a] >> 16) & 0xFFu),
                    (uint8_t)((pc[a] >> 8) & 0xFFu), (uint8_t)(pc[a] & 0xFFu));
        GX_TexCoord2f32(tx[a], ty[a]);
        GX_Position3f32(px[b], py[b], pz[b]);
        GX_Color4u8((uint8_t)((pc[b] >> 24) & 0xFFu), (uint8_t)((pc[b] >> 16) & 0xFFu),
                    (uint8_t)((pc[b] >> 8) & 0xFFu), (uint8_t)(pc[b] & 0xFFu));
        GX_TexCoord2f32(tx[b], ty[b]);
        GX_Position3f32(px[c], py[c], pz[c]);
        GX_Color4u8((uint8_t)((pc[c] >> 24) & 0xFFu), (uint8_t)((pc[c] >> 16) & 0xFFu),
                    (uint8_t)((pc[c] >> 8) & 0xFFu), (uint8_t)(pc[c] & 0xFFu));
        GX_TexCoord2f32(tx[c], ty[c]);
    }
    GX_End();
}

/* ---- G_TEXRECT / G_TEXRECTFLIP ---- */
static void gfx_tex_rect(u32 w0, u32 w1)
{
    gfx_st.tr_w0 = w0;
    gfx_st.tr_w1 = w1;
    gfx_st.tex_pending = 1;
}

static void gfx_tex_rdpalf1(u32 w0, u32 w1)
{
    (void)w0;
    gfx_st.tr_s = (int32_t)(w1 >> 16);
    gfx_st.tr_t = (int32_t)(w1 & 0xFFFFu);
}

static void gfx_tex_rdpalf2(u32 w0, u32 w1)
{
    (void)w0;
    gfx_st.tr_dsdx = (int32_t)(w1 >> 16);
    gfx_st.tr_dtdy = (int32_t)(w1 & 0xFFFFu);

    if (!gfx_st.tex_pending) return;
    gfx_st.tex_pending = 0;

    uint32_t w0r = gfx_st.tr_w0, w1r = gfx_st.tr_w1;
    uint32_t xh = (w0r >> 12) & 0xFFFu;
    uint32_t yh = w0r & 0xFFFu;
    uint32_t tile = (w1r >> 24) & 0xFFu;
    uint32_t xl = (w1r >> 12) & 0xFFFu;
    uint32_t yl = w1r & 0xFFFu;

    gfx_tex_bind((int)tile);

    const GfxTile *t = gfx_tile((int)tile);
    float tw = 1.0f, th = 1.0f;
    float uls = 0.0f, ult = 0.0f;
    if (t) {
        tw = (float)((((int)t->lrs - (int)t->uls) >> 2) + 1); /* tile size is 10.2 */
        th = (float)((((int)t->lrt - (int)t->ult) >> 2) + 1);
        uls = (float)t->uls / 4.0f;
        ult = (float)t->ult / 4.0f;
    }

    uint32_t cyc = (gfx_st.otm_h >> 20) & 3u;
    int copy = cyc == 2u;
    float dsdx = (float)gfx_st.tr_dsdx / 1024.0f;
    float dtdy = (float)gfx_st.tr_dtdy / 1024.0f;
    if (copy) { dsdx /= 4.0f; } /* COPY cycle: dsdx is 4 pixels/clock; dtdy is normal 1.0 */

    float s0, t0;
    if (bind_wrap_s || bind_shift_s) {
        int32_t s10 = gfx_tex_shift10((int32_t)gfx_st.tr_s, bind_shift_s);
        float stexel = (float)s10 / 32.0f;
        s0 = bind_wrap_s ? stexel : (stexel - uls);
    } else {
        s0 = (float)gfx_st.tr_s / 32.0f - uls;
    }
    if (bind_wrap_t || bind_shift_t) {
        int32_t t10 = gfx_tex_shift10((int32_t)gfx_st.tr_t, bind_shift_t);
        float ttexel = (float)t10 / 32.0f;
        t0 = bind_wrap_t ? ttexel : (ttexel - ult);
    } else {
        t0 = (float)gfx_st.tr_t / 32.0f - ult;
    }
    float x0 = (float)xl / 4.0f;
    float y0 = (float)yl / 4.0f;
    float x1 = (float)xh / 4.0f;
    float y1 = (float)yh / 4.0f;

    /* COPY and FILL cycles use an inclusive lower-right; 1-cycle and 2-cycle are exclusive. */
    if (cyc == 2u || cyc == 3u) { x1 += 1.0f; y1 += 1.0f; }

    float gw = bind_gx_w ? (float)bind_gx_w : tw;
    float gh = bind_gx_h ? (float)bind_gx_h : th;
    float stx0 = s0 / gw, sty0 = t0 / gh;
    float stx1 = (s0 + dsdx * (x1 - x0)) / gw;
    float sty1 = (t0 + dtdy * (y1 - y0)) / gh;

#ifdef TARGET_WII
    if (bind_alias) {
        /* alias texture is 2x the snap buffer: u = s_texel / w, v = (t_texel + top) / 240 */
        float aw = (float)bind_alias_w;
        float atop = (float)bind_alias_top;
        stx0 = s0 / aw;
        stx1 = (s0 + dsdx * (x1 - x0)) / aw;
        sty0 = (t0 + atop) / 240.0f;
        sty1 = (t0 + dtdy * (y1 - y0) + atop) / 240.0f;
    }
#endif

    if (bind_hint) {
        /* whole-image hint: u = s / gx_w, v = (t + row0) / gx_h (no uls/ult offset) */
        float raw_s = (float)gfx_st.tr_s / 32.0f;
        float raw_t = (float)gfx_st.tr_t / 32.0f + (float)bind_hint_row0;
        float gw = (float)bind_gx_w;
        float gh = (float)bind_gx_h;
        stx0 = raw_s / gw;
        sty0 = raw_t / gh;
        stx1 = (raw_s + dsdx * (x1 - x0)) / gw;
        sty1 = (raw_t + dtdy * (y1 - y0)) / gh;
    }

#ifdef PORT_TEXDUMP
    if (gfx_menu_overlap(x0, y0, x1, y1) && texrect_log_n < 256) {
        PORT_LOG("menu texrect %d: tile=%d fmt=%d siz=%d tlut=%d rect=(%.1f,%.1f,%.1f,%.1f) s=%d t=%d dsdx=%d dtdy=%d cc0=%08x cc1=%08x otm_l=%08x otm_h=%08x ac=%d rm=%d cvg=%d prim=%08x env=%08x fill=%08x blend=%08x cyc=%d\n",
                 texrect_log_n, (int)tile, t ? (int)t->fmt : -1, t ? (int)t->siz : -1,
                 (int)((gfx_st.otm_h >> 14) & 3u),
                 x0, y0, x1, y1,
                 gfx_st.tr_s, gfx_st.tr_t, gfx_st.tr_dsdx, gfx_st.tr_dtdy,
                 gfx_cc_get_w0(), gfx_cc_get_w1(), gfx_st.otm_l, gfx_st.otm_h,
                 (int)(gfx_st.otm_l & 3u), (int)((gfx_st.otm_l >> 3) & 0xFFu),
                 (int)((gfx_st.otm_h >> 12) & 7u),
                 gfx_st.prim, gfx_st.env, gfx_st.fill, gfx_st.blend,
                 (int)((gfx_st.otm_h >> 20) & 3u));
        texrect_log_n++;
    }
#endif

    gfx_state_count_draw();
    gfx_set_textured(1);
    if (copy) {
        /* COPY cycle type: texels go straight to the framebuffer.
         * GX_REPLACE reads the TevOrder colourID (the vertex colour), NOT the
         * texel, so it drew the primitive colour. Program the stage explicitly
         * so the output is the texel: d + ((1-c)*a + c*b) with a=TEXC, rest 0. */
        GX_SetNumTevStages(1);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
        GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
        gfx_cc_invalidate();
    } else
        gfx_cc_apply_tex(1);

    uint32_t col = gfx_st.prim;
    uint8_t cr = (uint8_t)((col >> 24) & 0xFFu);
    uint8_t cg = (uint8_t)((col >> 16) & 0xFFu);
    uint8_t cb = (uint8_t)((col >> 8) & 0xFFu);
    uint8_t ca = (uint8_t)(col & 0xFFu);

    gfx_rm_apply();
    GX_Begin(GX_QUADS, GX_VTXFMT2, 4);
    GX_Position2f32(x0, y0); GX_Color4u8(cr, cg, cb, ca); GX_TexCoord2f32(stx0, sty0);
    GX_Position2f32(x1, y0); GX_Color4u8(cr, cg, cb, ca); GX_TexCoord2f32(stx1, sty0);
    GX_Position2f32(x1, y1); GX_Color4u8(cr, cg, cb, ca); GX_TexCoord2f32(stx1, sty1);
    GX_Position2f32(x0, y1); GX_Color4u8(cr, cg, cb, ca); GX_TexCoord2f32(stx0, sty1);
    GX_End();
}

/* ---- install handlers ---- */
void gfx_tex_install(void (*table[256])(u32 w0, u32 w1))
{
    table[G_TEXRECT & 0xFF] = gfx_tex_rect;
    table[G_TEXRECTFLIP & 0xFF] = gfx_tex_rect;
    table[G_RDPHALF_1 & 0xFF] = gfx_tex_rdpalf1;
    table[G_RDPHALF_2 & 0xFF] = gfx_tex_rdpalf2;
}
