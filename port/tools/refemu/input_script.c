/* mupen64plus input plugin that replays the Wii port's input scripts, so the original ROM can be driven with exactly
 * the same presses as the port (reference frames for pixel comparison).
 *
 * Script (M64_INPUT_SCRIPT=<file>): "<frame> <hex N64 buttons> <stick_x> <stick_y> [port 1..4]", held until the next
 * line for that port. Frames are counted by the core's per-frame (VI) callback, the same unit as --testshots.
 * All four controllers are reported present (like DR_PADS=4 in the Wii harness).
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_common.h"
#include "m64p_frontend.h"
#include "m64p_plugin.h"
#include "m64p_types.h"
#include "m64p_debugger.h"

#define MAX_LINES 4096
#define MAX_SEGS 32

typedef struct { unsigned type; unsigned frame; unsigned buttons; int sx, sy; int port; unsigned target; unsigned seg; } Item;

static Item g_items[MAX_LINES];
static int g_n;
static unsigned g_seg_base[MAX_SEGS];
static unsigned char g_seg_done[MAX_SEGS];
static unsigned g_seg_target[MAX_SEGS];
static unsigned g_nsegs = 1;
static unsigned g_wait_seg = 1;
static volatile unsigned g_frame;
static unsigned g_main_no_addr = 0x800EBCF0u;
static CONTROL *g_controls;
static int g_started;

static ptr_CoreDoCommand g_cmd;
static ptr_DebugMemGetPointer g_memptr;
static ptr_DebugMemRead32 g_rd32;

/* Write the framebuffer the VI is showing (exact game pixels, no VI filter/overscan) as a binary PPM.
 * RDRAM is kept as host-order 32-bit words; a 16-bit pixel at byte address a is the high half of word a>>2 when
 * (a & 2) == 0. RGBA5551 -> 8-bit by bit replication. 32-bit framebuffers are RGBA8888 words. */
static void dump_fb(unsigned frame) {
    const char *dir = getenv("M64_FB_DIR");
    const uint32_t *ram;
    unsigned status, origin, width, height = 240, x, y, type;
    char path[512];
    FILE *f;
    if (!dir || !g_memptr || !g_rd32) return;
    ram = (const uint32_t *)g_memptr(M64P_DBG_PTR_RDRAM);
    status = g_rd32(0xA4400000u); origin = g_rd32(0xA4400004u) & 0xFFFFFFu; width = g_rd32(0xA4400008u) & 0xFFFu;
    type = status & 3u;
    if (!ram || width == 0 || width > 640 || type < 2) return;
    snprintf(path, sizeof path, "%s/ref_%u.ppm", dir, frame);
    if (!(f = fopen(path, "wb"))) return;
    fprintf(f, "P6\n%u %u\n255\n", width, height);
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++) {
            unsigned char rgb[3];
            if (type == 2) {
                unsigned a = origin + (y * width + x) * 2u;
                uint32_t w = ram[(a & 0x7FFFFFu) >> 2];
                unsigned p = (a & 2u) ? (w & 0xFFFFu) : (w >> 16);
                unsigned r = (p >> 11) & 31u, g = (p >> 6) & 31u, b = (p >> 1) & 31u;
                rgb[0] = (unsigned char)((r << 3) | (r >> 2)); rgb[1] = (unsigned char)((g << 3) | (g >> 2));
                rgb[2] = (unsigned char)((b << 3) | (b >> 2));
            } else {
                uint32_t w = ram[((origin + (y * width + x) * 4u) & 0x7FFFFFu) >> 2];
                rgb[0] = (unsigned char)(w >> 24); rgb[1] = (unsigned char)(w >> 16); rgb[2] = (unsigned char)(w >> 8);
            }
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
}
static unsigned g_shots[4096];
static int g_nshots, g_next_shot;

/* Per-frame (VI) callback: frame counter for the script, plus screenshots at M64_SHOTS="f1,f2,..." and a stop after
 * the last one (the front end's --testshots would replace this callback, so it is not used). */
static void process_waits(unsigned frame) {
    if (!g_rd32) return;
    if (g_wait_seg < g_nsegs) {
        unsigned mn = g_rd32(g_main_no_addr);
        if (mn == g_seg_target[g_wait_seg]) {
            g_seg_base[g_wait_seg] = frame;
            g_seg_done[g_wait_seg] = 1;
            fprintf(stderr, "script: W %d done f=%u\n", (int)g_seg_target[g_wait_seg], frame);
            g_wait_seg++;
        }
    }
}

static void frame_cb(unsigned int idx) {
    g_frame = idx;
    process_waits(idx);
    if (g_next_shot < g_nshots && idx >= g_shots[g_next_shot]) {
        if (getenv("M64_FB_DIR")) dump_fb(idx); else g_cmd(M64CMD_TAKE_NEXT_SCREENSHOT, 0, NULL);
        g_next_shot++;
    } else if (g_nshots && g_next_shot >= g_nshots && idx >= g_shots[g_nshots - 1] + 3) {
        g_cmd(M64CMD_STOP, 0, NULL);
    }
}

static void load_shots(void) {
    const char *s = getenv("M64_SHOTS");
    while (s && *s && g_nshots < 4096) {
        g_shots[g_nshots++] = (unsigned)strtoul(s, (char **)&s, 10);
        while (*s == ',' || *s == ' ') s++;
    }
}

static void load_script(void) {
    const char *path = getenv("M64_INPUT_SCRIPT");
    const char *a = getenv("M64_MAIN_NO_ADDR");
    FILE *f;
    char buf[256];
    if (a && *a) g_main_no_addr = (unsigned)strtoul(a, (char **)&a, 0);
    g_seg_done[0] = 1; g_seg_base[0] = 0; g_seg_target[0] = 0;
    if (!path || !(f = fopen(path, "r"))) return;
    unsigned cur_seg = 0;
    while (g_n < MAX_LINES && fgets(buf, sizeof buf, f)) {
        char *p = buf;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == 'W' || *p == 'w') {
            unsigned tgt = (unsigned)strtoul(p + 1, (char **)&p, 10);
            if (g_nsegs < MAX_SEGS) {
                unsigned s = g_nsegs++;
                g_seg_target[s] = tgt; g_seg_done[s] = 0; g_seg_base[s] = 0;
                g_items[g_n].type = 1; g_items[g_n].target = tgt; g_items[g_n].seg = s;
                g_items[g_n].frame = 0; g_items[g_n].buttons = 0;
                g_items[g_n].sx = g_items[g_n].sy = 0; g_items[g_n].port = 0;
                g_n++;
                cur_seg = s;
            }
            continue;
        }
        Item it;
        it.type = 0; it.frame = 0; it.buttons = 0; it.sx = 0; it.sy = 0; it.port = 1; it.target = 0; it.seg = cur_seg;
        if (sscanf(buf, "%u %x %d %d %d", &it.frame, &it.buttons, &it.sx, &it.sy, &it.port) >= 2) {
            if (it.port < 1 || it.port > 4) it.port = 1;
            g_items[g_n++] = it;
        }
    }
    fclose(f);
    fprintf(stderr, "input-script: %d items from %s\n", g_n, path);
}

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle core, void *ctx, void (*debug)(void *, int, const char *)) {
    (void)ctx; (void)debug;
    g_cmd = (ptr_CoreDoCommand)dlsym(core, "CoreDoCommand");
    g_memptr = (ptr_DebugMemGetPointer)dlsym(core, "DebugMemGetPointer");
    g_rd32 = (ptr_DebugMemRead32)dlsym(core, "DebugMemRead32");
    load_script();
    load_shots();
    if (g_cmd) g_cmd(M64CMD_SET_FRAME_CALLBACK, 0, (void *)frame_cb);
    g_started = 1;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void) { g_started = 0; return M64ERR_SUCCESS; }

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *type, int *version, int *api, const char **name, int *caps) {
    if (type) *type = M64PLUGIN_INPUT;
    if (version) *version = 0x010000;
    if (api) *api = 0x020100;
    if (name) *name = "Input script replay (drmario64 Wii port tests)";
    if (caps) *caps = 0;
    return M64ERR_SUCCESS;
}

EXPORT void CALL InitiateControllers(CONTROL_INFO info) {
    int i;
    g_controls = info.Controls;
    for (i = 0; i < 4; i++) {
        g_controls[i].Present = 1;
        g_controls[i].RawData = 0;
        g_controls[i].Plugin = PLUGIN_NONE;
    }
}

EXPORT void CALL GetKeys(int control, BUTTONS *keys) {
    int i;
    const Item *cur = NULL;
    unsigned b;
    keys->Value = 0;
    for (i = 0; i < g_n; i++) {
        if (g_items[i].type != 0) continue;
        if (g_items[i].port != control + 1) continue;
        unsigned seg = g_items[i].seg;
        if (!g_seg_done[seg]) continue;
        if (seg + 1u != g_wait_seg) continue;   /* only the latest completed segment plays (same rule as the port) */
        if (g_seg_base[seg] + g_items[i].frame <= g_frame) cur = &g_items[i];
    }
    if (!cur) return;
    b = cur->buttons & 0xFFFFu;
    /* N64 controller word (A=0x8000 ... C-right=0x0001) -> BUTTONS bitfield (byte-swapped order) */
    keys->Value = ((b >> 8) & 0xFFu) | ((b & 0xFFu) << 8);
    keys->X_AXIS = (signed char)cur->sx;
    keys->Y_AXIS = (signed char)cur->sy;
}

EXPORT void CALL ControllerCommand(int control, unsigned char *command) { (void)control; (void)command; }
EXPORT void CALL ReadController(int control, unsigned char *command) { (void)control; (void)command; }
EXPORT int CALL RomOpen(void) { return 1; }
EXPORT void CALL RomClosed(void) {}
EXPORT void CALL SDL_KeyDown(int keymod, int keysym) { (void)keymod; (void)keysym; }
EXPORT void CALL SDL_KeyUp(int keymod, int keysym) { (void)keymod; (void)keysym; }
