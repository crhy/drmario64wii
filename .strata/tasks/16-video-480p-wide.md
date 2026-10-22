# Task 16: 480p progressive + correct 16:9 presentation
Goal from _header.md: 480p, 16:9. Dolphin's default SYSCONF has widescreen ON and progressive scan ON, so the default
harness run exercises this path.
1. port/wii/main.c: pick the video mode explicitly. If CONF_GetProgressiveScan() > 0 and VIDEO_HaveComponentCable()
   use &TVNtsc480Prog (PAL60 regions too: CONF_GetEuRGB60/CONF_GetVideo -> TVEurgb60Hz480Prog). Otherwise keep
   VIDEO_GetPreferredMode(NULL). Do it in ONE helper `static GXRModeObj *port_pick_mode(void)` used by both main() and
   port_gx_setup() (today both call VIDEO_GetPreferredMode separately). Log once: "video: mode=%s w=%d h=%d wide=%d".
2. 16:9: when CONF_GetAspectRatio() == CONF_ASPECT_16_9, set mode->viWidth = 678 and
   mode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678) / 2 BEFORE VIDEO_Configure (standard libogc widescreen trick: the VI
   squeezes the 640-wide XFB so a 16:9 TV shows the 4:3 game with correct proportions, no stretched sprites). Keep the
   EFB/XFB at 640 wide. In 4:3 leave the mode untouched.
3. Do not change the renderer's 320x240 -> 640x480 scale. Note in NOTES.md how a future "wide backgrounds" option
   would widen only the background layer (do not implement).
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t14d_in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 90 .strata/scratch/t16 >/dev/null; L=.strata/scratch/t16/sd/drmario/log.txt; grep -a "video: mode=" $L | grep -aq "h=480 wide=1" && [ $(grep -c "port_pick_mode()" port/wii/main.c) -ge 2 ] && ! grep -q "VIDEO_GetPreferredMode(NULL);" <(sed -n '/port_gx_setup(void)/,/^}/p' port/wii/main.c) && ! grep -aq CRASH $L && grep -aq "main_no=5" $L && .strata/imgmax.py .strata/scratch/t16 --min-colors 64
