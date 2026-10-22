# Task 15c3: make the AESND audio output actually drain (port/ultra/os_stub.c, osAi* functions)
State (Claude, 2026-10-21): synthesis + naudio now work. The audio thread hands the first buffer to osAiSetNextBuffer,
then the output never drains: osAiSetNextBuffer keeps returning -1 (ring full), the libmus audio thread stops, music never
reports stopped, and the title's fade-out loop (dm_title_main.c, waits for dm_audio_is_stopped) hangs at main_no=3.
Proof: building with -DPORT_DIAG_NO_AIOUT (discards every buffer, returns 0) lets the game reach the menu (main_no=6).
Cause: the voice is set up with AESND_SetVoiceStream(pb, 0) and ai_voice_cb only refills on "not RUNNING". In libogc
(aesndlib.h) the refill idiom is STREAM mode: AESND_SetVoiceStream(pb, true); the callback is called with
VOICE_STATE_STREAM when the voice needs its next buffer -> call AESND_SetVoiceBuffer(pb, next, len) right there.
VOICE_STATE_RUNNING fires every DSP frame (ignore), VOICE_STATE_STOPPED when stopped.
Do this:
1. osAiSetFrequency: allocate the voice once, VOICE_STEREO16 (big-endian, same as N64), AESND_SetVoiceStream(pb, true),
   frequency = the requested rate. Start playing (AESND_SetVoiceBuffer + AESND_SetVoiceStop(pb, false)) on the FIRST
   queued buffer only.
2. ai_voice_cb (runs in the DSP interrupt; ai_lock/ai_unlock already disable IRQs - never use a mutex or PORT_LOG here):
   on VOICE_STATE_STREAM pop the oldest queued buffer and AESND_SetVoiceBuffer it; if the ring is empty, feed a static
   32-byte-aligned silence buffer (e.g. 1024 bytes of 0) and count an underrun.
3. Ring: 4 buffers of 8192 bytes (32-byte aligned, DCFlushRange after memcpy). Keep osAiGetLength = bytes queued not yet
   handed to AESND, osAiGetStatus = AI_STATUS_FIFO_FULL when all 4 are queued.
4. Log "audio: frames=%u underruns=%u" on frame 1 and every 600 frames (NOT only under PORT_LOG_VERBOSE).
5. Delete the PORT_DIAG_NO_AIOUT block when done.
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t15c3 >/dev/null; L=.strata/scratch/t15c3/sd/drmario/log.txt; ! grep -aq CRASH $L && ! grep -q "ISI exception\|DSI exception" .strata/scratch/t15c3/dolphin.log && grep -a "audio: frames=" $L | grep -aq "frames=[1-9][0-9][0-9]" && grep -aq "main_no=5" $L && ! grep -q PORT_DIAG_NO_AIOUT port/ultra/os_stub.c && .strata/imgmax.py .strata/scratch/t15c3 --min-colors 64
