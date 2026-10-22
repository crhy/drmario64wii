#ifndef GFX_H
#define GFX_H

/* Display-list renderer (task 08a): gfx_run_task is called from osSpTaskStartGo
 * for M_GFXTASK before the SP/DP done events.  It walks the task's F3DEX2 or
 * S2DEX display list; drawing is added in later tasks. */

/* The host-test GATE command does not pass -D_LANGUAGE_C (the N64/Wii builds
 * do), but PR/sptask.h only declares OSTask under that macro.  Define it here
 * when absent so gfx_run_task's prototype compiles everywhere. */
#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#include <stdint.h>

/* gfx.c and gfx_xform.c need OSTask for gfx_run_task; gfx_gx.c does not and
 * must not pull in ultralib types (it uses libogc instead).  Define GFX_NEED_SPTASK
 * before including this header to get the ultralib-dependent declarations. */
#ifdef GFX_NEED_SPTASK
#include <PR/sptask.h>
void gfx_run_task(OSTask *t);
#endif

/* Display-list state (task 08b).  gfx_xform.c owns the one global instance
 * (gfx_st) and registers per-opcode handlers into the walker's table via
 * gfx_xform_install().  Handlers only mutate this state; no drawing yet. */

typedef struct {
    /* 4x4 s15.16 matrices, row-major, stored as int32 (hi<<16|lo). */
    int32_t mv[4][4];          /* model view */
    int32_t pr[4][4];          /* projection */
    int32_t mstack[8][4][4];   /* model-view push stack */
    int     msp;
    /* viewport: vscale/vtrans in 1/4 pixel units (s16). */
    int32_t vscale[4];
    int32_t vtrans[4];
    /* mode words. */
    uint32_t geo;              /* geometry mode */
    uint32_t otm_h, otm_l;     /* other mode H/L */
    /* texture: scale s/t (u16 each), level, on/off. */
    uint32_t tex_s, tex_t, tex_level, tex_on;
    /* colours packed RGBA8 (0xRRGGBBAA). */
    uint32_t prim, env, fill, blend;
    /* scissor box x0,y0,x1,y1 (N64 screen units). */
    int scissor[4];
    /* colour / z image addresses. */
    uint32_t cimg, zimg;
    /* texture image state (G_SETTIMG). */
    uint32_t timg;             /* source address */
    uint8_t  img_fmt, img_siz; /* format, texel size code */
    uint16_t img_w;            /* image width in texels */
    /* 8 tile descriptors (G_SETTILE / G_SETTILESIZE). */
    struct {
        uint8_t  fmt, siz;
        uint16_t stride;       /* line length in longs */
        uint16_t tmem;         /* TMEM start (long index) */
        uint16_t palette;      /* TLUT entry offset */
        uint8_t  cms, cmt, masks, maskt, shifts, shiftt;
        int16_t  uls, ult, lrs, lrt; /* tile size in texels (10.5) */
    } tile[8];
    /* pending G_TEXRECT: rect word + s,t,dsdx,dtdy from the two RDPHALF words. */
    int      tex_pending;
    uint32_t tr_w0, tr_w1;     /* rect word (xh,yh / xl,yl) */
    int32_t  tr_s, tr_t, tr_dsdx, tr_dtdy; /* s10.5 / s5.10 */
    /* transformed vertex buffer, up to 32. */
    struct {
        int32_t cx, cy, cz, cw; /* clip coords, s15.16 (cw = perspective w) */
        int32_t sx, sy;        /* screen x,y, N64 320x240 space */
        uint32_t color;        /* RGBA8 */
        int32_t s, t;          /* texture coords */
    } vtx[32];
    int nvtx;
    const void *img_hint;
} gfx_state;

extern gfx_state gfx_st;

/* Near-plane clipping (task 10e).  A clip-space vertex: cx,cy,cz,cw (s15.16),
 * packed RGBA8 colour, and s,t texture coords. */
typedef struct {
    int32_t cx, cy, cz, cw;
    uint32_t color;
    int32_t s, t;
} GfxClipVtx;

/* Shared constants for the clip/depth path (task 10e).  W_EPS = 1/256 in s15.16
 * is the near-plane w threshold; G_ZBUFFER is the geometry-mode depth bit; Z_UPD
 * is the other-mode-L depth-update bit (PR/gbi.h F3DEX_GBI_2). */
#define W_EPS     256
#ifndef G_ZBUFFER
#define G_ZBUFFER 0x00000001u
#endif
#ifndef Z_UPD
#define Z_UPD     0x20u
#endif

/* Project one clip vertex to N64 screen space + GX ortho depth [0,1].  Reads
 * gfx_st.vscale/vtrans.  z = cz/cw mapped from [-1,1] to [0,1]. */
void gfx_project(int32_t cx, int32_t cy, int32_t cz, int32_t cw,
                 int32_t *sx, int32_t *sy, float *z);

/* Clip a triangle against the plane w = W_EPS (keeping w >= W_EPS).  Returns
 * the number of output vertices (0..4) written to out[]. */
int gfx_clip_tri(const GfxClipVtx in[3], GfxClipVtx out[4]);

/* Fill a 256-entry per-opcode handler table with gfx_xform.c's handlers. */
void gfx_xform_install(void (*table[256])(u32 w0, u32 w1));

/* Apply other-mode render state (alpha compare, blend, Z) before a draw. */
void gfx_rm_apply(void);

/* GX state cache (task 12e): textured-mode + rm caches, draw counters. */
void gfx_set_textured(int on);
void gfx_state_reset(void);
void gfx_state_count_draw(void);
void gfx_state_frame_end(void);
int  gfx_state_rm_check(uint32_t l, uint32_t h, uint32_t blend);

/* Current combine words (for texdump logging). */
uint32_t gfx_cc_get_w0(void);
uint32_t gfx_cc_get_w1(void);

/* Render-to-RAM (task 18): flush an active off-screen G_SETCIMG target. */
void gfx_rtt_flush_if_active(void);

#endif
