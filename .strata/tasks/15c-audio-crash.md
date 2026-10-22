# Task 15c: fix the RSP-thread crash in the audio interpreter (port/audio/naudio.c)
Ground truth found by Claude (2026-10-21), do NOT re-investigate the scheduler - it is fine:
- With synthesis on, the RSP helper LWP crashes on the FIRST audio task: SD log has
  `CRASH ex=4 pc=00000000 lr=00000000 ... r1=809bb398` and r1 is inside g_rsp_stack (port/ultra/os_rsp.c). pc=lr=0 means
  a saved return address on that stack was overwritten with 0 -> naudio wrote outside its buffers. After that no audio or
  gfx task ever completes, which looked like a "freeze". (Old "whole Wii freezes" reports were Dolphin's panic dialog
  blocking the headless emulator; the harness now disables it and logs CPU exceptions to dolphin.log.)
- Prime suspect, port/audio/naudio.c:37 `raddr(a) = dram_base + seg[0] + a`. On Wii the game's Acmd addresses are
  DIRECT pointers (osVirtualToPhysical is identity, see DESIGN.md), e.g. 0x80xxxxxx / 0x90xxxxxx. Resolve exactly like
  the gfx renderer: if dram_base != 0 (host tests) keep the old arena behaviour; else if a >= 0x80000000 use a as a
  pointer; else ptr = seg[(a>>24)&0xF] + (a & 0xFFFFFF) (segment table values are direct pointers).
1. Fix raddr as above. Keep port/tests/test_naudio.c passing (it uses naudio_set_dram_base with an arena).
2. Bounds: every DMEM access must stay inside dmem[]. c_dmemmove/c_loadbuff/c_savebuff/c_clearbuff/c_resample/c_adpcm/
   c_envmixer/c_mix/c_interleave: clamp counts so (addr & mask) + count <= NAUDIO_DMEM_SIZE (memmove in c_dmemmove
   currently can run past the end). Any RDRAM pointer that is not inside 0x80000000-0x817FFFFF or 0x90000000-0x93FFFFFF
   -> skip the command and PORT_LOG("naudio: bad addr op=%02x a=%08x") at most 8 times total.
3. Unknown opcode -> log once per opcode value (at most 16 lines), skip.
4. In os_rsp.c, before naudio_run, log once "naudio: first task n=%d ptr=%p" and after it the first time "naudio: first task done".
GATE: make -f Makefile.wii clean >/dev/null; gcc -O1 -Iport/include -Iport/audio port/tests/test_naudio.c port/audio/naudio.c -o .strata/scratch/t15a && .strata/scratch/t15a | grep -q "naudio tests OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t15c >/dev/null; L=.strata/scratch/t15c/sd/drmario/log.txt; grep -aq "naudio: first task done" $L && ! grep -aq CRASH $L && ! grep -q "ISI exception\|DSI exception" .strata/scratch/t15c/dolphin.log && grep -aq "audio: frames=" $L && grep -aq "main_no=5" $L && .strata/imgmax.py .strata/scratch/t15c --min-colors 64
