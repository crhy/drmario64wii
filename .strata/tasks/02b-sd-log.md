# Task 02b: logging to the SD card
printf does not reach Dolphin's log, so the port logs to the Wii SD card instead.
Create `port/wii/log.c` + `port/include/port_log.h`: `void port_log_init(void)` mounts the SD card with libfat
(`fatInitDefault()`, link `-lfat` before `-logc`), creates `sd:/drmario/` and opens `sd:/drmario/log.txt` for
writing; `void PORT_LOG(const char *fmt, ...)` writes one line, appends '\n' and fflush()es every call; it must be a
safe no-op if the SD card is missing. Call port_log_init() early in port/wii/main.c and PORT_LOG("drmario64 wii boot ok")
once, plus PORT_LOG("frame %d") at frames 1, 60 and 300. Add log.c to WII_SRCS.
.strata/dolphin-run.sh enables Dolphin's SD folder sync and copies the card to <outdir>/sd/ after the run. If the log
file does not appear, check whether Dolphin only syncs on a clean shutdown (it is killed by `timeout`, i.e. SIGTERM);
if so, change dolphin-run.sh to send SIGINT first (`timeout -s INT -k 10 ...`) and note it in NOTES.md.
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 25 .strata/scratch/t02b && grep -q "boot ok" .strata/scratch/t02b/sd/drmario/log.txt
