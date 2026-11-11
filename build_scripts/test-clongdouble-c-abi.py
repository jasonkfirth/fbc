#!/usr/bin/env python3
"""
Project: FreeBASIC compiler tests
File: test-clongdouble-c-abi.py
Purpose: Check C ABI calls for FreeBASIC's CLongDouble storage type.
Responsibilities: Compile a C caller and verify CLongDouble arguments and
  returns across the available native, C, and LLVM backends.
This file intentionally does NOT contain compiler ABI lowering code.
"""

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def run(arguments, working, environment=None):
    result = subprocess.run(
        [str(argument) for argument in arguments],
        cwd=working,
        env=environment,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    if result.returncode:
        command = shlex.join([str(argument) for argument in arguments])
        raise RuntimeError(
            f"exit {result.returncode}: {command}\n"
            f"{result.stdout}{result.stderr}"
        )
    return result.stdout


def check_backend(fbc, cc, include_directory, working, backend, options):
    support_source = working / "clongdouble-c-caller.c"
    support_source.write_text(
        "extern long double fbClongDoubleBasicReturn(long double value);\n"
        "long double fbClongDoubleFromC(long double value) {\n"
        "    return value + 0.5L;\n"
        "}\n"
        "int fbClongDoubleCCaller(void) {\n"
        "    return fbClongDoubleBasicReturn(1.0L) > 1.0L;\n"
        "}\n"
    )
    support_object = working / f"clongdouble-c-caller-{backend}.o"
    run(cc + [
        "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
        "-c", str(support_source), "-o", str(support_object),
    ], working)

    basic_source = working / f"clongdouble-c-abi-{backend}.bas"
    basic_source.write_text(
        '#include once "crt/longdouble.bi"\n'
        'Declare Function fbClongDoubleFromC CDecl Alias "fbClongDoubleFromC" '
        "(ByVal value As CLongDouble) As CLongDouble\n"
        'Declare Function fbClongDoubleCCaller CDecl Alias "fbClongDoubleCCaller" '
        "() As Long\n"
        'Function fbClongDoubleBasicReturn CDecl Alias "fbClongDoubleBasicReturn" '
        "(ByVal value As CLongDouble) As CLongDouble\n"
        "    Return value + CLongDoubleFromStringValue(StrPtr(!\"0.5\"))\n"
        "End Function\n"
        'Dim inputValue As CLongDouble = CLongDoubleFromStringValue(StrPtr(!"1.0"))\n'
        'Dim expectedValue As CLongDouble = CLongDoubleFromStringValue(StrPtr(!"1.5"))\n'
        "Dim resultValue As CLongDouble = fbClongDoubleFromC(inputValue)\n"
        "If resultValue <> expectedValue Then End 1\n"
        "If fbClongDoubleCCaller() <> 1 Then End 2\n"
        "End 0\n"
    )
    executable = working / f"clongdouble-c-abi-{backend}"
    run(fbc + options + [
        "-w", "all", "-i", str(include_directory),
        str(basic_source), str(support_object), "-x", str(executable),
    ], working)
    run([str(executable)], working)
    print(f"CLONGDOUBLE_C_ABI={backend.upper()}: PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", default="fbc")
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()

    root = args.root.resolve()
    include_directory = root / "inc"
    if not include_directory.is_dir():
        parser.error(f"FreeBASIC include directory is missing: {include_directory}")
    fbc = shlex.split(args.fbc)
    cc = shlex.split(args.cc)
    if not fbc or not cc:
        parser.error("--fbc and --cc must each contain a command")

    with tempfile.TemporaryDirectory(prefix="fbc-clongdouble-c-abi-") as temporary:
        working = Path(temporary)
        backends = [
            ("native", []),
            ("gcc", ["-gen", "gcc"]),
        ]
        llc = os.environ.get("LLC", "").strip() or "llc"
        if shutil.which(llc):
            backends.append(("llvm", ["-gen", "llvm"]))
        else:
            print("CLONGDOUBLE_C_ABI=LLVM: SKIP (llc not found)")

        for backend, options in backends:
            check_backend(fbc, cc, include_directory, working, backend, options)

        # Linux i386 returns native long double in x87 ST(0). Compile without
        # linking so the test does not require 32-bit system libraries.
        x86_object = working / "clongdouble-c-abi-linux-x86.o"
        x86_environment = os.environ.copy()
        x86_environment["AS"] = shutil.which("as") or "as"
        x86_environment["LD"] = shutil.which("ld") or "ld"
        x86_environment["GCC"] = cc[0]
        run(fbc + [
            "-target", "linux-x86", "-gen", "gas", "-w", "all", "-c",
            "-i", str(include_directory),
            str(working / "clongdouble-c-abi-native.bas"),
            "-o", str(x86_object),
        ], working, x86_environment)
        if not x86_object.is_file() or x86_object.stat().st_size == 0:
            raise RuntimeError("FBC did not produce the Linux i386 CLongDouble object")
        print("CLONGDOUBLE_C_ABI=GAS32: PASS (compile)")


if __name__ == "__main__":
    main()

# end of test-clongdouble-c-abi.py
