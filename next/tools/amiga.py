"""Amiga-specific decoding: planar graphics and load-file hunks."""
import struct

from iff import planar_row_to_chunky


def planar_image(data, width, height, depth, plane_stride, row_stride=None):
    """Decode planes stored one after another (plane p at p*plane_stride).
    Returns a list of rows of palette indices."""
    rowbytes = ((width + 15) // 16) * 2
    if row_stride is None:
        row_stride = rowbytes
    rows = []
    for y in range(height):
        planes = [data[p * plane_stride + y * row_stride:p * plane_stride + y * row_stride + rowbytes]
                  for p in range(depth)]
        rows.append(planar_row_to_chunky(planes, width))
    return rows


HUNK_HEADER = 0x3F3
HUNK_CODE = 0x3E9
HUNK_DATA = 0x3EA
HUNK_RELOC32 = 0x3EC
HUNK_END = 0x3F2


def first_hunk(data):
    """Return the contents of the first code/data hunk of an AmigaOS load file.
    Pointers inside stay as offsets relative to the hunk start (single-hunk files)."""
    pos = 0

    def long():
        nonlocal pos
        v = struct.unpack(">I", data[pos:pos + 4])[0]
        pos += 4
        return v

    if long() != HUNK_HEADER:
        raise ValueError("not a load file")
    while long() != 0:          # resident library names (none expected)
        pass
    _table_size, first, last = long(), long(), long()
    for _ in range(last - first + 1):
        long()                  # hunk sizes
    kind = long() & 0x3FFFFFFF
    if kind not in (HUNK_CODE, HUNK_DATA):
        raise ValueError(f"unexpected hunk type {kind:#x}")
    size = long() * 4
    return data[pos:pos + size]
