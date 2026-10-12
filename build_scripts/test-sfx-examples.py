#!/usr/bin/env python3
"""FreeBASIC Linux sfxlib example qualification
File: test-sfx-examples.py
Purpose: Run the original sound examples against the genuine null backend.
Responsibilities: Private media, bounded diagnostics, GUI closure and receipts.
This file contains no sound driver replacement or physical audio acceptance.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import time
import wave

from sdl3_xvfb import private_display


ROOT = Path(__file__).resolve().parents[1]
DUMP_FRAMES = 8192


def fingerprint(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--select")
    args = parser.parse_args()
    if os.name != "posix":
        parser.error("this runner qualifies native Linux and its owned X display")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    args.fbc = args.fbc.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = [source for source in sorted((ROOT / "examples/sfxlib").glob("*.bas"))
               if not args.select or args.select in source.name]
    if not sources:
        parser.error("no examples matched")
    # Shared private media avoids copying the same files for every executable.
    # Each program still sees the documented ExePath()/media location.
    shutil.copytree(ROOT / "examples/sfxlib/media", args.output / "media", dirs_exist_ok=True)
    fixture = args.output / "fft-fixture.txt"
    fixture.write_text("".join(f"{0.25 * math.sin(2 * math.pi * 440 * index / 44100):.12f}\n"
                               for index in range(DUMP_FRAMES)))
    source_hashes = {source.relative_to(ROOT).as_posix(): fingerprint(source) for source in sources}
    source_hashes["examples/sfxlib/example_common.bi"] = fingerprint(ROOT / "examples/sfxlib/example_common.bi")
    source_hashes["build_scripts/test-sfx-examples.py"] = fingerprint(Path(__file__).resolve())
    (args.output / "results.json").write_text(json.dumps({"status": "pending", "source_sha256": source_hashes}, indent=2) + "\n")
    spec = importlib.util.spec_from_file_location("sdl12_checks", ROOT / "build_scripts/test-sdl12-examples.py")
    gui = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gui)

    def check(item):
        backend, source = item
        directory = args.output / backend / source.stem
        directory.mkdir(parents=True, exist_ok=True)
        media = directory / "media"
        if not media.exists():
            media.symlink_to(args.output / "media", target_is_directory=True)
        assert media.resolve() == args.output / "media"
        binary = directory / "example"
        record = {"source": source.relative_to(ROOT).as_posix(), "backend": backend}
        environment = os.environ.copy()
        environment["SFXLIB_DRIVER"] = "null"
        environment["SFXLIB_DRIVER_DUMP"] = str(directory / "driver.txt")
        environment["SFXLIB_DRIVER_DUMP_FRAMES"] = str(DUMP_FRAMES)
        environment.pop("FB_GFX_DRIVER", None)
        environment.pop("FBGFX", None)
        command = [args.fbc, "-prefix", ROOT, "-i", ROOT / "inc", "-gen", backend,
                   "-mt", source, "-x", binary]
        compilation = subprocess.run([str(value) for value in command], env=environment,
                                     capture_output=True, text=True, timeout=60)
        (directory / "compile.log").write_text(compilation.stdout + compilation.stderr)
        record["compile_exit"] = compilation.returncode
        if compilation.returncode:
            record["status"] = "compile-failed"
            return record
        arguments = ["44100", str(fixture), "440"] if source.stem == "fastfft" else []
        try:
            if source.stem == "composer-grid":
                with private_display(directory, environment) as (session, display):
                    record["display"] = display
                    with (directory / "run.log").open("w") as log:
                        child = subprocess.Popen([str(binary)], cwd=directory, env=session,
                                                 stdout=log, stderr=subprocess.STDOUT,
                                                 stdin=subprocess.DEVNULL, start_new_session=True)
                        try:
                            # gfxlib2's X11 window has no _NET_WM_PID property.
                            # This authenticated server contains only our one
                            # child. Require its exact title and a unique match.
                            deadline = time.monotonic() + 10
                            window = None
                            while child.poll() is None and time.monotonic() < deadline:
                                found = gui.command(["xdotool", "search", "--onlyvisible", "--name",
                                                     "^sfxlib composer grid$"], session)
                                if found.returncode == 0 and found.stdout.strip():
                                    matches = found.stdout.splitlines()
                                    assert len(matches) == 1
                                    window = int(matches[0])
                                    break
                                time.sleep(.05)
                            if window is None:
                                raise RuntimeError("composer did not create its titled window")
                            time.sleep(.25)
                            capture = gui.capture(window, directory / "window.png", session)
                            assert capture["colors"] != 1
                            record["graphics"] = {"child_pid": child.pid, "window": window,
                                                  "ownership": "sole child on private authenticated display",
                                                  "capture": capture}
                            gui.request_close(window, session)
                            record["run_exit"] = child.wait(timeout=10)
                        finally:
                            gui.stop(child)
            else:
                run = subprocess.run([str(binary), *arguments], cwd=directory, env=environment,
                                     capture_output=True, text=True, timeout=30)
                (directory / "run.log").write_text(run.stdout + run.stderr)
                record["run_exit"] = run.returncode
            if record["run_exit"]:
                raise RuntimeError("original example returned " + str(record["run_exit"]))
            dump = directory / "driver.txt"
            if dump.exists():
                # Diagnostics are capped at the driver boundary. Check real
                # submitted samples without saving unbounded audio streams.
                samples = [float(line) for line in dump.read_text().splitlines()]
                assert len(samples) <= DUMP_FRAMES
                assert all(math.isfinite(value) and abs(value) <= 1.00001 for value in samples)
                record["driver_samples"] = len(samples)
                record["nonzero_driver_samples"] = sum(abs(value) > 0.000001 for value in samples)
            if source.stem in ("midi-synth-player", "voice-effects-stress"):
                text = (directory / "run.log").read_text()
                if source.stem == "midi-synth-player":
                    assert "PASS: MIDI was synthesized entirely through sfxlib" in text
                    captured = directory / "midi-synth-output.wav"
                else:
                    assert "PASS: exact 4, 16, and 32-voice stages completed" in text
                    captured = directory / "voice-effects-stress.wav"
                with wave.open(str(captured)) as audio:
                    assert audio.getnchannels() == 2 and audio.getsampwidth() == 2
                    assert audio.getframerate() == 44100 and audio.getnframes() > 44100
                    assert any(audio.readframes(4096))
                    record["captured_frames"] = audio.getnframes()
                record["capture_sha256"] = fingerprint(captured)
            record["status"] = "passed"
        except Exception as error:
            record.update(status="run-failed", error=str(error))
        return record

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        records = list(pool.map(check, [(backend, source) for backend in ("gcc", "gas64") for source in sources]))
    failures = [record for record in records if record["status"] != "passed"]
    receipt = {"status": "fail" if failures else "pass", "scope": "original Linux examples with genuine null audio backend",
               "compiler_sha256": fingerprint(args.fbc), "source_sha256": source_hashes,
               "driver_dump_limit": DUMP_FRAMES, "records": records}
    (args.output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"examples": len(sources), "checks": len(records), "passed": len(records) - len(failures),
                      "failures": failures}, indent=2), flush=True)
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
# end of test-sfx-examples.py
