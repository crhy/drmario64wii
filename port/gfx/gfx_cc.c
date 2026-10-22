/* Dr. Mario 64 -> Wii native port: N64 colour combiner -> GX TEV.
 *
 * G_SETCOMBINE (F3DEX_GBI_2, PR/gbi.h GCCc0w0/GCCc1w0/GCCc0w1/GCCc1w1):
 *   w0 & 0xFFFFFF = a0<<20 (4) | c0<<15 (5) | Aa0<<12 (3) | Ac0<<9 (3) | a1<<5 (4) | c1 (5)
 *   w1            = b0<<28 (4) | b1<<24 (4) | Aa1<<21 (3) | Ac1<<18 (3) | d0<<15 (3) |
 *                   Ab0<<12 (3) | Ad0<<9 (3) | d1<<6 (3) | Ab1<<3 (3) | Ad1 (3)
 * Each N64 cycle computes (A-B)*C+D for colour and alpha. The four slots have different selector tables.
 *
 * GX TEV computes d OP ((1-c)*a + c*b), so one cycle uses three stages:
 *   s0: a=0 b=A c=C d=0     ADD -> C*A          (no clamp)
 *   s1: a=0 b=B c=C d=PREV  SUB -> C*A - C*B    (no clamp)
 *   s2: a=0 b=D c=1 d=PREV  ADD -> (A-B)*C + D  (clamp)
 * Registers: PREV holds partial results and the final output; REG1 (C1/A1) holds the cycle-1 result that
 * cycle 2 reads as COMBINED; REG2 (C2/A2) holds the environment colour; PRIMITIVE comes from konst K0.
 * Untranslatable inputs (noise, LOD fractions, K4/K5, convert) are treated as 0, CENTER/SCALE as 1. */

#include <stdint.h>
#include <ogc/gx.h>
#include "gfx.h"
#include "port_log.h"

#define G_SETCOMBINE 0xfcu

enum { SLOT_A, SLOT_B, SLOT_C, SLOT_D };

static uint32_t cc_w0, cc_w1;
static int cc_have;
static int sel_c[2][4], sel_a[2][4]; /* [cycle][slot] */
static int cc_two;

/* Colour input for one slot. cycle2 selects whether COMBINED is the cycle-1 result (C1) or undefined (0). */
static uint8_t col_in(int slot, int s, int cycle2, uint8_t *kcsel)
{
    switch (s) {
    case 0: return cycle2 ? GX_CC_C1 : GX_CC_ZERO;          /* COMBINED */
    case 1: case 2: return GX_CC_TEXC;                      /* TEXEL0, TEXEL1 (= TEXEL0) */
    case 3: *kcsel = GX_TEV_KCSEL_K0; return GX_CC_KONST;   /* PRIMITIVE */
    case 4: return GX_CC_RASC;                              /* SHADE */
    case 5: return GX_CC_C2;                                /* ENVIRONMENT */
    }
    if (slot == SLOT_A) return s == 6 ? GX_CC_ONE : GX_CC_ZERO;           /* 6=1, 7=noise, 8-15=0 */
    if (slot == SLOT_B) return s == 6 ? GX_CC_HALF : GX_CC_ZERO;          /* 6=CENTER, 7=K4, 8-15=0 */
    if (slot == SLOT_D) return s == 6 ? GX_CC_ONE : GX_CC_ZERO;           /* 6=1, 7=0 */
    switch (s) {                                                          /* SLOT_C */
    case 6: return GX_CC_ONE;                                             /* SCALE */
    case 7: return cycle2 ? GX_CC_A1 : GX_CC_ZERO;                        /* COMBINED_ALPHA */
    case 8: case 9: return GX_CC_TEXA;                                    /* TEXEL0/1_ALPHA */
    case 10: *kcsel = GX_TEV_KCSEL_K0_A; return GX_CC_KONST;              /* PRIMITIVE_ALPHA */
    case 11: return GX_CC_RASA;                                           /* SHADE_ALPHA */
    case 12: return GX_CC_A2;                                             /* ENV_ALPHA */
    default: return GX_CC_ZERO;                                           /* LOD fractions, K5, 0 */
    }
}

/* Alpha input for one slot (A, B, D share a table; C differs at 0 and 6). */
static uint8_t alp_in(int slot, int s, int cycle2, uint8_t *kasel)
{
    switch (s) {
    case 1: case 2: return GX_CA_TEXA;
    case 3: *kasel = GX_TEV_KASEL_K0_A; return GX_CA_KONST;
    case 4: return GX_CA_RASA;
    case 5: return GX_CA_A2;
    case 7: return GX_CA_ZERO;
    }
    if (slot == SLOT_C) return GX_CA_ZERO;                                /* 0=LOD_FRAC, 6=PRIM_LOD_FRAC */
    if (s == 0) return cycle2 ? GX_CA_A1 : GX_CA_ZERO;                    /* COMBINED */
    *kasel = GX_TEV_KASEL_1; return GX_CA_KONST;                          /* 6 = 1 */
}

static void gfx_cc_setcombine(u32 w0, u32 w1)
{
    uint32_t m0 = w0 & 0xFFFFFFu, m1 = w1;
    sel_c[0][SLOT_A] = (m0 >> 20) & 0xF; sel_c[0][SLOT_C] = (m0 >> 15) & 0x1F;
    sel_a[0][SLOT_A] = (m0 >> 12) & 7;   sel_a[0][SLOT_C] = (m0 >> 9) & 7;
    sel_c[1][SLOT_A] = (m0 >> 5) & 0xF;  sel_c[1][SLOT_C] = m0 & 0x1F;
    sel_c[0][SLOT_B] = (m1 >> 28) & 0xF; sel_c[1][SLOT_B] = (m1 >> 24) & 0xF;
    sel_a[1][SLOT_A] = (m1 >> 21) & 7;   sel_a[1][SLOT_C] = (m1 >> 18) & 7;
    sel_c[0][SLOT_D] = (m1 >> 15) & 7;   sel_a[0][SLOT_B] = (m1 >> 12) & 7;
    sel_a[0][SLOT_D] = (m1 >> 9) & 7;    sel_c[1][SLOT_D] = (m1 >> 6) & 7;
    sel_a[1][SLOT_B] = (m1 >> 3) & 7;    sel_a[1][SLOT_D] = m1 & 7;
    if (w0 != cc_w0 || w1 != cc_w1 || !cc_have) {
        static uint32_t seen[64][2]; static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++) if (seen[i][0] == w0 && seen[i][1] == w1) known = 1;
        if (!known && nseen < 64) { seen[nseen][0] = w0; seen[nseen][1] = w1; nseen++; PORT_LOG("CC %08x %08x", w0, w1); }
    }
    cc_w0 = w0; cc_w1 = w1; cc_have = 1;
}

/* Program one cycle as stages base..base+2. last: write the final result to PREV, else to REG1. */
static void cc_cycle(int base, int cyc, int last, uint8_t texmap)
{
    int cycle2 = cyc == 1;
    uint8_t kc[3] = { GX_TEV_KCSEL_1, GX_TEV_KCSEL_1, GX_TEV_KCSEL_1 };
    uint8_t ka[3] = { GX_TEV_KASEL_1, GX_TEV_KASEL_1, GX_TEV_KASEL_1 };
    uint8_t cA = col_in(SLOT_A, sel_c[cyc][SLOT_A], cycle2, &kc[0]);
    uint8_t cC0 = col_in(SLOT_C, sel_c[cyc][SLOT_C], cycle2, &kc[0]);
    uint8_t cB = col_in(SLOT_B, sel_c[cyc][SLOT_B], cycle2, &kc[1]);
    uint8_t cC1 = col_in(SLOT_C, sel_c[cyc][SLOT_C], cycle2, &kc[1]);
    uint8_t cD = col_in(SLOT_D, sel_c[cyc][SLOT_D], cycle2, &kc[2]);
    uint8_t aA = alp_in(SLOT_A, sel_a[cyc][SLOT_A], cycle2, &ka[0]);
    uint8_t aC0 = alp_in(SLOT_C, sel_a[cyc][SLOT_C], cycle2, &ka[0]);
    uint8_t aB = alp_in(SLOT_B, sel_a[cyc][SLOT_B], cycle2, &ka[1]);
    uint8_t aC1 = alp_in(SLOT_C, sel_a[cyc][SLOT_C], cycle2, &ka[1]);
    uint8_t aD = alp_in(SLOT_D, sel_a[cyc][SLOT_D], cycle2, &ka[2]);
    uint8_t out = last ? GX_TEVPREV : GX_TEVREG1;
    int i;

    GX_SetTevColorIn(base, GX_CC_ZERO, cA, cC0, GX_CC_ZERO);
    GX_SetTevColorOp(base, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GX_SetTevAlphaIn(base, GX_CA_ZERO, aA, aC0, GX_CA_ZERO);
    GX_SetTevAlphaOp(base, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);

    GX_SetTevColorIn(base + 1, GX_CC_ZERO, cB, cC1, GX_CC_CPREV);
    GX_SetTevColorOp(base + 1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GX_SetTevAlphaIn(base + 1, GX_CA_ZERO, aB, aC1, GX_CA_APREV);
    GX_SetTevAlphaOp(base + 1, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);

    /* a=D, b=0, c=0 -> lerp = D; d + D */
    GX_SetTevColorIn(base + 2, cD, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
    GX_SetTevColorOp(base + 2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, out);
    GX_SetTevAlphaIn(base + 2, aD, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GX_SetTevAlphaOp(base + 2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, out);

    for (i = 0; i < 3; i++) {
        GX_SetTevKColorSel(base + i, kc[i]);
        GX_SetTevKAlphaSel(base + i, ka[i]);
        GX_SetTevOrder(base + i, texmap == GX_TEXMAP_NULL ? GX_TEXCOORDNULL : GX_TEXCOORD0, texmap, GX_COLOR0A0);
    }
}

/* Program the TEV for the current combine. Colours are refreshed on every call (fades change them often). */
static int cc_dirty;
static uint32_t last_prim, last_env; static int last_valid;
void gfx_cc_apply_tex(int textured)
{
    static uint32_t ap_w0, ap_w1; static int ap_valid, ap_tex, ap_two;
    uint32_t e = gfx_st.env, p = gfx_st.prim;
    GXColor env = { (u8)(e >> 24), (u8)(e >> 16), (u8)(e >> 8), (u8)e };
    GXColor prim = { (u8)(p >> 24), (u8)(p >> 16), (u8)(p >> 8), (u8)p };
    int tex = textured ? 1 : 0;

    if (!last_valid || p != last_prim) { GX_SetTevKColor(GX_KCOLOR0, prim); last_prim = p; }
    if (!last_valid || e != last_env) { GX_SetTevColor(GX_TEVREG2, env); last_env = e; }
    last_valid = 1;
    if (!cc_have) return;
    cc_two = ((gfx_st.otm_h >> 20) & 3) == 1; /* G_MDSFT_CYCLETYPE: 0=1CYC, 1=2CYC */
    if (!cc_dirty && ap_valid && ap_w0 == cc_w0 && ap_w1 == cc_w1 && ap_tex == tex && ap_two == cc_two) return;
    ap_valid = 1; ap_w0 = cc_w0; ap_w1 = cc_w1; ap_tex = tex; ap_two = cc_two; cc_dirty = 0;

    uint8_t texmap = tex ? GX_TEXMAP0 : GX_TEXMAP_NULL;
    GX_SetNumTevStages(cc_two ? 6 : 3);
    cc_cycle(0, 0, !cc_two, texmap);
    if (cc_two) cc_cycle(3, 1, 1, texmap);
}

/* Code that programs TEV itself (e.g. COPY-mode texrects) must call this so the next apply reprograms. */
void gfx_cc_invalidate(void) { cc_dirty = 1; last_valid = 0; gfx_state_reset(); }

void gfx_cc_install(void (*table[256])(u32 w0, u32 w1))
{
    table[G_SETCOMBINE & 0xFF] = gfx_cc_setcombine;
}

void gfx_cc_apply(void) { gfx_cc_apply_tex(gfx_st.tex_on ? 1 : 0); }

uint32_t gfx_cc_get_w0(void) { return cc_w0; }
uint32_t gfx_cc_get_w1(void) { return cc_w1; }
