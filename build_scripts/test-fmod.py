#!/usr/bin/env python3
"""
FreeBASIC FMOD 3.75 qualification
File: test-fmod.py
Purpose: Test native players and metadata with the genuine matching SDK.
Responsibilities: C byte-range comparison, Unicode fixtures and error paths.
This file contains no substitute decoder, driver or dependency installation.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import time


def run(arguments, directory, environment, expected=0):
    result = subprocess.run([str(value) for value in arguments], cwd=directory,
                            env=environment, capture_output=True, text=True,
                            encoding="utf-8", timeout=30, check=False)
    if result.returncode != expected:
        raise RuntimeError(f"exit {result.returncode}: {arguments}\n{result.stdout}{result.stderr}")
    return result.stdout


def make_fixtures(prefix, directory):
    source = (prefix / "media/jules.mp3").read_bytes()
    ordinary = {"TITLE": "Fixture Title", "ARTIST": "Fixture Artist", "ALBUM": "Fixture Album"}
    unicode_text = {"TITLE": "Caf\u00e9 \u03a9 \U0001f600", "ARTIST": "\u00c5ngstr\u00f6m", "ALBUM": "\u4e2d\u6587"}
    expected = {"untagged.mp3": {key: "" for key in ordinary}, "id3v1.mp3": ordinary}
    (directory / "untagged.mp3").write_bytes(source)
    footer = b"TAG" + ordinary["TITLE"].encode().ljust(30, b"\0")
    footer += ordinary["ARTIST"].encode().ljust(30, b"\0") + ordinary["ALBUM"].encode().ljust(30, b"\0")
    footer += b"2026" + b"Qualification".ljust(30, b"\0") + b"\0"
    assert len(footer) == 128
    (directory / "id3v1.mp3").write_bytes(source + footer)
    for name, encoding, codec, bom, version, labels in (
        ("id3v2.mp3", 0, "latin-1", b"", 3, ordinary),
        ("utf16le.mp3", 1, "utf-16-le", b"\xff\xfe", 3, unicode_text),
        ("utf16be.mp3", 1, "utf-16-be", b"\xfe\xff", 3, unicode_text),
        ("utf16be-no-bom.mp3", 2, "utf-16-be", b"", 4, unicode_text),
        ("utf8.mp3", 3, "utf-8", b"", 4, unicode_text),
    ):
        frames = b""
        for tag, key in (("TIT2", "TITLE"), ("TPE1", "ARTIST"), ("TALB", "ALBUM")):
            body = bytes([encoding]) + bom + labels[key].encode(codec)
            body += b"\0\0" if encoding in (1, 2) else b"\0"
            # These short frame lengths are identical in big-endian and
            # synchsafe form, so both ID3v2.3 and ID3v2.4 fixtures are valid.
            assert len(body) < 128
            frames += tag.encode() + struct.pack(">I", len(body)) + b"\0\0" + body
        length = len(frames)
        header = b"ID3" + bytes([version, 0, 0])
        header += bytes(((length >> 21) & 127, (length >> 14) & 127, (length >> 7) & 127, length & 127))
        (directory / name).write_bytes(header + frames + source)
        expected[name] = labels
    (directory / "invalid.mp3").write_bytes(b"This is not an MPEG audio stream.\n")
    return expected


def raw_tags(output):
    values = {}
    for line in output.splitlines():
        match = re.fullmatch(r"TAG (\S+)\s+(\d+)\s+(\d+)\s*([0-9a-f]*)", line)
        if match:
            values[match[1]] = (int(match[2]), int(match[3]), match[4])
    assert len(values) == 6, output
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--sdk-prefix", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--tool-dir", type=Path)
    parser.add_argument("--metadata-only", action="store_true",
                        help="qualify native metadata without claiming output-device support")
    parser.add_argument("--oss-proxy", type=Path)
    parser.add_argument("--oss-backend", type=Path)
    args = parser.parse_args()
    if args.metadata_only and args.oss_proxy:
        parser.error("--metadata-only does not start an output driver")
    if bool(args.oss_proxy) != bool(args.oss_backend):
        parser.error("supply both OSS proxy and backend paths")
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    prefix = args.sdk_prefix.resolve()
    fbc = args.fbc.resolve()
    windows = os.name == "nt"
    environment = os.environ.copy()
    environment["FB_EXAMPLE_AUTOPLAY"] = "1"
    compiler = [fbc, "-prefix", root, "-i", root / "inc", "-p", prefix / "lib", "-w", "all"]
    if windows:
        if args.tool_dir is None:
            parser.error("--tool-dir is required for the qualified Win64 SDK")
        tools = args.tool_dir.resolve()
        environment["PATH"] = str(prefix / "bin") + ";" + str(tools) + ";" + environment["PATH"]
        for name in ("gcc", "as", "ld", "ar"):
            environment[name.upper()] = str(tools / (name + ".exe"))
        gcc = tools / "gcc.exe"
        c_arguments = [prefix / "lib/fmod64vc.lib"]
        backends = ("gcc", "gas64")
        suffix = ".exe"
    else:
        assert (prefix / "lib/libfmod-3.75.so").read_bytes()[:5] == b"\x7fELF\x01"
        environment.pop("LD_PRELOAD", None)
        environment["LD_LIBRARY_PATH"] = str(prefix / "lib") + ":" + environment.get("LD_LIBRARY_PATH", "")
        compiler += ["-target", "linux", "-arch", "686"]
        gcc = "gcc"
        c_arguments = ["-m32", "-L" + str(prefix / "lib"), "-lfmod"]
        backends = ("gcc", "gas")
        suffix = ""
    fixtures = output / "fixtures"
    fixtures.mkdir(exist_ok=True)
    expected = make_fixtures(prefix, fixtures)
    native = output / ("tags-c" + suffix)
    # Library arguments follow the source so GNU ld also resolves ELF calls.
    run([gcc, "-Wall", "-Wextra", "-Werror", "-I" + str(prefix / "include"),
         root / "tests/bindings/fmod-tags.c", *c_arguments, "-o", native], output, environment)
    references = {name: raw_tags(run([native, fixtures / name], output, environment)) for name in expected}
    records = []
    for backend in backends:
        probe = output / ("tags-fb-" + backend + suffix)
        run(compiler + ["-gen", backend, "-i", root / "examples/sound/FMOD",
                        root / "tests/bindings/fmod-tags.bas", "-x", probe], output, environment)
        for name, labels in expected.items():
            observed = run([probe, fixtures / name], output, environment)
            assert "FMOD_TEXT_BOUNDARIES=PASS" in observed
            assert raw_tags(observed) == references[name], name
            for key, text in labels.items():
                assert "TEXT " + key + " " + text in observed, (name, observed)
            records.append({"test": "metadata", "backend": backend, "fixture": name, "status": "pass"})
        print("FMOD_METADATA=PASS " + backend + " fixtures=" + str(len(expected)), flush=True)

    node = Path("/dev/dsp")
    proxy = None
    log = None
    try:
        if args.oss_proxy:
            assert not node.exists(), "an existing OSS driver must not be replaced"
            log = (output / "oss-proxy.log").open("w")
            proxy = subprocess.Popen([str(args.oss_proxy.resolve()), "-f", "--adsp=", "--mixer=",
                                      "--max=4", "--umax=4", "--log=4",
                                      "--dsp-slave=" + str(args.oss_backend.resolve())],
                                     stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            deadline = time.monotonic() + 10
            while not node.exists() and proxy.poll() is None and time.monotonic() < deadline:
                time.sleep(0.1)
            assert node.exists() and proxy.poll() is None, "OSS device registration failed"
        for backend in backends:
            for relative in ("examples/manual/libraries/fmod1.bas", "examples/manual/libraries/fmod2.bas", "examples/sound/FMOD/mp3-player.bas"):
                binary = output / (Path(relative).stem + "-" + backend + suffix)
                run(compiler + ["-gen", backend, root / relative, "-x", binary], output, environment)
                if args.metadata_only:
                    records.append({"test": "example-compile", "source": relative,
                                    "backend": backend, "status": "pass"})
                    continue
                names = [root / "examples/sound/data/dne_trtn.mod"] if relative.endswith("fmod1.bas") else list(fixtures / name for name in expected)
                for media in names:
                    observed = run([binary, media], output, environment)
                    assert "FMOD playback completed." in observed
                    if media.name in expected:
                        assert all(value in observed for value in expected[media.name].values())
                    records.append({"test": "example", "source": relative, "backend": backend, "fixture": media.name, "status": "pass"})
                for bad_file in (fixtures / "invalid.mp3", output / "does-not-exist.mp3"):
                    observed = run([binary, bad_file], output, environment, expected=1)
                    assert "could not load" in observed
                    records.append({"test": "bad-file", "source": relative, "backend": backend, "status": "pass"})
                if not relative.endswith("fmod1.bas"):
                    assert "Usage:" in run([binary], output, environment, expected=1)
            print(("FMOD_EXAMPLE_COMPILE=PASS " if args.metadata_only else "FMOD_EXAMPLES=PASS ") + backend, flush=True)
    finally:
        if proxy is not None:
            proxy.terminate()
            try:
                proxy.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(proxy.pid, signal.SIGKILL)
                proxy.wait(timeout=5)
            if log is not None:
                log.close()
            assert not node.exists(), "task-owned OSS device survived driver shutdown"
    receipt = {"status": "pass", "platform": "Win64" if windows else "Linux i386",
               "records": records, "metadata_C_comparisons": len(expected) * len(backends),
               "compiler_sha256": hashlib.sha256(fbc.read_bytes()).hexdigest(),
               "driver_cleanup": "pass" if proxy is not None else "native Windows driver" if windows else "existing OSS driver"}
    receipt["scope"] = "metadata and example compilation only" if args.metadata_only else "metadata and playback"
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("FMOD_METADATA_QUALIFICATION=PASS" if args.metadata_only else "FMOD_QUALIFICATION=PASS", flush=True)


if __name__ == "__main__":
    main()

# end of test-fmod.py
