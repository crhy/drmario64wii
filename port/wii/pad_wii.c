/* Dr. Mario 64 -> Wii native port: real libogc controller layer (task 12a).

 * This file owns ALL libogc WPAD/PAD usage. It includes the REAL headers
 * (<gccore.h>, <wiiuse/wpad.h>) so the button constants and the expansion_t
 * ABI come from libogc itself, not from hand-copied declarations. It exports
 * a plain-C surface (PortPad + port_pad_scan, declared in pad_wii.h) that
 * os_cont.c talks to, so libogc's u32/s32 typedefs never leak into the
 * libultra layer.

 * The pure button-mapping helpers (port_map_wiimote_sideways / port_map_classic
 * / port_map_gc) are compiled in BOTH the Wii build and the host test (the
 * GATE builds this file with plain gcc + -DPAD_HOST_TEST). The libogc-only
 * parts (the includes and port_pad_scan) sit behind #ifndef PAD_HOST_TEST.

 * Mapping spec is .strata/tasks/11-wii-input.md; N64 bit values are copied
 * from include/PR/os_cont.h as #defines (this file must NOT include ultra
 * headers). */

#include "pad_wii.h"

/* ---------- N64 button bits (from include/PR/os_cont.h) ---------- */
#define N64_A        0x8000
#define N64_B        0x4000
#define N64_G        0x2000   /* Z */
#define N64_START    0x1000
#define N64_UP       0x0800
#define N64_DOWN     0x0400
#define N64_LEFT     0x0200
#define N64_RIGHT    0x0100
#define N64_L        0x0020
#define N64_R        0x0010
#define N64_C_UP     0x0008   /* E */
#define N64_C_DOWN   0x0004   /* D */
#define N64_C_LEFT   0x0002   /* C */
#define N64_C_RIGHT  0x0001   /* F */

/* Classic Controller masks in the LOW 16 bits. libogc's WPAD_CLASSIC_BUTTON_*
 * are these same bits shifted <<16; port_map_classic is fed (btns >> 16), so
 * we use the low-16 form here. Values verified against wpad.h. */
#define CLASSIC_UP      0x0001
#define CLASSIC_LEFT    0x0002
#define CLASSIC_ZR      0x0004
#define CLASSIC_X       0x0008
#define CLASSIC_A       0x0010
#define CLASSIC_Y       0x0020
#define CLASSIC_B       0x0040
#define CLASSIC_ZL      0x0080
#define CLASSIC_FULL_R  0x0200
#define CLASSIC_PLUS    0x0400
#define CLASSIC_HOME    0x0800
#define CLASSIC_MINUS   0x1000
#define CLASSIC_FULL_L  0x2000
#define CLASSIC_DOWN    0x4000
#define CLASSIC_RIGHT   0x8000

#ifdef PAD_HOST_TEST
/* Host test: no libogc available. Provide the REAL libogc WPAD/PAD button
 * values (verified against wpad.h / ogc/pad.h) so the pure mapping helpers
 * compile and can be exercised with fake held-button words. */
#define WPAD_BUTTON_2     0x0001
#define WPAD_BUTTON_1     0x0002
#define WPAD_BUTTON_B     0x0004
#define WPAD_BUTTON_A     0x0008
#define WPAD_BUTTON_MINUS 0x0010
#define WPAD_BUTTON_PLUS  0x1000
#define WPAD_BUTTON_HOME  0x0080
#define WPAD_BUTTON_LEFT  0x0100
#define WPAD_BUTTON_RIGHT 0x0200
#define WPAD_BUTTON_DOWN  0x0400
#define WPAD_BUTTON_UP    0x0800
#define PAD_BUTTON_LEFT   0x0001
#define PAD_BUTTON_RIGHT  0x0002
#define PAD_BUTTON_DOWN   0x0004
#define PAD_BUTTON_UP     0x0008
#define PAD_TRIGGER_Z     0x0010
#define PAD_TRIGGER_R     0x0020
#define PAD_TRIGGER_L     0x0040
#define PAD_BUTTON_A      0x0100
#define PAD_BUTTON_B      0x0200
#define PAD_BUTTON_X      0x0400
#define PAD_BUTTON_Y      0x0800
#define PAD_BUTTON_START  0x1000
#else
/* Wii build: the real libogc headers, real constants, real expansion_t. */
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

/* ---------- pure mapping helpers (compiled in BOTH builds) ---------- */

/* Wiimote held SIDEWAYS (D-pad on the left). Real WPAD_BUTTON_* bits.
 * Wii Up -> N64 D-Left, Down -> D-Right, Left -> D-Down, Right -> D-Up;
 * 2 -> A, 1 -> B, + -> START, - -> Z, A -> L, B -> R. */
unsigned short port_map_wiimote_sideways(unsigned held)
{
    unsigned short b = 0;
    if (held & WPAD_BUTTON_2)     b |= N64_A;
    if (held & WPAD_BUTTON_1)     b |= N64_B;
    if (held & WPAD_BUTTON_PLUS)  b |= N64_START;
    if (held & WPAD_BUTTON_MINUS) b |= N64_G;
    if (held & WPAD_BUTTON_A)     b |= N64_L;
    if (held & WPAD_BUTTON_B)     b |= N64_R;
    if (held & WPAD_BUTTON_UP)    b |= N64_LEFT;
    if (held & WPAD_BUTTON_DOWN)  b |= N64_RIGHT;
    if (held & WPAD_BUTTON_LEFT)  b |= N64_DOWN;
    if (held & WPAD_BUTTON_RIGHT) b |= N64_UP;
    return b;
}

/* Classic Controller, fed the LOW-16 classic layout (btns >> 16).
 * D-pad -> D-pad, a -> A, b -> B, + -> START, ZL -> Z, FULL_L -> L,
 * FULL_R -> R, y -> C-left, x -> C-up. */
unsigned short port_map_classic(unsigned btns)
{
    unsigned short b = 0;
    if (btns & CLASSIC_A)      b |= N64_A;
    if (btns & CLASSIC_B)      b |= N64_B;
    if (btns & CLASSIC_PLUS)   b |= N64_START;
    if (btns & CLASSIC_ZL)     b |= N64_G;
    if (btns & CLASSIC_FULL_L) b |= N64_L;
    if (btns & CLASSIC_FULL_R) b |= N64_R;
    if (btns & CLASSIC_Y)      b |= N64_C_LEFT;
    if (btns & CLASSIC_X)      b |= N64_C_UP;
    if (btns & CLASSIC_UP)     b |= N64_UP;
    if (btns & CLASSIC_DOWN)   b |= N64_DOWN;
    if (btns & CLASSIC_LEFT)   b |= N64_LEFT;
    if (btns & CLASSIC_RIGHT)  b |= N64_RIGHT;
    return b;
}

/* GameCube pad, real PAD_BUTTON_* bits. A->A B->B START->START Z->Z
 * L->L R->R, D-pad -> D-pad, C-stick -> C buttons. */
unsigned short port_map_gc(unsigned held)
{
    unsigned short b = 0;
    if (held & PAD_BUTTON_A)     b |= N64_A;
    if (held & PAD_BUTTON_B)     b |= N64_B;
    if (held & PAD_BUTTON_START) b |= N64_START;
    if (held & PAD_TRIGGER_Z)    b |= N64_G;
    if (held & PAD_TRIGGER_L)    b |= N64_L;
    if (held & PAD_TRIGGER_R)    b |= N64_R;
    if (held & PAD_BUTTON_UP)    b |= N64_UP;
    if (held & PAD_BUTTON_DOWN)  b |= N64_DOWN;
    if (held & PAD_BUTTON_LEFT)  b |= N64_LEFT;
    if (held & PAD_BUTTON_RIGHT)   b |= N64_RIGHT;
    return b;
}

/* ---------- pure merge helper (compiled in BOTH builds) ----------
 * Combine one port's wiimote + GameCube results. The wiimote contributes no
 * analog stick here (a sideways remote has none; the Classic stick is folded in
 * by port_pad_scan after this call), so the GC stick is used when the GC pad is
 * present. Returns 1 when a controller is present on the port. */
int port_pad_merge(int wii_present, unsigned wii_btn, int gc_present,
                   unsigned gc_btn, signed char gcx, signed char gcy,
                   PortPad *out)
{
    out->present = (wii_present || gc_present) ? 1 : 0;
    out->n64_buttons = (unsigned short)(wii_btn | gc_btn);
    if (gc_present) {
        out->stick_x = gcx;
        out->stick_y = gcy;
    } else {
        out->stick_x = 0;
        out->stick_y = 0;
    }
    return out->present;
}

#ifndef PAD_HOST_TEST
/* ---------- Wii-only: real libogc scanning + stick readout ---------- */
/* Classic Controller stick: raw unsigned position -> signed N64 stick value (+-80 at full deflection). */
static signed char port_stick_scale(int pos, int center, int min, int max)
{
    int d = pos - center;
    int span = d >= 0 ? (max - center) : (center - min);
    if (span <= 0) span = 32;
    int v = d * 80 / span;
    if (v > 80) v = 80;
    if (v < -80) v = -80;
    return (signed char)v;
}

void port_pad_scan(PortPad out[4])
{
    static int s_init = 0;
    if (!s_init) {
        WPAD_Init();
        PAD_Init();
        s_init = 1;
    }
    int i;
    for (i = 0; i < 4; i++) {
        out[i].present = 0;
        out[i].n64_buttons = 0;
        out[i].stick_x = 0;
        out[i].stick_y = 0;
    }

    /* PAD_ScanPads returns a BITMASK of connected GameCube pads (bit i = port
     * i), NOT a count. Wiimotes are probed per channel with WPAD_Probe. */
    WPAD_ScanPads();                 /* refresh Wiimote/Classic state (ButtonsHeld/Expansion read the last scan) */
    u32 pad_mask = PAD_ScanPads();

    for (i = 0; i < 4; i++) {
        u32 wtype = WPAD_EXP_NONE;
        int wii_present = (WPAD_Probe(i, &wtype) == WPAD_ERR_NONE);
        int gc_present = (pad_mask >> i) & 1;

        unsigned short wii_btn = 0;
        signed char wsx = 0, wsy = 0;
        if (wii_present) {
            if (wtype == WPAD_EXP_CLASSIC) {
                expansion_t exp;
                WPAD_Expansion(i, &exp);
                if (exp.type == WPAD_EXP_CLASSIC) {
                    wii_btn = port_map_classic(exp.classic.btns >> 16);
                    /* raw stick is unsigned around ljs.center; scale to the N64 range (about +-80) */
                    wsx = port_stick_scale(exp.classic.ljs.pos.x, exp.classic.ljs.center.x, exp.classic.ljs.min.x, exp.classic.ljs.max.x);
                    wsy = port_stick_scale(exp.classic.ljs.pos.y, exp.classic.ljs.center.y, exp.classic.ljs.min.y, exp.classic.ljs.max.y);
                } else {
                    wii_btn = port_map_wiimote_sideways(WPAD_ButtonsHeld(i));
                }
            } else {
                wii_btn = port_map_wiimote_sideways(WPAD_ButtonsHeld(i));
            }
        }

        unsigned short gc_btn = 0;
        signed char gsx = 0, gsy = 0;
        if (gc_present) {
            gc_btn = port_map_gc(PAD_ButtonsHeld(i));
            gsx = PAD_StickX(i);
            gsy = PAD_StickY(i);
        }

        port_pad_merge(wii_present, wii_btn, gc_present, gc_btn, gsx, gsy, &out[i]);

        /* If the wiimote (Classic) stick has the larger magnitude, it wins. */
        if (wii_present) {
            int wm = (int)wsx * (int)wsx + (int)wsy * (int)wsy;
            int gm = (int)out[i].stick_x * (int)out[i].stick_x +
                     (int)out[i].stick_y * (int)out[i].stick_y;
            if (wm > gm) {
                out[i].stick_x = wsx;
                out[i].stick_y = wsy;
            }
        }
    }

    /* port 1 always present so joyResponseCheck() returns >= 1 */
    out[0].present = 1;
}
#endif /* PAD_HOST_TEST */
