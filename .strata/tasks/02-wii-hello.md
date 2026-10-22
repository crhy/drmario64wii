# Task 02: Wii build skeleton
Create `Makefile.wii` at repo root (do not touch the existing Makefile) and `port/wii/main.c`.
- Makefile.wii uses `$(DEVKITPPC)/bin/powerpc-eabi-gcc`, flags `-O2 -mrvl -mcpu=750 -meabi -mhard-float -DGEKKO -DTARGET_WII -I$(DEVKITPRO)/libogc/include`,
  links with `-mrvl -L$(DEVKITPRO)/libogc/lib/wii -lwiiuse -lbte -logc -lm`, builds objects into build/wii/ and
  converts the ELF with `$(DEVKITPRO)/tools/bin/elf2dol` to `build/wii/drmario64.dol`. Source list is a variable
  `WII_SRCS` (only port/wii/main.c for now) so later tasks can add files.
- main.c: VIDEO_Init, 640x480 progressive-capable render mode (VIDEO_GetPreferredMode), GX init with a 640x480 EFB,
  WPAD_Init, then a loop that clears the screen to dark blue, draws one white quad with GX in 2D ortho, prints
  "drmario64 wii boot ok" once with printf, copies EFB to XFB and waits for VSync. Look at
  $DEVKITPRO/examples/wii/graphics/gx/triangle/source/*.c for the GX setup pattern.
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 25 .strata/scratch/t02 && test -s .strata/scratch/t02/last.png
