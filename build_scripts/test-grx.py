#!/usr/bin/env python3
"""
FreeBASIC GRX Win32 and Linux X11 qualification
File: test-grx.py
Purpose: Compare the genuine SDK's C and FreeBASIC layouts and native pixels.
Responsibilities: Bounded GUI runs, displayed colors and compiler profiles.
This file contains no SDK build, replacement renderer or keyboard injection.
"""
import argparse
from contextlib import nullcontext
from sdl3_xvfb import private_display
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def run(arguments, directory, environment):
    result = subprocess.run([str(value) for value in arguments], cwd=directory,
                            env=environment, capture_output=True, text=True,
                            timeout=30, check=False)
    if result.returncode:
        raise RuntimeError(f"exit {result.returncode}: {arguments}\n{result.stdout}{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--sdk-prefix", required=True, type=Path)
    parser.add_argument("--runtime-prefix", required=True, type=Path)
    parser.add_argument("--tool-dir", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    windows = os.name == "nt"
    suffix = ".exe" if windows else ""
    platform = "Win32" if windows else "Linux i386 X11"
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    prefix = args.sdk_prefix.resolve()
    tools = args.tool_dir.resolve()
    fbc = args.fbc.resolve()
    environment = os.environ.copy()
    environment["PATH"] = str(tools) + os.pathsep + environment["PATH"]
    environment["FB_EXAMPLE_AUTOPLAY"] = "1"
    for name in ("gcc", "as", "ld", "ar"):
        environment[name.upper()] = str(tools / (name + suffix))
    context = nullcontext((environment, {})) if windows else private_display(output, environment)
    with context as (environment, display):
        receipt = qualify(args, root, output, prefix, tools, fbc, environment, windows, suffix, platform, display)

    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")


def qualify(args, root, output, prefix, tools, fbc, environment, windows, suffix, platform, display):
    native = output / ("grx-c" + suffix)
    c_flags = [] if windows else ["-m32"]
    libraries = ["-lgrx20", "-lgdi32", "-luser32"] if windows else ["-lgrx20X", "-lX11", "-lm"]
    run([tools / ("gcc" + suffix), *c_flags, "-Wall", "-Wextra", "-Werror", "-I" + str(prefix / "include"),
         root / "tests/bindings/grx-native.c", "-L" + str(prefix / "lib"), *libraries, "-o", native], output, environment)
    reference = re.findall(r"\d+", run([native], output, environment))
    assert len(reference) == 10 and reference[:3] == ["4", "4", "2"]
    assert "GRX_NATIVE_WINDOW=PASS colors=256" in run([native, "window"], output, environment)
    helper = []
    if not windows:
        helper_object = output / "grx-x11-check.o"
        run([tools / "gcc", "-m32", "-Wall", "-Wextra", "-Werror", "-DGRX_CHECK_ONLY",
             "-I" + str(prefix / "include"), "-c", root / "tests/bindings/grx-native.c",
             "-o", helper_object], output, environment)
        helper = [helper_object]
    records = []
    for backend in ("gcc", "gas"):
        flags = [fbc, "-target", "win32" if windows else "linux", "-buildprefix", str(tools) + "/",
                 "-prefix", args.runtime_prefix.resolve(), "-i", root / "inc", "-p", prefix / "lib",
                 "-gen", backend, "-w", "all"]
        if not windows:
            flags += ["-arch", "686", "-d", "GRX_USE_X11"]
        probe = output / ("grx-fb-" + backend + suffix)
        run(flags + [root / "tests/bindings/grx-native.bas", *helper, "-x", probe], output, environment)
        assert re.findall(r"\d+", run([probe], output, environment)) == reference
        assert "GRX_FB_WINDOW=PASS colors=256" in run([probe, "window"], output, environment)
        binary = output / ("grx-example-" + backend + suffix)
        run(flags + [root / "examples/graphics/grx/grx.bas", "-x", binary], output, environment)
        assert "GRX color grid completed." in run([binary], output, environment)
        records.append({"backend": backend, "ABI": "pass", "native_window_pixels": 256,
                        "original_example": "pass"})
        print("GRX_NATIVE=PASS " + platform + " " + backend, flush=True)
    receipt = {"status": "pass", "platform": platform, "native_C_ABI_facts": reference,
               "native_C_window_pixels": 256, "records": records,
               "compiler_sha256": hashlib.sha256(fbc.read_bytes()).hexdigest(),
               "library_sha256": hashlib.sha256((prefix / ("lib/libgrx20.a" if windows else "lib/libgrx20X.a")).read_bytes()).hexdigest()}
    receipt["display"] = display
    print("GRX_QUALIFICATION=PASS", flush=True)
    return receipt


if __name__ == "__main__":
    main()

# end of test-grx.py
