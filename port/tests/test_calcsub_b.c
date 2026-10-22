#define _LANGUAGE_C
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#undef _SIZE_T
#undef _SIZE_T_DEF
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define s32 int32_t
#define s16 int16_t
#define u32 uint32_t
#define u64 uint64_t

void lc2wc(Mtx *arg0, s32 arg1, s32 arg2, s32 arg3, s32 *arg4, s32 *arg5, s32 *arg6);
void wc2lc(Mtx *arg0, s32 arg1, s32 arg2, s32 arg3, s32 *arg4, s32 *arg5, s32 *arg6);
void matrixMulL(const Mtx *m, const Mtx *n, Mtx *r);
void makeTransrateMatrix(Mtx *m, s32 xofs, s32 yofs, s32 zofs);
void makeScaleMatrix(Mtx *m, s32 scale);

static int failures = 0;
#define CHECK(cond, expr) do { if (!(cond)) { printf("FAIL %s\n", expr); failures++; } } while (0)

/* Zero a matrix then set the diagonal to 0x8000 (identity in this fixed-point space). */
static void set_identity(Mtx *m) {
    memset(m, 0, sizeof(Mtx));
    m->m[0][0] = 0x8000;
    m->m[1][1] = 0x8000;
    m->m[2][2] = 0x8000;
    m->m[3][3] = 0x8000;
}

int main(void) {
    /* ---- lc2wc ----
     * Identity matrix, args (0x10000, 0x20000, 0x30000).
     * For axis i: value = m[0][i]*arg1 + m[1][i]*arg2 + m[2][i]*arg3.
     *   i=0: 0x8000*0x10000 = 0x80000000; >>15 = 0x10000; + m[3][0](0) = 0x10000
     *   i=1: 0x8000*0x20000 = 0x100000000; >>15 = 0x20000
     *   i=2: 0x8000*0x30000 = 0x180000000; >>15 = 0x30000
     */
    {
        Mtx m;
        set_identity(&m);
        s32 o0 = 0, o1 = 0, o2 = 0;
        lc2wc(&m, 0x10000, 0x20000, 0x30000, &o0, &o1, &o2);
        CHECK(o0 == 0x10000, "lc2wc o0");
        CHECK(o1 == 0x20000, "lc2wc o1");
        CHECK(o2 == 0x30000, "lc2wc o2");
    }

    /* ---- wc2lc ----
     * Identity matrix, args (0x10000, 0x20000, 0x30000).
     * Subtract m[3][*] (all 0) -> args unchanged.
     * Row i: value = m[i][0]*arg1 + m[i][1]*arg2 + m[i][2]*arg3.
     *   i=0: 0x8000*0x10000 = 0x80000000; >>15 = 0x10000
     *   i=1: 0x8000*0x20000 = 0x100000000; >>15 = 0x20000
     *   i=2: 0x8000*0x30000 = 0x180000000; >>15 = 0x30000
     */
    {
        Mtx m;
        set_identity(&m);
        s32 o0 = 0, o1 = 0, o2 = 0;
        wc2lc(&m, 0x10000, 0x20000, 0x30000, &o0, &o1, &o2);
        CHECK(o0 == 0x10000, "wc2lc o0");
        CHECK(o1 == 0x20000, "wc2lc o1");
        CHECK(o2 == 0x30000, "wc2lc o2");
    }

    /* ---- matrixMulL ----
     * m = identity, n = translation (diag 0x8000, n[3][0]=0x10000, n[3][1]=0x20000, n[3][2]=0x30000).
     * r[i][j] (i,j<3) = sum_k m[i][k]*n[k][j] = 0x8000*n[i][j] (m identity).
     *   r[0][0] = 0x8000*0x8000 = 0x40000000; >>15 = 0x8000.
     *   r[0][1] = 0x8000*0 = 0.
     * r[3][i] = n[3][i] + (sum_k m[3][k]*n[k][i] >>15); m[3][k]=0 -> shifted 0 -> r[3][i]=n[3][i].
     *   r[3][0]=0x10000, r[3][1]=0x20000, r[3][2]=0x30000.
     * Forced: r[0][3]=r[1][3]=r[2][3]=0, r[3][3]=0x8000.
     */
    {
        Mtx m, n, r;
        set_identity(&m);
        set_identity(&n);
        n.m[3][0] = 0x10000;
        n.m[3][1] = 0x20000;
        n.m[3][2] = 0x30000;
        memset(&r, 0, sizeof(Mtx));
        matrixMulL(&m, &n, &r);
        CHECK(r.m[0][0] == 0x8000, "mul r00");
        CHECK(r.m[1][1] == 0x8000, "mul r11");
        CHECK(r.m[2][2] == 0x8000, "mul r22");
        CHECK(r.m[0][1] == 0, "mul r01");
        CHECK(r.m[3][0] == 0x10000, "mul r30");
        CHECK(r.m[3][1] == 0x20000, "mul r31");
        CHECK(r.m[3][2] == 0x30000, "mul r32");
        CHECK(r.m[3][3] == 0x8000, "mul r33");
        CHECK(r.m[0][3] == 0, "mul r03");
    }

    /* ---- makeTransrateMatrix ----
     * (0x10000, 0x20000, 0x30000): diag 0x8000, last row = offsets, m[3][3]=0x8000.
     */
    {
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeTransrateMatrix(&m, 0x10000, 0x20000, 0x30000);
        CHECK(m.m[0][0] == 0x8000, "trans m00");
        CHECK(m.m[1][1] == 0x8000, "trans m11");
        CHECK(m.m[2][2] == 0x8000, "trans m22");
        CHECK(m.m[3][0] == 0x10000, "trans m30");
        CHECK(m.m[3][1] == 0x20000, "trans m31");
        CHECK(m.m[3][2] == 0x30000, "trans m32");
        CHECK(m.m[3][3] == 0x8000, "trans m33");
        CHECK(m.m[0][1] == 0, "trans m01");
    }

    /* ---- makeScaleMatrix ----
     * scale = 0x4000: diag = 0x4000 (m[3][3]=0x8000), rest 0.
     */
    {
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeScaleMatrix(&m, 0x4000);
        CHECK(m.m[0][0] == 0x4000, "scale m00");
        CHECK(m.m[1][1] == 0x4000, "scale m11");
        CHECK(m.m[2][2] == 0x4000, "scale m22");
        CHECK(m.m[3][3] == 0x8000, "scale m33");
        CHECK(m.m[0][1] == 0, "scale m01");
        CHECK(m.m[3][0] == 0, "scale m30");
    }

    if (failures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d failures\n", failures);
    return 1;
}
