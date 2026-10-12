#!/usr/bin/env python3
"""FreeBASIC native JNI regression checks
File: test-jni.py
Purpose: Compare the binding with JDK headers and execute actual VM calls.
Responsibilities: Bounded subprocesses, isolated products and explicit receipts.
This file contains no JDK installation or guessed JNI declarations.
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
    result = subprocess.run([str(arg) for arg in command], cwd=directory,
                            env=environment, capture_output=True, text=True, timeout=60)
    log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
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
    parser.add_argument("--java-home", type=Path, required=True)
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--fbc-arg", action="append", default=[])
    parser.add_argument("--backend", nargs="+", choices=("gcc", "gas64"), default=["gcc", "gas64"])
    parser.add_argument("--output", type=Path, default=ROOT / "out/jni-native")
    args = parser.parse_args()
    args.java_home = args.java_home.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    windows = os.name == "nt"
    includes = [args.java_home / "include", args.java_home / "include" / ("win32" if windows else "linux")]
    library = args.java_home / ("lib" if windows else "lib/server")
    runtime = args.java_home / ("bin/server" if windows else "lib/server")
    variable = "PATH" if windows else "LD_LIBRARY_PATH"
    environment[variable] = str(runtime) + os.pathsep + str(args.java_home / "bin") + os.pathsep + environment.get(variable, "")
    suffix = ".exe" if windows else ""
    reference = output / ("reference" + suffix)
    reference.unlink(missing_ok=True)
    run([args.cc, *(arg for directory in includes for arg in ("-I", directory)),
         ROOT / "tests/bindings/jni-abi.c", "-o", reference], output, environment, output / "reference-build.log")
    expected = abi(run([reference], output, environment, output / "reference-run.log"))
    results = []
    for backend in args.backend:
        executable = output / ("jni-" + backend + suffix)
        executable.unlink(missing_ok=True)
        run([args.fbc.resolve(), *args.fbc_arg, "-i", ROOT / "inc", "-p", library,
             "-gen", backend, ROOT / "tests/bindings/jni-abi.bas", "-x", executable],
            output, environment, output / (backend + "-build.log"))
        text = run([executable], output, environment, output / (backend + "-run.log"))
        actual = abi(text)
        passed = actual == expected and "JNI_NATIVE_PASS" in text.splitlines()
        results.append(dict(backend=backend, reference=expected, actual=actual, passed=passed))
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(results, indent=2))
    return int(not all(result["passed"] for result in results))


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-jni.py
