#!/usr/bin/env python3
"""FreeBASIC compiler native backend regression runner.

File: test-compiler-backends.py
Purpose: Check external ABI names, inline assembly, and builtin declarations.
Responsibilities: Compile and execute focused fixtures with available backends.
This file intentionally does NOT replace the full language or corpus suites.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
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
    backends = options.backend or ["gcc"]
    clang = shutil.which(os.environ.get("CLANG") or "clang")
    if options.backend is None:
        if shutil.which(os.environ.get("LLC") or "llc"):
            backends.append("llvm")
        if clang:
            backends.append("clang")
    fixtures = [
        ("abi", "tests/compiler-support/backend-abi.bas", "backend ABI passed"),
        ("builtins", "tests/crt/builtin-backends.bas", ""),
        ("asm-registers", "tests/quirk/inline-asm-registers.bas", ""),
    ]
    with tempfile.TemporaryDirectory(prefix="fbc-backends-") as temporary:
        working = Path(temporary)
        try:
            target = subprocess.run([str(options.fbc.resolve()), "-print", "target"],
                                    cwd=working, text=True, capture_output=True,
                                    timeout=30, check=False)
            if target.returncode:
                print(f"compiler target query failed\n{target.stdout}{target.stderr}")
                return 1
            system, _, architecture = target.stdout.strip().partition("-")
            if architecture == "x86_64" and system in (
                    "linux", "android", "darwin", "freebsd", "openbsd", "netbsd",
                    "dragonfly", "solaris", "illumos", "haiku"):
                c_object = working / "backend-c-abi.o"
                c_build = subprocess.run([os.environ.get("GCC") or "gcc", "-m64", "-c",
                                          str(root / "tests/compiler-support/backend-c-abi.c"),
                                          "-o", str(c_object)], cwd=working, text=True,
                                         capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print(f"C ABI fixture compilation failed\n{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.extend((
                    ("c-abi", "tests/compiler-support/backend-c-abi.bas", "C aggregate ABI passed"),
                    ("c-abi-opt", "tests/compiler-support/backend-c-abi.bas", "C aggregate ABI passed"),
                ))
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"C ABI fixture: {error}")
            return 1
        for backend in backends:
            for name, source, expected in fixtures:
                executable = working / (backend + "-" + name)
                input_source = root / source
                if name.startswith("c-abi"):
                    input_source = working / (backend + "-" + name + ".bas")
                    shutil.copy2(root / source, input_source)
                command = [str(options.fbc.resolve()), "-prefix", str(root),
                           "-i", str(root / "inc"), "-gen", backend, "-v",
                           str(input_source), "-x", str(executable)]
                if name.startswith("c-abi"):
                    command += [str(c_object)]
                    if backend == "llvm":
                        command += ["-R"]
                if name.endswith("-opt"):
                    command += ["-O", "3"]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print(f"{backend}/{name}: compilation failed\n{built.stdout}{built.stderr}")
                        return 1
                    if backend == "llvm" and name.startswith("c-abi"):
                        # byval's alignment applies to the source pointer too.
                        # x86 tolerates many unaligned reads, so execution alone
                        # cannot establish the validity of that IR contract.
                        files = list(working.glob(input_source.stem + "*.ll"))
                        if not files:
                            print(f"{backend}/{name}: retained LLVM IR missing")
                            return 1
                        ir = files[0].read_text(encoding="utf-8")
                        slots = {match[0] for match in re.findall(
                            r"^\s*(%[\w.$-]+) = alloca [^\n]+, align (\d+)$", ir, re.M)
                                 if int(match[1]) >= 8}
                        arguments = []
                        for line in ir.splitlines():
                            if "call " in line:
                                arguments.extend(re.findall(
                                    r"byval\([^)]*\) align \d+ (%[\w.$-]+)", line))
                        if not arguments or any(arg not in slots for arg in arguments):
                            print(f"{backend}/{name}: byval argument lacks aligned local storage")
                            return 1
                    if backend == "clang":
                        # This must follow -Wall so Clang's expensive C-only
                        # warning analysis stays disabled for generated code.
                        diagnostic = built.stdout + built.stderr
                        enabled = diagnostic.find("-Wall")
                        disabled = diagnostic.find("-Wno-uninitialized")
                        if enabled < 0 or disabled < enabled:
                            print(f"{backend}/{name}: uninitialized-warning policy missing\n{diagnostic}")
                            return 1
                    executed = subprocess.run([str(executable)], cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"{backend}/{name}: {error}")
                    return 1
                if executed.returncode or executed.stdout.strip() != expected:
                    print(f"{backend}/{name}: execution failed\n{executed.stdout}{executed.stderr}")
                    return 1
                print(f"{backend}/{name}: passed")

        if "clang" in backends:
            if clang is None:
                print("clang: configured compiler is unavailable")
                return 1
            # LP64 does not determine uint64_t's C spelling. Check generated
            # prototypes without needing another target's SDK or linker.
            source = working / "builtin-target.bas"
            source.write_text('#include once "builtin.bi"\n'
                              'dim value as ulongint = &h0123456789abcdefull\n'
                              'value = __builtin_bswap64(value)\n', encoding="utf-8")
            targets = (
                ("linux", "x86_64-linux-gnu"),
                ("darwin", "x86_64-apple-darwin"),
                ("freebsd", "x86_64-unknown-freebsd"),
                ("openbsd", "x86_64-unknown-openbsd"),
                ("netbsd", "x86_64-unknown-netbsd"),
            )
            for target, triple in targets:
                command = [str(options.fbc.resolve()), "-prefix", str(root),
                           "-i", str(root / "inc"), "-gen", "clang", "-r",
                           "-target", target, "-arch", "x86_64", str(source)]
                try:
                    emitted = subprocess.run(command, cwd=working, text=True,
                                             capture_output=True, timeout=120, check=False)
                    if emitted.returncode:
                        print(f"clang/{target}: C emission failed\n{emitted.stdout}{emitted.stderr}")
                        return 1
                    checked = subprocess.run([clang, "--target=" + triple, "-fsyntax-only",
                                              "-nostdinc", str(source.with_suffix(".c"))],
                                             cwd=working, text=True, capture_output=True,
                                             timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"clang/{target}: {error}")
                    return 1
                if checked.returncode:
                    print(f"clang/{target}: C declarations failed\n{checked.stdout}{checked.stderr}")
                    return 1
                print(f"clang/{target}: C declarations passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-backends.py
