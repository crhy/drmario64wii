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

void makeMatrix(Mtx *m, s16 xrot, s16 yrot, s16 zrot, s32 xofs, s32 yofs, s32 zofs);
void makeXrotMatrix(Mtx *m, s16 xrot, s32 xofs, s32 yofs, s32 zofs);
void makeYrotMatrix(Mtx *m, s16 yrot, s32 xofs, s32 yofs, s32 zofs);
void makeZrotMatrix(Mtx *m, s16 zrot, s32 xofs, s32 yofs, s32 zofs);
void makeXZMatrix(Mtx *m, s16 xrot, s16 zrot);

static int failures = 0;
#define CHECK(cond, expr) do { if (!(cond)) { printf("FAIL %s\n", expr); failures++; } } while (0)

/* Trig values used in the tests (derived from calc_sintable in calcsub.c):
 *   sinL(0)      = calc_sintable[0]   = 0
 *   cosL(0)      = calc_sintable[0x100] = 0x7FFF   (a = 0+0x4000, upper=0x200, low=0)
 *   sinL(0x4000) = calc_sintable[0x100] = 0x7FFF
 *   cosL(0x4000) = calc_sintable[0x200] = 0        (a = 0x8000, upper=0x400, low=0)
 * Also 0x7FFF*0x7FFF = 0x3FFF0001; 0x3FFF0001 >> 15 = 0x7FFE;
 * -(0x3FFF0001) >> 15 (arithmetic) = -0x7FFF (floor of -32766.00003).
 * And 0x7FFF*0x7FFE = 0x3FE80002; 0x3FE80002 >> 15 = 0x7FFD. */

static void check_row(Mtx *m, int row, s32 e0, s32 e1, s32 e2, s32 e3, const char *tag) {
    if (m->m[row][0] == e0 && m->m[row][1] == e1 && m->m[row][2] == e2 && m->m[row][3] == e3) return;
    printf("FAIL %s: got %08X %08X %08X %08X want %08X %08X %08X %08X\n",
           tag, (u32)m->m[row][0], (u32)m->m[row][1], (u32)m->m[row][2], (u32)m->m[row][3],
           (u32)e0, (u32)e1, (u32)e2, (u32)e3);
    failures++;
}

int main(void) {
    /* ---- makeMatrix ---- */
    {
        /* (0,0,0): sin=0, cos=0x7FFF.
         * a=(0*0)>>15=0; b=(0x7FFF*0)>>15=0.
         * m[0][0]=(0*0 + 0x7FFF*0x7FFF)>>15 = 0x3FFF8001>>15 = 0x7FFF
         * m[0][1]=(0*0x7FFF)>>15=0; m[0][2]=(0*0 - 0x7FFF*0)>>15=0
         * m[1][0]=(0x7FFF*0 - 0*0x7FFF)>>15=0; m[1][1]=(0x7FFF*0x7FFF)>>15=0x7FFF
         * m[1][2]=(0x7FFF*0 + 0*0)>>15=0
         * m[2][0]=(0*0x7FFF)>>15=0; m[2][1]=-0=0; m[2][2]=(0x7FFF*0x7FFF)>>15=0x7FFF
         * m[3]=(1,2,3,0x8000) */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeMatrix(&m, 0, 0, 0, 1, 2, 3);
        check_row(&m, 0, 0x7FFE, 0, 0, 0, "makeMatrix(0,0,0) r0");
        check_row(&m, 1, 0, 0x7FFE, 0, 0, "makeMatrix(0,0,0) r1");
        check_row(&m, 2, 0, 0, 0x7FFE, 0, "makeMatrix(0,0,0) r2");
        check_row(&m, 3, 1, 2, 3, 0x8000, "makeMatrix(0,0,0) r3");
    }
    {
        /* (x=0x4000, y=0, z=0): sin_x=0x7FFF cos_x=0; sin_y=0 cos_y=0x7FFF;
         * sin_z=0 cos_z=0x7FFF.
         * a=(0*0x7FFF)>>15=0; b=(0x7FFF*0x7FFF)>>15=0x7FFF.
         * m[0][0]=(0*0 + 0x7FFF*0x7FFF)>>15=0x7FFF
         * m[0][1]=(0*0)>>15=0; m[0][2]=(0*0x7FFF - 0x7FFF*0)>>15=0
         * m[1][0]=(0x7FFF*0 - 0*0x7FFF)>>15=0; m[1][1]=(0x7FFF*0)>>15=0
         * m[1][2]=(0x7FFF*0x7FFF + 0*0)>>15=0x7FFF
         * m[2][0]=(0*0)>>15=0; m[2][1]=-0x7FFF; m[2][2]=(0x7FFF*0)>>15=0
         * m[3]=(0x10,0x20,0x30,0x8000) */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeMatrix(&m, 0x4000, 0, 0, 0x10, 0x20, 0x30);
        check_row(&m, 0, 0x7FFE, 0, 0, 0, "makeMatrix(0x4000,0,0) r0");
        check_row(&m, 1, 0, 0, 0x7FFD, 0, "makeMatrix(0x4000,0,0) r1");
        check_row(&m, 2, 0, -0x7FFF, 0, 0, "makeMatrix(0x4000,0,0) r2");
        check_row(&m, 3, 0x10, 0x20, 0x30, 0x8000, "makeMatrix(0x4000,0,0) r3");
    }
    {
        /* (x=0, y=0x4000, z=0): sin_x=0 cos_x=0x7FFF; sin_y=0x7FFF cos_y=0;
         * sin_z=0 cos_z=0x7FFF.
         * a=(0x7FFF*0)>>15=0; b=(0*0)>>15=0.
         * m[0][0]=(0*0 + 0x7FFF*0)>>15=0
         * m[0][1]=(0*0x7FFF)>>15=0; m[0][2]=(0*0 - 0x7FFF*0x7FFF)>>15 = -0x3FFF8001>>15 = -0x8000
         * m[1][0]=(0x7FFF*0 - 0*0)>>15=0; m[1][1]=(0x7FFF*0x7FFF)>>15=0x7FFF
         * m[1][2]=(0x7FFF*0 + 0*0x7FFF)>>15=0
         * m[2][0]=(0x7FFF*0x7FFF)>>15=0x7FFF; m[2][1]=-0=0; m[2][2]=(0*0x7FFF)>>15=0
         * m[3]=(-1,-2,-3,0x8000) */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeMatrix(&m, 0, 0x4000, 0, -1, -2, -3);
        check_row(&m, 0, 0, 0, -0x7FFF, 0, "makeMatrix(0,0x4000,0) r0");
        check_row(&m, 1, 0, 0x7FFE, 0, 0, "makeMatrix(0,0x4000,0) r1");
        check_row(&m, 2, 0x7FFE, 0, 0, 0, "makeMatrix(0,0x4000,0) r2");
        check_row(&m, 3, -1, -2, -3, 0x8000, "makeMatrix(0,0x4000,0) r3");
    }

    /* ---- makeXrotMatrix ---- */
    {
        /* xrot=0: sin=0 cos=0x7FFF. m = [0x8000,0,0,0; 0,0x7FFF,0,0; 0,0,0x7FFF,0; 1,2,3,0x8000] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeXrotMatrix(&m, 0, 1, 2, 3);
        check_row(&m, 0, 0x8000, 0, 0, 0, "makeXrotMatrix(0) r0");
        check_row(&m, 1, 0, 0x7FFF, 0, 0, "makeXrotMatrix(0) r1");
        check_row(&m, 2, 0, 0, 0x7FFF, 0, "makeXrotMatrix(0) r2");
        check_row(&m, 3, 1, 2, 3, 0x8000, "makeXrotMatrix(0) r3");
    }
    {
        /* xrot=0x4000: sin=0x7FFF cos=0. m[1][1]=0 m[1][2]=0x7FFF m[2][1]=-0x7FFF m[2][2]=0 */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeXrotMatrix(&m, 0x4000, 4, 5, 6);
        check_row(&m, 0, 0x8000, 0, 0, 0, "makeXrotMatrix(0x4000) r0");
        check_row(&m, 1, 0, 0, 0x7FFF, 0, "makeXrotMatrix(0x4000) r1");
        check_row(&m, 2, 0, -0x7FFF, 0, 0, "makeXrotMatrix(0x4000) r2");
        check_row(&m, 3, 4, 5, 6, 0x8000, "makeXrotMatrix(0x4000) r3");
    }
    {
        /* xrot=-0x4000: sinL(-0x4000): a=0xFFFFC000, upper=0x600, ret=calc_sintable[0x300]
         * = -0x8000 (table is asymmetric: sin(-0x4000) = -0x8000); cosL(-0x4000): a=0, ret=0.
         * m[1][1]=0 m[1][2]=-0x8000 m[2][1]=0x8000 m[2][2]=0 */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeXrotMatrix(&m, -0x4000, -4, -5, -6);
        check_row(&m, 0, 0x8000, 0, 0, 0, "makeXrotMatrix(-0x4000) r0");
        check_row(&m, 1, 0, 0, -0x8000, 0, "makeXrotMatrix(-0x4000) r1");
        check_row(&m, 2, 0, 0x8000, 0, 0, "makeXrotMatrix(-0x4000) r2");
        check_row(&m, 3, -4, -5, -6, 0x8000, "makeXrotMatrix(-0x4000) r3");
    }

    /* ---- makeYrotMatrix ---- */
    {
        /* yrot=0: sin=0 cos=0x7FFF: [0x7FFF,0,0,0; 0,0x8000,0,0; 0,0,0x7FFF,0; 1,2,3,0x8000] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeYrotMatrix(&m, 0, 1, 2, 3);
        check_row(&m, 0, 0x7FFF, 0, 0, 0, "makeYrotMatrix(0) r0");
        check_row(&m, 1, 0, 0x8000, 0, 0, "makeYrotMatrix(0) r1");
        check_row(&m, 2, 0, 0, 0x7FFF, 0, "makeYrotMatrix(0) r2");
        check_row(&m, 3, 1, 2, 3, 0x8000, "makeYrotMatrix(0) r3");
    }
    {
        /* yrot=0x4000: sin=0x7FFF cos=0: [0,0,-0x7FFF,0; 0,0x8000,0,0; 0x7FFF,0,0,0; ...] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeYrotMatrix(&m, 0x4000, 4, 5, 6);
        check_row(&m, 0, 0, 0, -0x7FFF, 0, "makeYrotMatrix(0x4000) r0");
        check_row(&m, 1, 0, 0x8000, 0, 0, "makeYrotMatrix(0x4000) r1");
        check_row(&m, 2, 0x7FFF, 0, 0, 0, "makeYrotMatrix(0x4000) r2");
        check_row(&m, 3, 4, 5, 6, 0x8000, "makeYrotMatrix(0x4000) r3");
    }
    {
        /* yrot=-0x4000: sin=-0x8000 cos=0: [0,0,0x8000,0; 0,0x8000,0,0; -0x8000,0,0,0; ...] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeYrotMatrix(&m, -0x4000, -4, -5, -6);
        check_row(&m, 0, 0, 0, 0x8000, 0, "makeYrotMatrix(-0x4000) r0");
        check_row(&m, 1, 0, 0x8000, 0, 0, "makeYrotMatrix(-0x4000) r1");
        check_row(&m, 2, -0x8000, 0, 0, 0, "makeYrotMatrix(-0x4000) r2");
        check_row(&m, 3, -4, -5, -6, 0x8000, "makeYrotMatrix(-0x4000) r3");
    }

    /* ---- makeZrotMatrix ---- */
    {
        /* zrot=0: sin=0 cos=0x7FFF: [0x7FFF,0,0,0; 0,0x7FFF,0,0; 0,0,0x8000,0; 1,2,3,0x8000] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeZrotMatrix(&m, 0, 1, 2, 3);
        check_row(&m, 0, 0x7FFF, 0, 0, 0, "makeZrotMatrix(0) r0");
        check_row(&m, 1, 0, 0x7FFF, 0, 0, "makeZrotMatrix(0) r1");
        check_row(&m, 2, 0, 0, 0x8000, 0, "makeZrotMatrix(0) r2");
        check_row(&m, 3, 1, 2, 3, 0x8000, "makeZrotMatrix(0) r3");
    }
    {
        /* zrot=0x4000: sin=0x7FFF cos=0: [0,0x7FFF,0,0; -0x7FFF,0,0,0; 0,0,0x8000,0; ...] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeZrotMatrix(&m, 0x4000, 4, 5, 6);
        check_row(&m, 0, 0, 0x7FFF, 0, 0, "makeZrotMatrix(0x4000) r0");
        check_row(&m, 1, -0x7FFF, 0, 0, 0, "makeZrotMatrix(0x4000) r1");
        check_row(&m, 2, 0, 0, 0x8000, 0, "makeZrotMatrix(0x4000) r2");
        check_row(&m, 3, 4, 5, 6, 0x8000, "makeZrotMatrix(0x4000) r3");
    }
    {
        /* zrot=-0x4000: sin=-0x8000 cos=0: [0,-0x8000,0,0; 0x8000,0,0,0; 0,0,0x8000,0; ...] */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeZrotMatrix(&m, -0x4000, -4, -5, -6);
        check_row(&m, 0, 0, -0x8000, 0, 0, "makeZrotMatrix(-0x4000) r0");
        check_row(&m, 1, 0x8000, 0, 0, 0, "makeZrotMatrix(-0x4000) r1");
        check_row(&m, 2, 0, 0, 0x8000, 0, "makeZrotMatrix(-0x4000) r2");
        check_row(&m, 3, -4, -5, -6, 0x8000, "makeZrotMatrix(-0x4000) r3");
    }

    /* ---- makeXZMatrix ---- */
    {
        /* (x=0, z=0): sin_z=0 cos_z=0x7FFF sin_x=0 cos_x=0x7FFF.
         * m[1][0]=(-0*0x7FFF)>>15=0; m[1][1]=(0x7FFF*0x7FFF)>>15=0x7FFF;
         * m[2][0]=(0*0)>>15=0; m[2][1]=(-0*0x7FFF)>>15=0 */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeXZMatrix(&m, 0, 0);
        check_row(&m, 0, 0x7FFF, 0, 0, 0, "makeXZMatrix(0,0) r0");
        check_row(&m, 1, 0, 0x7FFE, 0, 0, "makeXZMatrix(0,0) r1");
        check_row(&m, 2, 0, 0, 0x7FFF, 0, "makeXZMatrix(0,0) r2");
        check_row(&m, 3, 0, 0, 0, 0x8000, "makeXZMatrix(0,0) r3");
    }
    {
        /* (x=0x4000, z=0): sin_x=0x7FFF cos_x=0; sin_z=0 cos_z=0x7FFF.
         * m[1][0]=(-0*0)>>15=0; m[1][1]=(0x7FFF*0)>>15=0; m[1][2]=0x7FFF;
         * m[2][0]=(0*0x7FFF)>>15=0; m[2][1]=(-0x7FFF*0x7FFF)>>15=-0x8000; m[2][2]=0 */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeXZMatrix(&m, 0x4000, 0);
        check_row(&m, 0, 0x7FFF, 0, 0, 0, "makeXZMatrix(0x4000,0) r0");
        check_row(&m, 1, 0, 0, 0x7FFF, 0, "makeXZMatrix(0x4000,0) r1");
        check_row(&m, 2, 0, -0x7FFF, 0, 0, "makeXZMatrix(0x4000,0) r2");
        check_row(&m, 3, 0, 0, 0, 0x8000, "makeXZMatrix(0x4000,0) r3");
    }
    {
        /* (x=0, z=0x4000): sin_z=0x7FFF cos_z=0; sin_x=0 cos_x=0x7FFF.
         * m[0][0]=0; m[0][1]=0x7FFF;
         * m[1][0]=(-0x7FFF*0x7FFF)>>15=-0x8000; m[1][1]=(0*0x7FFF)>>15=0; m[1][2]=0;
         * m[2][0]=(0x7FFF*0)>>15=0; m[2][1]=(-0*0)>>15=0; m[2][2]=0x7FFF */
        Mtx m;
        memset(&m, 0, sizeof(Mtx));
        makeXZMatrix(&m, 0, 0x4000);
        check_row(&m, 0, 0, 0x7FFF, 0, 0, "makeXZMatrix(0,0x4000) r0");
        check_row(&m, 1, -0x7FFF, 0, 0, 0, "makeXZMatrix(0,0x4000) r1");
        check_row(&m, 2, 0, 0, 0x7FFF, 0, "makeXZMatrix(0,0x4000) r2");
        check_row(&m, 3, 0, 0, 0, 0x8000, "makeXZMatrix(0,0x4000) r3");
    }

    if (failures == 0) {
        printf("test_calcsub_c: all checks passed\n");
        return 0;
    }
    printf("test_calcsub_c: %d failures\n", failures);
    return 1;
}
