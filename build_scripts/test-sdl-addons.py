#!/usr/bin/env python3
"""Project: FreeBASIC SDL addon checks
File: test-sdl-addons.py
Purpose: Compile and run the addon examples with controlled local fixtures.
Responsibilities: Give each executable a private X server and retain real outcomes.
This file intentionally does NOT contain: public networking or simulated addon APIs.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import time
import wave

from PIL import Image
from sdl_example_paths import ADDON_EXAMPLES
from sdl3_xvfb import private_display


ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
CONSOLE = {"sound1", "sound2", "mixer2", "net2", "shadercross3"}


def stop(child):
    if child.poll() is None:
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            child.wait()
            return
        try:
            child.wait(timeout=2)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(child.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            child.wait(timeout=2)


def fixtures(work):
    directory = work / "fixtures"
    directory.mkdir(exist_ok=True)
    (directory / "example.rtf").write_text(
        r"{\rtf1\ansi{\fonttbl{\f0\fswiss Sans;}}{\info{\title FreeBASIC RTF}}"
        r"\f0\fs40 FreeBASIC SDL RTF\par\b Bold text\b0\par\i Italic text\i0\par}")
    with Image.new("RGB", (64, 64), (30, 90, 180)) as image:
        image.paste((240, 190, 50), (8, 8, 56, 56))
        image.save(directory / "example.png")
    with wave.open(str(directory / "tone.wav"), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(8000)
        output.writeframes(b"\0\0" * 800)
    shader = ROOT / "out/sdl3/sources/SDL3-3.4.18/test/testgpu/cube.vert.spv.h"
    bytecode = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})\b", shader.read_text()))
    if len(bytecode) < 20 or bytecode[:4] != b"\x03\x02\x23\x07":
        raise ValueError("Invalid pinned vertex shader fixture")
    (directory / "vertex.spv").write_bytes(bytecode)
    return directory


def check(item, args, environment, assets):
    backend, source = item
    directory = args.output / backend / source.stem
    directory.mkdir(parents=True, exist_ok=True)
    executable = directory / "example"
    command = [str(args.fbc), "-i", str(ROOT / "inc"), "-p", str(args.workdir / "stage/linux-x86_64/lib"),
               "-gen", backend, "-mt", str(source), "-x", str(executable)]
    compilation = subprocess.run(command, capture_output=True, text=True, timeout=120)
    (directory / "compile.log").write_text(compilation.stdout + compilation.stderr)
    record = {"source": str(source.relative_to(ROOT)), "backend": backend,
              "compile": {"exit_code": compilation.returncode, "log": str(directory / "compile.log")}}
    if compilation.returncode:
        record["status"] = "compile-failed"
        return record
    arguments = []
    if source.stem.startswith("rtf"):
        arguments = [str(assets / "example.rtf"), str(args.font)]
    elif source.stem.startswith("fontcache") or source.stem == "ttf2":
        arguments = [str(args.font)]
    elif source.stem.startswith("sound") or source.stem == "mixer2":
        arguments = [str(assets / "tone.wav")]
    elif source.stem == "image2":
        arguments = [str(assets / "example.png")]
    elif source.stem == "shadercross3":
        arguments = [str(assets / "vertex.spv")]
    try:
        with private_display(directory, environment) as (session, display):
            record["display"] = display
            with (directory / "run.log").open("w") as output:
                child = subprocess.Popen([str(executable)] + arguments, cwd=directory, env=session,
                                         stdout=output, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                         start_new_session=True)
                try:
                    if source.stem not in CONSOLE:
                        deadline = time.monotonic() + 10
                        window = None
                        while child.poll() is None and time.monotonic() < deadline:
                            query = subprocess.run(["xdotool", "search", "--onlyvisible", "--pid", str(child.pid)],
                                                   env=session, capture_output=True, text=True, timeout=3)
                            if query.returncode == 0 and query.stdout.strip():
                                window = query.stdout.splitlines()[-1]
                                break
                            time.sleep(0.03)
                        if window is None:
                            record["run"] = {"exit_code": child.poll(), "timed_out": child.poll() is None,
                                             "log": str(directory / "run.log")}
                            raise RuntimeError("Example did not create a visible window")
                        screenshot = directory / "window.png"
                        # SDL maps a window before an OpenGL context has
                        # finished starting. Under load the first frame can
                        # arrive later, so wait for drawn pixels within a
                        # fixed deadline instead of sampling an empty frame.
                        deadline = time.monotonic() + 5
                        while True:
                            subprocess.run(["import", "-window", window, str(screenshot)], env=session,
                                           capture_output=True, check=True, timeout=5)
                            with Image.open(screenshot) as image:
                                colors = image.convert("RGB").getcolors(maxcolors=1024)
                            if colors is None or len(colors) >= 2:
                                break
                            if child.poll() is not None or time.monotonic() >= deadline:
                                raise RuntimeError("Example window remained empty")
                            time.sleep(0.03)
                        record["screenshot"] = str(screenshot)
                    child.wait(timeout=20)
                    record["run"] = {"exit_code": child.returncode, "timed_out": False,
                                     "log": str(directory / "run.log")}
                except subprocess.TimeoutExpired:
                    record["run"] = {"exit_code": None, "timed_out": True, "log": str(directory / "run.log")}
                finally:
                    stop(child)
        run = record["run"]
        record["status"] = "passed" if run["exit_code"] == 0 and not run["timed_out"] else "run-failed"
        text = (directory / "run.log").read_text()
        if source.stem.startswith("sound") and "Decoded bytes:" not in text:
            record.update(status="run-failed", error="No decoded audio was observed")
        if source.stem == "shadercross3" and "Reflected inputs:" not in text:
            record.update(status="run-failed", error="No shader reflection was observed")
        if source.stem == "mixer2" and "Mixed bytes:" not in text:
            record.update(status="run-failed", error="No completed audio playback was observed")
        if source.stem == "net2" and "Loopback bytes:" not in text:
            record.update(status="run-failed", error="No loopback packet was observed")
    except Exception as error:
        record.update(status="run-failed", error=str(error))
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl-addons")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--backend", nargs="+", choices=("gcc", "gas64"), default=["gcc", "gas64"])
    parser.add_argument("--font", type=Path, default=ROOT / "examples/graphics/SDL1/data/Vera.ttf")
    parser.add_argument("--select")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--frames", type=int, default=120,
                        help="Keep graphical windows open long enough for display inspection")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if args.frames < 1:
        parser.error("--frames must be positive")
    args.workdir = args.workdir.resolve()
    args.output = args.output.resolve() if args.output else args.workdir / "native-tests"
    args.fbc = args.fbc.resolve()
    args.font = args.font.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    assets = fixtures(args.workdir)
    environment = os.environ.copy()
    environment.update(SDL_AUDIODRIVER="dummy", SDL_RENDER_DRIVER="software", LIBGL_ALWAYS_SOFTWARE="1",
                       FB_SDL_ADDON_FRAMES=str(args.frames))
    environment["LD_LIBRARY_PATH"] = str(args.workdir / "stage/linux-x86_64/lib") + os.pathsep + environment.get("LD_LIBRARY_PATH", "")
    sources = [EXAMPLES / path for path in ADDON_EXAMPLES
               if not args.select or args.select in path]
    if not sources:
        parser.error("No addon examples matched")
    for source in sources:
        if not source.is_file():
            parser.error("Addon example is missing: " + str(source))
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda item: check(item, args, environment, assets),
                                [(b, s) for b in args.backend for s in sources]))
    (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    failed = [r for r in results if r["status"] != "passed"]
    print(json.dumps({"checked": len(results), "failures": failed,
                      "report": str(args.output / "results.json")}, indent=2))
    return bool(failed)


if __name__ == "__main__":
    raise SystemExit(main())

# End of test-sdl-addons.py
