/* Dr. Mario 64 -> Wii native port: thread-context guard (task 29b).
 *
 * Catches what zeroes a thread's saved context or stack. A 64-byte 0x5A
 * canary sits at the LOW end of every LWP stack (stacks grow down, so the
 * low end is the deepest the stack can reach); a wild zero-write that runs
 * off the bottom of a stack, or that scribbles the stack arena, trips it.
 * The game thread slots (g_thread_stacks + g_sched slot fields) are checked
 * in os_sched.c; the RSP / pump / timer / log-flusher stacks are checked
 * here. All code is #ifdef TARGET_WII and cheap (one pass per VI). */

#include <stdint.h>
#include <string.h>
#include "port_log.h"

#ifdef TARGET_WII

#define GUARD_CANARY 0x5Au
#define GUARD_BYTES  64u
#define GUARD_MAX    40u

/* ---------- range registry: logged once at boot ("guard: ranges ...") ---------- */
typedef struct {
    const char *name;
    const unsigned char *low;
    unsigned size;
} GuardRange;

static GuardRange g_guard_ranges[GUARD_MAX];
static unsigned   g_guard_range_n;

/* ---------- canary registry: checked every VI ---------- */
typedef struct {
    const char *name;
    unsigned char *low;
    unsigned char copy[GUARD_BYTES];
} GuardCanary;

static GuardCanary g_guard_canaries[GUARD_MAX];
static unsigned    g_guard_canary_n;
static unsigned    g_guard_broke;

void port_guard_range(const char *name, const void *low, unsigned size)
{
    if (g_guard_range_n >= GUARD_MAX)
        return;
    GuardRange *r = &g_guard_ranges[g_guard_range_n++];
    r->name = name;
    r->low = (const unsigned char *)low;
    r->size = size;
}

void port_guard_canary(const char *name, void *stack_low)
{
    if (g_guard_canary_n >= GUARD_MAX)
        return;
    GuardCanary *c = &g_guard_canaries[g_guard_canary_n++];
    c->name = name;
    c->low = (unsigned char *)stack_low;
    memset(c->low, GUARD_CANARY, GUARD_BYTES);
    memcpy(c->copy, c->low, GUARD_BYTES);
}

void port_guard_boot_ranges(void)
{
    unsigned i;
    for (i = 0u; i < g_guard_range_n; i++) {
        const GuardRange *r = &g_guard_ranges[i];
        PORT_LOG("guard: ranges %s %08x-%08x",
                 r->name,
                 (uint32_t)(uintptr_t)r->low,
                 (uint32_t)(uintptr_t)r->low + r->size);
    }
}

/* Check the LWP-stack canaries (game slots are checked in os_sched.c).
 * On the FIRST mismatch log once and dump 8 words around the damage. */
void port_guard_check(unsigned frame)
{
    if (g_guard_broke)
        return;
    unsigned i;
    for (i = 0u; i < g_guard_canary_n; i++) {
        GuardCanary *c = &g_guard_canaries[i];
        unsigned j;
        for (j = 0u; j < GUARD_BYTES; j++) {
            if (c->low[j] != c->copy[j]) {
                g_guard_broke = 1;
                const uint32_t *w = (const uint32_t *)(uintptr_t)c->low;
                PORT_LOG("guard: %s canary broken (frame %u) at +%u exp=%02x got=%02x "
                         "w0=%08x w1=%08x w2=%08x w3=%08x w4=%08x w5=%08x w6=%08x w7=%08x",
                         c->name, frame, j,
                         c->copy[j], c->low[j],
                         w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]);
                return;
            }
        }
    }
}

#else /* not TARGET_WII: no-op stubs */

void port_guard_range(const char *name, const void *low, unsigned size)
{
    (void)name; (void)low; (void)size;
}
void port_guard_canary(const char *name, void *stack_low)
{
    (void)name; (void)stack_low;
}
void port_guard_boot_ranges(void) {}
void port_guard_check(unsigned frame) { (void)frame; }

#endif
