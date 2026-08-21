#!/usr/bin/env python3
"""
Project: FreeBASIC AROS release workflow
----------------------------------------

File: aros-pkg.py

Purpose:

    Create and validate the PKG1 archives understood by AROS C:Unpack.

Responsibilities:

    - encode paths and lengths with the AROS PKG1 byte layout
    - recursively package a deterministic directory tree
    - validate record boundaries, package size, and path padding
    - extract only safe relative paths for host-side verification

This file intentionally does NOT contain:

    - bzip2 compression
    - AROS package installation policy
    - FreeBASIC-specific staging rules

Format note:

    A PKG1 path-length field stores the number of bytes before the final
    padding byte.  The following path field is path_length + 1 bytes long and
    contains a NUL-terminated ISO-8859-1 name padded to a four-byte boundary.
    AROS ignores the padding after the first NUL.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path, PurePosixPath
import shutil
import struct
import sys
from typing import BinaryIO, Iterator


MAGIC = b"PKG"
VERSION = 1
HEADER_SIZE = 8
UINT32 = struct.Struct("!I")


class PackageError(Exception):
    """Raised when a package cannot be safely created or decoded."""


def _write_uint32(stream: BinaryIO, value: int) -> None:
    if value < 0 or value > 0xFFFFFFFF:
        raise PackageError(f"value does not fit in an unsigned 32-bit field: {value}")
    stream.write(UINT32.pack(value))


def _read_exact(stream: BinaryIO, count: int, description: str) -> bytes:
    data = stream.read(count)
    if len(data) != count:
        raise PackageError(f"truncated {description}")
    return data


def _read_uint32(stream: BinaryIO, description: str) -> int:
    return UINT32.unpack(_read_exact(stream, UINT32.size, description))[0]


def _encoded_path(path: PurePosixPath) -> bytes:
    path_text = path.as_posix()
    try:
        encoded = path_text.encode("iso-8859-1")
    except UnicodeEncodeError as error:
        raise PackageError(f"path is not representable in ISO-8859-1: {path_text}") from error

    if not encoded or b"\x00" in encoded:
        raise PackageError(f"invalid package path: {path_text}")

    padded_size = (len(encoded) + 4) & ~3
    return encoded + bytes(padded_size - len(encoded))


def _safe_relative_path(raw_path: bytes) -> PurePosixPath:
    terminator = raw_path.find(b"\x00")
    if terminator < 0:
        raise PackageError("package path is not NUL terminated")
    if any(raw_path[terminator:]):
        raise PackageError("package path has nonzero padding")

    try:
        path_text = raw_path[:terminator].decode("iso-8859-1")
    except UnicodeDecodeError as error:
        raise PackageError("package path is not valid ISO-8859-1") from error

    path = PurePosixPath(path_text)
    if not path_text or path.is_absolute() or ".." in path.parts:
        raise PackageError(f"unsafe package path: {path_text!r}")
    if any(part in ("", ".") for part in path.parts):
        raise PackageError(f"non-canonical package path: {path_text!r}")
    return path


def _files_below(root: Path) -> Iterator[tuple[PurePosixPath, Path]]:
    for directory, directory_names, file_names in os.walk(root):
        directory_names.sort()
        file_names.sort()
        host_directory = Path(directory)
        for file_name in file_names:
            host_path = host_directory / file_name
            if host_path.is_symlink():
                raise PackageError(f"symbolic links are not supported: {host_path}")
            if not host_path.is_file():
                raise PackageError(f"package input is not a regular file: {host_path}")
            relative = host_path.relative_to(root)
            yield PurePosixPath(relative.as_posix()), host_path


def create_package(source_root: Path, destination: Path) -> None:
    if not source_root.is_dir():
        raise PackageError(f"package source is not a directory: {source_root}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + ".new")

    try:
        with temporary.open("wb") as stream:
            stream.write(MAGIC)
            stream.write(bytes((VERSION,)))
            _write_uint32(stream, 0)

            for relative, host_path in _files_below(source_root):
                path_field = _encoded_path(relative)
                file_size = host_path.stat().st_size
                if file_size > 0xFFFFFFFF:
                    raise PackageError(f"file is too large for PKG1: {host_path}")

                _write_uint32(stream, len(path_field) - 1)
                stream.write(path_field)
                _write_uint32(stream, file_size)

                with host_path.open("rb") as source:
                    shutil.copyfileobj(source, stream, length=1024 * 1024)

            package_size = stream.tell()
            if package_size > 0xFFFFFFFF:
                raise PackageError("PKG1 archive exceeds the 4 GiB format limit")
            stream.seek(4)
            _write_uint32(stream, package_size)

        temporary.replace(destination)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise


def extract_package(source: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    package_size = source.stat().st_size

    with source.open("rb") as stream:
        magic = _read_exact(stream, len(MAGIC), "package magic")
        version = _read_exact(stream, 1, "package version")[0]
        declared_size = _read_uint32(stream, "package size")

        if magic != MAGIC:
            raise PackageError("archive does not have a PKG header")
        if version != VERSION:
            raise PackageError(f"unsupported PKG version: {version}")
        if declared_size != package_size:
            raise PackageError(
                f"declared package size {declared_size} does not match {package_size}"
            )

        while stream.tell() < package_size:
            path_length = _read_uint32(stream, "path length")
            if path_length > package_size - stream.tell() - 1:
                raise PackageError("package path extends beyond the archive")
            path_field = _read_exact(stream, path_length + 1, "path field")
            relative = _safe_relative_path(path_field)

            data_length = _read_uint32(stream, "file length")
            if data_length > package_size - stream.tell():
                raise PackageError(f"file data extends beyond the archive: {relative}")

            output = destination.joinpath(*relative.parts)
            output.parent.mkdir(parents=True, exist_ok=True)
            with output.open("wb") as target:
                remaining = data_length
                while remaining:
                    chunk = _read_exact(
                        stream,
                        min(remaining, 1024 * 1024),
                        f"file data for {relative}",
                    )
                    target.write(chunk)
                    remaining -= len(chunk)

        if stream.tell() != package_size:
            raise PackageError("package parser did not stop at the archive boundary")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create or extract an AROS PKG1 archive."
    )
    subparsers = parser.add_subparsers(dest="operation", required=True)

    create = subparsers.add_parser("create", help="create a PKG1 archive")
    create.add_argument("source", type=Path, help="directory to package")
    create.add_argument("destination", type=Path, help="output .pkg file")

    extract = subparsers.add_parser("extract", help="extract and validate PKG1")
    extract.add_argument("source", type=Path, help="input .pkg file")
    extract.add_argument("destination", type=Path, help="extraction directory")

    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.operation == "create":
            create_package(arguments.source, arguments.destination)
        else:
            extract_package(arguments.source, arguments.destination)
    except (OSError, PackageError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of aros-pkg.py
