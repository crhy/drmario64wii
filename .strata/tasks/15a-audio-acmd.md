# Task 15a: N64 audio microcode (n_aspMain) in C - command interpreter with host tests
libmus (lib/libmus/src/aud_thread.c) builds an Acmd list with alAudioFrame and submits it as an M_AUDTASK; on N64 the
RSP "n_aspMain" microcode executes it. Today the Wii build skips this (-DPORT_NO_SYNTH). This task writes the
interpreter only (no Wii audio output yet - that is 15b).
1. New port/audio/naudio.c + naudio.h: `void naudio_run(const uint64_t *cmds, int n)` executing the n_audio ABI:
   the command encodings are the macros in lib/libmus/src/n_abi.h (n_aADPCMdec, n_aPoleFilter, n_aEnvMixer,
   n_aInterleave, n_aLoadBuffer, n_aResample, n_aSaveBuffer, n_aSetVolume, n_aLoadADPCM) plus the shared ones in
   lib/ultralib/include/PR/abi.h (A_CLEARBUFF, A_SETBUFF, A_DMEMMOVE, A_SETLOOP, A_SEGMENT, A_MIXER, A_SPNOOP).
   Decode each macro's bit layout EXACTLY from those headers. State: a 4 KB "DMEM" byte array (big-endian 16-bit
   samples, same as N64), the 16 segment bases, ADPCM codebook (from LoadADPCM), loop pointer, volume/rate/target
   state from SetVolume. RDRAM addresses are direct pointers on Wii (osVirtualToPhysical is identity) after segment
   resolution.
   Semantics to follow (N64 ABI): CLEARBUFF zeroes DMEM; LOAD/SAVEBUFFER copy between RDRAM and DMEM; ADPCM decodes
   VADPCM 9-byte frames (1 header byte: scale<<4|predictor index; 16 samples of 4 bits) with the order-2 codebook
   predictor and A_INIT/A_LOOP state handling (state is 16 samples saved at an RDRAM address); RESAMPLE is the 4-tap
   polyphase resampler with pitch in 1.15 fixed point and its state (4 samples + fractional position) in RDRAM;
   ENVMIXER applies the left/right/dry/wet volume ramps from SetVolume to dry L/R and wet L/R buffers with saturation;
   INTERLEAVE merges L and R into stereo; POLEF is the 2-pole filter; MIXER adds with a gain and saturates.
   Write each command's semantics as a comment above its function, citing where libmus uses it (grep n_synthesizer
   / n_env.c / n_resample.c / n_load.c / n_pole... in lib/libmus/src).
2. Host tests port/tests/test_naudio.c (plain gcc, no libogc): CLEARBUFF; LOAD->SAVE round trip; DMEMMOVE;
   INTERLEAVE order; RESAMPLE at pitch 1.0 (0x8000) reproduces input after its 4-tap delay; MIXER saturation;
   ADPCM: encode a known 16-sample frame by hand with predictor 0 and a zero codebook and check the decode; ENVMIXER
   with constant volume 0x7fff gives ~input. Print "naudio tests OK".
GATE: gcc -O1 -Iport/include -Iport/audio port/tests/test_naudio.c port/audio/naudio.c -o .strata/scratch/t15a && .strata/scratch/t15a | grep -q "naudio tests OK" && make -f Makefile.wii >/dev/null 2>&1
