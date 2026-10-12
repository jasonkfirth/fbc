#!/usr/bin/env python3
"""FreeBASIC native QuickLZ regression checks
File: test-quicklz.py
Purpose: Compare the binding with release headers and execute real library calls.
Responsibilities: Bounded processes, fresh outputs and explicit ABI receipts.
This file contains no library download, installation or guessed C layout.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import re
import random
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def run(command, directory, environment, log):
    try:
        result = subprocess.run([str(arg) for arg in command], cwd=directory,
                                env=environment, capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired as error:
        def decoded(value):
            return value.decode("utf-8", errors="replace") if isinstance(value, bytes) else value or ""
        log.write_text(decoded(error.stdout) + decoded(error.stderr), encoding="utf-8")
        raise
    log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode or re.search(r"\berror [0-9]+:", result.stdout + result.stderr):
        raise RuntimeError("Command failed: " + str(log))
    return result.stdout


def abi(text):
    line = next((line for line in text.splitlines() if line.startswith("ABI ")), None)
    if line is None:
        raise ValueError("No ABI record was emitted")
    values = [int(value) for value in re.findall(r"\d+", line)]
    if len(values) != 12:
        raise ValueError("Incomplete ABI record")
    return values


def file_example(args, backend, library, output, environment):
    """Use the actual example with owned fixtures and externally checked bytes."""
    work = output / (backend + "-files")
    work.mkdir(exist_ok=True)
    executable = work / ("quicklz-example.exe" if os.name == "nt" else "quicklz-example")
    executable.unlink(missing_ok=True)
    run([args.fbc.resolve(), *args.fbc_arg, "-i", ROOT / "inc", "-p", library,
         "-w", "all", "-gen", backend, ROOT / "examples/compression/QuickLZ.bas", "-x", executable],
        work, environment, work / "compile.log")
    if not executable.is_file():
        raise RuntimeError("Example compiler produced no executable")
    checks = []

    def invoke(label, mode, source, destination, expected_exit):
        result = subprocess.run([str(executable), mode, str(source), str(destination)],
                                cwd=work, env=environment, capture_output=True, text=True, timeout=15)
        (work / (label + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8")
        if result.returncode != expected_exit:
            raise RuntimeError("Unexpected example exit: " + label + ": " + str(result.returncode))
        checks.append(label)

    packets = []
    for index, data in enumerate((b"a", b"A\0B" * 29, bytes(range(256)) * 1024,
                                  random.Random(20261011).randbytes(65536))):
        original = work / (str(index) + ".input")
        compressed = work / (str(index) + ".qlz")
        restored = work / (str(index) + ".restored")
        original.write_bytes(data)
        compressed.unlink(missing_ok=True)
        restored.unlink(missing_ok=True)
        invoke("compress-" + str(index), "c", original, compressed, 0)
        invoke("decompress-" + str(index), "d", compressed, restored, 0)
        if restored.read_bytes() != data or original.read_bytes() != data:
            raise ValueError("Example round trip changed the binary contents")
        packets.append(compressed.read_bytes())
    if {packet[0] & 1 for packet in packets} != {0, 1}:
        raise ValueError("Fixtures did not cover both compressed and literal packets")
    protected = work / "protected.output"
    protected.write_bytes(b"retain this existing output")
    invoke("existing-output", "c", work / "0.input", protected, 2)
    if protected.read_bytes() != b"retain this existing output":
        raise ValueError("Nonempty destination was modified")
    input_before = (work / "0.input").read_bytes()
    invoke("same-input-output", "c", work / "0.input", work / "0.input", 2)
    if (work / "0.input").read_bytes() != input_before:
        raise ValueError("The input file was modified")
    rejected_output = work / "rejected.output"
    rejected_output.unlink(missing_ok=True)
    empty = work / "empty.input"
    empty.write_bytes(b"")
    invoke("empty-input", "c", empty, rejected_output, 2)
    invoke("missing-input", "c", work / "absent.input", rejected_output, 2)
    invoke("output-open-error", "c", work / "0.input", work / "absent-parent/output", 2)
    packet = packets[2]
    wrong_level = bytes([packet[0] ^ 12]) + packet[1:]
    over_budget = packet[:5] + struct.pack("<I", 16 * 1024 * 1024 + 1) + packet[9:]
    # An explicit invalid match token must be rejected by the actual C build.
    bad_match = bytes([0x47]) + struct.pack("<II", 17, 64) + struct.pack("<II", 0x80000003, 0)
    invalid_packets = [b"", b"\x46", b"\x46\0\0", packet[:-1], packet + b"extra",
                       wrong_level, over_budget, bad_match,
                       bytes([0x44, 4, 2, 65])]  # Literal size disagrees with payload.
    for index, invalid in enumerate(invalid_packets):
        source = work / "invalid.qlz"
        source.write_bytes(invalid)
        invoke("invalid-packet-" + str(index), "d", source, rejected_output, 5)
        if rejected_output.exists():
            raise ValueError("Rejected packet created an output file")
    large = work / "oversized.input"
    try:
        # A bounded temporary file checks rejection before buffer allocation.
        with large.open("wb") as stream:
            stream.truncate(16 * 1024 * 1024 + 401)
        invoke("oversized-encode", "c", large, rejected_output, 2)
        invoke("oversized-decode", "d", large, rejected_output, 5)
    finally:
        large.unlink(missing_ok=True)
    return dict(checks=checks, round_trips=4, passed=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--headers", type=Path, required=True)
    parser.add_argument("--libdir", type=Path, required=True)
    parser.add_argument("--fbc", type=Path, default=ROOT / "bin/fbc")
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--fbc-arg", action="append", default=[])
    parser.add_argument("--cc-arg", action="append", default=[])
    parser.add_argument("--file-example", action="store_true", help="Also exercise the file example; requires a memory-safe C library")
    parser.add_argument("--backend", nargs="+", choices=("gcc", "gas", "gas64"), default=["gcc", "gas64"])
    parser.add_argument("--output", type=Path, default=ROOT / "out/quicklz-native")
    args = parser.parse_args()
    headers = args.headers.resolve()
    library = args.libdir.resolve()
    if not (headers / "quicklz.h").is_file() or not library.is_dir():
        parser.error("Matching C headers and a native library directory are required")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    windows = os.name == "nt"
    variable = "PATH" if windows else "LD_LIBRARY_PATH"
    environment[variable] = str(library) + os.pathsep + environment.get(variable, "")
    suffix = ".exe" if windows else ""
    reference = output / ("reference" + suffix)
    reference.unlink(missing_ok=True)
    run([args.cc, *args.cc_arg, "-I", headers, ROOT / "tests/bindings/quicklz-abi.c", "-o", reference],
        output, environment, output / "reference-build.log")
    expected = abi(run([reference], output, environment, output / "reference-run.log"))
    results = []
    for backend in args.backend:
        executable = output / ("quicklz-" + backend + suffix)
        executable.unlink(missing_ok=True)
        run([args.fbc.resolve(), *args.fbc_arg, "-i", ROOT / "inc", "-p", library,
             "-w", "all", "-gen", backend, ROOT / "tests/bindings/quicklz-abi.bas", "-x", executable],
            output, environment, output / (backend + "-build.log"))
        text = run([executable], output, environment, output / (backend + "-run.log"))
        actual = abi(text)
        passed = actual == expected and "QUICKLZ_NATIVE_PASS" in text.splitlines()
        record = dict(backend=backend, reference=expected, actual=actual, passed=passed)
        if args.file_example:
            record["file_example"] = file_example(args, backend, library, output, environment)
        results.append(record)
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(results, indent=2))
    return int(not all(result["passed"] for result in results))


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-quicklz.py
