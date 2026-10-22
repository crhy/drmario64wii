/* Host test for the pure controller mapping helpers in pad_wii.c (task 12a).
 * Built with plain gcc + -DPAD_HOST_TEST (see the GATE). Feeds fake held-button
 * words (using the REAL libogc bit values) to the mapping functions and checks
 * the N64 bits they produce. */

#include <stdio.h>
#include "pad_wii.h"

/* N64 bits (include/PR/os_cont.h layout) we expect out. */
#define N64_A     0x8000
#define N64_B     0x4000
#define N64_G     0x2000
#define N64_START 0x1000
#define N64_UP    0x0800
#define N64_DOWN  0x0400
#define N64_LEFT  0x0200
#define N64_RIGHT 0x0100
#define N64_R     0x0010

/* Real libogc WPAD/PAD input bits (wpad.h / ogc/pad.h). */
#define WPAD_UP    0x0800
#define WPAD_DOWN  0x0400
#define WPAD_LEFT  0x0100
#define WPAD_RIGHT 0x0200
#define WPAD_2     0x0001
#define WPAD_1     0x0002
#define WPAD_PLUS  0x1000
#define WPAD_MINUS 0x0010
#define CLASSIC_UP     0x0001
#define CLASSIC_FULL_R 0x0200
#define CLASSIC_PLUS   0x0400
#define GC_START       0x1000

extern unsigned short port_map_wiimote_sideways(unsigned held);
extern unsigned short port_map_classic(unsigned btns);
extern unsigned short port_map_gc(unsigned held);

static int fails = 0;

static void check(const char *what, unsigned short got, unsigned short want)
{
    if (got != want) {
        printf("FAIL %s: got %04x want %04x\n", what, got, want);
        fails++;
    }
}

int main(void)
{
    /* Sideways Wiimote: Wii Up -> N64 D-Left, Down -> D-Right,
     * Left -> D-Down, Right -> D-Up, 2->A, 1->B, PLUS->START, MINUS->Z. */
    check("wm up",    port_map_wiimote_sideways(WPAD_UP),    N64_LEFT);
    check("wm down",  port_map_wiimote_sideways(WPAD_DOWN),  N64_RIGHT);
    check("wm left",  port_map_wiimote_sideways(WPAD_LEFT),  N64_DOWN);
    check("wm right", port_map_wiimote_sideways(WPAD_RIGHT), N64_UP);
    check("wm 2",     port_map_wiimote_sideways(WPAD_2),     N64_A);
    check("wm 1",     port_map_wiimote_sideways(WPAD_1),     N64_B);
    check("wm plus",  port_map_wiimote_sideways(WPAD_PLUS),  N64_START);
    check("wm minus", port_map_wiimote_sideways(WPAD_MINUS), N64_G);

    /* Classic: UP(0x0001)->D-up, FULL_R(0x0200)->R, PLUS(0x0400)->START. */
    check("cl up",    port_map_classic(CLASSIC_UP),     N64_UP);
    check("cl fullR", port_map_classic(CLASSIC_FULL_R), N64_R);
    check("cl plus",  port_map_classic(CLASSIC_PLUS),   N64_START);

    /* GC: START(0x1000)->START. */
    check("gc start", port_map_gc(GC_START), N64_START);

    /* Presence logic: PAD_ScanPads() is a BITMASK (bit i = port i), not a
     * count. mask 0b0110 -> ports 1 and 2 present, ports 0 and 3 absent. */
    unsigned mask = 0x6u; /* 0b0110 */
    int i;
    for (i = 0; i < 4; i++) {
        PortPad pp;
        int gc_present = (int)((mask >> i) & 1u);
        int want = gc_present; /* no wiimote in this sub-test */
        int got = port_pad_merge(0, 0, gc_present, 0, 0, 0, &pp);
        if (got != want) {
            printf("FAIL presence port %d: got %d want %d\n", i, got, want);
            fails++;
        }
        if (pp.present != want) {
            printf("FAIL presence.out port %d: got %d want %d\n", i, pp.present, want);
            fails++;
        }
    }

    /* Both a wiimote and a GC pad on one port -> buttons ORed, GC stick used. */
    {
        PortPad pp;
        int pr = port_pad_merge(1, N64_A, 1, N64_B, 3, -4, &pp);
        if (pr != 1) { printf("FAIL merge present: got %d\n", pr); fails++; }
        if (pp.n64_buttons != (unsigned short)(N64_A | N64_B)) {
            printf("FAIL merge or: got %04x want %04x\n", pp.n64_buttons, (unsigned)(N64_A | N64_B));
            fails++;
        }
        if (pp.stick_x != 3 || pp.stick_y != -4) {
            printf("FAIL merge stick: got %d,%d want 3,-4\n", pp.stick_x, pp.stick_y);
            fails++;
        }
    }

    if (fails == 0)
        printf("pad map OK\n");
    return fails ? 1 : 0;
}
