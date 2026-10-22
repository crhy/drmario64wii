# Task 01: recon notes (no code changes)
Write `.strata/NOTES.md` with these sections, using grep over src/ and include/ (NOT lib/ultralib):
1. "libultra API used": every `os*`, `gu*`, `gsp*`/`gdp*` function-like call, `al*`/`n_al*`, `__os*` symbol referenced from src/ and lib/libmus, with the file(s) that use it. One line each.
2. "Asset loading": how a segment listed in src/boot/boot_data.c gets from ROM to RAM (which functions DMA it,
   whether it is gzip-expanded, which buffer it lands in, how the game then addresses it: gSPSegment segment
   numbers or absolute pointers). Quote the key function names and file:line.
3. "Assembly files": every .s file under src/ and lib/libmus with one line on what it does.
4. "Threads and priorities": every osCreateThread call with entry, priority and stack.
5. "Microcode": which graphics/audio microcodes the game actually starts (grep osSpTaskStart callers / OSTask setup).
Keep it under 250 lines.
GATE: grep -q "Asset loading" .strata/NOTES.md && grep -q osCreateThread .strata/NOTES.md && [ $(wc -l < .strata/NOTES.md) -gt 40 ]
