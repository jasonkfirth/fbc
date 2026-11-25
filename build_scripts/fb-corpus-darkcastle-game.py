#!/usr/bin/env python3
"""FreeBASIC browser corpus. Stage the Dark Castle game with workshop assets.

fb-corpus-darkcastle-game.py prepares a pinned source snapshot and a room
saved by the browser editor. It generates the replacement art and build
profile. The original game owns its menus, map loading and movement.
"""

import argparse
import csv
from io import BytesIO
import json
from pathlib import Path
import struct
import subprocess
import tarfile
from zipfile import ZipFile


REVISION = "e418191f7244f4a9ba13c933663d96b314571981"
ORIGIN = "https://github.com/diegomarin75-projects/DarkCastle"


def pcx(path, width, height, palette, pixels):
    # The engine's PCX decoder reads an indexed plane and a trailing RGB
    # palette. These even-width rows use indices below the RLE marker.
    header = bytearray(128)
    header[:4] = bytes((10, 5, 1, 8))
    struct.pack_into("<HH", header, 8, width - 1, height - 1)
    header[65] = 1
    struct.pack_into("<HH", header, 66, width, 1)
    colors = bytes(channel for color in palette for channel in color)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + pixels + b"\x0c" + colors.ljust(768, b"\0"))


def workshop_assets(project, room):
    maps = project / "maps"
    maps.mkdir()
    text = room.read_text(encoding="ascii")
    header, body = text.split("\n", 1)
    fields = next(csv.reader([header]))
    if len(fields) != 17 or fields[2:5] != ["9", "9", "16"] or fields[-1].strip() != "Stone workshop":
        raise ValueError("Supply the Stone workshop map saved by the browser editor")
    # MapWrite stores the named entry flag immediately before the room name.
    # Start beside the raised floor so every direction has room to move.
    fields[11:13] = ["4", "6"]
    fields[15] = "1"
    (maps / "workshop.map").write_text(",".join(fields[:-1]) + ',"' + fields[-1] + '"\n' + body,
                                      encoding="ascii", newline="\n")

    assets = {"data/objects.dat": "/objects.dat", "maps/workshop.map": "/maps/workshop.map"}
    for folder, filename, accent in (("wall", "wall0001.pcx", 6), ("ground", "wall0002.pcx", 20)):
        pixels = bytes(1 if y % 8 == 0 or (x + y // 8 % 2 * 8) % 16 == 0 else 2 + (x * 3 + y * 7) % 3
                       for y in range(64) for x in range(64))
        shades = [(0, 0, 0)] + [(24 + 11 * i + accent, 34 + 12 * i, 36 + 10 * i) for i in range(1, 5)]
        path = f"textures/{folder}/{filename}"
        pcx(project / path, 64, 64, shades, pixels)
        assets[path] = "/" + path

    # An original amber marker gives the navigation demo a visible actor.
    # Index zero uses the same magenta transparency key as the PCX renderer.
    marker = bytes(2 if 7 <= x <= 16 and 6 <= y <= 13 else
                   1 if 5 <= x <= 18 and 12 <= y <= 25 else
                   3 if 6 <= x <= 17 and 25 < y <= 30 else 0
                   for y in range(32) for x in range(24))
    actor = "textures/entity/warden.pcx"
    pcx(project / actor, 24, 32, [(255, 0, 255), (242, 157, 47), (255, 215, 118), (75, 49, 45)], marker)
    assets[actor] = "/" + actor
    states = ",".join(["WARDEN", "WALK", "01", "1"] + ["warden.pcx"] * 8 + ["0"] * 8)
    (project / "data/etypes.dat").write_text("1\nWARDEN,warden.pcx,1,0,0\n" + states + "\n", encoding="ascii")
    (project / "data/linkpoints.dat").write_bytes(b"")
    assets.update({"data/etypes.dat": "/etypes.dat", "data/linkpoints.dat": "/linkpoints.dat"})
    return assets


def browser_changes(project, editor):
    # Reuse the editor's filesystem and texture fixes where both players
    # call the same routines. The game owns its own entry map and menu.
    changes = [line for line in editor["replacements"] if line[0] != "src/gamlib.bas"
               or "Enumerate maps" in line[2] or "timing display" in line[2]]
    main = (project / "src/DCastle.bas").read_text(encoding="latin-1")
    start = main.index("  pipefile = FREEFILE\n", main.index("SUB ScanEntryPoints\n"))
    end = main.index("  numepm = i", start)
    scan = '''  filename = DIR("maps/*.map")
  WHILE filename <> "" ANDALSO i < MAXENTRY
    MapReadAttr(filename,rattr)
    IF rattr.epmap = 1 THEN
      epmap(i) = filename
      epnam(i) = rattr.mname
      i = i + 1
    ENDIF
    filename = DIR()
  WEND
'''
    conlib = (project / "src/conlib.bas").read_text(encoding="latin-1")
    start_font = conlib.index("SUB PCXFPrintStrVS(")
    end_font = conlib.index("END SUB", start_font)
    font = r'''SUB PCXFPrintStrVS(px AS INTEGER, py AS INTEGER, strn AS STRING)
  DIM AS INTEGER position = px
  IF pcxcenter = 1 THEN position = (AX0 - LEN(strn) * 8) \ 2
  DRAW STRING (position, py), strn, modcol
'''
    # The native wait loop consumes INKEY events before the map can sample
    # held keys again. Poll timers and yield once per map tick in the browser.
    map_input = """    AutomSubExecute(0)
    SLEEP 10, 1"""
    return [*changes,
            ["src/DCastle.bas", "  InitPcxFonts()\n", ""],
            ["src/DCastle.bas", "  SCREEN 18,16,3,FB.GFX_FULLSCREEN", "  IF SCREENRES(AX0,AY0,32,3) <> 0 THEN END 1"],
            ["src/DCastle.bas", main[start:end], scan],
            ["src/DCastle.bas", "MapExecute(epmap(mcode-1000))", "MapExecute(RTRIM(epmap(mcode-1000)))"],
            ["src/DCastle.bas", "  MenuFadeScreen(1,0,30,1)", "  SCREENCOPY 1,0", 1],
            ["src/DCastle.bas", "  MenuFadeScreen(1,0,30,0)", "", 1],
            ["src/menlib.bas", "    SetBackground(bfile)", "    LINE (0,0)-(AX0-1,AY0-1), RGB(26,31,39), BF"],
            ["src/menlib.bi", "RGB(0,0,180)", "RGB(205,214,226)"],
            ["src/pcxlib.bi", "CONST TRANCOLHI = &HFF00FF", "CONST TRANCOLHI = RGB(255,0,255)"],
            ["src/conlib.bas", conlib[start_font:end_font], font],
            ["src/gamlib.bas", "'Force infobox refresh flag",
             "' The game owns page 2 as a full-screen cache while a room is active.\n"
             "DIM SHARED AS INTEGER gameRoomActive\n\n'Force infobox refresh flag"],
            ["src/gamlib.bas", "  WINDOWTITLE \"DARK CASTLE v1.0\"",
             "  gameRoomActive = 1\n  WINDOWTITLE \"DARK CASTLE v1.0\""],
            ["src/gamlib.bas", "      _MapWInitView()\n      _MapWStoreWindow(2)",
             "      _MapWInitView()\n      SCREENCOPY 1,2"],
            ["src/gamlib.bas", "\nEND SUB\n\n'Refresh daemon",
             "\n  gameRoomActive = 0\nEND SUB\n\n'Refresh daemon"],
            ["src/gamlib.bas", "  _MapWRestoreWindow(2)",
             "  IF gameRoomActive THEN\n    SCREENCOPY 2,1\n  ELSE\n    _MapWRestoreWindow(2)\n  ENDIF"],
            ["src/gamlib.bas", "    IF code = 0 THEN AutomSubExecute(1)", map_input],
            ["src/gamlib.bas", "        CASE -134,-107 'F12,Close window -> exit",
             "        CASE 27,-134,-107 'Escape, F12, or Close exits the room"],
            ["src/maplib.bas", '  IF cmap.object <> "" AND cmap.objheight < 2 THEN',
             "  ' Map files store an empty object name as a space-padded field.\n"
             '  IF RTRIM(cmap.object) <> "" AND cmap.objheight < 2 THEN'],
            ["src/maplib.bas", "  cmap = cmapptr[cx*(MAXCELLY+1)+cy]",
             "  IF cx < 0 OR cy < 0 OR cx >= rattrptr->ax OR cy >= rattrptr->ay THEN EXIT FUNCTION\n"
             "  cmap = cmapptr[cx*(MAXCELLY+1)+cy]"]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repository", "editor_source", "room", "profile", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    project = args.output / "DarkCastle"
    project.mkdir(parents=True)
    snapshot = subprocess.check_output(["git", "-C", str(args.repository), "archive", REVISION,
                                        "src", "LICENSE", "data/objects.dat"])
    with tarfile.open(fileobj=BytesIO(snapshot)) as archive:
        archive.extractall(project, filter="data")
    # The tested editor archive already contains the modern FreeBASIC fixes
    # for the shared engine. Retain the upstream game's separate main unit.
    with ZipFile(args.editor_source) as archive:
        for file in (project / "src").iterdir():
            if file.suffix in (".bas", ".bi") and file.name != "DCastle.bas":
                file.write_bytes(archive.read("src/" + file.name))
    assets = workshop_assets(project, args.room)
    (project / "SOURCE.txt").write_text(f"Dark Castle game {ORIGIN}/tree/{REVISION}\n"
                                       "The shared engine comes from the Dark Castle Map Editor source release.\n"
                                       "The browser build uses a workshop map and original replacement art.\n",
                                       encoding="utf-8")
    editor = next(row for row in json.loads(args.profile.read_bytes()) if row["slug"] == "darkcastle-editor")
    case = {"slug": "darkcastle", "project": project.name, "entry": "src/DCastle.bas", "source_origin": ORIGIN,
            "files": ["src/*.bi"], "units": ["src/*.bas"], "notices": ["SOURCE.txt"],
            "graphics_keyboard": True, "timeout": editor["timeout"], "flags": editor["flags"][2:] + ["-Wl", "-O0"],
            "assets": assets, "replacements": browser_changes(project, editor)}
    (args.output / "darkcastle-game.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-darkcastle-game.py
