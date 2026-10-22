# Task 10g: black box behind "PRESS ANY BUTTON"
The title screen is now correct (.strata/scratch/title-ok.png) except that "PRESS ANY BUTTON" has an opaque black box.
Find the draw for that text (log the texrect/tri, its texture format, combine words and othermode L/H for draws whose
texture is the 128x16-ish text strip; find it in a -DPORT_TEXDUMP run in sd:/drmario/tex/index.txt), decode those
words by hand into NOTES.md (gbi.h: G_CC_*, G_RM_*, G_AC_*), and fix the renderer so it matches what the N64 does for
that combination (likely: the text texture is IA or I format with alpha, drawn with alpha compare or blending that is
not applied on that path; or the black comes from a separate fill/texrect drawn behind it with prim alpha that should
be translucent). Do not change anything that would alter the rest of the title screen.
Check: in a frame where the text is visible, the pixel at the left end of the old box (x=232, y=345 in the 780x480
dump) must not be black. Use python + PIL.
GATE: make -f Makefile.wii && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t10g >/dev/null && python3 -c "from PIL import Image; import glob,sys; ok=any(sum(Image.open(p).convert('RGB').getpixel((232,345)))>60 for p in glob.glob('.strata/scratch/t10g/framedump_9*.png')); sys.exit(0 if ok else 1)"
