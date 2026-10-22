# Task 03: N64 symbol table for the Wii link
Write `port/tools/gen_syms.py` (python3, no extra deps). It runs `mips-linux-gnu-nm build/us/drmario64.us.elf`
and writes `port/gen/n64_syms.ld` containing `NAME = 0xVALUE;` lines for:
 (a) every symbol whose name matches `^(segment_|_?[A-Za-z0-9_]*Segment)[A-Za-z0-9_]*(RomStart|RomEnd|VramStart|VramEnd|_ROM_START|_ROM_END|_VRAM|_VRAM_END|_BSS_START|_BSS_END)` or similar
     segment boundary symbols (look at include/segment_symbols.h to learn the exact naming macros produce),
 (b) every symbol whose value is below 0x10000000 (segmented asset addresses like 0x05001234), excluding
     absolute/size-only symbols that are not real addresses (keep only nm types D d B b R r T t A).
Sort by name, no duplicates. Add a Makefile.wii rule so `port/gen/n64_syms.ld` is regenerated when the N64 ELF
changes, and pass it to the link (`-Wl,port/gen/n64_syms.ld` or `-T` as an extra script; it must not replace the
default devkitPPC script). Print the count of symbols written.
GATE: python3 port/tools/gen_syms.py && [ $(wc -l < port/gen/n64_syms.ld) -gt 100 ] && grep -q "ROM_START\|RomStart" port/gen/n64_syms.ld && make -f Makefile.wii
