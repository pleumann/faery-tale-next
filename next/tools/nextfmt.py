"""Helpers for ZX Spectrum Next data formats."""


def rgb4_to_next9(rgb4):
    """Amiga 0x0RGB (4 bits per channel) -> Next 9-bit colour as (byte1, byte2) for NEXTREG 0x44."""
    r = (rgb4 >> 8) & 15
    g = (rgb4 >> 4) & 15
    b = rgb4 & 15
    return rgb8_to_next9(r * 17, g * 17, b * 17)


def rgb8_to_next9(r, g, b):
    r3 = (r * 7 + 127) // 255
    g3 = (g * 7 + 127) // 255
    b3 = (b * 7 + 127) // 255
    return ((r3 << 5) | (g3 << 2) | (b3 >> 1), b3 & 1)


def layer2_320_column_major(rows, width=320, height=256, xoff=0, yoff=0, fill=0):
    """Place an image (list of index rows) into a 320x256 Layer 2 buffer (column-major: x*256+y)."""
    buf = bytearray([fill]) * (width * height)
    for y, row in enumerate(rows):
        yy = y + yoff
        if not 0 <= yy < height:
            continue
        for x, v in enumerate(row):
            xx = x + xoff
            if 0 <= xx < width:
                buf[xx * height + yy] = v
    return buf
