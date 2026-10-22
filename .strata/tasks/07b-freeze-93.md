# Task 07b: find and fix the freeze at frame ~93 (bisection task)
Claude fixed several scheduler bugs (see the end of tasks/07-boot-game.md and NOTES.md): idle hand-off
(sched_current_slot_locked returned an empty slot when idle), port_preempt_check acting for helper LWPs, osSendMesg from
non-game threads (now routed through port_post_event), and port_present writing the 640x480 Wii XFB into the game's
320x240 N64 framebuffer (now uses the port's own XFBs). Result: the game boots, the title code runs, ~93 frames are
presented and gfx tasks arrive, then EVERYTHING stops at the same point every run:
- last log lines: PI DMA "pi in d=1d5820 v=80a4bd20 s=2e00" (ROM segment_189D40 into audio_memory), "pi sent mq=...".
- after that no PORT_LOG from any thread, no presents, and the debug sentinel (port/wii/sentinel.c, fed by the VI IRQ
  callback, writes sd:/drmario/sentinel.txt through its own FILE every 30 retraces) also stops at n=90. So either all
  interrupts stop (CPU stuck with MSR[EE]=0 or a crash) or every SD write blocks (libfat / IOS IPC stuck).
- gdb via .strata/dolphin-gdb.sh no longer works (Dolphin's stub drops the connection right after `continue`); do not
  spend time on it. libogc's default crash screen shows nothing (frames are black).
Do this:
1. Decide "IRQs dead" vs "SD dead": add a second sentinel output that needs no SD: in the VI callback, every 60
   retraces toggle the VI border/background by writing a different colour into BOTH XFBs' first 16 lines (YUYV words,
   e.g. 0x10801080 black / 0xEB80EB80 white) and DCFlushRange them; Dolphin keeps scanning out the XFB, and since the
   picture changes, frame dumps continue if IRQs are alive. Check with .strata/imgstat.py on the late frame PNGs.
2. If IRQs are alive: find what blocks the SD (e.g. a thread parked while inside libfat; memory corruption of libfat/IPC
   structures by a game DMA/memset: check every osPiStartDma/bzero/memset destination range against the DOL's own
   symbols with powerpc-eabi-nm -n -S build/wii/drmario64.elf). Fix the cause.
3. If IRQs are dead: bisect with PORT_LOG markers in the code that runs after the segment_189D40 load (find which game
   function requests it: grep segment_189D40 / the ROM table entry, then follow the music/sound code in
   src/main_segment/audio and lib/libmus) until you find the last marker reached; look there for a loop that polls
   hardware (AI/SP/DP/VI status via osAiGetStatus/osAiGetLength/__osSpGetStatus/osDpGetStatus stubs, IO_READ of N64
   registers at 0xA4xxxxxx) or for MSR changes. Fix the stub to return what libultra would.
Remove the temporary markers when done (keep the sentinel files but stop calling them unless built with PORT_DEBUG).
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t07b > .strata/scratch/t07b.out; grep -a "^present" .strata/scratch/t07b/sd/drmario/log.txt | wc -l | awk '{exit !($1 >= 1000)}' || [ $(grep -ac "gfx task" .strata/scratch/t07b/sd/drmario/log.txt) -ge 18 ]
