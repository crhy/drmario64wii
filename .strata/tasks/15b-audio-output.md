# Task 15b: play the game's audio on the Wii (AESND), remove PORT_NO_SYNTH
Uses port/audio/naudio.c from 15a.
1. M_AUDTASK jobs (port/ultra/os_rsp.c / os_sp.c) run naudio_run on the task's command list (t->t.data_ptr,
   data_size/8 commands) on the RSP LWP, then post OS_EVENT_SP as today.
2. Audio interface: osAiSetFrequency(f) returns the real rate it will use (32000 or 48000; keep the value libmus asked
   for if AESND supports it, else pick the nearest and resample is NOT needed if you open AESND at that rate),
   osAiSetNextBuffer(buf, size) copies the interleaved big-endian 16-bit stereo buffer into a ring of 3 output buffers
   (32-byte aligned, DCFlushRange) played by one AESND voice (AESND_Init in main.c, AESND_AllocateVoice with a
   callback that queues the next buffer, or the stream API) ; osAiGetLength returns the bytes still queued,
   osAiGetStatus reports busy when all buffers are full. Keep timing driven by the game (it calls once per audio frame).
3. Remove -DPORT_NO_SYNTH from Makefile.wii and the PORT_NO_SYNTH workaround in dm_audio_is_stopped (sound.c) so music
   really stops; the START test from task 13 must still pass.
4. Log once per 600 audio frames "audio: frames=%u underruns=%u" (non-verbose).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t11in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t15b >/dev/null; grep -aq "audio: frames=" .strata/scratch/t15b/sd/drmario/log.txt && grep -a "main_no=" .strata/scratch/t15b/sd/drmario/log.txt | grep -aqv "main_no=[03]$" && ! grep -q PORT_NO_SYNTH Makefile.wii
