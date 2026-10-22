# Task 06: link the whole game into the DOL
Goal: `make -f Makefile.wii` links GAME_SRCS objects + port/ into build/wii/drmario64.dol with ZERO undefined symbols.
port/wii/main.c keeps its own main() for now and must not call game code yet (task 07 does that).
- lib/libmus/src/player_api.c, player_commands.c and player_fifo.c are #included by player.c on N64: remove them
  from GAME_SRCS and undo the TARGET_WII preambles task 04 added to them (and drop port/include/libmus_wii_externs.h
  if nothing else uses it). Do the same for any other .c that is #included by another .c (grep '#include ".*\.c"').
- Every undefined symbol: if it is a libultra function, implement it in port/ultra/ (group by area: os_pi.c, os_cache.c,
  os_vi.c, os_cont.c, os_eeprom.c, os_sp.c, os_misc.c). Real behaviour now for: osPiStartDma/osEPiStartDma/osPiRawStartDma
  (memcpy from the embedded ROM `port_rom` at the given ROM offset to dst, then send the completion message to the mq),
  osInvalDCache/osWritebackDCache/osInvalICache (DCFlushRange/DCInvalidateRange or no-op), osVirtualToPhysical (identity,
  see DESIGN.md), osInitialize, osGetMemSize (return 8 MB), osCartRomInit/osDriveRomInit (return a static OSPiHandle).
  Everything else (VI mode setters, controller, EEPROM, osSpTaskStart, osDpSetStatus, AI functions) gets a STUB that
  PORT_LOGs "STUB <name>" once and returns success; later tasks replace them.
- Embedded ROM: build/us/drmario64_uncompressed.us.z64 is converted by a Makefile.wii rule into an object
  (`powerpc-eabi-ld -r -b binary` or bin2s from $DEVKITPRO/tools/bin) exposing `const u8 port_rom[]` and `port_rom_size`;
  osPiStartDma reads from it. NOTE: check whether the compressed ROM is the right one (the game gzip-expands segments
  read from ROM; the uncompressed ROM may store them raw, then the game must not gunzip on Wii). Decide by checking
  how src/boot/util.c / UNCOMPRESSED_ROM behaves and record the decision in DESIGN.md.
- RSP microcode symbols (gspF3DEX2_fifoTextStart, aspMainTextStart, rspbootTextStart, ...): dummy byte arrays in port/ultra/ucode_syms.c.
- calcsub.s and src/libkmc/mmuldi3.s functions: C replacements in port/ (calcsub: read the .s and the GameCube note
  in NOTES.md; write equivalent C). libkmc functions that newlib provides: use newlib's.
- Symbol clashes between game code and libogc/newlib (e.g. sprintf, memcpy, sqrtf, bzero, guMtx*): prefer renaming the
  game's copy with `#define name game_name` in a port header under TARGET_WII, never edit libogc.
- Asset symbols come from port/gen/n64_syms.ld; if a symbol is both defined by a Wii object and listed there, drop it
  from the .ld (fix port/tools/gen_syms.py, not the generated file).
List in NOTES.md "Wii link" every stub you added.
GATE: make -f Makefile.wii 2>&1 | tee .strata/scratch/link.txt | tail -5; ! grep -qi "undefined reference" .strata/scratch/link.txt && test -s build/wii/drmario64.dol && make -j8 2>&1 | tail -1 | grep -q "us.z64: OK"
