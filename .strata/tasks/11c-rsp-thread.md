# Task 11c: run graphics tasks and presentation on an "RSP" thread, outside the game scheduler
ROOT CAUSE (found by Claude from the 11b logs): the main game thread (id 3, reads the controller in joyProcCore) is READY
but starves. The pump log shows "cur=17 ready=80028": the NN scheduler's graphics thread (id 17) holds the single game
CPU almost all the time, because osSpTaskStartGo (port/ultra/os_sp.c) runs gfx_run_task (the whole display list) on the
CPU, and osViSwapBuffer -> port_present (port/wii/main.c) blocks in GX_DrawDone, all while holding the scheduler token.
On a real N64 the RSP/RDP do this work in parallel and the CPU keeps running game threads.
Implement (new file port/ultra/os_rsp.c, add to Makefile.wii):
1. An "RSP" LWP at libogc priority 80 (game threads are 64, timer 90, pump 100) with a 64 KB static stack and a job
   ring of 16 entries {type, OSTask*, void *fb} guarded by an LWP mutex, plus a counting semaphore the RSP LWP waits on.
   Job types: GFX task, AUDIO task, PRESENT(fb).
2. osSpTaskStartGo (os_sp.c), called from a game thread: enqueue the job and RETURN IMMEDIATELY (do not run it).
   The RSP LWP runs gfx_run_task(t) for GFX jobs, then signals completion OUTSIDE the scheduler: add
   void port_os_event_post(int event) in os_sched.c that calls port_post_event(g_event_mq[event], &g_event_msg[event])
   (the event-pump path), and call it for OS_EVENT_SP then OS_EVENT_DP. AUDIO jobs: just post OS_EVENT_SP.
3. osViSwapBuffer (os_stub.c) enqueues PRESENT(frameptr) and returns; the RSP LWP calls port_present(fb), so the
   RSP LWP is the ONLY thread that writes the GX FIFO (call GX_SetCurrentGXThread() once at the top of the RSP LWP and
   remove it from gfx_run_task and port_present). Order is preserved because it is one FIFO ring.
4. The RSP LWP must never call scheduler functions that park (no osRecvMesg, no blocking osSendMesg); osSendMesg from a
   non-game LWP already routes to port_post_event. PORT_LOG is fine.
5. If the ring is full, the enqueuing game thread may wait on a "slot free" LWP semaphore (that is like the N64 CPU
   waiting on the RSP); log the first time it happens.
6. Log once per 600 presents: "rsp: presents=%u gfx=%u fps=%u.%u" using the time base, so we can see the frame rate.
7. port_post_event (os_events.c) now has THREE producers (VI IRQ, timer LWP, RSP LWP) and its _CPU_ISR_Disable is a no-op
   macro. Do item 1 of .strata/tasks/12c-events-timers.md here: port/wii/irqlock.c with port_irq_disable/restore
   wrapping libogc IRQ_Disable/IRQ_Restore, used around the ring push.
The scripted START test from task 11b must now pass (os_cont.c input script + PORT_INPUT_SCRIPT embedding already work:
the log shows "input press frame=300 buttons=1000" but the game never read it because thread 3 never ran).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t11in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t11c >/dev/null; L=.strata/scratch/t11c/sd/drmario/log.txt; grep -a "input script loaded" $L && grep -aq "input script loaded [1-9]" $L && grep -aq "input press" $L && grep -a "main_no=" $L | grep -av "main_no=[03]$" | grep -a "main_no=" && make -f Makefile.wii clean >/dev/null && make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t11c_reg >/dev/null && .strata/imgmax.py .strata/scratch/t11c_reg --min-colors 128
