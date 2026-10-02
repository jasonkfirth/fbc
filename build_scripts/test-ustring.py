#!/usr/bin/env python3
"""
Project: FreeBASIC tests
File: test-ustring.py
Purpose: Run the UTF-8 type suite, rejection tests, examples, and sanitizers.
Responsibilities: Build isolated native artifacts and report every failure.
This file intentionally does NOT contain UTF-8 implementations or downloads.
"""

import argparse
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile


def run(command, working, timeout=120):
    result = subprocess.run(command, cwd=working, text=True, capture_output=True,
                            timeout=timeout, check=False)
    if result.returncode:
        raise RuntimeError(f"{shlex.join(str(part) for part in command)}\n{result.stdout}{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--backend", choices=("gas", "gas64", "gcc", "llvm", "clang"), action="append")
    parser.add_argument("--no-sanitizers", action="store_true")
    options = parser.parse_args()
    root = options.root.resolve()
    fbc = options.fbc.resolve()
    prefix = [str(fbc), "-prefix", str(root), "-i", str(root / "inc")]
    libdir = Path(run(prefix + ["-print", "fblibdir"], root).strip())
    backends = options.backend or ["gcc"]
    if options.backend is None:
        target = run(prefix + ["-print", "target"], root).strip()
        if target.endswith("x86_64"):
            backends.insert(0, "gas64")
        if shutil.which(os.environ.get("LLC") or "llc"):
            backends.append("llvm")
        if shutil.which(os.environ.get("CLANG") or "clang"):
            backends.append("clang")

    with tempfile.TemporaryDirectory(prefix="fbc-ustring-") as temporary:
        working = Path(temporary)
        unit = root / "tests/fbcunit"
        includes = ["-i", str(unit / "inc"), "-i", str(unit / "src")]
        objects = []
        for name in ("fbcunit", "fbcunit_qb", "fbcunit_console", "fbcunit_report"):
            obj = working / (name + ".o")
            run(prefix + includes + ["-gen", "gcc", "-mt", "-d", "FBCU_NO_INCLIB", "-c",
                                     str(unit / "src" / (name + ".bas")), "-o", str(obj)], working)
            objects.append(str(obj))
        run(shlex.split(os.environ.get("AR", "ar")) + ["rcs", str(working / "libfbcunit.a")] + objects, working)
        sound_stub = working / "sound-stub.o"
        run(shlex.split(os.environ.get("CC", "gcc")) + ["-Wall", "-Wextra", "-Werror",
            "-Wno-unused-parameter", "-I", str(root / "src/sfxlib"), "-c",
            str(root / "tests/string/text-types-sfx-stub.c"), "-o", str(sound_stub)], working)

        for backend in backends:
            executable = working / ("tests-" + backend)
            flags = prefix + includes + ["-gen", backend, "-mt", "-g", "-exx", "-p", str(working)]
            run(flags + [str(root / "tests/fbc-tests.bas"), str(root / "tests/string/ustring.bas"),
                         str(root / "tests/string/text-types.bas"),
                         "-x", str(executable)], working)
            output = run([str(executable)], working)
            if not re.search(r"\b0\s+Total\b", output):
                raise RuntimeError(f"{backend}: missing passing test summary\n{output}")
            for dialect in ("fb", "fblite", "deprecated"):
                run(flags + ["-lang", dialect, "-c", str(root / "tests/string/text-types-api.bas"),
                             "-o", str(working / ("text-api-" + backend + "-" + dialect + ".o"))], working)
            run(flags + ["-c", str(root / "tests/string/text-types-gfx3-api.bas"),
                         "-o", str(working / ("gfx3-api-" + backend + ".o"))], working)
            printer = working / ("printer-" + backend)
            run(flags + [str(root / "tests/string/ustring-lprint.bas"),
                         "-x", str(printer)], working)
            run([str(printer)], working)

            rejected = sorted((root / "tests/string").glob("ustring-*-invalid.bas"))
            for source in rejected:
                result = subprocess.run(flags + ["-c", str(source), "-o", str(working / "invalid.o")],
                                        cwd=working, text=True, capture_output=True, timeout=30, check=False)
                if result.returncode == 0 or not re.search(r"error \d+:", result.stdout + result.stderr):
                    raise RuntimeError(f"{backend}: {source.name} was not rejected\n{result.stdout}{result.stderr}")

            examples = sorted((root / "examples/manual/strings").glob("ustring*.bas"))
            for source in examples:
                example = working / (source.stem + "-" + backend)
                run(flags + [str(source), "-x", str(example)], working)
                run([str(example)], working)
            libraries = [("gfx2", [])]
            if (libdir / "libfbgfx3.a").exists():
                libraries.append(("gfx3", ["-gfx3"]))
            for library, graphics_flags in libraries:
                graphics = working / (library + "-" + backend)
                run(flags + graphics_flags + [str(root / "tests/gfx/text-types.bas"), "-x", str(graphics)], working)
                run([str(graphics)], working)
                if library == "gfx3":
                    assets = working / ("assets-" + backend)
                    run(flags + graphics_flags + [str(root / "tests/gfx3/text-types.bas"), "-x", str(assets)], working)
                    run([str(assets)], working)
            profiler = working / ("profiler-" + backend)
            run(flags + [str(root / "tests/string/text-types-profile.bas"), "-x", str(profiler)], working)
            run([str(profiler)], working)
            sound = working / ("sound-" + backend)
            run(flags + [str(root / "tests/string/text-types-sfx.bas"), str(sound_stub), "-x", str(sound)], working)
            run([str(sound)], working)
            print(f"{backend}: language/API, graphics, profiler, sound, {len(rejected)} rejection tests, and {len(examples)} examples passed")

        if not options.no_sanitizers and sys.platform.startswith("linux"):
            executable = working / "runtime-sanitized"
            sources = [root / "tests/string/ustring-runtime.c"]
            sources.extend(root / "src/rtlib" / name for name in
                           ("ustr_core.c", "ustr_slice.c", "ustr_search.c", "ustr_case.c", "ustr_io.c",
                            "ustr_optional.c", "ustr_paths.c", "ustr_profile.c", "str_core.c", "io_printusg.c"))
            command = shlex.split(os.environ.get("CC", "gcc")) + [
                "-g", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(root / "src/rtlib"),
            ]
            for width, flags in (("UTF-32", []), ("UTF-16", ["-fshort-wchar"])):
                run(command + flags + [str(source) for source in sources] + [str(libdir / "libfb.a"),
                    "-lm", "-pthread", "-lncurses", "-o", str(executable)], working)
                print(width + ": " + run([str(executable)], working).strip())
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)

# end of test-ustring.py
