#!/usr/bin/env python3
"""Stage the pinned Tilengine sources and the FreeBASIC Mode 7 sample.

Read the Tilengine revision from the SDK and package its matching source
with the sample. The SDK builder owns the archives. Neither upstream
checkout needs to be edited.
"""

import argparse
from io import BytesIO
import json
from pathlib import Path
import re
import subprocess
import tarfile


ENGINE_ORIGIN = "https://github.com/megamarc/Tilengine"
BINDING_ORIGIN = "https://github.com/megamarc/FBTilengine"
SLUG = "tilengine-mode7"


def git(project, *arguments):
    return subprocess.check_output(["git", "-C", str(project), *arguments])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for parameter in ("engine", "binding", "sdk", "output"):
        parser.add_argument(parameter, type=Path)
    args = parser.parse_args()
    [source_notice] = (args.sdk / "lib").rglob("TILENGINE-SOURCE")
    match = re.search(r"Tilengine/tree/([0-9a-f]{40})\b", source_notice.read_text(encoding="ascii"))
    if match is None:
        raise ValueError("The SDK does not identify its Tilengine source revision")
    revision = match[1]

    project = args.output / "Tilengine"
    project.mkdir(parents=True, exist_ok=False)
    payload = git(args.engine, "archive", "--format=tar", revision, "LICENSE", "src", "include", "samples/Mode7.c")
    with tarfile.open(fileobj=BytesIO(payload)) as archive:
        archive.extractall(project, filter="data")
    binding_revision = git(args.binding, "rev-parse", "HEAD").decode("ascii").strip()
    (project / "Tilengine.bi").write_bytes(git(args.binding, "show", f"{binding_revision}:Tilengine.bi"))
    binding_license = "FBTilengine-LICENSE.txt"
    (project / binding_license).write_bytes(git(args.binding, "show", f"{binding_revision}:LICENSE"))
    source = "TILENGINE-SOURCE.txt"
    (project / source).write_text(
        f"Tilengine engine source {ENGINE_ORIGIN}/tree/{revision}\n"
        f"FreeBASIC binding {BINDING_ORIGIN}/tree/{binding_revision}\n"
        "The FreeBASIC sample adapts samples/Mode7.c and generates its own graphics.\n",
        encoding="utf-8",
    )
    case = {
        "slug": SLUG,
        "project": project.name,
        "source_origin": ENGINE_ORIGIN,
        "entry": "fb-corpus-tilengine-mode7.bas",
        "adapter": True,
        "files": ["Tilengine.bi", "samples/Mode7.c", "include/*.h", "src/*.c", "src/*.h"],
        "notices": [source, binding_license],
        "flags": ["-l", "SDL2", "-l", "png", "-l", "z"],
        "timeout": 330,
    }
    (args.output / f"{SLUG}.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-tilengine-stage.py
