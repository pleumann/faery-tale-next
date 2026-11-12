"""Raw block access to the Amiga disk image (game/image, 1760 blocks of 512 bytes)."""

BLOCK = 512


class Disk:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        if len(self.data) != 1760 * BLOCK:
            raise ValueError(f"{path}: unexpected size {len(self.data)}")

    def blocks(self, start, count):
        return self.data[start * BLOCK:(start + count) * BLOCK]
