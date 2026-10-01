#!/usr/bin/env python3
"""FreeBASIC compiler log harness regression tests.

File: test-compiler-test-harness.py
Purpose: Ensure passing, failing, incomplete, and timed out logs report status.
Responsibilities: Exercise the real make rules in an isolated temporary test tree.
This file intentionally does NOT run the complete language suite.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def test_compiler_selection(root: Path, working: Path) -> bool:
    """A compiler built by this invocation must replace the native fallback."""
    working.mkdir()
    (working / ".maketests-host").mkdir()
    for path, label in ((working / "new-fbc", "built"),
                        (working / ".maketests-host/fbc", "preserved")):
        path.write_text("#!/bin/sh\nprintf '%s\\n' '" + label + "'\n", encoding="utf-8")
        path.chmod(0o755)
    makefile = working / "GNUmakefile"
    makefile.write_text(
        "FBC_EXE := bin/fbc\nCC := gcc\nCROSS_BUILD :=\n"
        "include " + str(root / "mk/tests/harness.mk") + "\n"
        "all: bin/fbc\n\t@$(TEST_FBC_CMD)\n"
        "fallback:\n\t@$(TEST_FBC_CMD)\n"
        "bin/fbc:\n\t@mkdir -p bin\n\t@cp new-fbc bin/fbc\n",
        encoding="utf-8",
    )
    for label, arguments, expected in (
        ("native fallback", ["fallback"], "preserved"),
        ("compiler built by prerequisite", ["all"], "built"),
        ("cross build host compiler", ["all", "CROSS_BUILD=yes"], "preserved"),
    ):
        result = subprocess.run(["make", "--no-print-directory", "-s", *arguments],
                                cwd=working, text=True, capture_output=True,
                                timeout=30, check=False)
        if result.returncode != 0 or result.stdout.strip() != expected:
            print(label + ": wrong compiler selection\n" + result.stdout + result.stderr)
            return False
        print(label + ": passed")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    options = parser.parse_args()
    root = options.root.resolve()
    with tempfile.TemporaryDirectory(prefix="fbc-log-harness-") as temporary:
        if not test_compiler_selection(root, Path(temporary) / "selection"):
            return 1
        working = Path(temporary) / "tests"
        (working / "cases").mkdir(parents=True)
        for name in ("common.mk", "log-tests.mk"):
            shutil.copyfile(root / "tests" / name, working / name)
        (working / "dirlist.mk").write_text("DIRLIST_FB := cases\n", encoding="utf-8")
        fixture = working / "cases/probe.bas"
        fixture.write_text("'' TEST_MODE : COMPILE_ONLY_OK\nend 0\n", encoding="utf-8")
        # Paths passed by tests/common.mk must refer to the actual repository,
        # while generated sources, objects, and logs remain in the fixture.
        fbc = str(options.fbc.resolve()) + " -prefix " + str(root) + " -i " + str(root / "inc")
        command = ["make", "--no-print-directory", "-s", "-f", "log-tests.mk",
                   "FB_LANG=fb", "FBC=" + fbc, "rootdir=" + str(root)]

        def run(expected_success: bool, label: str, *targets: str) -> bool:
            result = subprocess.run(command + list(targets), cwd=working, text=True,
                                    capture_output=True, timeout=60, check=False)
            if (result.returncode == 0) != expected_success:
                print(label + ": wrong make status\n" + result.stdout + result.stderr)
                return False
            print(label + ": passed")
            return True

        if not run(True, "passing test", "all", "results"):
            return 1
        (working / "cases/probe.log").write_text("", encoding="utf-8")
        (working / "log-tests-results-fb.log").unlink()
        if not run(False, "incomplete record", "results"):
            return 1
        fixture.write_text("'' TEST_MODE : COMPILE_ONLY_OK\n#error deliberate harness failure\n", encoding="utf-8")
        (working / "cases/probe.log").unlink()
        if not run(False, "failing test", "all", "results"):
            return 1

        if shutil.which("timeout") is not None:
            tool = subprocess.run(["timeout", "--version"], capture_output=True,
                                  timeout=5, check=False)
            if tool.returncode == 0:
                fixture.write_text("'' TEST_MODE : COMPILE_ONLY_FAIL\nend 0\n", encoding="utf-8")
                for name in ("cases/probe.log", "log-tests-fb.inc",
                             "log-tests-results-fb.log"):
                    (working / name).unlink(missing_ok=True)
                # This process stands in for a compiler stuck on invalid input.
                # Its timeout must not satisfy COMPILE_ONLY_FAIL's exit-1 rule.
                sleeper = working / "sleep-compiler.sh"
                sleeper.write_text("#!/bin/sh\nexec sleep 5\n", encoding="utf-8")
                sleeper.chmod(0o755)
                if not run(False, "compiler timeout", "FBC=" + str(sleeper),
                           "LOG_TEST_TIMEOUT=1", "all", "results"):
                    return 1
                if "unexpected fbc exit code 124" not in (working / "cases/probe.log").read_text():
                    print("compiler timeout: missing timeout record")
                    return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-test-harness.py
