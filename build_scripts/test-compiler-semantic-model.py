#!/usr/bin/env python3
"""Project: FreeBASIC compiler tests
File: test-compiler-semantic-model.py
Purpose: Run the semantic sidecar's independent contract and compiler tests.
Responsibilities: Select a compiler and emission backends; keep artifacts private.
This file intentionally does NOT contain: semantic inference or exporter code.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
import unittest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--backend", choices=("gcc", "clang", "llvm", "gas64", "gas"), action="append")
    parser.add_argument("--test", action="append", help="Run only the named test method")
    options = parser.parse_args()
    root = options.root.resolve()
    if not options.fbc.is_file():
        parser.error("Compiler does not exist: " + str(options.fbc))
    sys.path.insert(0, str(root / "tests/semantic-sidecar"))
    from test_sidecar import SidecarTests

    SidecarTests.root = root
    SidecarTests.compiler = options.fbc.resolve()
    SidecarTests.backends = options.backend or ["gcc", "clang", "llvm"]
    if options.test:
        suite = unittest.TestSuite(SidecarTests(name) for name in options.test)
    else:
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(SidecarTests)
    return 0 if unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-semantic-model.py
