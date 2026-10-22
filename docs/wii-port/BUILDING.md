# Building the Wii port

All commands use `make -f Makefile.wii` at the repo root. Do not touch the N64 Makefile.

## Prerequisites

- **devkitPro / devkitPPC** — the toolchain (`powerpc-eabi-gcc`, `powerpc-eabi-objcopy`) and libogc headers/libs. `Makefile.wii` reads `DEVKITPRO` and `DEVKITPPC` from the environment; if unset it falls back to `/home/rhy/devkitpro` and `$DEVKITPRO/devkitPPC` (lines 5-10).
- **A completed N64 build** — the Wii build consumes two artifacts from the N64 build:
  - `build/us/drmario64.us.elf` — used to regenerate the symbol linker script `port/gen/n64_syms.ld` via `port/tools/gen_syms.py` (Makefile.wii lines 135-137). The script is passed as an extra positional `-Wl` script, so it only adds symbol definitions (segment boundaries, asset addresses) and never replaces devkitPPC's default link script.
  - `build/us/drmario64_uncompressed.us.z64` — embedded uncompressed into the ELF as `port_rom` / `port_rom_end` / `port_rom_size` via `objcopy -I binary` (lines 144-149). PI DMA copies from this blob at runtime.
- **`wit`** (Wii ISO toolkit) — only needed for the `iso` target; path configurable via `WIT ?= .strata/bin/wit`.

## Targets

| Target | Output |
|--------|--------|
| `all` (default) | `build/wii/drmario64.dol` |
| `objs` | game objects only, no link |
| `hbc` | Homebrew Channel package: `build/wii/apps/drmario64/{boot.dol,icon.png,meta.xml}` |
| `iso` | `build/wii/drmario64.iso` (fake-signed, ID `RDMP01`) |
| `clean` | removes `build/wii` |

The link (line 129-130) combines `WII_OBJS` (port/ code), `GAME_OBJS` (game sources recompiled for PowerPC), `ROM_OBJ`, optional `INPUT_SCRIPT_OBJ`, and the two linker scripts `port/gen/n64_syms.ld` + `port/gen/wii_text.ld`.

## HBC package

`make -f Makefile.wii hbc` copies the DOL to `boot.dol`, generates `icon.png` with `port/tools/make_icon.py`, and writes `meta.xml` with the version string (`VERSION := 0.1.0`). Drop the `apps/drmario64/` folder on the SD root for the Homebrew Channel.

## ISO

A bootable Wii disc needs system files from a real disc. Extract them once from any Wii disc image you own:

```
make -f Makefile.wii iso DISC_TEMPLATE=/path/to/any_wii_disc.wbfs
```

The first run extracts `boot.bin`, `bi2.bin`, `apploader.img`, ticket/tmd/cert into `build/disc_tmpl/` (survives `clean`, never committed); none of that disc's game files are used. `port/tools/dol_for_disc.py` zeroes the DOL's BSS field so the retail apploader accepts it. The result is a fake-signed ISO with ID `RDMP01`, region USA.

## Input-script builds

`PORT_INPUT_SCRIPT=path` on the make command line embeds that file's bytes as `port_input_script` / `port_input_script_end` (the weak externs `port/ultra/os_cont.c` reads). Without it the weak externs stay undefined and `os_cont.c` falls back to `sd:/drmario/input.txt` at runtime. Note: the gate passes `-DPORT_INPUT_SCRIPT` only as a compiler macro, not a make variable, so that block is inert there.

## Self-test build

`WII_SELFTEST=1` adds `port/ultra/test_threads.c` and `-DWII_SELFTEST=1` (lines 111-114).
