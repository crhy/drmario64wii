# Task 32: name plates show garbled glyphs instead of "NEW"
Task 28 (.strata/scratch/cmp/vscom/out, left = port, right = original): in the name-select screen the empty name plates
read "NEW" on the original but show garbled zig-zag glyphs on the port. Find the texture/format path used for that text
(texrect debug log, PORT_TEXDUMP) and fix the decode or texcoords in the renderer.
GATE: a=$(.strata/cmp_mode.sh vscom 60); echo "$a"; python3 -c "assert float('$(echo "$a" | awk '{print $2}')') < 20"
