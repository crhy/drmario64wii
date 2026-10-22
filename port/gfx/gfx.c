/* Dr. Mario 64 -> Wii native port: display-list walker (task 08a, no drawing).
 *
 * Walks F3DEX2 and S2DEX display lists without drawing:
 *  - G_DL (0xde): w0 = op | (n<<16); n==G_DL_PUSH(0) call (push return),
 *    G_DL_NOPUSH(1) branch.  18-deep return stack.
 *  - G_ENDDL (0xdf): pop the return stack; empty stack ends the list.
 *  - G_MOVEWORD (0xdb): w0 = op | (index<<16) | offset, w1 = data.
 *    index==G_MW_SEGMENT (0x06): offset = segment*4, data = base pointer;
 *    other move-word indices are ignored.
 *  - every other opcode: dispatch through a 256-entry handler table
 *    (NULL slot = default counter; later tasks fill the table).
 * S2DEX tasks (ucode == gspS2DEX_fifoTextStart) walk the same way; their
 * opcodes (G_OBJ_*, G_BG_*, G_SELECT_DL, G_RDPHALF_0...) hit the same table.
 * G_TEXRECT (0xe4) is followed by G_RDPHALF_1/2 as separate 8-byte commands,
 * so the plain 8-byte step handles them (real handling comes later).
 *
 * Address resolution (DESIGN.md): addr >= 0x80000000 is a direct pointer;
 * else seg = (addr>>24)&0xF, ptr = segtable[seg] + (addr & 0xFFFFFF).
 * Stops after 100000 commands per task ("gfx: runaway list").
 * Every 300 tasks logs "gfx stats tasks=%d cmds=%d" + 8 most frequent opcodes.
 */

#include <PR/ultratypes.h>
#include <PR/sptask.h>
#include <PR/gbi.h>
#include <stdint.h>
#include <stdio.h>
#define GFX_NEED_SPTASK
#include "gfx.h"
#include "port_log.h"
#ifdef TARGET_WII
#include "port_img_hint.h"
#endif

#define GFX_MAX_CMDS 100000u
#define GFX_DL_STACK 18
#define GFX_TOPN     8

/* GX drawing half (task 08c): gfx_gx.c fills the same table after gfx_xform. */
void gfx_gx_install(void (*table[256])(u32 w0, u32 w1));
void gfx_frame_start(void);
void gfx_tex_install(void (*table[256])(u32 w0, u32 w1));
void gfx_tmem_install(void (*table[256])(u32 w0, u32 w1));
void gfx_cc_install(void (*table[256])(u32 w0, u32 w1));

static u32 segtable[16];   /* segment bases (direct pointers) */
static u32 stat_tasks;     /* gfx tasks walked so far */
static u32 stat_cmds;      /* commands walked, all tasks */
static u32 max_cmds;       /* max commands in a single task */
static u32 opcount[256];   /* per-opcode counts (dispatched opcodes) */

extern int main_no;   /* enum_main_no in main.c; enums are int-compatible */

/* Per-opcode handler table; NULL slots use the default counter. */
static void (*optable[256])(u32 w0, u32 w1);
static int optable_ready;

static void gfx_op_default(u32 op, u32 w0, u32 w1) {
    (void)w0;
    (void)w1;
    static unsigned char seen5[256];   /* once per opcode for the whole run (opcount resets per task) */
    if (main_no == 5 && !seen5[op & 0xFF]) {
        seen5[op & 0xFF] = 1;
        PORT_LOG("gfx unhandled op=0x%02x in main_no=5", (int)op);
    }
    opcount[op]++;
}

extern char __text_start[];
extern char __etext[];
static int gfx_text_rejected;

void *gfx_resolve(u32 addr) {
    u32 p;
    if (addr >= 0x80000000u) {
        p = addr;
    } else {
        u32 seg = (addr >> 24) & 0xF;
        if (seg == 0)
            p = 0x80000000u | (addr & 0xFFFFFFu);
        else if (segtable[seg] == 0)
            return NULL;
        else
            p = segtable[seg] + (addr & 0xFFFFFFu);
    }
    if (p >= (u32)(uintptr_t)__text_start && p < (u32)(uintptr_t)__etext) {
        if (!gfx_text_rejected) {
            gfx_text_rejected = 1;
            PORT_LOG("gfx: resolved addr %08x lands in .text, rejected", addr);
        }
        return NULL;
    }
    return (void *)(uintptr_t)p;
}

static void gfx_stats_log(void) {
    char line[512];
    u32 top_op[GFX_TOPN];
    u32 top_cnt[GFX_TOPN];
    int k = 0;
    int op, i, j, len;

    len = snprintf(line, sizeof line, "gfx stats tasks=%d cmds=%d",
                   (int)stat_tasks, (int)stat_cmds);
    for (op = 0; op < 256; op++) {
        if (opcount[op] == 0)
            continue;
        i = 0;
        while (i < k && top_cnt[i] >= opcount[op])
            i++;
        if (i >= GFX_TOPN)
            continue;
        j = (k < GFX_TOPN ? k : GFX_TOPN - 1);
        while (j > i) {
            top_op[j] = top_op[j - 1];
            top_cnt[j] = top_cnt[j - 1];
            j--;
        }
        top_op[i] = (u32)op;
        top_cnt[i] = opcount[op];
        if (k < GFX_TOPN)
            k++;
    }
    for (i = 0; i < k && len < (int)sizeof line - 24; i++)
        len += snprintf(line + len, sizeof line - len, " op=%d=%d",
                        (int)top_op[i], (int)top_cnt[i]);
    PORT_LOG("%s", line);
}

#ifdef TARGET_WII
static u32 pimg_hints;
static int pimg_first_done;

static void gfx_hint_noop(u32 w0, u32 w1) {
    PortImgHint *h = (PortImgHint *)gfx_resolve(w1);
    if (h != NULL && h->magic == PORT_IMG_MAGIC) {
        pimg_hints++;
        gfx_st.img_hint = h;
        if (!pimg_first_done) {
            pimg_first_done = 1;
            PORT_LOG("pimg: first w=%d h=%d fmt=%d siz=%d", (int)h->w, (int)h->h, (int)h->fmt, (int)h->siz);
        }
    }
}
#endif

void gfx_run_task(OSTask *t) {
    const u32 *pc;
    const u32 *retstack[GFX_DL_STACK];
    int sp = 0;
    u32 cmds = 0;

    stat_tasks++;
    { extern void gfx_tex_task_begin(void); gfx_tex_task_begin(); }
    if (!optable_ready) {
        gfx_xform_install(optable);
        gfx_tmem_install(optable);
        gfx_gx_install(optable);
        gfx_tex_install(optable);
        gfx_cc_install(optable);
#ifdef TARGET_WII
        optable[G_NOOP & 0xFF] = gfx_hint_noop;
#endif
        optable_ready = 1;
    }
    {
        const u32 *dp = (const u32 *)gfx_resolve((u32)(uintptr_t)t->t.data_ptr);
        if (!dp) {
            PORT_LOG("gfx: bad data_ptr %x", (unsigned)(uintptr_t)t->t.data_ptr);
            return;
        }
        pc = dp;
    }
#ifdef TARGET_WII
    if (main_no == 5 && stat_tasks <= 400)
        PORT_LOG("gfx t%d ptr=%x sz=%x", (int)stat_tasks, (unsigned)(uintptr_t)t->t.data_ptr,
                 (unsigned)t->t.data_size);
#endif
    for (;;) {
        u32 w0, w1, op;

        if (++cmds > GFX_MAX_CMDS) {
            PORT_LOG("gfx: runaway list");
            break;
        }
        if (((uintptr_t)pc & 3u) != 0) {
            PORT_LOG("gfx dbg: misaligned pc=%x after %d cmds", (unsigned)(uintptr_t)pc, (int)cmds);
            break;
        }
        w0 = *pc;
        w1 = pc[1];
        op = w0 >> 24;
        if (stat_tasks <= 3 && cmds <= 60)
            PORT_LOG("gfx dbg: t%d cmd %d op=%02x w0=%08x w1=%08x pc=%x",
                     (int)stat_tasks, (int)cmds, (int)op, w0, w1, (unsigned)(uintptr_t)pc);
        pc += 2;
        if (stat_tasks <= 3 && cmds <= 65)
            PORT_LOG("gfx dbg: t%d after %d nextpc=%x", (int)stat_tasks, (int)cmds, (unsigned)(uintptr_t)pc);
        if (op == G_DL) {
            const u32 *dst = (const u32 *)gfx_resolve(w1);
            if (dst == NULL) {
                if (stat_tasks <= 5 || main_no == 5)
                    PORT_LOG("gfx dbg: DL NULL w1=%08x cmd=%d t=%d", w1, (int)cmds, (int)stat_tasks);
                break;
            }
            if (((w0 >> 16) & 0xFF) == G_DL_PUSH) {
                if (sp >= GFX_DL_STACK) {
                    PORT_LOG("gfx: dl stack overflow");
                    break;
                }
                retstack[sp++] = pc;
            }
            pc = dst;
            continue;
        }
        if (op == G_ENDDL) {
            if (sp == 0)
                break;
            pc = retstack[--sp];
            continue;
        }
        if (op == G_MOVEWORD) {
            if (((w0 >> 16) & 0xFF) == G_MW_SEGMENT)
                segtable[(w0 >> 2) & 0xF] = w1;
            continue;
        }
        if (optable[op] != NULL)
            optable[op](w0, w1);
        else
            gfx_op_default(op, w0, w1);
    }
#ifdef TARGET_WII
    gfx_rtt_flush_if_active();
    gfx_st.img_hint = NULL;
#endif
    stat_cmds += cmds;
#ifdef TARGET_WII
    if (cmds > max_cmds) max_cmds = cmds;
    if ((stat_tasks % 600) == 0 || (stat_cmds / 600) != ((stat_cmds - cmds) / 600))
        PORT_LOG("pimg: hints=%u maxc=%u", pimg_hints, max_cmds);
#endif
    if (stat_tasks <= 5)
        PORT_LOG("gfx dbg: t%d done cmds=%d sp=%d", (int)stat_tasks, (int)cmds, sp);
    if ((stat_tasks % 300) == 0)
        gfx_stats_log();
}
