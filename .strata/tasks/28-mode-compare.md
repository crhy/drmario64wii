# Task 28: compare every mode against the original N64 (data collection only, no fixes)
For each script in .strata/scratch/modes/*.txt (classic story vscom flash marathon scoreattack vs2p vs2p_flash
vs2p_scoreattack vs4p records options):
1. Port: make -f Makefile.wii clean; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=<script>;
   DR_43=1 DR_PADS=4 .strata/dolphin-run.sh build/wii/drmario64.dol 120 .strata/scratch/cmp/<mode>/port
2. Reference: port/tools/refemu/run-ref.sh <script> <frames 30..7200 step 3> .strata/scratch/cmp/<mode>/ref 1500
3. python3 port/tools/refcompare.py .strata/scratch/cmp/<mode>/port .strata/scratch/cmp/<mode>/ref
   .strata/scratch/cmp/<mode>/out --window 60 --worst 6 --port-log .strata/scratch/cmp/<mode>/port/sd/drmario/log.txt
   --ref-log .strata/scratch/cmp/<mode>/ref/run.log
Write a shell script .strata/scratch/cmp/run_all.sh that does all of this, run it, then write
.strata/scratch/cmp/SUMMARY.md: one table row per mode (frames compared, median diff, worst diff, and the 3 worst
8x8-block positions of the worst frame). Do not change any game or port code in this task.
GATE: test -x .strata/scratch/cmp/run_all.sh && for m in classic story vscom flash marathon scoreattack vs2p vs4p; do ls .strata/scratch/cmp/$m/out/cmp_*.png >/dev/null 2>&1 || { echo missing $m; exit 1; }; grep -q "^| *$m" .strata/scratch/cmp/SUMMARY.md || { echo no row $m; exit 1; }; done
