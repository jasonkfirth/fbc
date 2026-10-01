#!/usr/bin/env python3
"""
FreeBASIC classic AmigaOS build support
--------------------------------------

File: amiga-patch-toolchain.py

Purpose:
    Repair process return in the pinned Amiga GCC newlib startup object.

Responsibilities:
    - accept only the exact SDK object whose instruction layout was inspected
    - preserve an original copy and recognize an already repaired object
    - correct the saved frame used by exit(), without changing Hunk relocations

This file intentionally does NOT contain:
    - a general Hunk editor, SDK downloading, or runtime test orchestration

The pinned crt0.o was compiled with an A5 frame pointer. Its exit() restores
SP and then UNLK A5 immediately discards that restoration, returning into a
caller compiled with exit's noreturn contract. Capture the entry frame in A5
instead of its local SP, and restore A5 before the existing UNLK/RTS pair.
The original object hash makes both instruction offsets unambiguous.
"""

import argparse
import hashlib
from pathlib import Path


ORIGINAL_OBJECTS = {
    "crt0.o": "56d70c5a9cffac62ce67629baaba53727b9ac1c6f591844f408b01af0494470f",
    "libm020/crt0.o": "1ace5f7ff0a5a603505948e1b07a7accb4da17690f094de465f226f28a448e77",
}

# The SDK object has a 40-byte Hunk header before .text. The entry saves SP
# at text+12; exit restores SP at text+0x15c. Both instructions are two bytes,
# so replacement preserves every section length, branch, and relocation.
TEXT_OFFSET = 40
FIXES = (
    (TEXT_OFFSET + 12, bytes.fromhex("2b4f"), bytes.fromhex("2b4d")),
    (TEXT_OFFSET + 0x15C, bytes.fromhex("2e40"), bytes.fromhex("2a40")),
    # Startup pushes the address of __argv, whose command-line initializer
    # stores a pointer. Load that pointer instead, preserving the relocation.
    (TEXT_OFFSET + 0xE2, bytes.fromhex("4879"), bytes.fromhex("2f39")),
)


def patch_startup(path: Path, original_hash: str) -> None:
    original = path.with_suffix(".o.original")
    data = bytearray(path.read_bytes())

    restored = bytearray(data)
    for offset, old, new in FIXES:
        if data[offset:offset + 2] not in (old, new):
            raise SystemExit("Amiga startup instruction does not match the pinned SDK")
        restored[offset:offset + 2] = old
    if hashlib.sha256(restored).hexdigest() != original_hash:
        raise SystemExit("Unsupported Amiga crt0.o: use the documented pinned SDK")
    if all(data[offset:offset + 2] == new for offset, _, new in FIXES):
        print("Amiga newlib startup already repaired")
        return
    for offset, old, new in FIXES:
        data[offset:offset + 2] = new

    if original.exists() and hashlib.sha256(original.read_bytes()).hexdigest() != original_hash:
        raise SystemExit("Existing Amiga startup backup does not match the pinned SDK")
    if not original.exists():
        original.write_bytes(restored)
    temporary = path.with_suffix(".o.repaired")
    temporary.write_bytes(data)
    temporary.replace(path)
    print("Repaired pinned Amiga newlib process return")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prefix", type=Path)
    prefix = parser.parse_args().prefix
    for name, original_hash in ORIGINAL_OBJECTS.items():
        patch_startup(prefix / "m68k-amigaos/lib" / name, original_hash)

# end of amiga-patch-toolchain.py
