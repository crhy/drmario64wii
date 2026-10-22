#!/bin/bash
# Usage: dolphin-run.sh <dol|iso> <seconds> <outdir>. Headless Dolphin (flatpak) in Xvfb, throwaway user dir.
# Wii SD card image contents are extracted to <outdir>/sd/ after the run (game log: <outdir>/sd/drmario/log.txt).
# Saves every 30th frame PNG + the last one to <outdir>, Dolphin log to <outdir>/dolphin.log.
# Panic dialogs are off (a modal "Invalid read" dialog froze headless Dolphin and looked like a game hang);
# CPU exceptions (ISI/DSI) and bad memory accesses are logged to dolphin.log under W[PowerPC]/MEMMAP.
# Opt-in: DR_DUMPTEX=1 dumps textures to <outdir>/textures/; DR_HIRES=1 loads packs from <user>/Load/Textures (DR_HIRES_DIR copied there).
# Opt-in: DR_PADS=N plugs N emulated GameCube controllers into ports 1..N (default 1).
# Opt-in: DR_IRES=N renders at Nx internal resolution (6 = 4K) and dumps frames at that resolution.
# Opt-in: DR_43=1 boots with the Wii set to 4:3 (no widescreen squeeze) - used for pixel comparison with the N64 reference.
set -u; IN=$(realpath "$1"); SECS=$2; OUT=$(realpath -m "$3"); mkdir -p "$OUT"; rm -f "$OUT"/framedump_*.png "$OUT"/last.png  # never mix frames from older runs
H=$HOME/.var/app/org.DolphinEmu.dolphin-emu/harness-$$; mkdir -p $H/u/Config $H/sd; trap 'rm -rf $H' EXIT
cp "$IN" $H/; B=$H/$(basename "$IN")
PADS=${DR_PADS:-1}; SIDEV=$(for i in 0 1 2 3; do [ $i -lt $PADS ] && echo "SIDevice$i = 6" || echo "SIDevice$i = 0"; done)
cat > $H/u/Config/Dolphin.ini <<INI
[Interface]
UsePanicHandlers = False
ConfirmStop = False
ShowAnalyticsPrompt = False
[Analytics]
PermissionAsked = True
[Core]
$SIDEV
WiiSDCard = True
WiiSDCardAllowWrites = True
GFXBackend = OGL
EmulationSpeed = 1.0
[Movie]
DumpFrames = True
[DSP]
Backend = No Audio Output
INI
[ -n "${DR_HIRES_DIR:-}" ] && { mkdir -p $H/u/Load/Textures; cp -r "$DR_HIRES_DIR"/. $H/u/Load/Textures/; }
printf '[Settings]\nDumpEFBTarget = %s\nInternalResolution = %s\nInternalResolutionFrameDumps = %s\nDumpTextures = %s\nHiresTextures = %s\nDumpFramesAsImages = True\nPNGCompressionLevel = 1\n' "$([ -n "${DR_DUMPEFB:-}" ] && echo True || echo False)" "${DR_IRES:-1}" "$([ -n "${DR_IRES:-}" ] && echo True || echo False)" "$([ -n "${DR_DUMPTEX:-}" ] && echo True || echo False)" "$([ -n "${DR_HIRES:-}" ] && echo True || echo False)" > $H/u/Config/GFX.ini
printf '[Options]\nVerbosity = 4\nWriteToFile = True\n[Logs]\nOSREPORT = True\nOSREPORT_HLE = True\nPOWERPC = True\nMEMMAP = True\nBOOT = True\n' > $H/u/Config/Logger.ini
timeout -s INT $SECS xvfb-run -a -s "-screen 0 1280x720x24" flatpak run --filesystem=$H org.DolphinEmu.dolphin-emu -u $H/u ${DR_43:+-C SYSCONF.IPL.AR=False} -b -e "$B" > $H/dolphin.out 2>&1
RC=$?; echo "dolphin rc: $RC"
F=$H/u/Dump/Frames; ls $F 2>/dev/null | sort -t_ -k2 -n | awk 'NR%30==0' | while read p; do cp $F/$p "$OUT/"; done
L=$(ls $F 2>/dev/null | sort -t_ -k2 -n | tail -1); [ -n "$L" ] && cp $F/$L "$OUT/last.png"
rm -rf "$OUT/sd"; SDI=$(find $H/u -iname "*sd*.raw" | head -1); [ -n "$SDI" ] && 7z x -y -o"$OUT/sd" "$SDI" >/dev/null 2>&1; { [ -n "${DR_DUMPTEX:-}" ] || [ -n "${DR_DUMPEFB:-}" ]; } && { rm -rf "$OUT/textures"; cp -r $H/u/Dump/Textures "$OUT/textures" 2>/dev/null; }; head -c 50000000 $H/u/Logs/dolphin.log > "$OUT/dolphin.log" 2>/dev/null; cp $H/dolphin.out "$OUT/dolphin.out" 2>/dev/null
echo "frames: $(ls $F 2>/dev/null | wc -l), saved to $OUT"
