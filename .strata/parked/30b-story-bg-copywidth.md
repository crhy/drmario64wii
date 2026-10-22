# Task 30b: Story gameplay background white - check the 656-wide EFB copy (EFB is only 640 wide)
Facts (Claude, 2026-10-22, DR_DUMPEFB=1 run of .strata/scratch/modes/story.txt):
- Story's intro cutscene renders correctly. Gameplay only starts ~100 s into the script: use 150 s runs.
- Every EFB copy Dolphin dumped is 640x480 (display copies). The background alias copy in port/gfx/gfx_xform.c
  (GX_SetTexCopySrc(0, 0, rtt_w*2 = 656, 480) for the 328-wide bg_snap_buf) never appears as its own copy. The Wii EFB
  is 640 pixels wide, so a 656-wide copy source is outside the EFB (invalid/clipped). Classic may hide this because its
  visible background does not depend on the last 16 columns; Story's stage picture fills the screen.
Do: make the alias copy legal: copy at most 640 EFB columns (the N64 snapshot is 328 wide = 320 visible + 8 padding;
copy 640 wide = 320 N64 columns at 2x, and map texcoords so N64 columns >= 320 clamp to the last copied column).
Verify with DR_DUMPEFB=1 that a separate 640x480 (or smaller) alias copy now appears at the snapshot moment, and that
Story gameplay shows its stage picture. Note the result in NOTES.md.
GATE: a=$(.strata/cmp_mode.sh story 150); echo "$a"; b=$(.strata/cmp_mode.sh classic 120); echo "$b"; c=$(.strata/cmp_mode.sh flash 120); echo "$c"; python3 -c "a=float('$(echo "$a" | awk '{print $2}')'); b=float('$(echo "$b" | awk '{print $2}')'); c=float('$(echo "$c" | awk '{print $2}')'); print(a,b,c); assert a < 12 and b <= 32 and c <= 37"
