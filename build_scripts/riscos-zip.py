#!/usr/bin/env python3
"""
Project: FreeBASIC RISC OS package archive support
-------------------------------------------------

File: riscos-zip.py

Purpose:

    Create and verify ZIP archives that preserve RISC OS file types.

Responsibilities:

    - translate GCCSDK's `leaf,xyz` host naming convention to RISC OS types
    - write Acorn SparkFS metadata for every archive member
    - preserve source modification times in RISC OS five-byte time format
    - reject links, ambiguous member names, and malformed archive metadata

This file intentionally does NOT contain:

    - RiscPkg package metadata generation
    - FreeBASIC build logic
    - archive extraction or installation
"""

from __future__ import print_function

import argparse
import os
import re
import stat
import struct
import sys
import time
import zipfile


ACORN_EXTRA_ID = 0x4341
ACORN_EXTRA_MAGIC = b"ARC0"
DIRECTORY_FILETYPE = 0xFFD
DEFAULT_FILETYPE = 0xFFF
FILETYPE_PATTERN = re.compile(r"^(.*),([0-9a-fA-F]{3})$")
RISC_OS_EPOCH_OFFSET = 2_208_988_800


class ArchiveError(Exception):
    """Report invalid input or RISC OS metadata without a traceback."""


def split_filetype(leaf):
    """Return the archive leaf and file type encoded in a host leaf name."""

    match = FILETYPE_PATTERN.match(leaf)
    if match is None:
        return leaf, DEFAULT_FILETYPE

    archive_leaf = match.group(1)
    if not archive_leaf:
        raise ArchiveError("empty leaf before RISC OS file type: " + leaf)

    return archive_leaf, int(match.group(2), 16)


def riscos_timestamp(unix_timestamp):
    """Convert Unix seconds to the unsigned RISC OS centisecond clock."""

    centiseconds = int((unix_timestamp + RISC_OS_EPOCH_OFFSET) * 100)
    if centiseconds < 0:
        centiseconds = 0
    return centiseconds & ((1 << 40) - 1)


def sparkfs_extra(filetype, unix_timestamp):
    """Build one Acorn SparkFS extra-field record."""

    timestamp = riscos_timestamp(unix_timestamp)
    load_address = (
        0xFFF00000
        | ((filetype & 0xFFF) << 8)
        | ((timestamp >> 32) & 0xFF)
    )
    execution_address = timestamp & 0xFFFFFFFF
    attributes = 0x33

    return struct.pack(
        "<HH4sIIII",
        ACORN_EXTRA_ID,
        20,
        ACORN_EXTRA_MAGIC,
        load_address,
        execution_address,
        attributes,
        0,
    )


def zip_datetime(unix_timestamp):
    """Return a ZIP-compatible local timestamp."""

    value = time.localtime(unix_timestamp)[:6]
    year = min(max(value[0], 1980), 2107)
    return (year,) + value[1:]


def make_zip_info(member_name, source_stat, filetype, is_directory):
    """Create one Acorn-origin ZipInfo record."""

    info = zipfile.ZipInfo(member_name, zip_datetime(source_stat.st_mtime))
    info.create_system = 13
    info.create_version = 20
    info.extract_version = 20
    info.extra = sparkfs_extra(filetype, source_stat.st_mtime)

    if is_directory:
        info.compress_type = zipfile.ZIP_STORED
        info.external_attr = (stat.S_IFDIR | 0o755) << 16
        info.external_attr |= 0x10
    else:
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = (stat.S_IFREG | 0o644) << 16

    return info


def collect_members(source_root):
    """Return deterministic source/member tuples for a staging tree."""

    members = []
    archive_names = set()

    for directory, child_dirs, child_files in os.walk(source_root):
        child_dirs.sort()
        child_files.sort()

        relative_directory = os.path.relpath(directory, source_root)
        archive_directory = "" if relative_directory == "." else relative_directory

        for child_dir in child_dirs:
            source_path = os.path.join(directory, child_dir)
            if os.path.islink(source_path):
                raise ArchiveError("symbolic links are not supported: " + source_path)

            member_name = os.path.join(archive_directory, child_dir).replace(
                os.sep, "/"
            ) + "/"
            if member_name in archive_names:
                raise ArchiveError("duplicate archive member: " + member_name)
            archive_names.add(member_name)
            members.append(
                (source_path, member_name, DIRECTORY_FILETYPE, True)
            )

        for child_file in child_files:
            source_path = os.path.join(directory, child_file)
            if os.path.islink(source_path):
                raise ArchiveError("symbolic links are not supported: " + source_path)
            if not os.path.isfile(source_path):
                raise ArchiveError("unsupported source entry: " + source_path)

            archive_leaf, filetype = split_filetype(child_file)
            member_name = os.path.join(
                archive_directory, archive_leaf
            ).replace(os.sep, "/")
            if member_name in archive_names:
                raise ArchiveError("duplicate archive member: " + member_name)
            archive_names.add(member_name)
            members.append((source_path, member_name, filetype, False))

    members.sort(key=lambda member: member[1])
    return members


def create_archive(source_root, archive_path):
    """Create a RISC OS metadata-preserving ZIP archive."""

    source_root = os.path.abspath(source_root)
    archive_path = os.path.abspath(archive_path)

    if not os.path.isdir(source_root):
        raise ArchiveError("staging tree is not a directory: " + source_root)
    if archive_path == source_root or archive_path.startswith(source_root + os.sep):
        raise ArchiveError("archive output must be outside the staging tree")

    members = collect_members(source_root)
    if not members:
        raise ArchiveError("staging tree is empty: " + source_root)

    output_directory = os.path.dirname(archive_path)
    if not os.path.isdir(output_directory):
        os.makedirs(output_directory)

    temporary_path = archive_path + ".tmp"
    try:
        with zipfile.ZipFile(
            temporary_path,
            mode="w",
            compression=zipfile.ZIP_DEFLATED,
            compresslevel=9,
            allowZip64=True,
        ) as archive:
            for source_path, member_name, filetype, is_directory in members:
                source_stat = os.stat(source_path, follow_symlinks=False)
                info = make_zip_info(
                    member_name, source_stat, filetype, is_directory
                )
                if is_directory:
                    archive.writestr(info, b"")
                else:
                    with open(source_path, "rb") as source_file:
                        archive.writestr(info, source_file.read())
        os.replace(temporary_path, archive_path)
    finally:
        if os.path.exists(temporary_path):
            os.unlink(temporary_path)

    check_archive(archive_path)


def read_filetype(info):
    """Extract and validate the Acorn SparkFS file type from a member."""

    offset = 0
    while offset + 4 <= len(info.extra):
        field_id, field_size = struct.unpack_from("<HH", info.extra, offset)
        offset += 4
        field_end = offset + field_size
        if field_end > len(info.extra):
            raise ArchiveError("truncated extra field on " + info.filename)

        if field_id == ACORN_EXTRA_ID:
            if field_size != 20:
                raise ArchiveError("invalid SparkFS field size on " + info.filename)
            magic, load_address, _, _, reserved = struct.unpack_from(
                "<4sIIII", info.extra, offset
            )
            if magic != ACORN_EXTRA_MAGIC or reserved != 0:
                raise ArchiveError("invalid SparkFS metadata on " + info.filename)
            if (load_address & 0xFFF00000) != 0xFFF00000:
                raise ArchiveError("missing RISC OS file type on " + info.filename)
            return (load_address >> 8) & 0xFFF

        offset = field_end

    raise ArchiveError("missing SparkFS metadata on " + info.filename)


def check_archive(archive_path, requirements=None):
    """Validate origin metadata, file types, names, and member integrity."""

    archive_path = os.path.abspath(archive_path)
    if not os.path.isfile(archive_path):
        raise ArchiveError("archive does not exist: " + archive_path)

    with zipfile.ZipFile(archive_path, mode="r") as archive:
        members = archive.infolist()
        if not members:
            raise ArchiveError("archive has no members: " + archive_path)

        names = set()
        filetypes = {}
        for info in members:
            if info.filename in names:
                raise ArchiveError("duplicate archive member: " + info.filename)
            names.add(info.filename)

            if info.create_system != 13:
                raise ArchiveError("member is not Acorn-origin: " + info.filename)
            if FILETYPE_PATTERN.search(os.path.basename(info.filename.rstrip("/"))):
                raise ArchiveError("host file-type suffix leaked: " + info.filename)

            filetype = read_filetype(info)
            filetypes[info.filename] = filetype
            expected = DIRECTORY_FILETYPE if info.is_dir() else None
            if expected is not None and filetype != expected:
                raise ArchiveError("invalid directory file type: " + info.filename)

        corrupt_member = archive.testzip()
        if corrupt_member is not None:
            raise ArchiveError("CRC failure in archive member: " + corrupt_member)

        for member_name, expected_filetype in requirements or []:
            if member_name not in filetypes:
                raise ArchiveError("required archive member is missing: " + member_name)
            if filetypes[member_name] != expected_filetype:
                raise ArchiveError(
                    "required member {} has file type &{:03X}, expected &{:03X}".format(
                        member_name, filetypes[member_name], expected_filetype
                    )
                )

    return len(members)


def parse_arguments(argv):
    """Parse the command line without hidden defaults."""

    parser = argparse.ArgumentParser(
        description="Create or verify a RISC OS metadata-preserving ZIP archive."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    create_parser = subparsers.add_parser("create", help="create an archive")
    create_parser.add_argument("source", help="staging directory")
    create_parser.add_argument("archive", help="output ZIP path")

    check_parser = subparsers.add_parser("check", help="verify an archive")
    check_parser.add_argument("archive", help="ZIP path to verify")
    check_parser.add_argument(
        "--require",
        action="append",
        default=[],
        metavar="MEMBER=TYPE",
        help="require an archive member with one three-digit hexadecimal type",
    )

    return parser.parse_args(argv)


def main(argv):
    """Run one requested archive operation."""

    arguments = parse_arguments(argv)
    if arguments.command == "create":
        create_archive(arguments.source, arguments.archive)
        member_count = check_archive(arguments.archive)
        print("created {} with {} members".format(arguments.archive, member_count))
    else:
        requirements = []
        for requirement in arguments.require:
            member_name, separator, filetype_text = requirement.rpartition("=")
            if not separator or not member_name or not re.match(
                r"^[0-9a-fA-F]{3}$", filetype_text
            ):
                raise ArchiveError(
                    "--require must use MEMBER=TYPE with a three-digit type: "
                    + requirement
                )
            requirements.append((member_name, int(filetype_text, 16)))

        member_count = check_archive(arguments.archive, requirements)
        print("verified {} with {} members".format(arguments.archive, member_count))

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except (ArchiveError, OSError, zipfile.BadZipFile) as error:
        print("ERROR: {}".format(error), file=sys.stderr)
        sys.exit(1)


# end of riscos-zip.py
