# Task 29b: catch what zeroes a thread context (release blocker; 29a only HID the crash by shifting memory)
Facts: 4P Vs crashed at game frame ~286 with a fully zeroed thread context (SP=0, LR=0, all regs 0); crash.c did not
fire; any code/layout change moves or hides it (29a's guard array made it disappear). 29a: heap guard never touched,
so the game heap is probably not the culprit. Something writes zeros over a thread's saved context or stack.
Make it detectable regardless of layout (all #ifdef TARGET_WII, keep the code; it is cheap):
1. port/ultra/os_sched.c: for every game thread slot, place a 64-byte 0x5A canary at the LOW end of its stack
   (g_thread_stacks, stacks grow down) and keep a copy of each LWP id + entry. Check all canaries and the g_sched[] slot
   fields (t, entry, lwp, stack) every VI from the event pump; on the FIRST mismatch log once
   "guard: thread slot %d %s broken (frame %u, last running id %d)" and dump 8 words around the damage.
2. Same 64-byte canary at the low end of the RSP, pump, timer and log-flusher LWP stacks (port/ultra/os_rsp.c,
   os_events.c, port/wii/log.c).
3. Also canary the libogc-side: log the address range of each LWP's stack and of g_sched at boot ("guard: ranges ...").
4. Run 4P Vs (.strata/cmp_mode.sh vs4p) and vs2p several times; if a guard fires, find and fix the writer, then note the
   cause in NOTES.md. If no guard fires in 3 runs of each, note that in NOTES.md (the guards stay in as a tripwire).
GATE: make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && grep -rq "guard: thread slot" port && out=$(.strata/cmp_mode.sh vs4p 120); echo "$out"; grep -aq "guard: ranges" .strata/scratch/cmp/vs4p/port2/sd/drmario/log.txt && ! grep -aq "guard: thread slot .* broken" .strata/scratch/cmp/vs4p/port2/sd/drmario/log.txt && n=$(echo "$out" | awk '{print $6}') && [ "${n:-0}" -ge 150 ]
