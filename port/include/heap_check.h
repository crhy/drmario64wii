#ifndef HEAP_CHECK_H
#define HEAP_CHECK_H

/* Game-heap high-water + guard checks (task 29a): the game heap is Heap_bufferp
 * sized to the N64 layout; 4P Vs may allocate past its end and scribble over
 * whatever follows it in BSS. These hooks track the high-water mark, log the
 * first overflow, and verify a 4 KB 0xA5 guard placed after the heap end. */
void port_heap_note(const void *newTop, unsigned long size, const char *where);
void port_heap_guard_init(void);
void port_heap_guard_check(void);
unsigned port_heap_hw(void);

#endif
