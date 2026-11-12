"""Tiny PNG writer (8-bit palette images) using only the standard library."""
import struct
import zlib


def _chunk(tag, data):
    c = struct.pack(">I", len(data)) + tag + data
    return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


def write_png(path, rows, palette):
    """rows: list of byte rows of palette indices; palette: list of (r,g,b) 0..255."""
    height = len(rows)
    width = len(rows[0])
    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    pal = b"".join(bytes(c) for c in palette[:256])
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0)))
        f.write(_chunk(b"PLTE", pal))
        f.write(_chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(_chunk(b"IEND", b""))
