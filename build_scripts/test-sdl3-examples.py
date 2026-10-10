#!/usr/bin/env python3
"""Project: FreeBASIC SDL3 examples
File: test-sdl3-examples.py
Purpose: Compile the translated examples and run bounded native smoke checks.
Responsibilities: Stage upstream assets, keep outputs local, and report real status.
This file intentionally does NOT contain: hardware emulation or public network access.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time
from contextlib import contextmanager, nullcontext

from sdl3_network_checks import CHECKS
from sdl_example_paths import ADDON_EXAMPLES
from sdl3_xvfb import private_display


ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples/graphics/SDL3"


def run(command: list[str], directory: Path, environment: dict[str, str],
        log: Path, timeout: int) -> dict:
    with log.open("w") as output:
        output.write("command: " + repr(command) + "\n")
        output.flush()
        child = subprocess.Popen(command, cwd=directory, env=environment, stdin=subprocess.DEVNULL,
                                 stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = child.wait(timeout=timeout)
            return dict(exit_code=code, timed_out=False, log=str(log))
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGTERM)
            try:
                child.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.wait()
            return dict(exit_code=child.returncode, timed_out=True, log=str(log))


def stage_assets(work: Path, directory: Path) -> None:
    for package in sorted((work / "sources").iterdir()):
        for folder in (package / "test", package / "examples"):
            if not folder.exists():
                continue
            for source in folder.glob("*"):
                if source.suffix.lower() in (".png", ".wav", ".hex", ".mp3", ".ogg", ".jpg", ".gif"):
                    shutil.copy2(source, directory / source.name)


@contextmanager
def camera_access(work: Path):
    import fcntl
    # Private displays do not isolate V4L devices. Parallel backend checks
    # must take turns so one stream cannot invalidate the other's format ioctl.
    with (work / "camera-check.lock").open("a") as lock:
        deadline = time.monotonic() + 30
        while True:
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                if time.monotonic() >= deadline:
                    raise RuntimeError("Timed out waiting for the camera check")
                time.sleep(0.05)
        try:
            yield
        finally:
            fcntl.flock(lock, fcntl.LOCK_UN)


def check_example(name: str, executable: Path, directory: Path, work: Path,
                  environment: dict[str, str], font: Path, record: dict) -> None:
    if name in CHECKS:
        record["network"] = CHECKS[name](executable, directory, environment)
        record["status"] = "passed" if record["network"]["passed"] else "run-failed"
        return
    stage_assets(work, directory)
    arguments = []
    if name in ("sound/playsound.bas", "sound/playsound_simple.bas"):
        arguments = [str(directory / "sample.wav")]
    elif name in ("ttf/showfont.bas", "ttf/glfont.bas", "ttf/testgputext.bas"):
        arguments = [str(font)]
    elif name in ("image/showimage.bas", "image/showanim.bas", "image/showgpuimage.bas"):
        arguments = [str(directory / "sample.png")]
    elif name == "net/resolve-hostnames.bas":
        arguments = ["localhost"]
    run_environment = environment.copy()
    if name == "ttf/testapp.bas":
        # This test draws once, then waits for input between draws.
        run_environment["FB_SDL3_SMOKE_FRAMES"] = "1"
    if name.startswith("core/camera/"):
        run_environment["FB_SDL3_SMOKE_FRAMES"] = "180"
        run_environment["FB_SDL3_SMOKE_CAMERA"] = "1"
    context = camera_access(work) if name.startswith("core/camera/") else nullcontext()
    with context:
        record["run"] = run([str(executable)] + arguments, directory, run_environment,
                            directory / "run.log", 20)
    passed = record["run"]["exit_code"] == 0 and not record["run"]["timed_out"]
    record["status"] = "passed" if passed else "run-failed"
    if name.startswith("core/camera/"):
        text = (directory / "run.log").read_text()
        if passed and "SDL3 camera frame:" not in text:
            record["status"] = "run-failed"
            record["error"] = "Camera opened but no captured frame was observed"
        elif not passed and not record["run"]["timed_out"] and any(
                message in text.lower() for message in
                ("couldn't find any camera devices", "permission denied", "device or resource busy")):
            record["status"] = "hardware-unavailable"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", type=Path, default=ROOT / "out/sdl3")
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--backend", choices=("gcc", "gas64", "clang", "llvm"),
                        help="Select the compiler backend instead of its default")
    parser.add_argument("--output", type=Path,
                        help="Write executables and reports here instead of WORKDIR/examples")
    parser.add_argument("--font", type=Path,
                        default=Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--private-x", action="store_true",
                        help="Start and reap a separate authenticated Xvfb server for each example")
    parser.add_argument("--frames", type=int, default=16,
                        help="Number of frames in bounded graphical runs (default: 16)")
    parser.add_argument("--select", help="Restrict paths to this substring")
    args = parser.parse_args()
    if args.frames < 1:
        parser.error("--frames must be positive")
    work = args.workdir.resolve()
    output = args.output.resolve() if args.output else work / "examples"
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["LD_LIBRARY_PATH"] = str(work / "prefix/lib") + os.pathsep + environment.get("LD_LIBRARY_PATH", "")
    environment["SDL_AUDIODRIVER"] = "dummy"
    environment["SDL_RENDER_DRIVER"] = "software"
    environment["FB_SDL3_SMOKE_FRAMES"] = str(args.frames)
    environment["FB_SDL3_TEST_FONT"] = str(args.font.resolve())
    results = []
    for source in sorted(EXAMPLES.rglob("*.bas")):
        # The addon runner supplies its additional libraries and fixtures.
        if source.relative_to(ROOT / "examples").as_posix() in ADDON_EXAMPLES:
            continue
        relative = source.relative_to(EXAMPLES)
        name = relative.as_posix()
        if args.select and args.select not in name:
            continue
        directory = output / relative.with_suffix("")
        directory.mkdir(parents=True, exist_ok=True)
        executable = directory / source.stem
        helper = name == "ttf/editbox.bas"
        command = [str(args.fbc.resolve()), "-mt", "-i", str(ROOT / "inc"), "-i", str(EXAMPLES),
                   "-p", str(work / "prefix/lib"), str(source), "-maxerr", "5"]
        if args.backend:
            command += ["-gen", args.backend]
        if helper:
            command += ["-c", "-o", str(directory / "editbox.o")]
        else:
            command += ["-x", str(executable)]
        if name == "ttf/showfont.bas":
            command += [str(EXAMPLES / "ttf/editbox.bas"), "-m", "showfont"]
        record = dict(source=name, compile=run(command, ROOT, environment, directory / "compile.log", 120))
        if record["compile"]["exit_code"] or record["compile"]["timed_out"]:
            record["status"] = "compile-failed"
        elif helper:
            record["status"] = "helper-built"
        elif args.compile_only:
            record["status"] = "compiled"
        elif name.startswith("core/camera/") and not args.private_x:
            record["status"] = "compiled-needs-camera"
        else:
            try:
                context = private_display(directory, environment) if args.private_x else nullcontext((environment, None))
                with context as (run_environment, display):
                    if display is not None:
                        record["x"] = display
                    check_example(name, executable, directory, work, run_environment,
                                  args.font.resolve(), record)
            except (OSError, RuntimeError, subprocess.SubprocessError) as error:
                record["status"] = "run-failed"
                record["error"] = str(error)
        results.append(record)
        (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
        print(name, record["status"], flush=True)
    failures = sum(record["status"] in ("compile-failed", "run-failed") for record in results)
    print(f"{len(results)} examples checked; {failures} failures; reports: {output}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-sdl3-examples.py
