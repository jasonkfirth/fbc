#!/usr/bin/env python3
"""
FreeBASIC classic AmigaOS toolchain support
------------------------------------------

File: amiga-build-softfloat.py

Purpose:
    Build the CPU software floating-point provider omitted by the Amiga SDK.

Responsibilities:
    - verify the GCC 6.5 assembly source and SDK conversion objects
    - assemble native Hunk helpers with the port's 68020 soft-float contract
    - publish one complete archive before Amiga programs are linked

This file intentionally does NOT contain:
    - FreeBASIC arithmetic implementations or disk-based Amiga math libraries

The pinned SDK supplies arithmetic through mathieee*.library and omits those
operations from libgcc. GCC's original lb1sf68.S supplies CPU arithmetic; the
SDK's verified xfpgnulib objects supply bit-based format and integer conversion.
Both have the GCC Runtime Library Exception and are kept separate from libfb.
"""

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import urllib.request


SOURCE_URL = "https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-6.5.0/libgcc/config/m68k/lb1sf68.S"
SOURCE_SHA256 = "a6455a8b299bb903d6fc6b7654409f3fc539b2aad5ce4058e9f79b01b378e3d9"
LICENSES = {
    'COPYING3': '8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903',
    'COPYING.RUNTIME': '9d6b43ce4d8de0c878bf16b54d8e7a10d9bd42b75178153e3af6a815bdc90f74',
}
PARTS = ("floatex", "double", "float", "eqdf2", "nedf2", "gtdf2", "gedf2",
         "ledf2", "ltdf2", "eqsf2", "nesf2", "gtsf2", "gesf2", "lesf2", "ltsf2")
CONVERSIONS = {
    "fixdfsi": "336ae5f2acd998a9a9169e1fd7d698f3ed23df18ed215b0cf6cc2eab1e5c59d9",
    "fixsfsi": "078bb79bf23d7e62999db1e4a6b125a316ba0f9c20f77b53df10cd512c2778aa",
    "floatsidf": "1c8a6c0a8382377df8ae01b69bde05c93f18cba4c588c2dd2d0351fd8c6ebcbe",
    "floatsisf": "a54e460d63aa3a9cfe3bf082fcbdae1336ec5338daa487d9a92835c234d781e4",
    "floatunsidf": "6fb692d744663da14fe5c5d288302a7cec5e4f048035838807b4201598cc26e2",
    "floatunsisf": "e4a9dd890abea9487a361449a9153661c62c96bb3886133add167634bdabe199",
}


def build(prefix: Path, work: Path, output: Path) -> None:
    work.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    source = work / "lb1sf68.S"
    if not source.exists():
        with urllib.request.urlopen(SOURCE_URL, timeout=60) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != SOURCE_SHA256:
            raise SystemExit("GCC m68k assembly source hash mismatch")
        source.write_bytes(data)
    if hashlib.sha256(source.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise SystemExit("Cached GCC m68k assembly source hash mismatch")
    for name, expected in LICENSES.items():
        license_file = work / name
        if not license_file.exists():
            url = 'https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-6.5.0/' + name
            with urllib.request.urlopen(url, timeout=60) as response:
                data = response.read()
            if hashlib.sha256(data).hexdigest() != expected:
                raise SystemExit('GCC license hash mismatch: ' + name)
            license_file.write_bytes(data)
        if hashlib.sha256(license_file.read_bytes()).hexdigest() != expected:
            raise SystemExit('Cached GCC license hash mismatch: ' + name)
        shutil.copy2(license_file, output.parent / ('fbsoftfloat-' + name + '.txt'))

    compiler = prefix / "bin/m68k-amigaos-gcc"
    archiver = prefix / "bin/m68k-amigaos-ar"
    sdk_archive = prefix / "lib/gcc/m68k-amigaos/6.5.0b/libm020/libgcc.a"
    objects = []
    for part in PARTS:
        destination = work / (part + ".o")
        subprocess.run([str(compiler), "-m68020", "-msoft-float",
                        "-x", "assembler-with-cpp", "-DL_" + part,
                        "-c", str(source), "-o", str(destination)], check=True)
        objects.append(destination)

    for name, expected_hash in CONVERSIONS.items():
        member = "xfpgnulib__" + name + ".o"
        data = subprocess.check_output([str(archiver), "p", str(sdk_archive), member])
        if hashlib.sha256(data).hexdigest() != expected_hash:
            raise SystemExit("Unsupported SDK software conversion object: " + member)
        destination = work / member
        destination.write_bytes(data)
        objects.append(destination)

    conversion_source = Path(__file__).resolve().parent / "amiga/softfloat-convert.c"
    destination = work / "softfloat-convert.o"
    subprocess.run([str(compiler), "-m68020", "-msoft-float", "-O2",
                    "-ffreestanding", "-fno-builtin", "-Wall", "-Wextra",
                    "-c", str(conversion_source), "-o", str(destination)], check=True)
    objects.append(destination)

    temporary = output.with_suffix(".a.new")
    # ar replaces named members but retains unnamed old members. Always create
    # this temporary archive afresh so an interrupted older build cannot add
    # unrelated objects to a successful new archive.
    if temporary.exists():
        temporary.unlink()
    subprocess.run([str(archiver), "rcs", str(temporary),
                    *map(str, objects)], check=True)
    temporary.replace(output)
    print("Built Amiga CPU floating-point archive:", output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain-root", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    build(arguments.toolchain_root.resolve(), arguments.work.resolve(), arguments.output.resolve())

# end of amiga-build-softfloat.py
