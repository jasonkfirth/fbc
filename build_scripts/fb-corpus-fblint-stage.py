#!/usr/bin/env python3
"""Stage the local fblint player beside a compiled FreeBASIC browser build.

The player is served from the build directory. This script does not publish
the corpus snapshot or change its source or compiler outputs.
"""

import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    receipt = args.build / "build-results.json"
    rows = json.loads(receipt.read_text(encoding="utf-8"))
    [row] = [record for record in rows if record["slug"] == "fblint" and record["status"] == "linked"]
    directory = args.build / "apps" / "fblint"
    scripts = Path(__file__).resolve().parent
    for name in ("lint.html", "lint.js", "lint-worker.js"):
        content = (scripts / ("fb-corpus-fb" + name)).read_bytes()
        (directory / name).write_bytes(content)
        row["artifacts"][name] = hashlib.sha256(content).hexdigest()
    receipt.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

# end of fb-corpus-fblint-stage.py
