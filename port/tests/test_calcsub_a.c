/* Dr. Mario 64 -> Wii native port: calcsub host self-test.
 * Compiled standalone with -DTARGET_WII -DHOST_TEST (see the GATE command).
 * Every expected value below is hand-derived from src/main_segment/calcsub.s.
 *
 * get_angle/sinL/cosL return a 16-bit angle/value sign-extended into s32, so
 * the 16-bit checks compare only the low 16 bits (mask 0xFFFF) to avoid
 * sign-extension ambiguity. sqrtS/sqrtL return non-negative s32, compared
 * directly. waitEQ4 is exercised with *var already == data so it returns
 * immediately (reaching the next line proves it did not spin). */

#include <stdio.h>
#include <PR/ultratypes.h>

#ifdef HOST_TEST
#include <stdint.h>
#define s32 int32_t
#define s16 int16_t
#define u32 uint32_t
#define u8  uint8_t
#define u64 uint64_t
#define vs32 volatile int32_t
#endif

/* Prototypes declared manually so the host-test compile does not drag in
 * calcsub.h -> libultra.h -> version.h/ultra64.h (which need game-build
 * defines). The definitions live in port/game/calcsub.c. */
void waitEQ4(vs32 *var, s32 data);
s32 sqrtS(s32 value);
s32 sqrtL(s32 arg0, s32 arg1);
s32 get_angle(s32 x, s32 y);
s16 sinL(s16 angle);
s16 cosL(s16 angle);

static int g_pass = 1;
static int g_fail = 0;

#define CHECK16(name, got, exp) do { \
    unsigned g_ = (unsigned)(got) & 0xFFFFu; \
    unsigned e_ = (unsigned)(exp) & 0xFFFFu; \
    if (g_ == e_) { \
        printf("%s ok (0x%04X)\n", name, g_); \
    } else { \
        printf("%s FAIL got=0x%04X exp=0x%04X\n", name, g_, e_); \
        g_pass = 0; g_fail++; \
    } \
} while (0)

#define CHECK32(name, got, exp) do { \
    if ((got) == (exp)) { \
        printf("%s ok (%d)\n", name, (int)(got)); \
    } else { \
        printf("%s FAIL got=%d exp=%d\n", name, (int)(got), (int)(exp)); \
        g_pass = 0; g_fail++; \
    } \
} while (0)

int main(void)
{
    int r;

    /* ---------- sqrtS: (s32)sqrtf((float)value) ---------- */
    CHECK32("sqrtS(25)", sqrtS(25), 5);
    CHECK32("sqrtS(26)", sqrtS(26), 5);
    CHECK32("sqrtS(0)", sqrtS(0), 0);
    CHECK32("sqrtS(0x7FFFFFFF)", sqrtS(0x7FFFFFFF), 46340);

    /* ---------- sqrtL: isqrt((arg0<<32)|arg1) ---------- */
    CHECK32("sqrtL(0,25)", sqrtL(0, 25), 5);
    CHECK32("sqrtL(1,0)", sqrtL(1, 0), 65536);
    CHECK32("sqrtL(3,4)", sqrtL(3, 4), 113511);
    CHECK32("sqrtL(0,0)", sqrtL(0, 0), 0);
    CHECK32("sqrtL(0,0xFFFFFFFF)", sqrtL(0, (s32)0xFFFFFFFFu), 65535);

    /* ---------- get_angle (safe: a0 < 0x10000000) ---------- */
    CHECK16("get_angle(0,0)", get_angle(0, 0), 0x0000);
    CHECK16("get_angle(0,1)", get_angle(0, 1), 0x4000);
    CHECK16("get_angle(0,-1)", get_angle(0, -1), 0xC000);
    CHECK16("get_angle(-1,0)", get_angle(-1, 0), 0x8000);
    CHECK16("get_angle(100,1)", get_angle(100, 1), 0x0068);
    CHECK16("get_angle(100,100)", get_angle(100, 100), 0x1FFF);
    CHECK16("get_angle(-100,50)", get_angle(-100, 50), 0x6D1D);
    CHECK16("get_angle(1000,300)", get_angle(1000, 300), 0x0BDF);
    CHECK16("get_angle(3,4)", get_angle(3, 4), 0x25C9);
    CHECK16("get_angle(5,-12)", get_angle(5, -12), 0xD015);
    CHECK16("get_angle(-7,-7)", get_angle(-7, -7), 0x9FFF);

    /* ---------- sinL ---------- */
    CHECK16("sinL(0x0000)", sinL((s16)0x0000), 0x0000);
    CHECK16("sinL(0x4000)", sinL((s16)0x4000), 0x7FFF);
    CHECK16("sinL(0x2000)", sinL((s16)0x2000), 0x5A82);
    CHECK16("sinL(0x1000)", sinL((s16)0x1000), 0x30FC);
    CHECK16("sinL(0x8000)", sinL((s16)0x8000), 0x0000);
    CHECK16("sinL(0x1234)", sinL((s16)0x1234), 0x374D);
    CHECK16("sinL(0x3FFF)", sinL((s16)0x3FFF), 0x7FFF);
    CHECK16("sinL(0x7FFF)", sinL((s16)0x7FFF), 0x0003);

    /* ---------- cosL ---------- */
    CHECK16("cosL(0x0000)", cosL((s16)0x0000), 0x7FFF);
    CHECK16("cosL(0x4000)", cosL((s16)0x4000), 0x0000);
    CHECK16("cosL(0x2000)", cosL((s16)0x2000), 0x5A82);
    CHECK16("cosL(0x1000)", cosL((s16)0x1000), 0x7642);
    CHECK16("cosL(0x8000)", cosL((s16)0x8000), 0x8000);
    CHECK16("cosL(0x1234)", cosL((s16)0x1234), 0x736F);

    /* ---------- waitEQ4: *var already == data -> returns immediately ---------- */
    {
        vs32 v1 = 5;
        vs32 v2 = 0;
        vs32 v3 = -12345;
        waitEQ4(&v1, 5);
        waitEQ4(&v2, 0);
        waitEQ4(&v3, -12345);
        printf("waitEQ4 ok (3 immediate returns)\n");
    }

    if (g_fail == 0) {
        printf("CALCSUB PASS\n");
        return 0;
    }
    printf("CALCSUB FAIL (%d)\n", g_fail);
    (void)r;
    return 1;
}
