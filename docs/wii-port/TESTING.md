# Testing the Wii port

## Headless runs

`.strata/dolphin-run.sh <dol|iso> <seconds> <outdir>` runs the DOL/ISO in headless Dolphin (flatpak) inside Xvfb with a throwaway user dir. It saves every 30th frame PNG plus `last.png` to `<outdir>`, the Dolphin log to `<outdir>/dolphin.log`, and extracts the Wii SD card image to `<outdir>/sd/` (game log: `<outdir>/sd/drmario/log.txt`). Panic dialogs are off; CPU exceptions and bad memory accesses land in `dolphin.log` under `W[PowerPC]`/`MEMMAP`.

Options (environment):

| Var | Effect |
|-----|--------|
| `DR_PADS=N` | plug N emulated GameCube controllers into ports 1..N (default 1) |
| `DR_43=1` | boot with the Wii set to 4:3 (no widescreen squeeze) — required for pixel comparison with the N64 reference |
| `DR_IRES=N` | render at Nx internal resolution (6 = 4K), dumps at that resolution |
| `DR_DUMPTEX=1` | dump textures to `<outdir>/textures/` |
| `DR_HIRES=1` | load HD packs from `<user>/Load/Textures` (`DR_HIRES_DIR` copied there) |

## Input scripts

Scripts drive the game through menus deterministically. Format (`port/ultra/os_cont.c` lines 49-60): lines `"<frame> <hex buttons> <stick_x> <stick_y>"` held until the next line; a line `W <n>` waits until `main_no == n` (`enum_main_no`, `include/main.h`), and the frame the wait completes becomes the base for the following lines (frame numbers after a W are relative to that wait's completion; lines before the first W keep absolute frames). Each completed wait logs `script: W %d done f=%u`.

The script is either a build-time embedded blob — `make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=path/to/script.txt` (the Makefile embeds the bytes as `port_input_script`/`port_input_script_end`) — or, with no blob, `sd:/drmario/input.txt` read at runtime. Either way the log shows `input script loaded N lines`.

Mode scripts live in `.strata/scratch/modes/<mode>.txt` (classic, vs2p, vs4p, story, flash).

## Reference renderer

`port/tools/refemu/run-ref.sh <input-script|/dev/null> <comma-separated frame list> <outdir> [timeout]` renders the ORIGINAL N64 ROM with mupen64plus + the angrylion cycle-accurate RDP, driven by the same port input script (the `input_script.c` plugin is rebuilt when newer than the `.so`). Framebuffer dumps `ref_<frame>.ppm` are converted to PNG. The reference frames are cached in `.strata/scratch/cmp/<mode>/ref` (from task 28).

## Frame comparison

`port/tools/refcompare.py` compares Wii port frame dumps with reference N64 frames: both are normalised to 320x240 with an area filter, port frames are matched against ref frames within ±`--window`, and it writes `report.txt` plus `cmp_N.png` heat maps. `.strata/cmp_mode.sh <mode> [seconds]` rebuilds with the mode's `PORT_INPUT_SCRIPT`, runs with `DR_43=1 DR_PADS=4`, and calls `refcompare.py` with `--window 60 --worst 6` plus `--port-log`/`--ref-log`; it prints `median <x> worst <y> frames <n>`.

## HD texture packs

`port/tools/hd_pack.py` builds a Dolphin HD texture pack (ID `RDMP01`) from `DR_DUMPTEX=1` dumps: per texture it bleeds colour under `alpha==0`, upscales colour and alpha separately with Real-ESRGAN (anime model), and recombines.

## Host unit tests

`port/tests/` holds host tests built with plain gcc and a `-D..._HOST_TEST` define (no devkitPPC): `test_calcsub_a/b/c/d.c`, `test_gfx_clip.c`, `test_gfx_texconv.c`, `test_gfx_tmem.c`, `test_gfx_wrap.c`, `test_gfx_xform.c`, `test_naudio.c`, `test_pad_map.c`. `pad_wii.c` and `gfx_tmem.c` compile in two modes (host test with fallback `#defines` using the real WPAD/PAD bit values; Wii build with real headers). The texconv test must supply a `gfx_mem()` shim or the link fails. `port/ultra/test_threads.c` is the in-DOL thread self-test, enabled with `WII_SELFTEST=1` (log must contain `SELFTEST PASS`).
