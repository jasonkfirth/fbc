#!/usr/bin/env bash
#
# Project: FreeBASIC Windows 95 package
# -------------------------------------
#
# File: win95-package-freebasic.sh
#
# Purpose:
#
#     Create a distributable ZIP archive from a completed Windows 95
#     FreeBASIC package tree.
#
# Responsibilities:
#
#     - validate the package identity and required Win95 toolchain files
#     - refresh the package checksum manifest
#     - create a version-named ZIP archive
#
# This file intentionally does NOT contain:
#
#     - compiler or runtime construction
#     - Windows 95 VM qualification
#     - MinGW or binutils downloads
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

PACKAGE_ROOT=""
OUT_DIR="$ROOT/out/win95/packages"

die() {
	echo "ERROR: $*" >&2
	exit 1
}

usage() {
	cat <<EOF
Usage: ./build_scripts/win95-package-freebasic.sh --package-root DIR [options]

Options:
  --package-root DIR  Completed FreeBASIC-VERSION-win95 package tree.
  --out-dir DIR       Archive output directory. Default: $OUT_DIR
  -h, --help          Show this help.

Output:
  win95_freebasic-VERSION.zip
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package-root)
			[ "$#" -ge 2 ] && [ -n "$2" ] || die "$1 requires a directory"
			PACKAGE_ROOT="$2"
			shift 2
			;;
		--out-dir)
			[ "$#" -ge 2 ] && [ -n "$2" ] || die "$1 requires a directory"
			OUT_DIR="$2"
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			die "unknown option: $1"
			;;
	esac
done

[ -n "$PACKAGE_ROOT" ] || die "--package-root is required"
[ -d "$PACKAGE_ROOT" ] || die "package directory not found: $PACKAGE_ROOT"
command -v sha256sum >/dev/null 2>&1 || die "required tool not found: sha256sum"
command -v zip >/dev/null 2>&1 || die "required tool not found: zip"
command -v unzip >/dev/null 2>&1 || die "required tool not found: unzip"

PACKAGE_ROOT="$(cd "$PACKAGE_ROOT" && pwd)"
PACKAGE_NAME="$(basename "$PACKAGE_ROOT")"
case "$PACKAGE_NAME" in
	FreeBASIC-*-win95) ;;
	*) die "package directory must be named FreeBASIC-VERSION-win95" ;;
esac
VERSION="${PACKAGE_NAME#FreeBASIC-}"
VERSION="${VERSION%-win95}"

for package_file in \
	README-WIN95.TXT \
	msvcrt40.dll \
	fbc.exe \
	bin/win32/as.exe \
	bin/win32/ld.exe \
	bin/win32/msvcrt.dll \
	lib/win32/libfb.a \
	lib/win32/libfbmt.a \
	lib/win32/libffi.a \
	lib/win32/libfbgfx.a \
	lib/win32/libsfx.a; do
	[ -s "$PACKAGE_ROOT/$package_file" ] ||
		die "required package file is missing or empty: $package_file"
done

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
ARCHIVE="$OUT_DIR/win95_freebasic-${VERSION}.zip"

(
	cd "$PACKAGE_ROOT"
	find . -type f ! -name SHA256SUMS.txt -print0 |
		sort -z |
		xargs -0 sha256sum > SHA256SUMS.txt
)

rm -f "$ARCHIVE"
(
	cd "$(dirname "$PACKAGE_ROOT")"
	zip -X -q -r "$ARCHIVE" "$PACKAGE_NAME"
)
unzip -tq "$ARCHIVE"
sha256sum "$ARCHIVE" > "$ARCHIVE.sha256"

echo "Created $ARCHIVE"

# end of win95-package-freebasic.sh
