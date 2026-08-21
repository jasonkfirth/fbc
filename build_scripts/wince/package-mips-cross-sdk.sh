#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE MIPS release workflow
# --------------------------------------------------
#
# File: build_scripts/wince/package-mips-cross-sdk.sh
#
# Purpose:
#
#     Create a relocatable Linux-hosted FreeBASIC cross-SDK package for
#     MIPS III Windows CE systems.
#
# Responsibilities:
#
#     - validate the host compiler and complete MIPS target runtime
#     - stage headers, rtlib, gfxlib2, sfxlib, libffi, and compiler-rt
#     - bundle the pinned MIPS PE binutils and Windows CE import libraries
#     - create and byte-verify downloadable tar.xz and ZIP archives
#     - compile representative programs with each extracted package
#
# This file intentionally does NOT contain:
#
#     - compiler, runtime, or toolchain construction
#     - a host Clang distribution
#     - Windows CE ROM acquisition or emulator startup
#     - ARM Windows CE package policy
#
# Package boundary:
#
#     Clang remains a host prerequisite because it is maintained by current
#     Ubuntu releases. The obsolete MIPS PE binutils are bundled because no
#     supported Ubuntu package provides that Windows CE target.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

PACKAGE_OUTDIR="${WINCE_PACKAGE_OUTDIR:-$ROOT/out/wince/packages/compiler}"
VALIDATION_OUTDIR="${WINCE_PACKAGE_VALIDATION_OUTDIR:-$ROOT/out/wince/packages/validation/mips}"
TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$ROOT/out/wince/mips-toolchain}"
PACKAGE_REVISION="${WINCE_PACKAGE_REVISION:-1}"
KEEP_STAGE=0
STAGE_ROOT=""
TAR_EXTRACT_ROOT=""
ZIP_EXTRACT_ROOT=""

##############################################################################
# Helpers
##############################################################################

die() {
	echo "ERROR: $*" >&2
	exit 1
}

msg() {
	echo
	echo "==> $*"
}

require_value() {
	local option="$1"
	local value="${2-}"

	[ -n "$value" ] || die "$option requires a value"
}

require_command() {
	command -v "$1" >/dev/null 2>&1 ||
		die "required host tool not found: $1"
}

cleanup() {
	[ "$KEEP_STAGE" -eq 0 ] || return 0

	if [ -n "$STAGE_ROOT" ] &&
	   [[ "$STAGE_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-mips-stage.* ]] &&
	   [ -d "$STAGE_ROOT" ]; then
		rm -rf -- "$STAGE_ROOT"
	fi
	if [ -n "$TAR_EXTRACT_ROOT" ] &&
	   [[ "$TAR_EXTRACT_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-mips-tar.* ]] &&
	   [ -d "$TAR_EXTRACT_ROOT" ]; then
		rm -rf -- "$TAR_EXTRACT_ROOT"
	fi
	if [ -n "$ZIP_EXTRACT_ROOT" ] &&
	   [[ "$ZIP_EXTRACT_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-mips-zip.* ]] &&
	   [ -d "$ZIP_EXTRACT_ROOT" ]; then
		rm -rf -- "$ZIP_EXTRACT_ROOT"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/package-mips-cross-sdk.sh [options]

Options:
  --out-dir DIR         Package directory. Default: out/wince/packages/compiler
  --validation-dir DIR  Extracted-package smoke output.
                        Default: out/wince/packages/validation/mips
  --toolchain-dir DIR   Prepared MIPS PE toolchain.
                        Default: out/wince/mips-toolchain
  --revision N          Package revision. Default: 1
  --keep-stage          Preserve archive validation trees
  -h, --help            Show this help

Outputs:
  FreeBASIC-VERSION-rREV-wince-mips-cross-linux-x86_64.tar.xz
  FreeBASIC-VERSION-rREV-wince-mips-cross-linux-x86_64.zip
  FreeBASIC-VERSION-rREV-wince-mips-cross-linux-x86_64.SHA256SUMS

The packaged wrapper requires a host clang command. No Ubuntu MIPS cross-GCC
package is required.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--out-dir)
			require_value "$1" "${2-}"
			PACKAGE_OUTDIR="$2"
			shift 2
			;;
		--validation-dir)
			require_value "$1" "${2-}"
			VALIDATION_OUTDIR="$2"
			shift 2
			;;
		--toolchain-dir)
			require_value "$1" "${2-}"
			TOOLCHAIN_ROOT="$2"
			shift 2
			;;
		--revision)
			require_value "$1" "${2-}"
			PACKAGE_REVISION="$2"
			shift 2
			;;
		--keep-stage)
			KEEP_STAGE=1
			shift
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

case "$PACKAGE_REVISION" in
	''|*[!0-9]*|0) die "--revision must be a positive integer" ;;
esac

##############################################################################
# Input validation
##############################################################################

for tool in clang diff file grep readelf sha256sum tar unzip zip; do
	require_command "$tool"
done

VERSION_FILE="$ROOT/mk/version.mk"
COMPILER="$ROOT/bin/fbc"
RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"
WRAPPER="$ROOT/src/tools/wince/fbc-wince-mips"

[ -f "$VERSION_FILE" ] || die "version file not found: $VERSION_FILE"
[ -x "$COMPILER" ] || die "prepared host compiler not found: $COMPILER"
[ -x "$WRAPPER" ] || die "Windows CE MIPS wrapper not found: $WRAPPER"
[ -d "$ROOT/inc" ] || die "FreeBASIC include tree not found"
[ -d "$RUNTIME_ROOT" ] || die "Windows CE MIPS runtime not found"
[ -d "$TOOLCHAIN_ROOT/bin" ] || die "MIPS PE toolchain not found"

FBVERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$VERSION_FILE")"
[ -n "$FBVERSION" ] || die "could not read FBVERSION from $VERSION_FILE"

COMPILER_DESCRIPTION="$(file -b "$COMPILER")"
[[ "$COMPILER_DESCRIPTION" == *"ELF 64-bit LSB"* ]] ||
	die "host compiler is not a Linux x86_64 executable: $COMPILER_DESCRIPTION"
readelf -h "$COMPILER" | grep -q 'Machine:.*Advanced Micro Devices X86-64' ||
	die "prepared compiler has the wrong host machine type"

REQUIRED_RUNTIME_FILES=(
	crt0.o
	fbrt0.o
	fbrt1.o
	fbrt2.o
	libfb.a
	libfbmt.a
	libfbgfx.a
	libfbgfxmt.a
	libsfx.a
	libsfxmt.a
	libffi.a
	libclang_rt.builtins-mips.a
	libcoredll.a
	libwinsock.a
	include/ffi.h
	include/ffitarget.h
	licenses/libffi/LICENSE
	licenses/compiler-rt/LICENSE.TXT
)
for runtime_file in "${REQUIRED_RUNTIME_FILES[@]}"; do
	[ -f "$RUNTIME_ROOT/$runtime_file" ] ||
		die "incomplete Windows CE MIPS runtime: missing $runtime_file"
done

REQUIRED_TOOLCHAIN_FILES=(
	bin/mips-wince-pe-ar
	bin/mips-wince-pe-dlltool
	bin/mips-wince-pe-ld
	bin/mips-wince-pe-objdump
	bin/mips-wince-pe-ranlib
	include/windows.h
	lib/libcoredll.a
	lib/libwinsock.a
	licenses/binutils/COPYING3
	licenses/mingw/DISCLAIMER
	MANIFEST.txt
)
for toolchain_file in "${REQUIRED_TOOLCHAIN_FILES[@]}"; do
	[ -f "$TOOLCHAIN_ROOT/$toolchain_file" ] ||
		die "incomplete MIPS PE toolchain: missing $toolchain_file"
done

##############################################################################
# Package staging and archive validation
##############################################################################

mkdir -p "$PACKAGE_OUTDIR" "$VALIDATION_OUTDIR"
PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
VALIDATION_OUTDIR="$(cd "$VALIDATION_OUTDIR" && pwd)"
[ "$PACKAGE_OUTDIR" != / ] || die "refusing to use / as package directory"

STAGE_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-mips-stage.XXXXXX")"
TAR_EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-mips-tar.XXXXXX")"
ZIP_EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-mips-zip.XXXXXX")"

PACKAGE_NAME="FreeBASIC-${FBVERSION}-r${PACKAGE_REVISION}-wince-mips-cross-linux-x86_64"
PACKAGE_ROOT="$STAGE_ROOT/$PACKAGE_NAME"
PACKAGE_RUNTIME="$PACKAGE_ROOT/lib/freebasic/wince-mips32el"
PACKAGE_TOOLCHAIN="$PACKAGE_ROOT/toolchain/mips-wince-pe"
TAR_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.tar.xz"
ZIP_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.zip"
SUMS_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.SHA256SUMS"

msg "staging $PACKAGE_NAME"
mkdir -p "$PACKAGE_ROOT/bin" "$PACKAGE_ROOT/include/freebasic" \
	"$PACKAGE_RUNTIME" "$PACKAGE_ROOT/toolchain" "$PACKAGE_ROOT/doc"
cp "$COMPILER" "$PACKAGE_ROOT/bin/fbc"
cp "$WRAPPER" "$PACKAGE_ROOT/bin/fbc-wince-mips"
chmod 755 "$PACKAGE_ROOT/bin/fbc" "$PACKAGE_ROOT/bin/fbc-wince-mips"
cp -a "$ROOT/inc/." "$PACKAGE_ROOT/include/freebasic/"
cp -a "$RUNTIME_ROOT/." "$PACKAGE_RUNTIME/"
cp -a "$TOOLCHAIN_ROOT" "$PACKAGE_TOOLCHAIN"
cp "$ROOT/readme.txt" "$PACKAGE_ROOT/doc/FreeBASIC-readme.txt"
if [ -f "$ROOT/docs/wince.md" ]; then
	cp "$ROOT/docs/wince.md" "$PACKAGE_ROOT/doc/wince.md"
fi

cat > "$PACKAGE_ROOT/README.txt" <<EOF
FreeBASIC $FBVERSION Windows CE MIPS cross-SDK

This package runs on Linux x86_64 and produces little-endian MIPS III O32
Windows CE executables. Extract it as one directory and run:

    bin/fbc-wince-mips hello.bas -x hello.exe

Host requirement:

    sudo apt-get update
    sudo apt-get install -y clang

The package includes the pinned MIPS PE binutils, Windows CE declarations and
import libraries, rtlib, gfxlib2, sfxlib, libffi, compiler-rt builtins, and all
FreeBASIC headers. It does not require gcc-mips-linux-gnu or any other Linux
MIPS cross-GCC package.

The compiler is a Linux-hosted cross-SDK. It is not an on-device Windows CE
compiler because maintained Windows CE systems do not provide the native GCC
or Clang supply chain needed by FreeBASIC's generated code.
EOF

msg "creating Windows CE MIPS compiler archives"
rm -f -- "$TAR_PATH" "$ZIP_PATH" "$SUMS_PATH"
tar --sort=name --owner=0 --group=0 --numeric-owner \
	-C "$STAGE_ROOT" -cJf "$TAR_PATH" "$PACKAGE_NAME"
(
	cd "$STAGE_ROOT"
	zip -X -q -r "$ZIP_PATH" "$PACKAGE_NAME"
)

tar -xJf "$TAR_PATH" -C "$TAR_EXTRACT_ROOT"
unzip -q "$ZIP_PATH" -d "$ZIP_EXTRACT_ROOT"
diff -qr "$PACKAGE_ROOT" "$TAR_EXTRACT_ROOT/$PACKAGE_NAME"
diff -qr "$PACKAGE_ROOT" "$ZIP_EXTRACT_ROOT/$PACKAGE_NAME"
unzip -tq "$ZIP_PATH" >/dev/null || die "ZIP archive validation failed"

##############################################################################
# Extracted-package compilation validation
##############################################################################

validate_package() {
	local extracted_root="$1"
	local label="$2"
	local package_root="$extracted_root/$PACKAGE_NAME"
	local output_prefix="$VALIDATION_OUTDIR/$label"
	local program

	"$package_root/bin/fbc-wince-mips" -O 0 -exx \
		"$ROOT/tests/wince/basic_file.bas" \
		-x "$output_prefix-basic-file.exe"
	"$package_root/bin/fbc-wince-mips" -O 0 -exx \
		"$ROOT/tests/wince/gfx_link.bas" \
		-x "$output_prefix-gfx-link.exe"
	"$package_root/bin/fbc-wince-mips" -O 0 -exx \
		"$ROOT/tests/wince/sfx_link.bas" \
		-x "$output_prefix-sfx-link.exe"

	for program in \
		"$output_prefix-basic-file.exe" \
		"$output_prefix-gfx-link.exe" \
		"$output_prefix-sfx-link.exe"; do
		"$package_root/toolchain/mips-wince-pe/bin/mips-wince-pe-objdump" \
			-f "$program" | grep -F 'file format pei-mips' >/dev/null ||
			die "package validation did not produce MIPS PE: $program"
	done
}

msg "compiling representative programs with extracted packages"
validate_package "$TAR_EXTRACT_ROOT" tar
validate_package "$ZIP_EXTRACT_ROOT" zip

(
	cd "$PACKAGE_OUTDIR"
	sha256sum "$(basename "$TAR_PATH")" "$(basename "$ZIP_PATH")" \
		> "$(basename "$SUMS_PATH")"
)

echo
echo "==> Windows CE MIPS compiler package created and validated"
echo "    $TAR_PATH"
echo "    $ZIP_PATH"
echo "    $SUMS_PATH"

# end of build_scripts/wince/package-mips-cross-sdk.sh
