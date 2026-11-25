#!/usr/bin/env python3
"""Prepare VisualFBEditor's alignment command for the browser corpus.

Stage the license and the exact upstream procedure behind the port.
The desktop checkout supplies provenance and stays intact.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    dest = args.output / "VisualFBEditor"
    dest.mkdir(parents=True)
    revision = subprocess.check_output(["git", "-C", str(args.project), "rev-parse", "HEAD"], text=True).strip()
    origin = "https://github.com/XusinboyBekchanov/VisualFBEditor"
    files = ["src/TabWindow.bas", "license.txt"]
    upstream, license_text = (subprocess.check_output(
        ["git", "-C", str(args.project), "show", f"{revision}:{name}"]) for name in files)
    [method] = re.findall(rb"(?ms)^Sub TabWindow\.FormatWithBasisWord\b.*?^End Sub\r?$", upstream)
    (dest / "reference.txt").write_bytes(method + b"\n")
    (dest / "license.txt").write_bytes(license_text)
    (dest / "SOURCE.txt").write_text(
        f"Original VisualFBEditor procedure {origin}/blob/{revision}/src/TabWindow.bas\n"
        "reference.txt contains the upstream FormatWithBasisWord implementation.\n",
        encoding="utf-8",
    )
    (dest / "sample.bas").write_text(
        "Const Small = 7\nConst LargestName = 11\nConst Ratio = Small + 1\n",
        encoding="utf-8",
    )
    case = {
        "slug": "visualfb-align",
        "project": dest.name,
        "entry": "fb-corpus-visualfb-align.bas",
        "source_origin": origin,
        "adapter": True,
        "files": ["reference.txt"],
        "notices": ["SOURCE.txt"],
        "assets": {"sample.bas": "/sample.bas"},
        "flags": ["-Wl", "-sASYNCIFY=0", "-Wl", "-sEXPORTED_RUNTIME_METHODS=FS"],
    }
    (args.output / "visualfb-align.json").write_text(json.dumps([case], indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-visualfb-stage.py
