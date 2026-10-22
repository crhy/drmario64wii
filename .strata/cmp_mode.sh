#!/bin/bash
# Re-run ONE mode on the port and compare with the cached original-N64 frames (.strata/scratch/cmp/<mode>/ref, from task 28).
# Usage: .strata/cmp_mode.sh <mode> [seconds]   -> prints "median <x> worst <y> frames <n>" and writes .strata/scratch/cmp/<mode>/out2/
set -u; M=$1; SECS=${2:-120}; C=.strata/scratch/cmp/$M
make -f Makefile.wii clean >/dev/null && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/modes/$M.txt >/dev/null 2>&1 || { echo "build failed"; exit 1; }
rm -rf $C/port2 $C/out2; DR_43=1 DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol $SECS $C/port2 >/dev/null 2>&1
python3 port/tools/refcompare.py $C/port2 $C/ref $C/out2 --window 60 --worst 6 --port-log $C/port2/sd/drmario/log.txt --ref-log $C/ref/run.log | sed -E 's/^([0-9]+) frames: median diff ([0-9.]+), worst ([0-9.]+)/median \2 worst \3 frames \1/'
