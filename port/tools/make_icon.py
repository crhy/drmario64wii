#!/usr/bin/env python3
# Generate the HBC icon (128x48 PNG): dark blue background, red/yellow pill.
import sys
from PIL import Image, ImageDraw

out = sys.argv[1] if len(sys.argv) > 1 else "build/wii/apps/drmario64/icon.png"
W, H = 128, 48
img = Image.new("RGBA", (W, H), (16, 24, 64, 255))
d = ImageDraw.Draw(img)

cx, cy = W // 2, H // 2
r = 16
L = 26
import math
ang = math.radians(45)
dx, dy = math.cos(ang), math.sin(ang)

# capsule center line endpoints
x1, y1 = cx - L * dx, cy - L * dy
x2, y2 = cx + L * dx, cy + L * dy

# yellow half (draw full capsule in yellow first)
d.line((x1, y1, x2, y2), fill=(240, 200, 40, 255), width=2 * r)
d.ellipse((x1 - r, y1 - r, x1 + r, y1 + r), fill=(240, 200, 40, 255))
d.ellipse((x2 - r, y2 - r, x2 + r, y2 + r), fill=(240, 200, 40, 255))

# red half: clip to the half-plane on one side of the perpendicular through center
# perpendicular direction
px, py = -dy, dx
mask = Image.new("RGBA", (W, H), (0, 0, 0, 0))
md = ImageDraw.Draw(mask)
md.line((x1, y1, x2, y2), fill=(220, 40, 40, 255), width=2 * r)
md.ellipse((x1 - r, y1 - r, x1 + r, y1 + r), fill=(220, 40, 40, 255))
md.ellipse((x2 - r, y2 - r, x2 + r, y2 + r), fill=(220, 40, 40, 255))

# keep only pixels on the red side of the perpendicular line through (cx,cy)
red = Image.new("RGBA", (W, H), (0, 0, 0, 0))
rd = ImageDraw.Draw(red)
# build a big rectangle covering the red half-plane
big = 400
a0x, a0y = cx - px * big, cy - py * big
a1x, a1y = cx + px * big, cy + py * big
b0x, b0y = cx - dx * big, cy - dy * big
b1x, b1y = cx + dx * big, cy + dy * big
rd.polygon([(a0x, a0y), (a1x, a1y), (b1x, b1y), (b0x, b0y)], fill=(220, 40, 40, 255))
mask = Image.composite(mask, Image.new("RGBA", (W, H), (0, 0, 0, 0)), red)
img = Image.alpha_composite(img, mask)

img.save(out)
