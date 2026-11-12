#!/usr/bin/env python3
"""Convert the original Amiga game data into Spectrum Next formats.

Usage: convert.py <game-dir> <out-sd-root> <preview-dir>
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from iff import read_ilbm            # noqa: E402
from nextfmt import layer2_320_column_major  # noqa: E402
from png import write_png            # noqa: E402


def palette_bytes(cmap, count=32):
    """IFF CMAP -> Amiga RGB4 words (little-endian), the format the game code fades from."""
    out = bytearray()
    for i in range(count):
        r, g, b = cmap[i] if i < len(cmap) else (0, 0, 0)
        out += struct.pack("<H", ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4))
    return out


def convert_fullscreen(game, name, outdir, prevdir):
    """320x200 ILBM -> 320x256 column-major Layer 2 image (centred vertically) + palette."""
    img = read_ilbm(os.path.join(game, name))
    l2 = layer2_320_column_major(img.pixels, yoff=(256 - img.height) // 2)
    base = name.upper()
    with open(os.path.join(outdir, base + ".L2"), "wb") as f:
        f.write(l2)
    with open(os.path.join(outdir, base + ".PAL"), "wb") as f:
        f.write(palette_bytes(img.cmap))
    write_png(os.path.join(prevdir, name + ".png"), img.pixels, img.cmap + [(0, 0, 0)] * 256)


def main():
    game, sdroot, prevdir = sys.argv[1:4]
    outdir = os.path.join(sdroot, "FTA")
    os.makedirs(outdir, exist_ok=True)
    os.makedirs(prevdir, exist_ok=True)
    convert_fullscreen(game, "page0", outdir, prevdir)


if __name__ == "__main__":
    main()
