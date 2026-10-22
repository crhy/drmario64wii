# Task 29a: 4-player Vs crash - check for a game-heap overflow (write code FIRST, then run)
Facts from task 29 (Strata, 3 sessions): in 4P Vs (.strata/scratch/modes/vs4p.txt) the port crashes at game frame ~286
with a fully ZEROED thread context (SP=0, LR=0, all registers 0); port/wii/crash.c does not fire; adding logging moves
or hides the crash (memory-layout dependent). FIFO size is not the cause. Hypothesis (Claude): 4P allocates the most
game memory; the game heap (Heap_bufferp / BUFFER_MALLOC, sized to the N64 layout in task 14b1) overflows and writes
over whatever follows it, e.g. libogc thread control blocks or LWP stacks.
Do this (all under #ifdef TARGET_WII, N64 build must still match):
1. Find the game's heap allocator(s) (grep BUFFER_MALLOC, Heap_bufferp, heapTop in src/ and include/). Add a high-water
   check: after each allocation compare the new top with the heap end; on the first overflow log ONCE
   "heap: OVERFLOW by %u bytes in %s" (function/file), and every scene change log "heap: hw=%u of %u".
2. Put a 4 KB guard of 0xA5 bytes right after the heap end (enlarge the heap array by 4 KB for it) and check it from the
   log flusher once per second; log "heap: guard broken at +%u" once if any byte changed.
3. If an overflow is found: enlarge the heap enough for 4P (+25% margin) and say so in NOTES.md. If none is found,
   write in NOTES.md that the heap is ruled out and where the guard check stood.
GATE: make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && grep -rq "heap: hw=" src port include && out=$(.strata/cmp_mode.sh vs4p 120); echo "$out"; grep -aq "heap: hw=" .strata/scratch/cmp/vs4p/port2/sd/drmario/log.txt && n=$(echo "$out" | awk '{print $6}') && [ "${n:-0}" -ge 150 ]
