/* Dr. Mario 64 -> Wii native port: plain-C controller interface (task 12a).
 *
 * pad_wii.c owns ALL libogc WPAD/PAD usage and the Wiimote/Classic/GC -> N64
 * button mapping. os_cont.c talks to it only through this plain-C surface, so
 * libogc's u32/s32 typedefs and its expansion_t ABI never leak into the
 * libultra layer. */
#ifndef PAD_WII_H
#define PAD_WII_H

typedef struct {
    int present;               /* controller connected on this port */
    unsigned short n64_buttons;/* N64 button bits (PR/os_cont.h layout) */
    signed char stick_x, stick_y;
} PortPad;

/* Merge one port's wiimote + GameCube results into out; returns 1 if a
 * controller is present on the port, 0 otherwise. wii_btn / gc_btn are already
 * N64 button bits. When both are present the buttons are ORed and the stick
 * with the larger magnitude wins. Pure (no libogc), so it is compiled in the
 * Wii build and the host test alike. */
int port_pad_merge(int wii_present, unsigned wii_btn, int gc_present,
                   unsigned gc_btn, signed char gcx, signed char gcy,
                   PortPad *out);

void port_pad_scan(PortPad out[4]);

#endif /* PAD_WII_H */
