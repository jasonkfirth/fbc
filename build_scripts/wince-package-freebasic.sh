#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE release workflow
# ------------------------------------------------
#
# File: wince-package-freebasic.sh
#
# Purpose:
#
#     Create a relocatable Linux-hosted FreeBASIC cross-SDK package for
#     Windows CE ARM.
#
# Responsibilities:
#
#     - validate the host compiler and complete Windows CE ARM runtime
#     - stage headers, rtlib, gfxlib2, sfxlib, libffi, and the target wrapper
#     - create individually downloadable tar.xz and ZIP archives
#     - extract and byte-compare each archive with its staging tree
#     - compile representative programs with the extracted package
#     - preserve a package-built executable for emulator qualification
#
# This file intentionally does NOT contain:
#
#     - FreeBASIC, libffi, or CeGCC construction
#     - Windows CE ROM installation
#     - emulator startup or fbctests orchestration
#     - Windows CE MIPS package policy
#
# Package boundary:
#
#     The archive contains FreeBASIC's target-facing pieces, but does not
#     duplicate CeGCC.  Windows CE has no maintained native GCC environment
#     suitable for this compiler, so this is deliberately a Linux cross-SDK
#     rather than an on-device compiler package.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

WORK_ROOT="${WINCE_WORK_ROOT:-$ROOT/out/wince/work}"
PACKAGE_OUTDIR="${WINCE_PACKAGE_OUTDIR:-$ROOT/out/wince/packages/compiler}"
VALIDATION_OUTDIR="${WINCE_PACKAGE_VALIDATION_OUTDIR:-$ROOT/out/wince/packages/validation/arm}"
TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
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

run() {
	echo "==> $*"
	"$@"
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
	   [[ "$STAGE_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-stage.* ]] &&
	   [ -d "$STAGE_ROOT" ]; then
		rm -rf -- "$STAGE_ROOT"
	fi

	if [ -n "$TAR_EXTRACT_ROOT" ] &&
	   [[ "$TAR_EXTRACT_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-tar-extract.* ]] &&
	   [ -d "$TAR_EXTRACT_ROOT" ]; then
		rm -rf -- "$TAR_EXTRACT_ROOT"
	fi

	if [ -n "$ZIP_EXTRACT_ROOT" ] &&
	   [[ "$ZIP_EXTRACT_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-zip-extract.* ]] &&
	   [ -d "$ZIP_EXTRACT_ROOT" ]; then
		rm -rf -- "$ZIP_EXTRACT_ROOT"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince-package-freebasic.sh [options]

Options:
  --work-dir DIR       Prepared source tree. Default: out/wince/work
  --out-dir DIR        Compiler package directory. Default: out/wince/packages/compiler
  --validation-dir DIR Package smoke output. Default: out/wince/packages/validation/arm
  --image NAME         CeGCC container image. Default: $TOOLCHAIN_IMAGE
  --revision N         Package revision. Default: 1
  --keep-stage         Preserve archive validation trees.
  -h, --help           Show this help.

Outputs:
  FreeBASIC-VERSION-rREV-wince-arm-cross-linux-x86_64.tar.xz
  FreeBASIC-VERSION-rREV-wince-arm-cross-linux-x86_64.zip
  validation/arm/package-basic-file.exe
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--work-dir)
			require_value "$1" "${2-}"
			WORK_ROOT="$2"
			shift 2
			;;
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
		--image)
			require_value "$1" "${2-}"
			TOOLCHAIN_IMAGE="$2"
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

for tool in diff docker file readelf sha256sum tar unzip zip; do
	require_command "$tool"
done

VERSION_FILE="$WORK_ROOT/mk/version.mk"
COMPILER="$WORK_ROOT/bin/fbc"
RUNTIME_DIR="$WORK_ROOT/lib/freebasic/wince-arm"
WRAPPER="$WORK_ROOT/src/tools/wince/fbc-wince-arm"

[ -f "$VERSION_FILE" ] || die "version file not found: $VERSION_FILE"
[ -x "$COMPILER" ] || die "prepared host compiler not found: $COMPILER"
[ -d "$WORK_ROOT/inc" ] || die "FreeBASIC include tree not found: $WORK_ROOT/inc"
[ -d "$RUNTIME_DIR" ] || die "Windows CE ARM runtime not found: $RUNTIME_DIR"
[ -x "$WRAPPER" ] || die "Windows CE ARM compiler wrapper not found: $WRAPPER"

FBVERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$VERSION_FILE")"
[ -n "$FBVERSION" ] || die "could not read FBVERSION from $VERSION_FILE"

COMPILER_DESCRIPTION="$(file -b "$COMPILER")"
[[ "$COMPILER_DESCRIPTION" == *"ELF 64-bit LSB"* ]] ||
	die "prepared compiler is not a Linux x86_64 executable: $COMPILER_DESCRIPTION"
readelf -h "$COMPILER" | grep -q 'Machine:.*Advanced Micro Devices X86-64' ||
	die "prepared compiler has the wrong machine type"

REQUIRED_RUNTIME_FILES=(
	fbrt0.o
	fbrt1.o
	fbrt2.o
	libfb.a
	libfbmt.a
	libffi.a
	libfbgfx.a
	libfbgfxmt.a
	libsfx.a
	libsfxmt.a
)

for runtime_file in "${REQUIRED_RUNTIME_FILES[@]}"; do
	[ -f "$RUNTIME_DIR/$runtime_file" ] ||
		die "incomplete Windows CE ARM runtime: missing $runtime_file"
done

[ -f "$RUNTIME_DIR/include/ffi.h" ] || die "libffi public header is missing"
[ -f "$RUNTIME_DIR/include/ffitarget.h" ] || die "libffi target header is missing"
[ -f "$RUNTIME_DIR/licenses/libffi/LICENSE" ] || die "libffi license is missing"

docker image inspect "$TOOLCHAIN_IMAGE" >/dev/null 2>&1 ||
	die "CeGCC container image does not exist: $TOOLCHAIN_IMAGE"

##############################################################################
# Package staging
##############################################################################

mkdir -p "$PACKAGE_OUTDIR" "$VALIDATION_OUTDIR"
PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
VALIDATION_OUTDIR="$(cd "$VALIDATION_OUTDIR" && pwd)"
[ "$PACKAGE_OUTDIR" != / ] || die "refusing to use / as the package directory"

STAGE_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-stage.XXXXXX")"
TAR_EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-tar-extract.XXXXXX")"
ZIP_EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-zip-extract.XXXXXX")"

PACKAGE_NAME="FreeBASIC-${FBVERSION}-r${PACKAGE_REVISION}-wince-arm-cross-linux-x86_64"
PACKAGE_ROOT="$STAGE_ROOT/$PACKAGE_NAME"
PACKAGE_RUNTIME="$PACKAGE_ROOT/lib/freebasic/wince-arm"
TAR_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.tar.xz"
ZIP_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.zip"

msg "staging $PACKAGE_NAME"
run mkdir -p \
	"$PACKAGE_ROOT/bin" \
	"$PACKAGE_ROOT/include/freebasic" \
	"$PACKAGE_RUNTIME" \
	"$PACKAGE_ROOT/doc"

run cp "$COMPILER" "$PACKAGE_ROOT/bin/fbc"
run cp "$WRAPPER" "$PACKAGE_ROOT/bin/fbc-wince-arm"
run chmod 755 "$PACKAGE_ROOT/bin/fbc" "$PACKAGE_ROOT/bin/fbc-wince-arm"
run cp -a "$WORK_ROOT/inc/." "$PACKAGE_ROOT/include/freebasic/"
run cp -a "$RUNTIME_DIR/." "$PACKAGE_RUNTIME/"
run cp "$WORK_ROOT/readme.txt" "$PACKAGE_ROOT/doc/FreeBASIC-readme.txt"

if [ -f "$WORK_ROOT/debian/copyright" ]; then
	run cp "$WORK_ROOT/debian/copyright" "$PACKAGE_ROOT/doc/copyright"
fi
if [ -f "$WORK_ROOT/docs/wince.md" ]; then
	run cp "$WORK_ROOT/docs/wince.md" "$PACKAGE_ROOT/doc/wince.md"
fi

cat > "$PACKAGE_ROOT/README.txt" <<EOF
FreeBASIC $FBVERSION Windows CE ARM cross-SDK

This package runs on Linux x86_64 and produces ARMv4T software-float Windows
CE executables. Extract it as one directory and invoke bin/fbc-wince-arm.

The wrapper requires the arm-mingw32ce CeGCC tools on PATH. The reproducible
FreeBASIC build uses the pinned toolchain image named in docs/wince.md; the
compiler archive intentionally does not redistribute that external toolchain.

Example:

    bin/fbc-wince-arm hello.bas -x hello.exe

The package contains the normal and multithreaded runtime, gfxlib2, sfxlib,
libffi, and all FreeBASIC headers. It is a host cross-SDK, not an on-device
compiler: maintained Windows CE systems do not provide the native GCC supply
chain that fbc needs to turn generated C into applications.
EOF

##############################################################################
# Archive creation and structural validation
##############################################################################

msg "creating Windows CE ARM compiler archives"
rm -f -- "$TAR_PATH" "$ZIP_PATH"
run tar --sort=name --owner=0 --group=0 --numeric-owner \
	-C "$STAGE_ROOT" -cJf "$TAR_PATH" "$PACKAGE_NAME"
(
	cd "$STAGE_ROOT"
	zip -X -q -r "$ZIP_PATH" "$PACKAGE_NAME"
)

run tar -xJf "$TAR_PATH" -C "$TAR_EXTRACT_ROOT"
run unzip -q "$ZIP_PATH" -d "$ZIP_EXTRACT_ROOT"
run diff -qr "$PACKAGE_ROOT" "$TAR_EXTRACT_ROOT/$PACKAGE_NAME"
run diff -qr "$PACKAGE_ROOT" "$ZIP_EXTRACT_ROOT/$PACKAGE_NAME"

TAR_CONTENTS="$TAR_EXTRACT_ROOT/tar-contents.txt"
tar -tf "$TAR_PATH" > "$TAR_CONTENTS"
for archive_member in \
	"$PACKAGE_NAME/bin/fbc" \
	"$PACKAGE_NAME/bin/fbc-wince-arm" \
	"$PACKAGE_NAME/lib/freebasic/wince-arm/libfbmt.a" \
	"$PACKAGE_NAME/lib/freebasic/wince-arm/libfbgfxmt.a" \
	"$PACKAGE_NAME/lib/freebasic/wince-arm/libsfxmt.a" \
	"$PACKAGE_NAME/lib/freebasic/wince-arm/libffi.a"; do
	grep -Fqx "$archive_member" "$TAR_CONTENTS" ||
		die "compiler archive is missing $archive_member"
done
unzip -tq "$ZIP_PATH" >/dev/null || die "ZIP archive validation failed"

##############################################################################
# Extracted-package compilation validation
##############################################################################

msg "compiling representative programs with the extracted package"
run docker run --rm \
	--user "$(id -u):$(id -g)" \
	-v "$TAR_EXTRACT_ROOT:/validation" \
	-v "$WORK_ROOT:/source:ro" \
	-w /validation \
	"$TOOLCHAIN_IMAGE" \
	bash -ceu '
		package_root="/validation/$1"
		cp /source/tests/wince/basic_file.bas /validation/package-basic-file.bas
		cp /source/tests/wince/gfx_link.bas /validation/package-gfx-link.bas
		cp /source/tests/wince/sfx_link.bas /validation/package-sfx-link.bas
		"$package_root/bin/fbc-wince-arm" -O 0 \
			/validation/package-basic-file.bas \
			-x /validation/package-basic-file.exe
		"$package_root/bin/fbc-wince-arm" -O 0 -mt \
			/validation/package-gfx-link.bas \
			-x /validation/package-gfx-link.exe
		"$package_root/bin/fbc-wince-arm" -O 0 -mt \
			/validation/package-sfx-link.bas \
			-x /validation/package-sfx-link.exe
	' -- "$PACKAGE_NAME"

for smoke_name in basic-file gfx-link sfx-link; do
	smoke_path="$TAR_EXTRACT_ROOT/package-$smoke_name.exe"
	[ -s "$smoke_path" ] || die "package smoke executable is missing: $smoke_name"
	run cp "$smoke_path" "$VALIDATION_OUTDIR/package-$smoke_name.exe"
done

run docker run --rm \
	-v "$VALIDATION_OUTDIR:/validation:ro" \
	"$TOOLCHAIN_IMAGE" \
	bash -ceu '
		for executable in /validation/package-*.exe; do
			arm-mingw32ce-objdump -f "$executable" |
				grep -Eq "file format pei?-arm-wince-little"
		done
	'

(
	cd "$PACKAGE_OUTDIR"
	sha256sum "$(basename "$TAR_PATH")" "$(basename "$ZIP_PATH")" \
		> "$PACKAGE_NAME.SHA256SUMS"
)

msg "Windows CE ARM compiler package ready"
ls -lh "$TAR_PATH" "$ZIP_PATH" "$PACKAGE_OUTDIR/$PACKAGE_NAME.SHA256SUMS"

if [ "$KEEP_STAGE" -eq 1 ]; then
	echo "Staging tree: $STAGE_ROOT"
	echo "Tar validation tree: $TAR_EXTRACT_ROOT"
	echo "ZIP validation tree: $ZIP_EXTRACT_ROOT"
fi

# end of build_scripts/wince-package-freebasic.sh
