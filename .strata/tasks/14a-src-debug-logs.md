# Task 14a: silence the remaining per-frame debug logs
A 40 s run still writes thousands of debug lines (each PORT_LOG costs SD I/O, and a game thread blocked in SD I/O keeps
the scheduler CPU token, starving the main thread). Counts from .strata/scratch/t13: "bcast mq=" x6700, "gfx submit"
x2240, "title loop recv done" x225, "contsend rc=" x225, "pi pre"/"pi sent"/"pi in"/"pi cp", "inv", "inv2", "drin",
"drblk", "drpre", "gx state changes/frame", "sched time id=".
1. `grep -rn PORT_LOG src lib port` and wrap every per-frame or per-DMA log in `#ifdef PORT_LOG_VERBOSE` ... `#endif`
   around the WHOLE statement (never between an `if` and its body - that exact mistake broke event delivery in 12d).
   Keep one-time boot milestones and error logs (CRASH, "ring full", "table full", "runaway list", 600-frame stats
   like "rsp: presents=" and "tex stats").
2. Keep every src/ change inside #ifdef TARGET_WII so the N64 ROM still matches.
GATE: make 2>&1 | tail -3 | grep -q ": OK" && make -f Makefile.wii >/dev/null 2>REGRESSION1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t14a >/dev/null && .strata/imgmax.py .strata/scratch/t14a --min-colors 128 && test $(grep -acE "bcast mq=|gfx submit|title loop recv|contsend|^pi |^inv|^dr(in|blk|pre)" .strata/scratch/t14a/sd/drmario/log.txt) -lt 20
