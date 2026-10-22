#include "buffers.h"
#include "unk.h"

#ifdef TARGET_WII
/* On N64 this buffer's size comes from the linker layout: it runs from 0x80124610 to the next segment
 * (buffer2 / gfx_freebuf at 0x80205000), 0xE09F0 bytes, and the game uses all of it as its heap.
 * With UNK_SIZE (1) the heap overran gfx_freebuf, the framebuffers and everything after them. */
u8 Heap_bufferp[0x80205000 - 0x80124610] BSS;
/* 4 KB guard (task 29a): sits between the heap and gfx_freebuf (buffer2.c, the
 * next object in link order). Filled with 0xA5 by port_heap_guard_init; if the
 * heap overflows it scribbles here first, and port_heap_guard_check reports it.
 * A separate array (not an enlarged Heap_bufferp) so gfx_freebuf keeps its
 * original offset and gfxInit(gfx_freebuf) is not corrupted at boot. */
u8 heap_guard[4096] BSS;
#else
u8 Heap_bufferp[UNK_SIZE] BSS;
#endif
