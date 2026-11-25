#!/usr/bin/env python3
"""Stage Pyromax Dax and generate the Copper Circuit workshop assets.

Read the original atlas coordinates as an interface to the renderer. Draw
new graphics and courses without loading original bitmaps, maps, or media.
The compiler and browser qualification run after this staging step.
"""

import argparse
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess


TRANSPARENT = (255, 0, 255)
INK = (14, 30, 48)
METAL = (31, 62, 88)
MINT = (70, 212, 185)
PAPER = (219, 241, 240)
PLAYER = (255, 74, 112)
SHOT = (255, 230, 92)

# Five bits per row, most significant bit at the left edge. The display
# font has uppercase glyphs and shares them with the lowercase range.
GLYPHS = {
    "A": (14,17,17,31,17,17,17), "B": (30,17,17,30,17,17,30), "C": (14,17,16,16,16,17,14),
    "D": (30,17,17,17,17,17,30), "E": (31,16,16,30,16,16,31), "F": (31,16,16,30,16,16,16),
    "G": (14,17,16,23,17,17,15), "H": (17,17,17,31,17,17,17), "I": (14,4,4,4,4,4,14),
    "J": (7,2,2,2,18,18,12), "K": (17,18,20,24,20,18,17), "L": (16,16,16,16,16,16,31),
    "M": (17,27,21,21,17,17,17), "N": (17,25,21,19,17,17,17), "O": (14,17,17,17,17,17,14),
    "P": (30,17,17,30,16,16,16), "Q": (14,17,17,17,21,18,13), "R": (30,17,17,30,20,18,17),
    "S": (15,16,16,14,1,1,30), "T": (31,4,4,4,4,4,4), "U": (17,17,17,17,17,17,14),
    "V": (17,17,17,17,17,10,4), "W": (17,17,17,21,21,27,17), "X": (17,17,10,4,10,17,17),
    "Y": (17,17,10,4,4,4,4), "Z": (31,1,2,4,8,16,31),
    "0": (14,17,19,21,25,17,14), "1": (4,12,4,4,4,4,14), "2": (14,17,1,2,4,8,31),
    "3": (30,1,1,14,1,1,30), "4": (2,6,10,18,31,2,2), "5": (31,16,16,30,1,1,30),
    "6": (14,16,16,30,17,17,14), "7": (31,1,2,4,8,8,8), "8": (14,17,17,14,17,17,14),
    "9": (14,17,17,15,1,1,14), "!": (4,4,4,4,4,0,4), "?": (14,17,1,2,4,0,4),
    ":": (0,4,4,0,4,4,0), ".": (0,0,0,0,0,4,4), ",": (0,0,0,0,0,4,8),
    "-": (0,0,0,14,0,0,0), "+": (0,0,4,14,4,0,0), "/": (1,1,2,4,8,16,16),
    "[": (14,8,8,8,8,8,14), "]": (14,2,2,2,2,2,14), "'": (4,4,8,0,0,0,0),
    "=": (0,0,14,0,14,0,0), "*": (0,4,21,14,21,4,0), "<": (2,4,8,16,8,4,2),
    ">": (8,4,2,1,2,4,8), "_": (0,0,0,0,0,0,31), " ": (0,) * 7,
}


class Bitmap:
    """24-bit BGR pixels in row order. Pyromax's loader keys out magenta."""

    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = bytearray(bytes(reversed(TRANSPARENT)) * width * height)

    def box(self, x, y, width, height, color):
        left = max(x, 0)
        right = min(x + width, self.width)
        if left >= right:
            return
        line = bytes(reversed(color)) * (right - left)
        for row in range(max(y, 0), min(y + height, self.height)):
            start = (row * self.width + left) * 3
            self.pixels[start:start + len(line)] = line

    def label(self, x, y, text, scale, color=PAPER):
        for letter in text.upper():
            for row, bits in enumerate(GLYPHS.get(letter, GLYPHS["?"])):
                for column in range(5):
                    if bits & (16 >> column):
                        self.box(x + column * scale, y + row * scale, scale, scale, color)
            x += 6 * scale

    def centered(self, x, y, width, height, text, scale, color=PAPER):
        self.label(x + (width - (len(text) * 6 - 1) * scale) // 2,
                   y + (height - 7 * scale) // 2, text, scale, color)

    def save(self, path):
        stride = self.width * 3
        padding = bytes(-stride % 4)
        size = (stride + len(padding)) * self.height
        with path.open("wb") as file:
            file.write(struct.pack("<2sIHHI", b"BM", size + 54, 0, 0, 54))
            file.write(struct.pack("<IiiHHIIiiII", 40, self.width, self.height, 1, 24, 0, size, 0, 0, 0, 0))
            for line in range(self.height - 1, -1, -1):
                file.write(self.pixels[line * stride:(line + 1) * stride])
                file.write(padding)


def grid(bitmap, cell_size):
    return [(x, y, cell_size, cell_size) for y in range(0, bitmap.height, cell_size)
            for x in range(0, bitmap.width, cell_size)]


def packed(source, name):
    text = (source / f"uvcoord_{name}.bi").read_text(encoding="ascii")
    width, height, count = (int(re.search(rf"#define\s+\w+_{key}\s+(\d+)", text)[1])
                            for key in ("BITMAP_WIDTH", "BITMAP_HEIGHT", "NUM_IMAGES"))
    values = re.findall(r"^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\b", text, re.M)
    slots = [tuple(map(int, value)) for value in values]
    if len(slots) != count or any(x + w > width or y + h > height or w <= 0 or h <= 0 for x, y, w, h in slots):
        raise ValueError(f"Invalid {name} atlas coordinates")
    return Bitmap(width, height), slots


def skyline(bitmap, x, y, width, height, variant):
    for row in range(height):
        light = 20 + row * 28 // height + variant * 4
        bitmap.box(x, y + row, width, 1, (light - 9, light, light + 20))
    for i in range(9):
        left = width * i // 9
        right = width * (i + 1) // 9
        top = height - (height * (2 + (i * 7 + variant) % 5)) // 13
        bitmap.box(x + left, y + top, right - left, height - top, (20, 43, 65))
        for lamp in range(left + 5, right - 1, 12):
            bitmap.box(x + lamp, y + top + 4, 2, 2, (59, 144, 147))


def generate_images(output):
    images = output / "images"
    images.mkdir()
    source = output / "source"
    bitmap = Bitmap(256, 256)
    for index, (x, y, _, _) in enumerate(grid(bitmap, 16)):
        if index < 95:
            bitmap.label(x + 2, y + 1, chr(32 + index), 2, (255, 255, 255))
    bitmap.save(images / "kroma256.bmp")

    bitmap = Bitmap(256, 256)
    for index, (x, y, _, _) in enumerate(grid(bitmap, 32)):
        bitmap.box(x + 7, y + 6, 19, 22, INK)
        bitmap.box(x + 9, y + 4, 16, 20, PLAYER)
        bitmap.box(x + 13, y + 8, 12, 6, (82, 34, 74))
        bitmap.box(x + 19, y + 9, 4, 3, PAPER)
        bitmap.box(x + 9, y + 24, 5, 5 - index % 2, PLAYER)
        bitmap.box(x + 20, y + 24, 5, 4 + index % 2, PLAYER)
    bitmap.save(images / "snipe.bmp")

    bitmap = Bitmap(1024, 32)
    for index, (x, y, width, height) in enumerate(grid(bitmap, 32)):
        if index == 0:
            continue
        color = (35, 76, 109) if index in (2, 14, 15, 17) else (62, 91, 120)
        bitmap.box(x, y, width, height, color)
        bitmap.box(x, y, width, 4, MINT if index in (2, 15) else METAL)
        bitmap.box(x, y + 30, width, 2, INK)
        bitmap.box(x + 3, y + 13, width - 7, 2, METAL)
        if index == 25:
            bitmap.box(x + 2, y + 4, width - 4, 12, (205, 126, 56))
            bitmap.label(x + 12, y + 7, "+", 1)
    bitmap.save(images / "tiles.bmp")

    bitmap = Bitmap(64, 64)
    for index, (x, y, _, _) in enumerate(grid(bitmap, 16)):
        # The player's shot animation takes the last four incendiary cells.
        if index >= 12:
            bitmap.box(x + 4, y + 6, 8 + index - 12, 4, SHOT)
            bitmap.box(x + 1, y + 7, 4, 2, (230, 118, 44))
        else:
            bitmap.box(x + 3, y + 3, 10, 10, METAL)
            bitmap.label(x + 5, y + 4, str(index % 10), 1, MINT)
    bitmap.save(images / "incendiaries.bmp")

    bitmap = Bitmap(32, 32)
    bitmap.box(12, 9, 8, 14, (245, 157, 55))
    bitmap.box(8, 13, 16, 6, (245, 157, 55))
    bitmap.save(images / "flare.bmp")

    for name in ("enemies_sprite", "explosions_sprite", "gui_sprite", "seasons_sprite", "splashes"):
        bitmap, slots = packed(source, name)
        for index, (x, y, width, height) in enumerate(slots):
            if (name == "seasons_sprite" and 4 <= index < 8) or (name == "gui_sprite" and index == 10):
                skyline(bitmap, x, y, width, height, index % 4)
            elif name == "gui_sprite" and index == 0:
                bitmap.box(x + 3, y + 12, width - 6, height - 24, INK)
                bitmap.centered(x, y + 12, width, 58, "PYROMAX DAX", 5)
                bitmap.centered(x, y + 70, width, 34, "COPPER CIRCUIT WORKSHOP", 2, MINT)
            elif name == "gui_sprite" and 2 <= index < 8:
                labels = ("PLAY", "OPTIONS", "CONTROLS", "RECORDS", "CREDITS", "EXIT")
                bitmap.box(x + 5, y + 5, width - 10, height - 10, METAL)
                bitmap.centered(x, y, width, height, labels[index - 2], 2)
            elif name == "gui_sprite" and 12 <= index <= 17:
                bitmap.box(x + 2, y + 2, width - 4, height - 4, METAL)
                bitmap.centered(x, y, width, height, "WORKSHOP", 2)
            elif name == "splashes":
                bitmap.box(x + 8, y + 3, width - 16, height - 6, METAL)
                bitmap.centered(x, y, width, height, "COPPER CIRCUIT", 4, MINT)
            elif name == "explosions_sprite":
                pulse = max(2, min(width, height) // 4 + index % 4)
                bitmap.box(x + (width - pulse) // 2, y + (height - pulse) // 2, pulse, pulse, (241, 145, 71))
            elif name == "enemies_sprite" and index in (47, 48):
                # Warp.Draw places the two gate layers on either side of the player.
                if index == 47:
                    bitmap.box(x + 12, y + 13, width - 24, height - 13, INK)
                    bitmap.box(x + 18, y + 20, width - 36, height - 20, (47, 111, 121))
                else:
                    bitmap.box(x + 9, y + 8, width - 18, 9, METAL)
                    bitmap.box(x + 14, y + 11, width - 28, 3, MINT)
                    bitmap.box(x + 9, y + 15, 7, height - 15, METAL)
                    bitmap.box(x + width - 16, y + 15, 7, height - 15, METAL)
            else:
                # Transparent margins keep the silhouettes legible and
                # preserve sprite origins in the packed atlas.
                margin = min(width, height) // 6
                bitmap.box(x + margin, y + margin, width - margin * 2, height - margin * 2, METAL)
                bitmap.box(x + margin + 1, y + height // 2, max(1, width - margin * 2 - 2), 2, MINT)
        bitmap.save(images / f"{name}.bmp")


def generate_courses(output):
    maps = output / "maps"
    maps.mkdir()
    # Both routes have closed side walls to preserve column offsets when
    # the native parser trims each row. Special tokens are Windows-1252.
    width, height = 52, 22
    for stage in (1, 2):
        layout = [list("+" + " " * (width - 2) + "+") for _ in range(height)]
        for row in range(16, height):
            layout[row] = list(("#" if row == 16 else "+") * width)
        layout[15][3] = "¤"
        layout[15][12] = "Ç"
        # The gate is 64 pixels tall. Spawn it clear of the floor so its
        # first collision is with the top tile and it settles at ground level.
        if stage == 1:
            layout[13][21] = "§"
            layout[13][10:14] = list("####")
            layout[15][17] = "ƒ"
        else:
            layout[15][21] = "J"
            layout[13][39] = "§"
            layout[14][13:17] = list("####")
            layout[16][25:28] = list("@@@")
        (maps / f"level{stage:02}.txt").write_bytes(("\n".join("".join(row) for row in layout) + "\n").encode("cp1252"))
        warp = output / f"scripts/warps/level{stage:02}/warp01.txt"
        warp.parent.mkdir(parents=True)
        warp.write_text(f"SpawnTileX = 3\nSpawnTileY = 15\nLevelNum = {3 - stage}\nSeasonType = 0\n", encoding="ascii")
        dialog = output / f"scripts/dialogs/level{stage:02}/dialog01.txt"
        dialog.parent.mkdir(parents=True)
        dialog.write_text("THE LIT GATE LEADS TO THE NEXT COURSE.\n", encoding="ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("project", "profile", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    case = next(row for row in json.loads(args.profile.read_text(encoding="utf-8")) if row["slug"] == "pyromax-dax")
    dest = args.output / case["project"]
    dest.mkdir(parents=True, exist_ok=False)
    for name in {case["entry"], *case["files"], *case["units"], "LICENSE"}:
        target = dest / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(args.project / name, target)
    generate_images(dest)
    generate_courses(dest)
    revision = subprocess.check_output(["git", "-C", str(args.project), "rev-parse", "HEAD"], text=True).strip()
    notice = dest / "WORKSHOP.txt"
    notice.write_text("Pyromax Dax engine, Richard Eric M. Lope.\n"
                      f"Corpus source revision {revision}.\n"
                      "Copper Circuit uses original geometric artwork and original courses.\n"
                      "The browser workshop uses silent audio.\n", encoding="ascii")
    case.pop("preload")
    case["slug"] = "pyromax-workshop"
    case["source_origin"] = "https://github.com/relminator/Pyromax-Dax.git"
    case["notices"] = [notice.name]
    case["asset_mode"] = "preload"
    # Asyncify has to examine the whole engine during the final Wasm link.
    case["timeout"] = 660
    case["assets"] = {file.relative_to(dest).as_posix(): "/" + file.relative_to(dest).as_posix()
                      for subdir in ("images", "maps", "scripts") for file in sorted((dest / subdir).rglob("*"))
                      if file.is_file()}
    (args.output / "pyromax-workshop.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-pyromax-stage.py
