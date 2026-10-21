#!/usr/bin/env python3
"""Project: FreeBASIC source hygiene
File: check-source-artifacts.py
Purpose: Reject generated analysis files inside maintained implementation trees.
Responsibilities: Check source directories without requiring a Git checkout.
This file intentionally does NOT inspect build products under obj directories.
"""

import argparse
import os
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    root = parser.parse_args().root.resolve()
    source = root / "src"
    if not source.is_dir():
        parser.error("source directory is missing: " + str(source))
    artifacts: list[str] = []
    errors: list[OSError] = []
    # Native qualification uses source archives as well as Git worktrees.
    # Inspect the actual source paths so tracked files cannot evade this check
    # through ignore rules, and an archive needs no Git metadata to run it.
    for directory, subdirectories, files in os.walk(source, onerror=errors.append):
        subdirectories[:] = [name for name in subdirectories if name != "obj"]
        for name in files:
            if name.endswith((".dump", ".ctu-info")):
                artifacts.append((Path(directory) / name).relative_to(root).as_posix())
    if errors:
        print("SOURCE_ARTIFACTS_FAIL source scan was incomplete:")
        for error in errors[:20]:
            print("  " + str(error))
        return 1
    if artifacts:
        print("SOURCE_ARTIFACTS_FAIL generated analysis files in source directories:")
        for name in sorted(artifacts)[:20]:
            print("  " + name)
        if len(artifacts) > 20:
            print("  ... " + str(len(artifacts) - 20) + " more")
        return 1
    print("SOURCE_ARTIFACTS_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of check-source-artifacts.py
