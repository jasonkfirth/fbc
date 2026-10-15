#!/usr/bin/env python3
"""FreeBASIC Linux sound write regression runner
File: test-sfx-linux-write.py
Purpose: Qualify bounded native driver writes with controlled SDK responses.
Responsibilities: Matching scalar/SIMD builds, strict diagnostics and receipts.
This file contains no playback substitute or hardware acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", type=Path, default=Path(shutil.which("gcc") or "gcc"))
    parser.add_argument("--bits", type=int, choices=(32, 64), default=struct.calcsize("P") * 8)
    parser.add_argument("--ubsan", action="store_true", help="use Clang undefined-behavior trap mode")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sources = [root / "tests/bindings/sfx-linux-write.c", root / "src/sfxlib/sfx_convert.c"]
    if args.bits == 64:
        sources.append(root / "src/sfxlib/x86_64/sfx_simd.c")
    guarded = [*sources, root / "src/sfxlib/linux/sfx_driver_alsa.c",
               root / "src/sfxlib/linux/sfx_driver_pulse.c"]
    hashes = {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest() for path in guarded}
    receipt = {"status": "pending", "bits": args.bits, "source_sha256": hashes,
               "scope": "unit-only controlled API results; no playback claim"}
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    command = [args.cc.resolve(), "-m" + str(args.bits), "-O1", "-Wall", "-Wextra", "-Werror",
               "-ffunction-sections", "-fdata-sections"]
    if args.ubsan:
        command += ["-fsanitize=undefined", "-fsanitize-trap=all"]
    binary = output / "write-regression"
    command += [*sources, "-Wl,--gc-sections",
                "-Wl,--wrap=snd_pcm_writei,--wrap=snd_pcm_wait,--wrap=snd_pcm_prepare,--wrap=nanosleep,--wrap=pa_simple_write",
                "-lasound", "-lpulse-simple", "-lpulse", "-lm", "-o", binary]
    result = subprocess.run([str(value) for value in command], capture_output=True, text=True, timeout=60)
    (output / "compile.log").write_text(result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    (output / "run.log").write_text(result.stdout + result.stderr)
    if result.returncode or "SFX_LINUX_WRITE=PASS" not in result.stdout:
        raise RuntimeError(result.stdout + result.stderr)
    receipt.update(status="pass", cc=str(args.cc.resolve()), ubsan=args.ubsan,
                   command=[str(value) for value in command], output=result.stdout)
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(result.stdout.strip(), flush=True)


if __name__ == "__main__":
    main()
# end of test-sfx-linux-write.py
