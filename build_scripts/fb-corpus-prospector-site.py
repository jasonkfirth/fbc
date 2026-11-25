#!/usr/bin/env python3
"""Stage a compiled Prospector for the browser checker and catalog.

Check the build hashes and package the altered sources and original notice.
The browser checker qualifies this output before the release is published.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import zipfile


def digest(file):
    with file.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def text(file, value):
    file.write_text(value, encoding="utf-8", newline="\n")


def make_archive(archive_path, build, row, scripts):
    inputs = {}
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name in sorted(row["sources"]):
            path = Path(name)
            if path.parts[0] in {"data", "config"}:
                path = Path("pack") / path
            source = (build / path).resolve()
            if not source.is_relative_to(build):
                raise ValueError(f"Invalid staged source: {path}")
            inputs[path.as_posix()] = digest(source)
            archive.write(source, path.as_posix())
        for name in ("fb-corpus-prospector.py", "fb-corpus-prospector.js", "fb-corpus-prospector.html"):
            archive.write(scripts / name, name)
        archive.writestr("inputs.json", json.dumps({"revision": row["revision"], "sources": inputs}, indent=2) + "\n")
        archive.writestr("BUILD.txt", f"""Prospector browser corpus edition, altered source

Prospector R197 by Matthias Mennel. The game and the graphics driver are
based on the RVIP port at https://github.com/roguelikes/prospector.
Corpus checkout revision: {row['revision']}

Rebuild from the unpacked archive using the FreeBASIC source tree, the staged
Windows fbc-js SDK and that SDK's Emscripten command shim:

  python fb-corpus-prospector.py --prepared . <fbc-source> <sdk> <emcc.cmd> <new-build-directory>

Serve the three prospector-core files as index.js, index.wasm and index.data,
alongside fb-corpus-prospector.js and fb-corpus-prospector.html renamed to
player.js and play.html. See README.txt for the original license and credits.
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("site", type=Path)
    parser.add_argument("stylesheet", type=Path)
    args = parser.parse_args()
    build, site = args.build.resolve(), args.site.resolve()
    scripts = Path(__file__).resolve().parent
    [row] = json.loads((build / "build-results.json").read_text(encoding="utf-8"))
    if row["status"] != "linked" or row["slug"] != "prospector":
        raise ValueError("The Prospector core has not linked")
    names = {"index." + suffix: "prospector-core." + suffix for suffix in ("js", "wasm", "data")}
    for name, source in names.items():
        if row["artifacts"].get(name, row["artifacts"].get(source)) != digest(build / source):
            raise ValueError(f"Core artifact changed: {source}")
    player = site / "prospector"
    player.mkdir(parents=True, exist_ok=False)
    for name, source in names.items():
        os.link(build / source, player / name)
    shutil.copyfile(scripts / "fb-corpus-prospector.html", player / "play.html")
    shutil.copyfile(scripts / "fb-corpus-prospector.js", player / "player.js")
    os.link(build / "README.txt", player / "license.txt")
    os.link(args.stylesheet.resolve(), site / "site.css")
    make_archive(player / "source.zip", build, row, scripts)
    text(player / "index.html", """<!doctype html>
<html lang="en">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Prospector</title>
<link rel="stylesheet" href="../site.css">
<main>
<nav><a href="../">Catalog</a></nav>
<h1>Prospector</h1>
<p>Captain a ship through an unexplored sector. Chart planets, trade at stations, and lead an away team in the original FreeBASIC game.</p>
<p>This edition uses Prospector’s character display. Use a keyboard to play and the in-game Save and Quit command to keep your run.</p>
<a class="run" href="play.html">Run in browser</a>
<footer><a href="https://github.com/roguelikes/prospector">Upstream project</a> · <a href="source.zip">Download source</a> · <a href="license.txt">License</a></footer>
</main>
</html>
<!-- end of index.html -->
""")
    text(site / "index.html", """<!doctype html>
<html lang="en">
<meta charset="utf-8">
<title>FreeBASIC browser corpus</title>
<main><ul>
<li><h2><a href="prospector/">Prospector</a></h2><p>Explore a sector, trade at stations, and lead an away team in a space roguelike.</p></li>
</ul></main>
</html>
<!-- end of index.html -->
""")
    text(site / "SHA256SUMS.txt", "".join(f"{digest(file)}  {file.relative_to(site).as_posix()}\n"
         for file in sorted(site.rglob("*")) if file.is_file()))
    app = build / "apps" / "prospector"
    app.mkdir(parents=True, exist_ok=False)
    row["artifacts"] = {}
    for name in (*names, "player.js", "index.html"):
        source = player / ("play.html" if name == "index.html" else name)
        os.link(source, app / name)
        row["artifacts"][name] = digest(source)
    text(build / "build-results.json", json.dumps([row], indent=2) + "\n")


if __name__ == "__main__":
    main()

# end of fb-corpus-prospector-site.py
