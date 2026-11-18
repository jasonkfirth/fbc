#!/usr/bin/env python3
"""
Project: FreeBASIC compiler regression tests
File: llvm-backend-regressions.py
Purpose: Run native LLVM backend checks on Linux x86-64.
Responsibilities: Check the conversion and layout fixtures, varargs diagnostics,
and DWARF in the linked debug fixture.
This file intentionally does not contain platform selection or compiler code.
"""

from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory


def run(argv, directory, expected_failure=None):
    result = subprocess.run(
        argv, cwd=directory, check=False, capture_output=True, text=True, timeout=30
    )
    output = result.stdout + result.stderr
    if expected_failure is None:
        if result.returncode != 0 or "ignoring invalid debug info" in output:
            raise RuntimeError(f"{argv!r} exited {result.returncode}\n{output}")
    elif result.returncode == 0 or expected_failure not in output:
        raise RuntimeError(f"{argv!r} did not report {expected_failure!r}\n{output}")
    return output


def main():
    compiler = sys.argv[1:]
    if compiler and compiler[0] == "--":
        compiler.pop(0)
    if not compiler:
        raise SystemExit("pass the FreeBASIC compiler command after --")

    source_dir = Path(__file__).resolve().parent
    with TemporaryDirectory(prefix="fbc-llvm-regressions-") as temporary:
        directory = Path(temporary)
        fixtures = (
            "llvm-conversions.bas",
            "llvm-packed-layout.bas",
            "llvm-legacy-varargs.bas",
            "llvm-source-debug.bas",
            "llvm-source-debug.bi",
        )
        for name in fixtures:
            shutil.copyfile(source_dir / name, directory / name)

        options = [*compiler, "-gen", "llvm"]
        for source in fixtures[:3]:
            for level in ("0", "2"):
                run([*options, "-O", level, source, "-x", "check"], directory)
                run(["./check"], directory)

        for name in ("VA_FIRST", "VA_ARG", "VA_NEXT"):
            run(
                [*options, "-d", f"TEST_{name}", "-r", "llvm-legacy-varargs.bas"],
                directory,
                expected_failure="Legacy varargs are unsupported",
            )

        run([*options, "-g", "-O", "0", "llvm-source-debug.bas", "-x", "debug"], directory)
        if run(["./debug"], directory).strip() != "43":
            raise RuntimeError("unexpected output from the debug fixture")
        info = run(["readelf", "--debug-dump=info", "debug"], directory)
        for name in ("DW_TAG_compile_unit", "DEBUG_PROBE", "ARGUMENT", "LOCAL_VALUE",
                     "DW_TAG_lexical_block", "INNER_VALUE", "PAYLOAD", "ITEMS"):
            if name not in info:
                raise RuntimeError(f"missing {name} from the linked DWARF")
        lines = run(["readelf", "--debug-dump=decodedline", "debug"], directory)
        for name in ("llvm-source-debug.bas", "llvm-source-debug.bi"):
            if name not in lines:
                raise RuntimeError(f"missing {name} from the linked line table")

    print("LLVM conversions, aggregate layout, varargs and source debug checks passed")


if __name__ == "__main__":
    main()

# end of llvm-backend-regressions.py
