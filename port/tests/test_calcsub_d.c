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
#define s64 int64_t

void matrixConv(const Mtx *m, Mtx *mtx, s32 shift);
void matrixCopyL(Mtx *dst, const Mtx *src);
void rotpointL(s16 angle, s32 center_x, s32 center_y, s32 *x, s32 *y);
s32 defangleL(s32 a1, s32 a2);
s32 distanceS(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1);
s32 distanceL(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1);
s32 sqrt_a2b2(s32 a, s32 b);
s32 sqrt_abc(s32 a, s32 b, s32 c);
s32 muldiv(s32 a, s32 b, s32 c);
void makeVect(s16 arg0, s16 arg1, s32 *arg2, s32 *arg3, s32 *arg4);

static int failures = 0;
#define CHECK(cond, expr) do { if (!(cond)) { printf("FAIL %s\n", expr); failures++; } } while (0)

/* matrixConv reads/writes the matrix as s32 words, so check via a word view. */
static void check_words(Mtx *m, int base, s32 w0, s32 w1, const char *tag) {
    const s32 *p = (const s32 *)&m->m[0][0];
    if (p[base] == w0 && p[base + 1] == w1) return;
    printf("FAIL %s: got %08X %08X want %08X %08X\n",
           tag, (u32)p[base], (u32)p[base + 1], (u32)w0, (u32)w1);
    failures++;
}

static void fill_src(Mtx *m) {
    memset(m, 0, sizeof(Mtx));
    m->m[0][0] = 1; m->m[0][1] = 2; m->m[0][2] = 3; m->m[0][3] = 4;
    m->m[1][0] = 5; m->m[1][1] = 6; m->m[1][2] = 7; m->m[1][3] = 8;
}

int main(void) {
    /* ---- matrixConv shift=0: 8 iters of <<1 ---- */
    {
        Mtx src, mtx;
        fill_src(&src);
        memset(&mtx, 0, sizeof(Mtx));
        matrixConv(&src, &mtx, 0);
        check_words(&mtx, 0, 0, 0, "matrixConv(0) w0-1");
        check_words(&mtx, 2, 0, 0, "matrixConv(0) w2-3");
        check_words(&mtx, 4, 0, 0, "matrixConv(0) w4-5");
        check_words(&mtx, 6, 0, 0, "matrixConv(0) w6-7");
        check_words(&mtx, 8, 0x20000, 0x40000, "matrixConv(0) w8-9");
        check_words(&mtx, 10, 0x60000, 0x80000, "matrixConv(0) w10-11");
        check_words(&mtx, 12, 0xA0000, 0xC0000, "matrixConv(0) w12-13");
        check_words(&mtx, 14, 0xE0000, 0x100000, "matrixConv(0) w14-15");
    }
    /* ---- matrixConv shift=1: 6 iters <<2, then 2 iters <<1 ---- */
    {
        Mtx src, mtx;
        fill_src(&src);
        memset(&mtx, 0, sizeof(Mtx));
        matrixConv(&src, &mtx, 1);
        check_words(&mtx, 0, 0, 0, "matrixConv(1) w0-1");
        check_words(&mtx, 2, 0, 0, "matrixConv(1) w2-3");
        check_words(&mtx, 4, 0, 0, "matrixConv(1) w4-5");
        check_words(&mtx, 6, 0, 0, "matrixConv(1) w6-7");
        check_words(&mtx, 8, 0x40000, 0x80000, "matrixConv(1) w8-9");
        check_words(&mtx, 10, 0xC0000, 0x100000, "matrixConv(1) w10-11");
        check_words(&mtx, 12, 0x140000, 0x180000, "matrixConv(1) w12-13");
        check_words(&mtx, 14, 0xE0000, 0x100000, "matrixConv(1) w14-15");
    }
    /* ---- matrixConv shift=-1: 6 iters >>0, then 2 iters <<1 ---- */
    {
        Mtx src, mtx;
        fill_src(&src);
        memset(&mtx, 0, sizeof(Mtx));
        matrixConv(&src, &mtx, -1);
        check_words(&mtx, 0, 0, 0, "matrixConv(-1) w0-1");
        check_words(&mtx, 2, 0, 0, "matrixConv(-1) w2-3");
        check_words(&mtx, 4, 0, 0, "matrixConv(-1) w4-5");
        check_words(&mtx, 6, 0, 0, "matrixConv(-1) w6-7");
        check_words(&mtx, 8, 0x10000, 0x20000, "matrixConv(-1) w8-9");
        check_words(&mtx, 10, 0x30000, 0x40000, "matrixConv(-1) w10-11");
        check_words(&mtx, 12, 0x50000, 0x60000, "matrixConv(-1) w12-13");
        check_words(&mtx, 14, 0xE0000, 0x100000, "matrixConv(-1) w14-15");
    }

    /* ---- matrixCopyL: copy 4x4 ---- */
    {
        Mtx src, dst;
        fill_src(&src);
        memset(&dst, 0, sizeof(Mtx));
        matrixCopyL(&dst, &src);
        CHECK(dst.m[0][0] == 1 && dst.m[0][1] == 2 && dst.m[0][2] == 3 && dst.m[0][3] == 4, "matrixCopyL r0");
        CHECK(dst.m[1][0] == 5 && dst.m[1][1] == 6 && dst.m[1][2] == 7 && dst.m[1][3] == 8, "matrixCopyL r1");
        CHECK(dst.m[2][0] == 0 && dst.m[2][1] == 0 && dst.m[2][2] == 0 && dst.m[2][3] == 0, "matrixCopyL r2");
        CHECK(dst.m[3][0] == 0 && dst.m[3][1] == 0 && dst.m[3][2] == 0 && dst.m[3][3] == 0, "matrixCopyL r3");
    }

    /* ---- rotpointL ---- */
    {
        /* angle=0: sin=0 cos=0x7FFF. x=100,y=200 -> x=99,y=199 */
        s32 x = 100, y = 200;
        rotpointL(0, 0, 0, &x, &y);
        CHECK(x == 99 && y == 199, "rotpointL(0,0,0,100,200)");
    }
    {
        /* angle=0x4000: sin=0x7FFF cos=0. x=100,y=200 -> x=-200,y=99 */
        s32 x = 100, y = 200;
        rotpointL(0x4000, 0, 0, &x, &y);
        CHECK(x == -200 && y == 99, "rotpointL(0x4000,0,0,100,200)");
    }
    {
        /* angle=0, center=(10,20), x=110,y=220 -> x=109,y=219 */
        s32 x = 110, y = 220;
        rotpointL(0, 10, 20, &x, &y);
        CHECK(x == 109 && y == 219, "rotpointL(0,10,20,110,220)");
    }

    /* ---- defangleL ---- */
    CHECK(defangleL(0, 0x4000) == 0x4000, "defangleL(0,0x4000)");
    CHECK(defangleL(0, -0x4000) == -0x4000, "defangleL(0,-0x4000)");
    CHECK(defangleL(0x3000, 0x1000) == -0x2000, "defangleL(0x3000,0x1000)");

    /* ---- distanceS ---- */
    CHECK(distanceS(0, 0, 0, 3, 4, 0) == 5, "distanceS 3-4-0");
    CHECK(distanceS(0, 0, 0, 1, 1, 1) == 1, "distanceS 1-1-1");
    CHECK(distanceS(10, 20, 30, 13, 24, 30) == 5, "distanceS 3-4-0 offset");

    /* ---- distanceL ---- */
    CHECK(distanceL(0, 0, 0, 3, 4, 0) == 5, "distanceL 3-4-0");
    CHECK(distanceL(0, 0, 0, 1, 1, 1) == 1, "distanceL 1-1-1");
    CHECK(distanceL(0, 0, 0, 0, 0, 5) == 5, "distanceL 0-0-5");

    /* ---- sqrt_a2b2 ---- */
    CHECK(sqrt_a2b2(3, 4) == 5, "sqrt_a2b2(3,4)");
    CHECK(sqrt_a2b2(1, 1) == 1, "sqrt_a2b2(1,1)");
    CHECK(sqrt_a2b2(0, 0) == 0, "sqrt_a2b2(0,0)");

    /* ---- sqrt_abc ---- */
    CHECK(sqrt_abc(1, 2, 2) == 3, "sqrt_abc(1,2,2)");
    CHECK(sqrt_abc(0, 0, 0) == 0, "sqrt_abc(0,0,0)");
    CHECK(sqrt_abc(3, 4, 0) == 5, "sqrt_abc(3,4,0)");

    /* ---- muldiv ---- */
    CHECK(muldiv(3, 4, 5) == 2, "muldiv(3,4,5)");
    CHECK(muldiv(-3, 4, 5) == -2, "muldiv(-3,4,5)");
    CHECK(muldiv(10, 10, 3) == 33, "muldiv(10,10,3)");

    /* ---- makeVect ---- */
    {
        s32 a2, a3, a4;
        makeVect(0, 0, &a2, &a3, &a4);
        CHECK(a2 == 0 && a3 == 0 && a4 == 0x7FFE, "makeVect(0,0)");
    }
    {
        s32 a2, a3, a4;
        makeVect(0x4000, 0, &a2, &a3, &a4);
        CHECK(a2 == 0 && a3 == -0x7FFF && a4 == 0, "makeVect(0x4000,0)");
    }
    {
        s32 a2, a3, a4;
        makeVect(0, 0x4000, &a2, &a3, &a4);
        CHECK(a2 == 0x7FFE && a3 == 0 && a4 == 0, "makeVect(0,0x4000)");
    }

    if (failures == 0) {
        printf("test_calcsub_d: all checks passed\n");
        return 0;
    }
    printf("test_calcsub_d: %d failures\n", failures);
    return 1;
}
