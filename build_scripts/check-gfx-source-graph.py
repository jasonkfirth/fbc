#!/usr/bin/env python3
"""FreeBASIC graphics build validation.

File: check-gfx-source-graph.py
Purpose: Keep shared graphics APIs alongside platform presentation modules.
Responsibilities: Inspect the canonical Make source selection without compiling.
This file intentionally does NOT implement source discovery or graphics APIs.
"""

import argparse
from pathlib import Path
import subprocess
import tempfile


def validate(root: Path, make: str) -> int:
    with tempfile.TemporaryDirectory(prefix="fbc-gfx-source-graph-") as temporary:
        work = Path(temporary).resolve()
        assert work.is_relative_to(Path(tempfile.gettempdir()).resolve())
        makefile = work / "GNUmakefile"
        makefile.write_text(
            "rootdir := " + root.as_posix() + "\n"
            "mkpath := " + (root / "mk").as_posix() + "\n"
            "srcdir := " + (root / "src").as_posix() + "\n"
            "TARGET_OS := riscos\n"
            "TARGET_ARCH := arm\n"
            "ARM_VER := v4\n"
            "include " + (root / "mk/source-graph.mk").as_posix() + "\n"
            ".PHONY: report\n"
            "report:\n"
            "\t$(info GFX_SOURCES=$(GFX_SRC))\n"
            "\t@:\n",
            encoding="utf-8",
        )
        # Included dependency rules must never choose a compiler object as
        # the default target of this read-only graph query.
        result = subprocess.run(
            [make, "--no-print-directory", "-s", "-f", str(makefile), "report"],
            cwd=root, capture_output=True, text=True, timeout=30,
        )
    if result.returncode:
        raise ValueError(result.stderr.strip() or "Make source graph query failed")
    rows = [line.removeprefix("GFX_SOURCES=") for line in result.stdout.splitlines()
            if line.startswith("GFX_SOURCES=")]
    if len(rows) != 1:
        raise ValueError("Make source graph query did not return one source list")
    selected = [Path(name).resolve() for name in rows[0].split()]
    # Platform files replace common units by basename. Wimp window
    # presentation must coexist with BASIC WINDOW coordinate mapping.
    required = (
        root / "src/gfxlib2/gfx_window.c",
        root / "src/gfxlib2/riscos/gfx_wimp.c",
    )
    for source in required:
        if selected.count(source.resolve()) != 1:
            raise ValueError("RISC OS graphics graph must select " +
                             source.relative_to(root).as_posix() + " exactly once")
    print("Graphics source graph: RISC OS keeps shared WINDOW and native Wimp modules")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--make", default="make")
    options = parser.parse_args()
    try:
        return validate(options.root.resolve(strict=True), options.make)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(str(error))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-gfx-source-graph.py
