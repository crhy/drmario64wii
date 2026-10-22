/* Dr. Mario 64 -> Wii native port: game-heap overflow detector (task 29a).
 * Heap_bufferp is sized to the N64 layout (0x80205000-0x80124610 = 0xE09F0);
 * the array itself is 4 KB larger, and that extra 4 KB is a guard filled with
 * 0xA5. port_heap_note() runs after every BUFFER_* allocation: it keeps the
 * high-water offset, logs "heap: OVERFLOW by %u bytes in %s" once on the first
 * top past the heap end, and port_heap_hw() feeds the per-scene-change
 * "heap: hw=%u of %u" line. port_heap_guard_check() (log flusher, once per
 * second) scans the guard and logs "heap: guard broken at +%u" once. */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "heap_check.h"
#include "port_log.h"

extern unsigned char Heap_bufferp[];
extern unsigned char heap_guard[];

#define HEAP_WII_SIZE (0x80205000u - 0x80124610u)
#define HEAP_GUARD_SIZE 4096u

static bool overflowLogged = false;
static bool guardBrokenLogged = false;
static uintptr_t hwTop = 0;

void port_heap_guard_init(void)
{
    memset(heap_guard, 0xA5, HEAP_GUARD_SIZE);
}

void port_heap_note(const void *newTop, unsigned long size, const char *where)
{
    uintptr_t off;

    (void)size;
    off = (uintptr_t)newTop - (uintptr_t)Heap_bufferp;
    if (off > hwTop)
        hwTop = off;
    if (off > HEAP_WII_SIZE && !overflowLogged) {
        overflowLogged = true;
        PORT_LOG("heap: OVERFLOW by %u bytes in %s", (unsigned)(off - HEAP_WII_SIZE), where);
    }
}

unsigned port_heap_hw(void)
{
    return (unsigned)hwTop;
}

void port_heap_guard_check(void)
{
    const unsigned char *guard = heap_guard;
    unsigned i;

    if (guardBrokenLogged)
        return;
    for (i = 0; i < HEAP_GUARD_SIZE; i++) {
        if (guard[i] != 0xA5) {
            guardBrokenLogged = true;
            PORT_LOG("heap: guard broken at +%u", i);
            return;
        }
    }
}
