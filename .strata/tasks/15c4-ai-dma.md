# Task 15c4: replace AESND with the Wii audio-interface DMA (port/ultra/os_stub.c osAi*, port/wii/main.c)
Ground truth (Claude, 2026-10-21): in many builds the AESND voice never plays (its callback never runs), so
osAiGetLength() keeps growing, libmus computes a NEGATIVE frame size (aud_samples.c: frame_samples - samples_left),
alAudioFrame returns 0 commands forever, music never "stops" and the title hangs at main_no=3. Which builds fail depends
on code layout, so AESND/Dolphin's AESnd HLE is not reliable here. The N64 AI is a plain DMA FIFO of big-endian 16-bit
stereo - exactly what the Wii AI DMA plays with NO DSP microcode. Use it instead.
libogc API (ogc/audio.h, already verified): AUDIO_Init(NULL); AUDIO_SetDSPSampleRate(AI_SAMPLERATE_32KHZ or _48KHZ);
AUDIO_RegisterDMACallback(void (*cb)(void)); AUDIO_InitDMA(u32 addr, u32 len) (addr 32-byte aligned, len multiple of 32,
pass (u32)ptr); AUDIO_StartDMA(); AUDIO_StopDMA(); AUDIO_GetDMABytesLeft().
The AI DMA interrupt fires when the hardware has latched the current block and is ready for the next address: in the
callback call AUDIO_InitDMA(next block) (NOT StartDMA again). If the hardware gets no new address it replays the old one.
Do this:
1. main.c: replace AESND_Init() with AUDIO_Init(NULL). Makefile.wii: drop -laesnd. Remove every AESND reference.
2. os_stub.c: ring of 4 buffers x 8192 bytes, 32-byte aligned, plus a 32-byte-aligned 2048-byte silence buffer.
   osAiSetNextBuffer(buf,size): if 4 queued return -1; memcpy into the tail slot, round len up to a multiple of 32
   (pad with zeros), DCFlushRange, enqueue; if DMA not running: AUDIO_InitDMA(slot,len) + AUDIO_StartDMA(), mark it
   "playing". Ring indices are shared with the IRQ callback: guard them with port_irq_disable()/port_irq_restore()
   (already used by ai_lock/ai_unlock) - never a mutex, never PORT_LOG inside the callback.
3. DMA callback: the block that was playing is finished -> drop it; if a queued block exists AUDIO_InitDMA(it) and make
   it "playing", else AUDIO_InitDMA(silence) and count an underrun.
4. osAiGetLength = AUDIO_GetDMABytesLeft() of the playing block + bytes of blocks queued behind it (this is what libmus
   expects from the N64 AI: never more than ~2 frames). osAiGetStatus = AI_STATUS_FIFO_FULL when 4 are queued.
   osAiSetFrequency(f): 32000 -> AI_SAMPLERATE_32KHZ, 48000 -> _48KHZ, return f; anything else -> 32 kHz, return 32000.
5. Log "audio: frames=%u underruns=%u" on frame 1 and every 600 frames (outside the callback).
6. Remove the PORT_DIAG_NO_AIOUT block. Diagnostics under PORT_SCHED_DUMP2 (scheduler dump in log.c/os_sched.c,
   nnsched client walk, aud_thread zero-cmds log, os_rsp drvr.head check) may stay; they are off by default.
GATE: make -f Makefile.wii clean >/dev/null; make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t15c4 >/dev/null; L=.strata/scratch/t15c4/sd/drmario/log.txt; ! grep -aq CRASH $L && ! grep -q "ISI exception\|DSI exception" .strata/scratch/t15c4/dolphin.log && grep -a "audio: frames=" $L | grep -aq "frames=[1-9][0-9][0-9]" && grep -aq "main_no=5" $L && ! grep -rq "AESND\|PORT_DIAG_NO_AIOUT" port/ultra/os_stub.c port/wii/main.c Makefile.wii && .strata/imgmax.py .strata/scratch/t15c4 --min-colors 64
