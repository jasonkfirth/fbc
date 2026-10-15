#!/usr/bin/env python3
"""FreeBASIC native variadic floating-point regression
File: test-vararg-float.py
Purpose: Verify floating argument delivery through the native C va_arg ABI.
Responsibilities: Both amd64 backends, exact values and source provenance.
This file contains no ABI emulation or compiler installation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--cc", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sources = (Path(__file__).resolve(), root / "tests/bindings/vararg-float.c",
               root / "tests/bindings/vararg-float.bas")
    records = []
    receipt = {"status": "pending", "records": records,
               "compiler_sha256": hashlib.sha256(args.fbc.resolve().read_bytes()).hexdigest(),
               "source_sha256": {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
    def save():
        (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    save()
    environment = os.environ.copy()
    tools = args.cc.resolve().parent
    environment["PATH"] = str(tools) + os.pathsep + environment["PATH"]
    suffix = ".exe" if os.name == "nt" else ""
    if os.name == "nt":
        for name in ("gcc", "as", "ld", "ar"):
            environment[name.upper()] = str(tools / (name + ".exe"))
    def compile_command(command, label):
        result = subprocess.run([str(p) for p in command], env=environment,
                                capture_output=True, text=True, timeout=30)
        (output / (label + ".log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(label + ": " + result.stdout + result.stderr)
    obj = output / "native.o"
    compile_command([args.cc.resolve(), "-Wall", "-Wextra", "-Werror", "-c", sources[1], "-o", obj], "native-compile")
    for backend in ("gcc", "gas64"):
        binary = output / ("case-" + backend + suffix)
        compile_command([args.fbc.resolve(), "-prefix", root, "-i", root / "inc", "-gen", backend,
                         sources[2], obj, "-x", binary], backend + "-compile")
        result = subprocess.run([str(binary)], env=environment, capture_output=True, text=True, timeout=5)
        records.append({"backend": backend, "status": "pass" if result.returncode == 0 and "VARARG_FLOAT=PASS" in result.stdout else "fail",
                        "exit_code": result.returncode, "output": result.stdout + result.stderr})
        save()
    receipt["status"] = "pass" if all(r["status"] == "pass" for r in records) else "failed"
    save()
    print("VARARG_FLOAT_REGRESSION=" + receipt["status"].upper(), flush=True)
    return 0 if receipt["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-vararg-float.py
