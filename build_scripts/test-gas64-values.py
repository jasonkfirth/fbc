#!/usr/bin/env python3
"""FreeBASIC GAS64 value regression
File: test-gas64-values.py
Purpose: Check integer FRAC conversion and native C aggregate return stores.
Responsibilities: Both amd64 backends, exact fields, canaries and source pins.
This file contains no compiler installation or substitute C ABI.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("fbc", "cc", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    paths = [Path(__file__).resolve(), root / "tests/compiler-support/gas64-frac-integer.bas",
             root / "tests/bindings/struct-return-store.bas", root / "tests/bindings/struct-return-store.c",
             root / "tests/bindings/struct-return-packed.bas", root / "tests/bindings/struct-return-packed.c"]
    records = []
    receipt = {"status": "pending", "records": records,
               "compiler_sha256": hashlib.sha256(args.fbc.resolve().read_bytes()).hexdigest(),
               "source_sha256": {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
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
                                capture_output=True, text=True, timeout=40)
        (output / (label + ".log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(label + ": " + result.stdout + result.stderr)
    obj = output / "native.o"
    compile_command([args.cc.resolve(), "-Wall", "-Wextra", "-Werror", "-c", paths[3], "-o", obj], "native-compile")
    packed_obj = output / "packed.o"
    compile_command([args.cc.resolve(), "-Wall", "-Wextra", "-Werror", "-c", paths[5], "-o", packed_obj], "packed-native-compile")
    for backend in ("gcc", "gas64"):
        for source, marker in ((paths[1], "GAS64_FRAC_INTEGER=PASS"), (paths[2], "STRUCT_RETURN_STORE=PASS"),
                               (paths[4], "PACKED_RETURN=PASS")):
            binary = output / (source.stem + "-" + backend + suffix)
            objects = [obj] if source == paths[2] else ([packed_obj] if source == paths[4] else [])
            compile_command([args.fbc.resolve(), "-prefix", root, "-i", root / "inc", "-gen", backend,
                             source, *objects, "-x", binary], binary.stem + "-compile")
            result = subprocess.run([str(binary)], env=environment, capture_output=True, text=True, timeout=5)
            if result.returncode or marker not in result.stdout:
                raise RuntimeError(binary.stem + ": " + result.stdout + result.stderr)
            records.append({"source": source.relative_to(root).as_posix(), "backend": backend,
                            "status": "pass", "output": result.stdout})
            save()
    receipt["status"] = "pass"
    save()
    print("GAS64_NATIVE_VALUES=PASS programs=" + str(len(records)), flush=True)


if __name__ == "__main__":
    main()
# end of test-gas64-values.py
