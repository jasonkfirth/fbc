#!/usr/bin/env python3
"""Stage a generated BRPGEngine map and the converter's build profile.

The sample is generated from a small coastal grid. The upstream checkout
supplies the MIT notice and the provenance of the converter.
"""

import argparse
import json
from pathlib import Path
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    stage = args.output / "BRPGEngine"
    stage.mkdir(parents=True)
    revision = subprocess.check_output(["git", "-C", str(args.project), "rev-parse", "HEAD"], text=True).strip()
    origin = "https://github.com/fangcun010/BRPGEngine"
    (stage / "LICENSE").write_bytes((args.project / "LICENSE").read_bytes())
    (stage / "SOURCE.txt").write_text(
        f"BRPGEngine converter source {origin}/blob/{revision}/BRpg/Samples/Map2Bto4B.bas\n"
        "The port reads the compact map file directly. Its sample was generated for the browser catalog.\n",
        encoding="utf-8",
    )
    width, height = 24, 16
    sample = bytearray(struct.pack("<HH", width, height))
    for row in range(height):
        for column in range(width):
            shore = 4 + abs(row - 7) // 3
            ground = 0x8001 if column < shore else 2
            overlay = 0
            if column == shore or row == 9:
                ground = 3
            if column > shore + 2 and column not in (12, 13) and row != 9 and (row * 11 + column * 7) % 19 < 3:
                ground = 0x8004
                overlay = 5
            sample.extend(struct.pack("<HH", ground, overlay))
    (stage / "sample.map").write_bytes(sample)

    case = {
        "slug": "brpg-map",
        "project": stage.name,
        "entry": "fb-corpus-brpg-map.bas",
        "source_origin": origin,
        "adapter": True,
        "notices": ["SOURCE.txt"],
        "assets": {"sample.map": "/input.map"},
        "flags": ["-Wl", "-sASYNCIFY=0", "-Wl", "-sEXPORTED_RUNTIME_METHODS=FS"],
    }
    (args.output / "brpg-map.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-brpg-stage.py
