# Dr. Mario 64 - native Wii port

This is a native port of the Dr. Mario 64 decompilation to the Nintendo Wii. It is
**not an emulator**. The game's C sources are recompiled for PowerPC with
`powerpc-eabi-gcc` (see `Makefile.wii`), and the parts of the N64 runtime the game
depends on (libultra, the RSP microcode, the VI, the controllers, the EEPROM) are
re-implemented on top of libogc in the `port/` tree. Graphics are drawn with the
Wii's native GX pipeline, not by interpreting an N64 GPU at runtime.

See `.strata/DESIGN.md` for the full design. In short:

- Game code (`src/boot`, `src/main_segment`, `src/assets`, `lib/libmus`) is compiled
  unchanged where possible; both N64 and Wii are big-endian 32-bit, so structs, Gfx
  words and asset data keep their layout. Port fixes inside `src/` go behind
  `#ifdef TARGET_WII`.
- `lib/ultralib` is **not** compiled for Wii. `port/ultra/` re-implements the libultra
  functions the game calls on top of libogc: threads (LWP), message queues, timers,
  VI events, PI DMA, controllers, EEPROM, and `osSpTaskStart`.
- The matching uncompressed ROM is embedded in the DOL (`port_rom` symbols in
  `Makefile.wii`); PI DMA is a `memcpy` from it. The ROM image is built from the user's
  own baserom and is never committed.
- `port/gfx/` interprets the F3DEX2 + S2DEX display lists the game produces and draws
  them with GX. The N64 320x240 framebuffer maps to a 640x480 EFB (scale 2).
- `port/audio/naudio.c` executes the Acmd lists `libmus` + `n_audio` produce and feeds
  32 kHz stereo out through the Wii's AI DMA.

## Status

Release 0.1.0 (2026-10-22). Verified in Dolphin:

- Boot, title, attract demo, menus, 1-player Classic, 2-player Vs and 4-player Vs games
  played to the end (180 s runs each): no crashes, no frozen picture, music and sound on,
  60 game frames per second.
- 480p when the console is set to progressive + component cable; 16:9 consoles get correct
  4:3 proportions.
- Saves (records/settings) on the SD card.

Known issues (cosmetic, being worked on):

- Story mode: the gameplay stage background is white (pills, viruses and characters are fine).
- Flash mode: the background shows stripes and some panel frames are missing.
- Main menu: the mascot / mode-preview panel on the right shows a pink square and a black
  box; the cursor glove has a black square behind it; empty name plates show garbled glyphs
  instead of "NEW".
- Some HD textures are not replaced yet (the pack was made before the last renderer fixes).
- Not yet tested on real Wii hardware.

## How the docs are organised

- `README.md` - this file: what the port is, status, legal notice.
- `BUILDING.md` - toolchains, make targets, packaging (HBC / ISO), input-script builds.
- `N64-ON-WII.md` - how each N64 subsystem is re-implemented on the Wii, one section each,
  with notes on where behaviour differs from the N64 and why.
- `TESTING.md` - the headless Dolphin harness, input scripts, the reference renderer,
  pixel comparison, host unit tests, and the HD texture pipeline.
- `LESSONS.md` - the important bugs found during the port and their causes.

## Legal

You need your **own** copy of the Dr. Mario 64 ROM. The build is driven by a baserom you
provide; the embedded ROM image (`build/us/drmario64_uncompressed.us.z64`) and the
generated ISO are built from it and are **never committed** to this repo. There is no ROM,
ISO, or game data in this repository. The release ISO and `boot.dol` contain the game data
and are for personal use only - do not distribute them.
