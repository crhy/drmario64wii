# Task 15c2: run audio tasks synchronously on Wii (fix the stall after the first audio task)
State after 15c: naudio no longer crashes. But exactly ONE audio task runs, then gfx stops at ~17 tasks and the game never
leaves the title (main_no=3).
Cause (Claude, from src/main_segment/nnsched.c nnScExecuteAudio): the audio thread and the gfx thread both receive on
sc->rspMQ. On N64 the audio thread yields the running gfx task, takes its SP message, runs audio, then restarts the gfx
task (yieldFlag 1) or re-sends a message to rspMQ (yieldFlag 2). On Wii osSpTaskYield is a no-op and the RSP LWP runs
gfx tasks to completion, so osSpTaskYielded==0 -> yieldFlag=2 every audio frame -> an EXTRA message goes to rspMQ, the
SP messages of gfx and audio land on the wrong threads, and the queue fills (osSendMesg BLOCK parks forever).
Fix (do exactly this, do not touch the scheduler in port/ultra/os_sched.c):
1. Add `void port_audio_task_run(OSTask *t)` in port/ultra/os_rsp.c: naudio_set_dram_base(0); naudio_run(t->t.data_ptr,
   t->t.data_size/8) - synchronous, on the CALLING thread (naudio is plain C, it never touches GX). Keep the
   "naudio: first task n=... / first task done" logs here. Declare it in port/include/port_audio.h (new, tiny).
2. nnsched.c nnScExecuteAudio, inside `#ifdef TARGET_WII ... #else <original code> #endif`: skip the whole yield block
   (no osSpTaskYield, no osRecvMesg on rspMQ, yieldFlag stays 0); replace `osSpTaskStart(&task->list);
   osRecvMesg(&sc->rspMQ, ...)` with `port_audio_task_run(&task->list.t ? ...)` i.e. pass the OSTask (task->list is an
   OSTask). Keep sc->curAudioTask set/cleared, the graphicsTaskSuspended/waitMQ send, and the final
   osSendMesg(task->msgQ, task->msg). The N64 build must still match (`make` prints ": OK").
3. os_sp.c / os_rsp.c: M_AUDTASK must no longer be enqueued to the RSP LWP (if anything still calls osSpTaskStart with
   an audio task, run it synchronously too and post OS_EVENT_SP as before; log once "audio task via osSpTaskStart").
4. Remove periodic debug logs added during 15c that fire every N frames (keep the once-only ones) unless under
   PORT_LOG_VERBOSE.
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t15c2 >/dev/null; L=.strata/scratch/t15c2/sd/drmario/log.txt; grep -aq "naudio: first task done" $L && ! grep -aq CRASH $L && ! grep -q "ISI exception\|DSI exception" .strata/scratch/t15c2/dolphin.log && grep -aq "audio: frames=" $L && grep -aq "main_no=5" $L && .strata/imgmax.py .strata/scratch/t15c2 --min-colors 64
