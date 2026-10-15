#!/usr/bin/env python3
"""FreeBASIC OpenAL example failure regression
File: test-openal-failures.py
Purpose: Check source and buffer ownership with controlled SDK responses.
Responsibilities: Native SDK declarations, both amd64 backends and exit status.
This file contains no playback replacement or physical audio acceptance.
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
    parser.add_argument("--cc", type=Path, default=Path("/usr/bin/gcc"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = root / "examples/sound/OpenAL/wav-player.bas"
    native = root / "tests/bindings/openal-failures.c"
    hashes = {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
              for path in (source, native)}
    receipt = {"status": "pending", "source_sha256": hashes}
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    functions = ("alutInit", "alutExit", "alutGetError", "alutCreateBufferFromFile",
                 "alGetError", "alGenSources", "alSourcei", "alSourcef", "alSourcefv",
                 "alListenerfv", "alSourcePlay", "alSourcePause", "alSourceStop",
                 "alDeleteSources", "alDeleteBuffers")
    wraps = ",".join("--wrap=" + name for name in functions)
    records = []

    def compile_command(command):
        result = subprocess.run([str(value) for value in command], capture_output=True,
                                text=True, timeout=30)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    obj = output / "fault.o"
    compile_command([args.cc.resolve(), "-Wall", "-Wextra", "-Werror", "-c", native, "-o", obj])
    for backend in ("gcc", "gas64"):
        binary = output / ("case-" + backend)
        compile_command([args.fbc.resolve(), "-prefix", root, "-i", root / "inc", "-gen", backend,
                         "-mt", source, obj, "-Wl", wraps, "-x", binary])
        for mode in ("normal", "init", "buffer", "source", "configure", "play", "stop",
                     "delete-source", "delete-buffer", "exit"):
            environment = os.environ.copy()
            environment.update(AL_TEST_FAILURE=mode, FB_EXAMPLE_AUTOPLAY="1")
            result = subprocess.run([str(binary), "controlled.wav"], env=environment,
                                    capture_output=True, text=True, timeout=5)
            if result.returncode != (0 if mode == "normal" else 1) or "OPENAL_FAILURE=PASS" not in result.stdout:
                raise RuntimeError(mode + ": " + result.stdout + result.stderr)
            records.append({"backend": backend, "mode": mode, "status": "pass", "output": result.stdout})
    receipt.update(status="pass", records=records,
                   scope="controlled amd64 API failures and ownership; no playback claim")
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("OPENAL_FAILURE_REGRESSION=PASS cases=" + str(len(records)), flush=True)


if __name__ == "__main__":
    main()
# end of test-openal-failures.py
