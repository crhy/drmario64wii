#!/usr/bin/env python3
"""imgmax.py <dir> --min-colors N: max distinct colours over all framedump PNGs in dir; exit 1 if below N."""
import sys, glob
from PIL import Image
d = sys.argv[1]; n = int(sys.argv[sys.argv.index("--min-colors") + 1])
best = (0, "")
for p in glob.glob(d + "/framedump_*.png"):
    try: c = len(set(Image.open(p).convert("RGB").getdata()))
    except Exception: continue
    if c > best[0]: best = (c, p)
print(f"max colors={best[0]} in {best[1]}"); sys.exit(0 if best[0] >= n else 1)
