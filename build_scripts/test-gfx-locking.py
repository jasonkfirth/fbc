#!/usr/bin/env python3
"""Project: FreeBASIC graphics tests
File: test-gfx-locking.py
Purpose: Link current graphics lifecycle code against native MT libraries.
Responsibilities: Run instrumented real-lock regressions in bounded scratch space.
This file intentionally does NOT rebuild the compiler or require a display.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


def run(command, **options):
    result = subprocess.run([str(arg) for arg in command], capture_output=True, text=True,
                            timeout=45, check=False, **options)
    if result.returncode:
        raise RuntimeError(" ".join(str(arg) for arg in command) + "\n" + result.stdout + result.stderr)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--libdir", type=Path, required=True)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--ldflags", default="")
    parser.add_argument("--receipt", type=Path, help="Write successful input identities and the native result")
    parser.add_argument("--control-source", type=Path, help="Expect this older gfx_screen.c to fail the regression")
    options = parser.parse_args()
    root = options.root.resolve()
    libdir = options.libdir.resolve()
    with tempfile.TemporaryDirectory(prefix="fbc-gfx-locking-") as temporary:
        work = Path(temporary)
        objects = []
        sources = [options.control_source or root / "src/gfxlib2/core/gfx_screen.c",
                   root / "src/gfxlib2/image/gfx_lzw.c", root / "src/gfxlib2/image/gfx_lzw_enc.c",
                   root / "tests/gfxlib2/locking.c"]
        for index, source in enumerate(sources):
            obj = work / (str(index) + ".o")
            run([options.cc, "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra", "-Werror=implicit-function-declaration",
                 "-DENABLE_MT", "-I" + str(root / "src/rtlib"), "-I" + str(root / "src/gfxlib2/core"),
                 "-c", source, "-o", obj])
            objects.append(obj)
        executable = work / ("locking.exe" if os.name == "nt" else "locking")
        libraries = (["-lgdi32", "-lwinmm", "-ldxguid", "-ldinput8", "-lopengl32", "-lws2_32", "-lole32", "-luuid"]
                     if os.name == "nt" else ["-lX11", "-lXext", "-lXpm", "-lXrandr", "-lXrender", "-lGL", "-lncurses", "-lffi"])
        if sys.platform.startswith("linux"):
            libraries += ["-ldl"]
            # Optional graphics backends are pulled in by the native driver
            # table even when this regression selects the null driver.
            for package in ("libdrm", "gbm", "egl"):
                result = subprocess.run(["pkg-config", "--libs", package], capture_output=True,
                                        text=True, timeout=10, check=False)
                if result.returncode == 0:
                    libraries += shlex.split(result.stdout)
        elif sys.platform.startswith("freebsd"):
            libraries += ["-L/usr/local/lib"]
        run([options.cc, *objects, "-Wl,--start-group", libdir / "libfbgfxmt.a", libdir / "libfbmt.a", "-Wl,--end-group",
             *["-Wl,--wrap=" + name for name in ("fb_Lock", "fb_Unlock", "fb_GraphicsLock", "fb_GraphicsUnlock", "calloc")],
             *libraries, "-lm", "-pthread", *shlex.split(options.ldflags), "-o", executable])
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=15, check=False)
        print(result.stdout + result.stderr, end="")
        if options.control_source:
            if result.returncode != 88 or "runtime lock entered after graphics" not in result.stderr:
                raise RuntimeError("older control did not expose the lock-order defect")
            print("GFX_LOCKING_CONTROL_PASS older mode path rejected")
        elif result.returncode or "GFX_LOCKING_PASS" not in result.stdout:
            raise RuntimeError("graphics lock regression failed")
        if options.receipt:
            inputs = [Path(source) for source in sources] + [libdir / name for name in ("libfbgfxmt.a", "libfbmt.a")]
            identities = {}
            for source in inputs:
                with source.open("rb") as stream:
                    identities[str(source)] = hashlib.file_digest(stream, "sha256").hexdigest()
            options.receipt.parent.mkdir(parents=True, exist_ok=True)
            options.receipt.write_text(json.dumps({"result": "PASS", "control": bool(options.control_source),
                                                   "inputs": identities, "output": result.stdout + result.stderr},
                                                  indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        raise SystemExit("GFX_LOCKING_FAIL " + str(error))

# end of test-gfx-locking.py
