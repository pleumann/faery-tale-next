"""Minimal IFF ILBM reader (BMHD, CMAP, BODY with ByteRun1) for the Faery Tale data.

Returns images as a list of rows of palette indices; no dependency outside the stdlib.
"""
import struct


class Ilbm:
    def __init__(self, width, height, depth, pixels, cmap):
        self.width = width
        self.height = height
        self.depth = depth
        self.pixels = pixels  # list of rows, each a bytearray of indices
        self.cmap = cmap      # list of (r, g, b) 0..255, may be empty


def _byterun1(data, pos, nbytes):
    out = bytearray()
    while len(out) < nbytes:
        n = data[pos]
        pos += 1
        if n < 128:
            out += data[pos:pos + n + 1]
            pos += n + 1
        elif n > 128:
            out += bytes([data[pos]]) * (257 - n)
            pos += 1
        # n == 128: no-op
    if len(out) != nbytes:
        raise ValueError("ByteRun1 row overrun")
    return out, pos


def read_ilbm(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[0:4] != b"FORM" or data[8:12] != b"ILBM":
        raise ValueError(f"{path}: not an ILBM")
    pos = 12
    end = 8 + struct.unpack(">I", data[4:8])[0]
    bmhd = None
    cmap = []
    body = None
    while pos + 8 <= end:
        tag = data[pos:pos + 4]
        size = struct.unpack(">I", data[pos + 4:pos + 8])[0]
        chunk = data[pos + 8:pos + 8 + size]
        if tag == b"BMHD":
            bmhd = struct.unpack(">HHhhBBBBHBBhh", chunk[:20])
        elif tag == b"CMAP":
            cmap = [tuple(chunk[i:i + 3]) for i in range(0, size - size % 3, 3)]
        elif tag == b"BODY":
            body = chunk
        pos += 8 + size + (size & 1)
    if bmhd is None or body is None:
        raise ValueError(f"{path}: missing BMHD or BODY")
    width, height, _x, _y, depth, masking, compression = bmhd[:7]
    rowbytes = ((width + 15) // 16) * 2
    planes_per_row = depth + (1 if masking == 1 else 0)
    pixels = []
    p = 0
    for _ in range(height):
        planes = []
        for _ in range(planes_per_row):
            if compression == 1:
                row, p = _byterun1(body, p, rowbytes)
            else:
                row = body[p:p + rowbytes]
                p += rowbytes
            planes.append(row)
        pixels.append(planar_row_to_chunky(planes[:depth], width))
    return Ilbm(width, height, depth, pixels, cmap)


def planar_row_to_chunky(planes, width):
    """Convert one row given as a list of plane byte strings into palette indices."""
    out = bytearray(width)
    for x in range(width):
        byte = x >> 3
        bit = 7 - (x & 7)
        v = 0
        for i, plane in enumerate(planes):
            v |= ((plane[byte] >> bit) & 1) << i
        out[x] = v
    return out
