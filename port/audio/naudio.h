#ifndef NAUDIO_H
#define NAUDIO_H

/*
 * n_aspMain (n_audio) microcode command interpreter, ported to C for Wii.
 *
 * The N64 RSP microcode n_aspMain executes the Acmd command list built by
 * libmus (see lib/libmus/src/n_abi.h for the bit encodings).  On Wii we
 * decode and run the same list in software.
 *
 * Semantics are taken from the libultralib synth files that build the
 * equivalent old-ABI lists:
 *   lib/ultralib/src/audio/load.c      (LOADADPCM, LOADBUFF, CLEARBUFF, DMEMMOVE)
 *   lib/ultralib/src/audio/save.c      (SETBUFF, INTERLEAVE, SAVEBUFF)
 *   lib/ultralib/src/audio/resample.c  (RESAMPLE, DMEMMOVE, SETBUFF)
 *   lib/ultralib/src/audio/env.c       (SETVOL, ENVMIXER)
 *   lib/ultralib/src/audio/mainbus.c   (MIXER)
 *   lib/ultralib/src/audio/reverb.c    (POLEF)
 *   lib/ultralib/src/audio/synthesizer.c (SEGMENT)
 */

#include <stdint.h>

#define NAUDIO_DMEM_SIZE 4096
#define NAUDIO_SAMPLES   184   /* n_audio SAMPLES, see n_synthInternals.h */

/* opcodes and flags mirror lib/ultralib/include/PR/abi.h.  abi.h defines these
 * as object-like macros; undefine them first so the enum below is legal when
 * this header is included after <PR/abi.h>. */
#undef A_SPNOOP
#undef A_ADPCM
#undef A_CLEARBUFF
#undef A_ENVMIXER
#undef A_LOADBUFF
#undef A_RESAMPLE
#undef A_SAVEBUFF
#undef A_SEGMENT
#undef A_SETBUFF
#undef A_SETVOL
#undef A_DMEMMOVE
#undef A_LOADADPCM
#undef A_MIXER
#undef A_INTERLEAVE
#undef A_POLEF
#undef A_SETLOOP
enum {
    A_SPNOOP=0, A_ADPCM=1, A_CLEARBUFF=2, A_ENVMIXER=3, A_LOADBUFF=4,
    A_RESAMPLE=5, A_SAVEBUFF=6, A_SEGMENT=7, A_SETBUFF=8, A_SETVOL=9,
    A_DMEMMOVE=10, A_LOADADPCM=11, A_MIXER=12, A_INTERLEAVE=13,
    A_POLEF=14, A_SETLOOP=15
};
#define A_INIT     0x01
#define A_CONTINUE 0x00
#define A_LOOP     0x02

/* Run n command words (each uint64_t = w0<<32 | w1) from the Acmd list. */
void naudio_run(const uint64_t *cmds, int n);

/* Host hook: RDRAM addresses in the command stream are 32-bit offsets added to
   this base (0 on Wii, where addresses are direct pointers). */
void naudio_set_dram_base(uint64_t base);

#endif
