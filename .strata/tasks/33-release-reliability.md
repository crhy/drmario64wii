# Task 33 (release blocker): frozen picture from dropped OS events
Facts (Claude, 2026-10-22, release check): the 1P Classic script build (.strata/scratch/modes/classic.txt) froze the
picture at frame 450 (main menu, mode 1) while game logic and input continued (presses logged at frame 644). The SD log
shows "ev/sec: post=1560 ok=1434 drop=125": port_deliver_event (port/ultra/os_sched.c) DROPS events when the target
OSMesgQueue is full. On the N64 the receiver runs at once so queues never fill; in the port they do. Dropping an SP/DP
(gfx task done), timer or retrace event can leave a thread waiting forever -> frozen picture / dead thread.
Do:
1. Log once per second which queues the drops go to (queue address + event type + count) to see the culprits.
2. Never lose an event: when the target queue is full, keep the event in a per-queue pending list (or leave it in the
   event ring) and deliver it as soon as the receiver takes a message (osRecvMesg path) - the same order, no drops.
   Keep VI/retrace semantics the same as libultra otherwise.
3. The ev/sec log must then show drop=0.
GATE: export DEVKITPRO=$HOME/devkitpro DEVKITPPC=$HOME/devkitpro/devkitPPC; make 2>&1 | grep -q ": OK" && for m in classic vs2p vs4p; do make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/modes/$m.txt >/dev/null 2>&1 || exit 1; DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 180 .strata/scratch/t33/$m >/dev/null; L=.strata/scratch/t33/$m/sd/drmario/log.txt; n=$(ls .strata/scratch/t33/$m/framedump_*.png | wc -l); echo "$m frames=$n"; [ $n -ge 340 ] || exit 1; grep -aq CRASH $L && exit 1; grep -aq "guard: .*broken" $L && exit 1; grep -a "ev/sec" $L | tail -3 | grep -aq "drop=[1-9]" && exit 1; done; echo ALL OK
