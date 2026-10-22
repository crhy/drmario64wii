You are porting Dr. Mario 64 (a fully matching N64 decompilation, repo `/home/rhy/drmariowii/repo`) to a native Wii program.
Goal: a Wii ISO that plays perfectly in Dolphin, sideways Wiimote controls, 480p/16:9, rendered with GX so Dolphin can upscale and load hi-res texture packs.
Tools: devkitPPC + libogc are at `~/devkitpro` (export DEVKITPRO=$HOME/devkitpro DEVKITPPC=$HOME/devkitpro/devkitPPC). The N64 build already works (`make` -> build/us/drmario64.us.z64: OK); keep it working, never edit src/ in a way that breaks it (use `#ifdef TARGET_WII` for port changes).
Headless Dolphin test: `.strata/dolphin-run.sh <file.dol|iso> <seconds> <outdir>` saves frame PNGs to <outdir>/ and Dolphin's log to <outdir>/dolphin.log. Never run Dolphin outside that script (it uses a virtual screen; never touch the real desktop).
Design and decisions: `.strata/DESIGN.md` — read it first, follow it, and append what you learn to `.strata/NOTES.md`.
Rules: Do NOT commit, push, stash, checkout or reset. Stay inside the repo. Never put files in /tmp (it is RAM); use `.strata/scratch/`.
Each reply is capped at 8192 output tokens INCLUDING thinking: think in a few lines, then act. WRITE FILES FIRST. Context is 64k: never read whole large files, use grep -n and read at most 80 lines around a hit.
Finish with a 3-line summary: what you did, how you checked it, what is left.
IMPORTANT: the session ENDS the moment you reply with text and no tool call. Never write "I'll continue..." — just make the next tool call. Only reply with plain text when the task is finished and you have run its GATE command yourself and it passed.
Limit exploration to about 10 tool calls, then start writing files.
You cannot see images: use `.strata/imgstat.py <png>` (distinct colours, % non-black) to judge frame dumps, and PORT_LOG counters to check what the renderer did.
NEVER edit .strata/dolphin-run.sh, .strata/dolphin-gdb.sh, .strata/run-step.sh or .strata/queue2.sh (the test harness). If you think the harness is wrong, write why in NOTES.md and stop. (A task once switched Dolphin to the Null video backend, which made every frame black and hid all rendering for a day.)
