/* Dr. Mario 64 -> Wii native port: text canary (task 15d).
 *
 * Detects when game code overwrites .text (e.g. a physical-address write
 * 0x00xxxxxx landing on 0x80xxxxxx in Dolphin's flat RAM).  Off by default;
 * enable with -DPORT_TEXT_CANARY. */

#include <stdint.h>
#include <string.h>
#include "port_log.h"

#ifdef PORT_TEXT_CANARY

extern char __text_start[];
extern char __etext[];

#define CANARY_PAGE_ADDR 0x80009000u
#define CANARY_PAGE_WORDS 1024u

static uint32_t g_page_copy[CANARY_PAGE_WORDS];
static int g_armed;
static int g_logged;

void port_canary_boot(void)
{
    const uint32_t *text = (const uint32_t *)(uintptr_t)__text_start;
    const uint32_t *end  = (const uint32_t *)(uintptr_t)__etext;
    uint32_t sum = 0;
    const uint32_t *p;
    for (p = text; p < end; p++)
        sum += *p;
    PORT_LOG("canary: boot sum=%08x range=%08x-%08x", sum,
             (uint32_t)__text_start, (uint32_t)__etext);
    memcpy(g_page_copy, (const void *)(uintptr_t)CANARY_PAGE_ADDR,
           CANARY_PAGE_WORDS * 4u);
    g_armed = 1;
}

/* kind: 0=gfx task N, 1=audio task N, 2=pi dma (a=dev p=dram s=size) */
void port_canary_check(int kind, unsigned n, uint32_t a, void *p, uint32_t s)
{
    if (!g_armed || g_logged)
        return;
    const uint32_t *page = (const uint32_t *)(uintptr_t)CANARY_PAGE_ADDR;
    unsigned i;
    for (i = 0u; i < CANARY_PAGE_WORDS; i++) {
        if (page[i] != g_page_copy[i]) {
            g_logged = 1;
            uint32_t off = CANARY_PAGE_ADDR + i * 4u;
            if (kind == 2) {
                PORT_LOG("canary: text changed after pi dma dev=%x dram=%p size=%x off=%08x "
                         "w0=%08x w1=%08x w2=%08x w3=%08x w4=%08x w5=%08x w6=%08x w7=%08x",
                         a, p, s, off,
                         page[i],
                         i + 1u < CANARY_PAGE_WORDS ? page[i + 1u] : 0u,
                         i + 2u < CANARY_PAGE_WORDS ? page[i + 2u] : 0u,
                         i + 3u < CANARY_PAGE_WORDS ? page[i + 3u] : 0u,
                         i + 4u < CANARY_PAGE_WORDS ? page[i + 4u] : 0u,
                         i + 5u < CANARY_PAGE_WORDS ? page[i + 5u] : 0u,
                         i + 6u < CANARY_PAGE_WORDS ? page[i + 6u] : 0u,
                         i + 7u < CANARY_PAGE_WORDS ? page[i + 7u] : 0u);
            } else if (kind == 1) {
                PORT_LOG("canary: text changed after audio task %u off=%08x "
                         "w0=%08x w1=%08x w2=%08x w3=%08x w4=%08x w5=%08x w6=%08x w7=%08x",
                         n, off,
                         page[i],
                         i + 1u < CANARY_PAGE_WORDS ? page[i + 1u] : 0u,
                         i + 2u < CANARY_PAGE_WORDS ? page[i + 2u] : 0u,
                         i + 3u < CANARY_PAGE_WORDS ? page[i + 3u] : 0u,
                         i + 4u < CANARY_PAGE_WORDS ? page[i + 4u] : 0u,
                         i + 5u < CANARY_PAGE_WORDS ? page[i + 5u] : 0u,
                         i + 6u < CANARY_PAGE_WORDS ? page[i + 6u] : 0u,
                         i + 7u < CANARY_PAGE_WORDS ? page[i + 7u] : 0u);
            } else {
                PORT_LOG("canary: text changed after gfx task %u off=%08x "
                         "w0=%08x w1=%08x w2=%08x w3=%08x w4=%08x w5=%08x w6=%08x w7=%08x",
                         n, off,
                         page[i],
                         i + 1u < CANARY_PAGE_WORDS ? page[i + 1u] : 0u,
                         i + 2u < CANARY_PAGE_WORDS ? page[i + 2u] : 0u,
                         i + 3u < CANARY_PAGE_WORDS ? page[i + 3u] : 0u,
                         i + 4u < CANARY_PAGE_WORDS ? page[i + 5u] : 0u,
                         i + 5u < CANARY_PAGE_WORDS ? page[i + 5u] : 0u,
                         i + 6u < CANARY_PAGE_WORDS ? page[i + 6u] : 0u,
                         i + 7u < CANARY_PAGE_WORDS ? page[i + 7u] : 0u);
            }
            return;
        }
    }
}

#else /* PORT_TEXT_CANARY not defined: no-op stubs */

void port_canary_boot(void) {}
void port_canary_check(int kind, unsigned n, uint32_t a, void *p, uint32_t s)
{
    (void)kind; (void)n; (void)a; (void)p; (void)s;
}

#endif
