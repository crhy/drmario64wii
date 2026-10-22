# Task 11b: make the input script actually load, and prove START leaves the title screen
Task 11 added controller input (port/ultra/os_cont.c) and an optional embedded input script, but the script never loads:
- `make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t11in/input.txt` fails with
  `ld: cannot find build/wii/input_script.o`: the `$(ELF):` rule (Makefile.wii line ~97) lists $(INPUT_SCRIPT_OBJ) in its
  recipe but not in its prerequisites, so make never builds it. Add $(INPUT_SCRIPT_OBJ) to the prerequisites.
- Check the objcopy --redefine-sym names match what objcopy really emits (run `$(OBJCOPY)` then
  `powerpc-eabi-nm build/wii/input_script.o`); fix them if not. Script lines are "<frame> <hex buttons> <stick_x> <stick_y>".
- In os_cont.c, PORT_LOG every script press as `input press frame=<n> buttons=<hex>` (once per line that takes effect).
- In the existing mp loop log (src, `mp loop main_no=`), it is fine as is; do not change src/ for this task.
The baseline run with no script logs main_no=0 then main_no=3 (title). START (0x1000) pressed at frames 300 and 420 must move
the game past the title, so main_no must take a value other than 0 and 3, or the log must show the title menu advancing.
If START is pressed but the game ignores it, find out why (e.g. the game reads buttons through a contpad/joy layer in
src/main_segment/joy.c that needs edge detection over several frames, or press frames hit before the title accepts input:
try holding START for 10 frames and pressing at later frames by editing .strata/scratch/t11in/input.txt) and fix os_cont.c.
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/t11in/input.txt >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 40 .strata/scratch/t11b >/dev/null; L=.strata/scratch/t11b/sd/drmario/log.txt; grep -a "input script loaded" $L && grep -aq "input script loaded [1-9]" $L && grep -aq "input press" $L && grep -a "main_no=" $L | grep -av "main_no=[03]$" | grep -a "main_no="
