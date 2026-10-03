#!/usr/bin/env python3
"""FreeBASIC Debian package include validation.

File: check-debian-includes.py
Purpose: Check that every installed include file belongs to one Debian package.
Responsibilities: Detect missing and overlapping core and binding manifests.
This file intentionally does NOT build or install Debian packages.
"""

from __future__ import annotations

import argparse
import fnmatch
from pathlib import Path
import sys


def check_includes(root: Path) -> int:
    prefix = "usr/include/freebasic/"
    patterns = []
    for package in ("freebasic", "freebasic-bindings"):
        manifest = root / "debian" / (package + ".install")
        for line in manifest.read_text(encoding="utf-8").splitlines():
            if line.startswith(prefix):
                patterns.append((package, line[len(prefix):]))

    # install-includes copies the entire inc tree. dh_install recursively
    # copies directory entries, while dh_missing rejects unassigned files.
    includes = root / "inc"
    files = sorted(path for path in includes.rglob("*") if path.is_file())
    if not files:
        raise ValueError("the include tree contains no files")

    errors = []
    for path in files:
        name = path.relative_to(includes).as_posix()
        owners = [package for package, pattern in patterns
                  if fnmatch.fnmatchcase(name, pattern) or name.startswith(pattern + "/")]
        if not owners:
            errors.append(name + ": no Debian package owner")
        elif len(owners) != 1:
            errors.append(name + ": multiple Debian package entries (" + ", ".join(owners) + ")")
    if errors:
        raise ValueError("\n".join(errors))
    return len(files)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    args = parser.parse_args()
    try:
        count = check_includes(args.root.resolve())
    except (OSError, ValueError) as error:
        print("ERROR: " + str(error), file=sys.stderr)
        return 1
    print(f"Debian include manifests cover all {count} files exactly once")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-debian-includes.py
