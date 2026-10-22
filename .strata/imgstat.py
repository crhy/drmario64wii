#!/usr/bin/env python3
"""imgstat.py <png> [--min-colors N] [--min-nonblack PCT]: prints distinct colours and % non-black pixels; exit 1 if below the thresholds."""
import sys
from PIL import Image
a = sys.argv; im = Image.open(a[1]).convert("RGB"); px = list(im.getdata())
colors = len(set(px)); nb = 100.0 * sum(1 for p in px if max(p) > 16) / len(px)
print(f"{a[1]}: {im.size[0]}x{im.size[1]} colors={colors} nonblack={nb:.1f}%")
mc = int(a[a.index("--min-colors") + 1]) if "--min-colors" in a else 0
mn = float(a[a.index("--min-nonblack") + 1]) if "--min-nonblack" in a else 0
sys.exit(0 if colors >= mc and nb >= mn else 1)
