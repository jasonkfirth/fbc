#!/usr/bin/env python3
"""Project: FreeBASIC SDL Windows checks
File: test-sdl-windows.py
Purpose: Compile SDL1, SDL2, and SDL3 bindings and examples for Windows.
Responsibilities: Check COFF objects and PE executables with both native backends.
This file intentionally does NOT contain: SDK downloads or Windows emulation.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import subprocess

from sdl_example_paths import ADDON_EXAMPLES

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = re.compile(r'#\s*include\s+(?:once\s+)?"SDL[23]?[/\\]', re.IGNORECASE)
PROFILES = (("win32", "gcc", "i686-w64-mingw32"),
            ("win32", "gas", "i686-w64-mingw32"),
            ("win64", "gcc", "x86_64-w64-mingw32"),
            ("win64", "gas64", "x86_64-w64-mingw32"))
ARM_PROFILE = ("win32-aarch64", "clang", "aarch64-w64-mingw32")
MINIMUM_EXAMPLES = {
    "SDL": "graphics/SDL1/core/pixel.bas",
    "SDL_image": "graphics/SDL1/image/image_test1.bas",
    "SDL_mixer": "graphics/SDL1/mixer/music_test1.bas",
    "SDL_net": "graphics/SDL1/net/net_httpget.bas",
    "SDL_ttf": "graphics/SDL1/ttf/ttf.bas",
    "SDL_sound": "graphics/SDL1/sound/sound1.bas",
    "SDL_gfx": "graphics/SDL1/gfx/gfx_line.bas",
    "SDL_gpu": "graphics/SDL1/gpu/gpu1.bas",
    "SDL_rtf": "graphics/SDL1/rtf/rtf1.bas",
    "SDL_FontCache": "graphics/SDL1/fontcache/fontcache1.bas",
    "SDL_Pango": "graphics/SDL1/pango/pango1.bas",
    "SDL2": "graphics/SDL2/core/sdl2-hello.bas",
    "SDL2_image": "graphics/SDL2/image/image2.bas",
    "SDL2_mixer": "graphics/SDL2/mixer/mixer2.bas",
    "SDL2_net": "graphics/SDL2/net/net2.bas",
    "SDL2_ttf": "graphics/SDL2/ttf/ttf2.bas",
    "SDL2_sound": "graphics/SDL2/sound/sound2.bas",
    "SDL2_gfx": "graphics/SDL2/gfx/gfx2.bas",
    "SDL2_gpu": "graphics/SDL2/gpu/gpu2.bas",
    "SDL2_rtf": "graphics/SDL2/rtf/rtf2.bas",
    "SDL2_FontCache": "graphics/SDL2/fontcache/fontcache2.bas",
    "SDL2_bgi": "graphics/SDL2/bgi/bgi2.bas",
    "SDL3": "graphics/SDL3/core/renderer/01-clear/clear.bas",
    "SDL3_image": "graphics/SDL3/image/showimage.bas",
    "SDL3_mixer": "graphics/SDL3/mixer/basics/01-load-and-play/load-and-play.bas",
    "SDL3_net": "graphics/SDL3/net/datagram.bas",
    "SDL3_ttf": "graphics/SDL3/ttf/showfont.bas",
    "SDL3_sound": "graphics/SDL3/sound/playsound_simple.bas",
    "SDL3_gfx": "graphics/SDL3/gfx/gfx3.bas",
    "SDL3_bgi": "graphics/SDL3/bgi/bgi3.bas",
    "SDL3_rtf": "graphics/SDL3/rtf/rtf3.bas",
    "SDL3_shadercross": "graphics/SDL3/shadercross/shadercross3.bas",
}


# -------------------------------------------------------------------------
# Cross-toolchain invocation
# -------------------------------------------------------------------------


def invoke(command, environment, directory, log):
    with log.open("w") as output:
        output.write("command: " + repr(command) + "\n")
        output.flush()
        try:
            result = subprocess.run(command, cwd=directory, env=environment,
                                    stdout=output, stderr=subprocess.STDOUT, timeout=120)
            return {"exit_code": result.returncode, "timed_out": False, "log": str(log)}
        except subprocess.TimeoutExpired:
            return {"exit_code": None, "timed_out": True, "log": str(log)}


def environment_for(triple, toolchain):
    environment = os.environ.copy()
    bindir = toolchain / "bin"
    environment["PATH"] = str(bindir) + os.pathsep + environment["PATH"]
    for key, suffix in (("GCC", "gcc"), ("AS", "as"), ("LD", "ld"),
                        ("AR", "ar"), ("DLLTOOL", "dlltool"), ("WINDRES", "windres")):
        environment[key] = str(bindir / (triple + "-" + suffix))
    if triple == "aarch64-w64-mingw32":
        environment["CLANG"] = str(bindir / (triple + "-clang"))
        environment["LD"] = str(bindir / "ld.lld")
    return environment


def checked(result):
    return result["exit_code"] == 0 and not result["timed_out"]


# -------------------------------------------------------------------------
# Header and example compilation
# -------------------------------------------------------------------------


def check(item, args):
    target, backend, triple, source, header = item
    toolchain = args.arm_toolchain if target == "win32-aarch64" else args.toolchain
    environment = environment_for(triple, toolchain)
    relative = source.relative_to(ROOT / ("inc" if header else "examples"))
    directory = args.output / target / backend / ("headers" if header else "examples") / relative.with_suffix("")
    directory.mkdir(parents=True, exist_ok=True)
    record = {"source": str(source.relative_to(ROOT)), "target": target, "backend": backend}
    if header:
        include = directory / "include.bas"
        include.write_text("'' Project: FreeBASIC SDL Windows checks\n"
                           "'' File: include.bas\n"
                           "'' Purpose: Check one public binding in an independent module.\n"
                           "'' Responsibilities: Include the named header.\n"
                           "'' This file intentionally does NOT contain: application code.\n"
                           '#include once "' + relative.as_posix() + '"\n'
                           "'' End of include.bas\n")
        input_source = include
    else:
        input_source = source
    helper = not header and relative.as_posix() == "graphics/SDL3/ttf/editbox.bas"
    base = [str(args.fbc), "-prefix", str(args.prefix), "-i", str(ROOT / "inc"),
            "-i", str(ROOT / "examples/graphics/SDL3"), "-target", target if target == "win32-aarch64" else triple,
            "-gen", backend, "-mt"]
    obj = directory / "example.o"
    compilation = base + ["-c", str(input_source), "-o", str(obj)]
    if not header and not helper:
        # -c normally creates a module constructor. Main examples need the
        # real main parameters, including __FB_ARGC__ and __FB_ARGV__.
        compilation += ["-m", source.stem]
    if args.reuse_objects and obj.is_file():
        record["compile"] = {"exit_code": 0, "timed_out": False,
                             "log": str(directory / "compile.log"), "reused": True}
    else:
        record["compile"] = invoke(compilation, environment, directory, directory / "compile.log")
    if not checked(record["compile"]):
        record["status"] = "compile-failed"
        return record
    inspector = (["llvm-readobj", "--file-headers"] if target == "win32-aarch64" else
                 [str(toolchain / "bin" / (triple + "-objdump")), "-f"])
    inspection = subprocess.run(inspector + [str(obj)],
                                env=environment, capture_output=True, text=True, timeout=10)
    (directory / "object.log").write_text(inspection.stdout + inspection.stderr)
    expected = "pe-i386" if target == "win32" else "IMAGE_FILE_MACHINE_ARM64" if target == "win32-aarch64" else "pe-x86-64"
    if inspection.returncode or expected not in inspection.stdout:
        record.update(status="object-failed", error="Object has the wrong Windows architecture")
        return record
    record.update(status="header-built" if header else "helper-built" if helper else "object-built",
                  object=str(obj))
    if header or helper or args.object_only:
        return record
    sdk = args.sdk32 if target == "win32" else args.sdk_arm if target == "win32-aarch64" else args.sdk64
    libraries = sdk / "lib" if (sdk / "lib").is_dir() else sdk
    executable = directory / "example.exe"
    # FreeBASIC keeps a main module's #inclib choices in the driver, rather
    # than in that main object's compile-time information section. Reparse
    # the main source during linking so those library choices are preserved.
    link = base + ["-p", str(libraries), str(source), "-m", source.stem, "-x", str(executable)]
    if relative.as_posix() == "graphics/SDL3/ttf/showfont.bas":
        editbox = args.output / target / backend / "examples/graphics/SDL3/ttf/editbox/example.o"
        link += [str(editbox)]
    record["link"] = invoke(link, environment, directory, directory / "link.log")
    if not checked(record["link"]):
        record["status"] = "link-failed"
        return record
    inspection = subprocess.run(inspector + [str(executable)],
                                env=environment, capture_output=True, text=True, timeout=10)
    (directory / "executable.log").write_text(inspection.stdout + inspection.stderr)
    expected = "pei-i386" if target == "win32" else "IMAGE_FILE_MACHINE_ARM64" if target == "win32-aarch64" else "pei-x86-64"
    if inspection.returncode or expected not in inspection.stdout:
        record.update(status="executable-failed", error="Executable has the wrong Windows architecture")
    else:
        record.update(status="passed", executable=str(executable))
    return record


# -------------------------------------------------------------------------
# Case selection and reports
# -------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--toolchain", type=Path, default=ROOT / "out/sdl-windows/toolchain/usr")
    parser.add_argument("--prefix", type=Path, default=ROOT / "out/sdl-windows/fb-prefix")
    parser.add_argument("--sdk32", type=Path, default=ROOT / "out/sdl-windows/sdk/win32")
    parser.add_argument("--sdk64", type=Path, default=ROOT / "out/sdl-windows/sdk/win64")
    parser.add_argument("--arm-toolchain", type=Path)
    parser.add_argument("--sdk-arm", type=Path, default=ROOT / "out/sdl-addons/stage/win32-aarch64")
    parser.add_argument("--addons", action="store_true", help="Include companion-library examples")
    parser.add_argument("--addon-only", action="store_true", help="Check only the companion-library examples")
    parser.add_argument("--minimum", action="store_true",
                        help="Link one example per packaged library and compile every SDL header")
    parser.add_argument("--output", type=Path, default=ROOT / "out/sdl-windows/validation")
    parser.add_argument("--object-only", action="store_true")
    parser.add_argument("--reuse-objects", action="store_true",
                        help="Reuse COFF checks from OUTPUT; rebuild the main source when linking")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--select", help="Restrict source and header paths to this substring")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    for key in ("fbc", "toolchain", "prefix", "sdk32", "sdk64", "output"):
        setattr(args, key, getattr(args, key).resolve())
    profiles = list(PROFILES)
    if args.arm_toolchain:
        args.arm_toolchain = args.arm_toolchain.resolve()
        args.sdk_arm = args.sdk_arm.resolve()
        profiles.append(ARM_PROFILE)
    args.output.mkdir(parents=True, exist_ok=True)
    if args.minimum and args.addon_only:
        parser.error("--minimum and --addon-only select different example sets")
    if args.minimum:
        examples = [ROOT / "examples" / path for path in sorted(set(MINIMUM_EXAMPLES.values()))]
        # SDL3_ttf's showfont also needs the separately compiled editor module.
        examples.append(ROOT / "examples/graphics/SDL3/ttf/editbox.bas")
        for path in examples:
            if not path.is_file():
                parser.error("Minimum example is missing: " + str(path))
    elif args.addon_only:
        examples = [ROOT / "examples" / path for path in ADDON_EXAMPLES]
    else:
        examples = [p for p in sorted((ROOT / "examples").rglob("*.bas"))
                    if INCLUDE.search(p.read_text(errors="replace")) or
                    p.relative_to(ROOT / "examples").parts[:2] in
                    (("graphics", "SDL1"), ("graphics", "SDL2"), ("graphics", "SDL3"))]
    headers = ([] if args.addon_only else
               [p for family in ("SDL", "SDL2", "SDL3") for p in sorted((ROOT / "inc" / family).glob("*.bi"))])
    items = [(t, b, triple, p, header) for t, b, triple in profiles
             for header, sources in ((False, examples), (True, headers)) for p in sources
             if not args.select or args.select in str(p.relative_to(ROOT))]
    if not items:
        parser.error("No SDL examples or headers matched")
    # showfont needs editbox's separately compiled object for this profile.
    helpers = [item for item in items if item[3].name == "editbox.bas"]
    results = [check(item, args) for item in helpers]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results.extend(pool.map(lambda item: check(item, args), [item for item in items if item not in helpers]))
    failures = [r for r in results if r["status"].endswith("failed")]
    report = {"object_only": args.object_only, "profiles": profiles, "results": results}
    if args.minimum:
        report["minimum_examples"] = MINIMUM_EXAMPLES
    (args.output / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"checks": len(results), "failed": len(failures),
                      "failures": [{k: r[k] for k in ("source", "target", "backend", "status")} for r in failures],
                      "report": str(args.output / "results.json")}, indent=2))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-sdl-windows.py
