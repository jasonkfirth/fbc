#!/usr/bin/env python3
"""FreeBASIC compiler storage regression runner.

File: test-compiler-storage.py
Purpose: Exercise the real allocation, container, and compiler buffer modules.
Responsibilities: Build native backend variants and check valid and rejected inputs.
This file intentionally does NOT test language-level string representations.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--backend", choices=("gcc", "llvm", "clang"), action="append")
    options = parser.parse_args()
    root = options.root.resolve()
    compiler = root / "src/compiler"
    backends = options.backend or ["gcc"]
    if options.backend is None:
        if shutil.which(os.environ.get("LLC") or "llc"):
            backends.append("llvm")
        if shutil.which(os.environ.get("CLANG") or "clang"):
            backends.append("clang")
    sources = [root / "tests/compiler-support/storage.bas"]
    sources.extend(compiler / name for name in (
        "support/allocate.bas", "support/containers/list.bas",
        "support/containers/flist.bas", "support/containers/stack.bas",
        "support/containers/pool.bas", "support/strings/dstr.bas",
    ))
    rejected = (
        "add-overflow", "multiply-overflow", "align-overflow", "negative-size",
        "zero-allocation", "invalid-alignment", "negative-buffer",
    )
    with tempfile.TemporaryDirectory(prefix="fbc-storage-") as temporary:
        working = Path(temporary)
        for backend in backends:
            executable = working / (backend + "-storage")
            command = [str(options.fbc.resolve()), "-prefix", str(root),
                       "-i", str(root / "inc"), "-i", str(compiler), "-gen", backend,
                       "-exx", "-m", "storage", "-x", str(executable),
                       *(str(source) for source in sources)]
            try:
                built = subprocess.run(command, cwd=working, text=True, capture_output=True,
                                       timeout=120, check=False)
                if built.returncode:
                    print(f"{backend}: compilation failed\n{built.stdout}{built.stderr}")
                    return 1
                valid = subprocess.run([str(executable)], cwd=working, text=True,
                                       capture_output=True, timeout=30, check=False)
                if valid.returncode or valid.stdout.strip() != "compiler storage passed":
                    print(f"{backend}: storage failed\n{valid.stdout}{valid.stderr}")
                    return 1
                for case in rejected:
                    invalid = subprocess.run([str(executable), case], cwd=working, text=True,
                                             capture_output=True, timeout=10, check=False)
                    output = invalid.stdout + invalid.stderr
                    if invalid.returncode == 0 or "runtime error 4" not in output.lower():
                        print(f"{backend}/{case}: invalid size was not rejected safely\n{output}")
                        return 1
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"{backend}: {error}")
                return 1
            print(f"{backend}: storage and {len(rejected)} rejected-size cases passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-storage.py
