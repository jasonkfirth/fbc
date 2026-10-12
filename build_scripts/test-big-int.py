#!/usr/bin/env python3
"""FreeBASIC native big_int regression checks
File: test-big-int.py
Purpose: Compare the binding with release headers and execute real library calls.
Responsibilities: Bounded processes, fresh outputs and explicit ABI receipts.
This file contains no library download, installation or guessed C layout.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def run(command, directory, environment, log):
    try:
        result = subprocess.run([str(arg) for arg in command], cwd=directory,
                                env=environment, capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired as error:
        def decoded(value):
            return value.decode("utf-8", errors="replace") if isinstance(value, bytes) else value or ""
        log.write_text(decoded(error.stdout) + decoded(error.stderr), encoding="utf-8")
        raise
    log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode or re.search(r"\berror [0-9]+:", result.stdout + result.stderr):
        raise RuntimeError("Command failed: " + str(log))
    return result.stdout


def abi(text):
    line = next((line for line in text.splitlines() if line.startswith("ABI ")), None)
    if line is None:
        raise ValueError("No ABI record was emitted")
    values = [int(value) for value in re.findall(r"\d+", line)]
    if len(values) != 12:
        raise ValueError("Incomplete ABI record")
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--headers", type=Path, required=True)
    parser.add_argument("--libdir", type=Path, required=True)
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--fbc-arg", action="append", default=[])
    parser.add_argument("--cc-arg", action="append", default=[])
    parser.add_argument("--backend", nargs="+", choices=("gcc", "gas", "gas64"), default=["gcc", "gas64"])
    parser.add_argument("--output", type=Path, default=ROOT / "out/big-int-native")
    args = parser.parse_args()
    headers = args.headers.resolve()
    library = args.libdir.resolve()
    if not (headers / "big_int.h").is_file() or not library.is_dir():
        parser.error("Matching C headers and a native library directory are required")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    windows = os.name == "nt"
    variable = "PATH" if windows else "LD_LIBRARY_PATH"
    environment[variable] = str(library) + os.pathsep + environment.get(variable, "")
    suffix = ".exe" if windows else ""
    reference = output / ("reference" + suffix)
    reference.unlink(missing_ok=True)
    run([args.cc, *args.cc_arg, "-I", headers, ROOT / "tests/bindings/big-int-abi.c", "-o", reference],
        output, environment, output / "reference-build.log")
    expected = abi(run([reference], output, environment, output / "reference-run.log"))
    results = []
    for backend in args.backend:
        executable = output / ("big-int-" + backend + suffix)
        executable.unlink(missing_ok=True)
        run([args.fbc.resolve(), *args.fbc_arg, "-i", ROOT / "inc", "-p", library,
             "-w", "all", "-gen", backend, ROOT / "tests/bindings/big-int-abi.bas", "-x", executable],
            output, environment, output / (backend + "-build.log"))
        text = run([executable], output, environment, output / (backend + "-run.log"))
        actual = abi(text)
        passed = actual == expected and "BIG_INT_NATIVE_PASS" in text.splitlines()
        results.append(dict(backend=backend, reference=expected, actual=actual, passed=passed))
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(results, indent=2))
    return int(not all(result["passed"] for result in results))


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-big-int.py
