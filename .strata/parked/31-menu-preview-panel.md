# Task 31: menu preview panel and cursor glove render wrong (pink square, black box, black square behind the glove)
Task 28 (.strata/scratch/cmp/classic/out etc., left = port, right = original): in every main-menu screen the right-hand
panel should show the mascot (yellow ball with blue hat) and a small mode-preview picture; the port shows a pink
square and a tall black rectangle instead, and the cursor glove has a black square behind it. Same family as NOTES "19b
clues" (transparent texels drawn opaque, option plates black). Trace the draws covering those screen areas in main_no==6
(texrect debug log in port/gfx/gfx_tex.c, PORT_TEXDUMP) and fix the renderer (alpha/blend/combiner/format), not the game.
GATE: a=$(.strata/cmp_mode.sh options 60); echo "$a"; python3 -c "assert float('$(echo "$a" | awk '{print $2}')') < 20"
