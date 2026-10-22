# Task 08a: display-list walker (no drawing yet)
Background: the game runs (music synthesis off via -DPORT_NO_SYNTH, leave it). osSpTaskStartGo in port/ultra/os_sp.c gets ~60 M_GFXTASK/s. Presentation is port_present() in port/wii/main.c (copies EFB -> the port's own XFBs; NEVER write the game's fb). Full design of the renderer: .strata/scratch/08-gfx-core-full.md (read it once). Opcode values/bit layouts: lib/ultralib/include/PR/gbi.h with F3DEX_GBI_2 (grep, never guess).
Write port/gfx/gfx.c + port/gfx/gfx.h (add gfx.c to WII_SRCS), at most ~350 lines:
- gfx_run_task(OSTask *t): called from osSpTaskStartGo for M_GFXTASK BEFORE it sends the SP/DP done events.
  Walks t->t.data_ptr: G_DL (call and branch; 18-deep stack), G_ENDDL, G_MOVEWORD/G_MW_SEGMENT (segment table of 16).
  Address resolution: addr >= 0x80000000 is a direct pointer; else seg=(addr>>24)&0xF, ptr=segtable[seg]+(addr&0xFFFFFF).
  Every other opcode: call a per-opcode handler function pointer table (256 entries) defaulting to a counter; later
  tasks fill the table. F3DEX2 commands are 8 bytes (w0,w1); G_TEXRECT is followed by 2 G_RDPHALF commands (handled later).
  Stop safely after 100000 commands (log "gfx: runaway list").
  S2DEX tasks (t->t.ucode == gspS2DEX_fifoTextStart): walk the same way (S2DEX opcodes go through the same table).
- Every 300 tasks PORT_LOG one line: "gfx stats tasks=%d cmds=%d" plus the 8 most frequent opcodes "op=count".
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 30 .strata/scratch/t08a >/dev/null && grep -a "gfx stats" .strata/scratch/t08a/sd/drmario/log.txt | tail -2 && ! grep -aq "runaway" .strata/scratch/t08a/sd/drmario/log.txt && grep -aq "gfx stats" .strata/scratch/t08a/sd/drmario/log.txt
