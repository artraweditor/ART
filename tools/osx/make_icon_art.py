#!/usr/bin/env python3
"""Generate tools/osx/ART.icon/Assets/ART-logo-full.png from the ART logo.

The Liquid Glass icon masks the artwork with a rounded square, which cuts off
the transparent corners of the logo.  This extends the rainbow brush strokes
outwards (a clean colour ramp, red at the top-left corner to purple at the
bottom-right one, with the logo's own strokes composited on top) so that the
whole canvas is covered.  Everything inside the black outline of the white
square is left untouched.

    python3 tools/osx/make_icon_art.py
"""
import os
import numpy as np
from PIL import Image

here = os.path.dirname(os.path.abspath(__file__))
src = os.path.join(here, '../../data/images/png/ART-logo-1024.png')
dst = os.path.join(here, 'ART.icon/Assets/ART-logo-full.png')

# centre of the black-outlined square and a rectangle just inside its outer edge
CX, CY = 506.0, 499.0
INSIDE = (113, 899, 105, 893)  # x0, x1, y0, y1

# (position along the way round the frame, colour); 0 = top-left, 1 = bottom-right
RAMP = [(0.00, (255, 0, 0)), (0.12, (255, 20, 0)), (0.20, (255, 140, 0)),
        (0.28, (255, 200, 0)), (0.36, (120, 215, 0)), (0.44, (0, 200, 0)),
        (0.50, (0, 185, 220)), (0.60, (0, 100, 170)), (0.70, (20, 30, 160)),
        (0.78, (110, 20, 170)), (1.00, (115, 18, 170))]

a = np.asarray(Image.open(src).convert('RGBA')).astype(float) / 255.0
h, w = a.shape[:2]
yy, xx = np.mgrid[0:h, 0:w]
ang = np.arctan2(yy - CY, xx - CX)
tl = np.arctan2(-1, -1)
d = np.abs((ang - tl + np.pi) % (2 * np.pi) - np.pi) / np.pi
pos = [p for p, _ in RAMP]
bg = np.stack([np.interp(d, pos, [c[k] for _, c in RAMP]) for k in range(3)],
              -1) / 255.0
x0, x1, y0, y1 = INSIDE
inside = (xx >= x0) & (xx <= x1) & (yy >= y0) & (yy <= y1)
al = a[..., 3:4]
out = np.empty_like(a)
out[..., :3] = np.where(inside[..., None], a[..., :3],
                        a[..., :3] * al + bg * (1 - al))
out[..., 3] = np.where(inside, a[..., 3], 1.0)
Image.fromarray((out * 255 + 0.5).astype(np.uint8), 'RGBA').save(dst)
print('wrote', os.path.normpath(dst))
