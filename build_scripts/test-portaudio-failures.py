#!/usr/bin/env python3
"""FreeBASIC PortAudio example failure regression
File: test-portaudio-failures.py
Purpose: Check cleanup and callback bounds with controlled SDK responses.
Responsibilities: Native C declarations, both backends and exact exit status.
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
    source = root / "examples/sound/portaudio-sine.bas"
    native = root / "tests/bindings/portaudio-failures.c"
    hashes = {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest() for path in (source, native)}
    (output / "results.json").write_text(json.dumps({"status": "pending", "source_sha256": hashes}, indent=2) + "\n")
    wraps = "--wrap=Pa_Initialize,--wrap=Pa_Terminate,--wrap=Pa_GetDefaultOutputDevice,--wrap=Pa_GetDeviceInfo,--wrap=Pa_GetHostApiInfo,--wrap=Pa_OpenDefaultStream,--wrap=Pa_StartStream,--wrap=Pa_StopStream,--wrap=Pa_CloseStream,--wrap=Pa_Sleep"
    records = []

    def compile_command(command):
        result = subprocess.run([str(value) for value in command], capture_output=True, text=True, timeout=30)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    for bits, backends in ((64, ("gcc", "gas64")), (32, ("gcc", "gas"))):
        obj = output / ("fault" + str(bits) + ".o")
        compile_command([args.cc.resolve(), "-m" + str(bits), "-Wall", "-Wextra", "-Werror", "-c", native, "-o", obj])
        for backend in backends:
            binary = output / ("case-" + str(bits) + "-" + backend)
            flags = ["-target", "linux", "-arch", "686"] if bits == 32 else []
            compile_command([args.fbc.resolve(), "-prefix", root, "-i", root / "inc", "-gen", backend,
                             "-mt", *flags, source, obj, "-Wl", wraps, "-x", binary])
            for mode in ("normal", "init", "device", "info", "host", "open", "start", "stop", "close", "terminate"):
                environment = os.environ.copy()
                environment["PA_TEST_FAILURE"] = mode
                result = subprocess.run([str(binary)], env=environment, capture_output=True, text=True, timeout=5)
                if result.returncode != (0 if mode == "normal" else 1) or "PORTAUDIO_FAILURE=PASS" not in result.stdout:
                    raise RuntimeError(result.stdout + result.stderr)
                records.append({"bits": bits, "backend": backend, "mode": mode, "status": "pass", "output": result.stdout})
    receipt = {"status": "pass", "scope": "controlled API failures and callback memory bounds; no playback claim",
               "source_sha256": hashes, "records": records}
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("PORTAUDIO_FAILURE_REGRESSION=PASS cases=" + str(len(records)), flush=True)


if __name__ == "__main__":
    main()
# end of test-portaudio-failures.py
