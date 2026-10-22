#!/bin/bash
# Reference frames from the ORIGINAL N64 ROM (mupen64plus + angrylion cycle-accurate RDP), driven by a port input script.
# Usage: run-ref.sh <input-script|/dev/null> <comma-separated frame list> <outdir> [timeout-seconds]
set -u; R=${REFEMU_DIR:-/home/rhy/drmariowii/refemu}; HERE=$(cd "$(dirname "$0")" && pwd); SCRIPT=$(realpath "$1"); SHOTS=$2; OUT=$(realpath -m "$3"); TMO=${4:-600}
# Rebuild the input-script plugin when its source is newer than the .so (so W/segment edits take effect).
SO="$R/input-script/mupen64plus-input-script.so"; SRC="$HERE/input_script.c"
if [ ! -f "$SO" ] || [ "$SRC" -nt "$SO" ]; then mkdir -p "$(dirname "$SO")"; gcc -O2 -fPIC -shared -I"$R/mupen64plus-core/src/api" -o "$SO" "$SRC" -ldl; fi
. "$R/env.sh"; mkdir -p "$OUT"; rm -f "$OUT"/*.png
CFG=$OUT/cfg; mkdir -p "$CFG"
ROM=/home/rhy/drmariowii/repo/build/us/drmario64.us.z64
M64_FB_DIR=$OUT M64_INPUT_SCRIPT=$SCRIPT M64_SHOTS=$SHOTS LD_LIBRARY_PATH=$R/sysroot/usr/lib/x86_64-linux-gnu:$R/mupen64plus-core/projects/unix \
timeout $TMO xvfb-run -a -s "-screen 0 1024x768x24" "$R/mupen64plus-ui-console/projects/unix/mupen64plus" --nosaveoptions --noosd \
  --corelib "$R/mupen64plus-core/projects/unix/libmupen64plus.so.2" --configdir "$CFG" --datadir "$R/mupen64plus-core/data" \
  --gfx "$R/angrylion-rdp-plus/build/mupen64plus-video-angrylion-plus.so" --rsp "$R/mupen64plus-rsp-cxd4/projects/unix/mupen64plus-rsp-cxd4-sse2.so" \
  --input "$R/input-script/mupen64plus-input-script.so" --audio dummy --emumode 1 --sshotdir "$OUT" "$ROM" > "$OUT/run.log" 2>&1
echo "rc=$? fb=$(ls "$OUT"/ref_*.ppm 2>/dev/null | wc -l)"
# Framebuffer dumps (ref_<frame>.ppm, exact game pixels) -> PNG.
for f in "$OUT"/ref_*.ppm; do [ -e "$f" ] && python3 -c "import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[1][:-4]+'.png')" "$f" && rm "$f"; done
exit 0
# (old) Rename screenshots to ref_<frame>.png (they are written in M64_SHOTS order).
i=0; IFS=, read -ra FR <<< "$SHOTS"; for f in $(ls "$OUT"/*.png 2>/dev/null | sort); do [ $i -lt ${#FR[@]} ] && mv "$f" "$OUT/ref_${FR[$i]}.png"; i=$((i+1)); done
