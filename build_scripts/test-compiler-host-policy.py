#!/usr/bin/env python3
"""FreeBASIC compiler host arithmetic regression runner.

File: test-compiler-host-policy.py
Purpose: Build and execute extracted host policies through each native backend.
Responsibilities: Select policy sources, retain failing output, check execution.
This file intentionally does NOT emulate DOS or a RISC OS operating system.
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
    policies = (
        ("generic", "support/numeric/fp-policy.bas", "support/numeric/fp-bits.bas", []),
        ("dos", "platform/dos/fp-policy.bas", "support/numeric/fp-bits.bas", []),
        ("riscos-bits", "support/numeric/fp-policy.bas", "platform/riscos/fp-bits.bas", ["-d", "FB_TEST_APCS_BITS"]),
    )
    with tempfile.TemporaryDirectory(prefix="fbc-host-policy-") as temporary:
        working = Path(temporary)
        for backend in backends:
            for name, arithmetic, bits, defines in policies:
                executable = working / (backend + "-" + name)
                # ASSERT requires -g; runtime error checks alone omit assertions.
                command = [str(options.fbc.resolve()), "-prefix", str(root),
                           "-i", str(root / "inc"), "-i", str(compiler), "-gen", backend,
                           "-g", "-exx", "-m", "host-float-policy", "-x", str(executable),
                           *defines, str(root / "tests/compiler-support/host-float-policy.bas"),
                           str(compiler / arithmetic), str(compiler / bits)]
                try:
                    compiled = subprocess.run(command, cwd=working, text=True, capture_output=True,
                                              timeout=120, check=False)
                    if compiled.returncode:
                        print(f"{backend}/{name}: compilation failed\n{compiled.stdout}{compiled.stderr}")
                        return 1
                    executed = subprocess.run([str(executable)], cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"{backend}/{name}: {error}")
                    return 1
                if executed.returncode or executed.stdout.strip() != "host float policy passed":
                    print(f"{backend}/{name}: execution failed\n{executed.stdout}{executed.stderr}")
                    return 1
                print(f"{backend}/{name}: passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-host-policy.py
