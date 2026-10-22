# Dr. Mario 64 -> Wii native port: design

## Shape
- New tree `port/` (Wii-only code) + `Makefile.wii` at repo root. Output: `build/wii/drmario64.dol`, later `build/wii/drmario64.iso`.
- Game code (src/boot, src/main_segment, src/assets, lib/libmus) is compiled UNCHANGED where possible with
  powerpc-eabi-gcc, `-DTARGET_WII -DVERSION_US`. Both N64 and Wii are big-endian 32-bit, so structs, Gfx words
  and asset data stay as-is. Port fixes inside src/ go behind `#ifdef TARGET_WII`.
- lib/ultralib is NOT compiled for Wii. `port/ultra/` re-implements the libultra functions the game calls on top
  of libogc: threads (LWP), message queues (own impl with mutex+cond), timers/osGetTime, VI events (VIDEO retrace
  callback -> osSendMesg), PI DMA (memcpy from the ROM image), controllers (Wiimote/Classic/GC -> OSContPad),
  EEPROM (file on SD/NAND), osSpTaskStart (gfx task -> port/gfx renderer; audio task -> port/audio RSP ABI in C).
- ROM data: the game DMAs asset segments from ROM addresses. The Wii build embeds the matching uncompressed ROM
  (build/us/drmario64_uncompressed.us.z64, built from the user's baserom; never committed) and PI DMA copies from
  it. SEGMENT_ROM_START/END symbols come from a generated table (from the N64 build map).
- Asset segments that share VRAM (exclusive_ram) contain absolute N64 VRAM pointers. Option A: link each asset
  segment for Wii at its own address and replace "DMA+expand into buffer" with pointing at the linked copy.
  Decide after recon (task 01) and record the decision here.
- Graphics: `port/gfx/` interprets F3DEX2 + S2DEX display lists (segments, matrices, vertices, tris, tex loads,
  TLUTs, combiner, render modes, tex rects, fill rects, obj sprites/bg rect) and draws with GX. N64 320x240 maps
  to a 640x480 EFB (scale 2); 16:9 by widening the viewport for backgrounds later. Textures are uploaded as
  GX textures (converted from N64 formats, cached by address+CRC) so Dolphin's texture dump / hi-res packs work.
- Audio: libmus + n_audio produce Acmd lists for aspMain. `port/audio/` executes those commands in C and
  feeds 32 kHz stereo to ASND/AESND.
- Input: sideways Wiimote: D-pad=stick/D-pad (rotated), 1=B, 2=A, +=Start, -=Z/L, B=R. Classic and GC pad map 1:1.
- Debug: `printf` goes to Dolphin's log (OSReport); the port exposes `PORT_LOG(...)`.

## Decisions after recon (task 01)
- Asset segments are NOT compiled for Wii. They stay in the embedded ROM image and are loaded exactly like on N64
  (osPiStartDma = memcpy from the ROM image, gzip expand runs unchanged). main_segment is linked into the DOL, so
  the boot path's "DMA+decompress main_segment" is skipped under TARGET_WII (call the entry directly).
- Symbols that game code takes from asset segments (segmented addresses like 0x05xxxxxx) and the
  SEGMENT_ROM_START/END/VRAM symbols are given their N64 values through a generated linker script
  `port/gen/n64_syms.ld` made by `port/tools/gen_syms.py` from build/us/drmario64.us.elf (powerpc-eabi-nm can't
  read MIPS; use `mips-linux-gnu-nm`).
- osVirtualToPhysical / OS_K0_TO_PHYSICAL are identity on Wii (pointers stay 0x80/0x90...). The renderer resolves
  a display-list address as: if addr >= 0x80000000 it is a direct pointer; else segment = (addr>>24)&0xF,
  ptr = segtable[segment] + (addr & 0xFFFFFF) where segtable values are direct pointers.
- RSP microcode .s files (src/rsp) are not built; their Start/End symbols become small dummy arrays in port/ultra.
- Asm: calcsub.s and libkmc/mmuldi3.s get C replacements in port/; entry.s is not used. libkmc (KMC libc) is
  replaced by newlib where names clash; keep libkmc-only functions if the game needs them.

## UNCOMPRESSED_ROM decision (task 06)
- Define -DUNCOMPRESSED_ROM for the Wii build. src/boot/util.c:DecompressRomToRam then uses DmaData_RomToRam
  (raw DMA/memcpy) instead of expand_gzip. The embedded ROM image (build/us/drmario64_uncompressed.us.z64,
  6581872 bytes) stores segments raw, so the game must NOT gunzip on Wii. util.c gained a guarded
  #include "dmadata.h" under #ifdef UNCOMPRESSED_ROM so DmaData_RomToRam is declared.
- ROM embedded via powerpc-eabi-objcopy -I binary -O elf32-powerpc -B powerpc on build/wii/port_rom.bin,
  --redefine-sym to port_rom / port_rom_end / port_rom_size. osPiStartDma = memcpy from port_rom + devAddr.
