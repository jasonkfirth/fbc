#!/usr/bin/env python3
"""Stage the fb-VNC viewer and its browser WebSocket transport.

Copy the viewer and omaGUI sources, then generate exact source replacements
for the shared transport loops and browser URL entry. The corpus runner
applies the replacements while building. The upstream checkout stays intact.
"""

import argparse
import json
from pathlib import Path
import shutil


NETWORK = "src/network.bas"
WEBSOCKET = "fb-corpus-fbvnc-websocket"
UNITS = ("network", "des", "threads", "rfb", "scaler", "ui")


def replacements():
    yield NETWORK, "\nfunction NetOpen(", "\n#ifndef __FB_JS__\nfunction NetOpen("
    native_write = (
        "\nprivate function NetWriteChunk( byval handle as integer, byval buffer as const ubyte ptr, "
        "byval count as integer, byref errorText as string ) as integer\n"
        "\tput #handle, , buffer[0], count\n"
        "\tif( err <> 0 ) then\n"
        '\t\terrorText = "TCP write failed with FreeBASIC error " & str( err ) & "."\n'
        "\t\treturn 0\n\tend if\n\treturn -1\nend function\n"
    )
    yield NETWORK, "\nfunction NetReadAvailable(", (
        native_write + f'\n#else\n#include once "{WEBSOCKET}.bi"\n#endif\n\nfunction NetReadAvailable('
    )
    for variable, offset, blank in (("maximumBytes", "0", ""), ("requested", "totalRead", "\n")):
        old_read = (
            f"\t\t\tget #handle, , buffer[{offset}], {variable}, bytesRead\n{blank}"
            "\t\t\tif( err <> 0 ) then\n"
            '\t\t\t\terrorText = "TCP read failed with FreeBASIC error " & str( err ) & "."\n'
            "\t\t\t\treturn 0\n\t\t\tend if"
        )
        yield NETWORK, old_read, (
            f"\t\t\tif( NetReadSomeNow( handle, @buffer[{offset}], {variable}, bytesRead, errorText ) = 0 ) then return 0"
        )
    yield NETWORK, (
        "\t\tput #handle, , buffer[totalWritten], requested\n\n"
        "\t\tif( err <> 0 ) then\n"
        '\t\t\terrorText = "TCP write failed with FreeBASIC error " & str( err ) & "."\n'
        "\t\t\treturn 0\n\t\tend if"
    ), "\t\tif( NetWriteChunk( handle, @buffer[totalWritten], requested, errorText ) = 0 ) then return 0"

    entry = "\tdim as string value = trim( serverText )"
    yield "src/ui.bas", entry, entry + (
        "\n#ifdef __FB_JS__\n"
        '\tif( lcase( left( value, 6 ) ) <> "wss://" andalso lcase( left( value, 5 ) ) <> "ws://" ) then\n'
        '\t\terrorText = "Enter a ws:// or wss:// endpoint for the VNC server."\n'
        "\t\treturn 0\n\tend if\n"
        '\thost = value\n\tport = 0\n\terrorText = ""\n\treturn -1\n#else'
    )
    footer = "\treturn -1\nend function\n\nprivate sub DrawPanel("
    yield "src/ui.bas", footer, footer.replace("end function", "#endif\nend function")
    yield "src/ui.bas", '"VNC server", rgb(', '"WebSocket endpoint", rgb('
    yield "src/ui.bas", "Use host:display or host::port. Press Tab to change fields.", (
        "Use a ws:// or wss:// endpoint. Tab switches fields."
    )
    yield "src/main.bas", "options.serverText = VNC_DEFAULT_HOST", 'options.serverText = ""'
    yield "src/main.bas", (
        '\tprint "  host:display       Traditional VNC display number, for example host:1"\n'
        '\tprint "  host::port         Literal TCP port, for example host::5901"'
    ), '\tprint "  wss://host/path    WebSocket carrying the VNC byte stream"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("project", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    project = args.output / "fb-VNC"
    project.mkdir(parents=True)
    units = [f"src/{name}.bas" for name in UNITS]
    entry = "src/main.bas"
    files = [
        "src/*.bi", "src/omaGUI-main/omaGUI.bi", "src/omaGUI-main/src/*/*.bi",
        "src/omaGUI-main/src/*/*.bas", "src/omaGUI-main/assets/fonts/font_omagui_*.bi",
        "src/omaGUI-main/assets/fonts/font_latin1_fallback.bi", "src/omaGUI-main/LICENSE",
    ]
    for pattern in (entry, *units, *files):
        members = sorted(args.project.glob(pattern))
        if not members:
            raise ValueError(f"No files matched {pattern}")
        for member in members:
            target = project / member.relative_to(args.project)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(member, target)

    for extension in ("bi", "js"):
        name = f"{WEBSOCKET}.{extension}"
        shutil.copyfile(Path(__file__).with_name(name), project / "src" / name)
    files.append(f"src/{WEBSOCKET}.js")
    case = {
        "slug": "fb-vnc-browser",
        "project": "fb-VNC",
        "entry": entry,
        "units": units,
        "files": files,
        "include_dirs": ["src", "src/omaGUI-main"],
        "source_origin": "https://github.com/fbxl-sj0/fb-VNC.git",
        "graphics_keyboard": True,
        "timeout": 660,
        "flags": ["-d", "OMAGUI_REDISTRIBUTABLE_FONTS", "-Wl", "--js-library",
                  "-Wl", f"{WEBSOCKET}.js", "-Wl", "-sSTACK_SIZE=2097152"],
        "replacements": list(replacements()),
    }
    (args.output / "fb-vnc-browser.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-fbvnc-stage.py
