# Task 34: write the port documentation (docs/wii-port/), for people reading the repo on GitHub
Sources: .strata/DESIGN.md, .strata/NOTES.md (long: read it in parts with grep -n "^## " first), the code in port/,
Makefile.wii, port/tools/, .strata/dolphin-run.sh, .strata/cmp_mode.sh. Write plain, accurate Markdown; explain WHY,
cite file paths. No marketing. Do not include game data, ROM-derived images or copyrighted text. Files:
1. docs/wii-port/README.md - what the port is (native PowerPC recompile of the matching decompilation, rendered with
   GX, not an emulator), status (release 0.1.0: 1P/2P/4P verified in Dolphin; known issues list from
   /home/rhy/drmariowii/release-0.1.0/README.md), how the docs are organised. Legal: you need your own ROM; no ROM,
   ISO or game data in this repo.
2. docs/wii-port/BUILDING.md - toolchains (devkitPPC/libogc, N64 build first), make targets (make, make -f
   Makefile.wii, hbc, iso with DISC_TEMPLATE and why: retail apploader, dol_for_disc.py BSS), input-script builds.
3. docs/wii-port/N64-ON-WII.md - how each N64 subsystem is re-implemented on the Wii, one section each: threads and
   the cooperative scheduler (port/ultra/os_sched.c), messages/events and the event pump (no dropped events, task 33),
   VI retrace, timers and osGetTime/osSetTime (frame pacing bug), PI DMA from the embedded ROM, controllers (Wiimote
   sideways/Classic/GC, per-port detection), EEPROM on SD, RSP: graphics tasks (F3DEX2/S2DEX display-list walker ->
   GX: matrices, vertices/clipping, TMEM emulation incl. LOADBLOCK odd-line swap and quadricated TLUT, tile origin,
   wrap/mirror/mask, combiner -> TEV, render modes/blender/alpha, COPY/FILL inclusive edges), off-screen render
   targets (G_SETCIMG -> GPU alias, 640-wide EFB limit), whole-image texture hints; audio tasks (naudio ABI
   interpreter, run synchronously) and output via AI DMA (why not AESND). Note each place that differs from N64
   behaviour and why.
4. docs/wii-port/TESTING.md - Dolphin harness (.strata/dolphin-run.sh options DR_PADS, DR_43, DR_IRES, DR_DUMPTEX,
   DR_DUMPEFB, DR_HIRES), input scripts (5 columns, W <main_no> waits, latest-segment rule), the reference renderer
   (port/tools/refemu: mupen64plus + angrylion, exact framebuffer dumps), refcompare.py and cmp_mode.sh, host unit tests
   in port/tests and how to run them, the HD texture pack pipeline (port/tools/hd_pack.py).
5. docs/wii-port/LESSONS.md - the important bugs found and their causes (from NOTES.md), short, one paragraph each.
GATE: for f in README BUILDING N64-ON-WII TESTING LESSONS; do test -s docs/wii-port/$f.md || { echo missing $f; exit 1; }; done; grep -q "osSetTime" docs/wii-port/N64-ON-WII.md && grep -q "TMEM" docs/wii-port/N64-ON-WII.md && grep -q "refcompare" docs/wii-port/TESTING.md && grep -q "DISC_TEMPLATE" docs/wii-port/BUILDING.md && [ $(cat docs/wii-port/*.md | wc -w) -ge 3500 ]
