#!/usr/bin/env bash
#
# Project: FreeBASIC AROS release workflow
# ----------------------------------------
#
# File: aros-package-developer.sh
#
# Purpose:
#
#     Package the AROS Developer tree required to compile and link programs
#     with a matching native FreeBASIC release.
#
# Responsibilities:
#
#     - validate the native GCC and binutils command set for one AROS target
#     - stage the complete SYS:Developer tree without host build artifacts
#     - create and verify an AROS PKG1 archive
#     - create a ZIP containing the individually downloadable package
#
# This file intentionally does NOT contain:
#
#     - AROS source or toolchain construction
#     - FreeBASIC compiler and runtime construction
#     - emulator startup or guest test execution
#
# Package layout:
#
#     The FreeBASIC startup file uses Developer:bin for GCC and elf2hunk.
#     Delivering this tree at SYS:Developer keeps that stable AROS convention
#     and avoids embedding a second compiler toolchain below SYS:FreeBASIC.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
PACKAGE_OUTDIR="${AROS_PACKAGE_OUTDIR:-$AROS_ROOT/packages}"
TARGET=""
PACKAGE_REVISION="${AROS_PACKAGE_REVISION:-1}"
KEEP_STAGE=0
STAGE_ROOT=""
EXTRACT_ROOT=""

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

cleanup() {
    if [ "$KEEP_STAGE" -eq 0 ]; then
        if [ -n "$STAGE_ROOT" ] &&
           [[ "$STAGE_ROOT" == "$PACKAGE_OUTDIR"/.developer-stage.* ]] &&
           [ -d "$STAGE_ROOT" ]; then
            rm -rf -- "$STAGE_ROOT"
        fi

        if [ -n "$EXTRACT_ROOT" ] &&
           [[ "$EXTRACT_ROOT" == "$PACKAGE_OUTDIR"/.developer-extract.* ]] &&
           [ -d "$EXTRACT_ROOT" ]; then
            rm -rf -- "$EXTRACT_ROOT"
        fi
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-package-developer.sh --target ARCH [options]

Required:
  --target ARCH       AROS architecture: x86_64, m68k, or arm.

Options:
  --aros-root DIR     AROS workspace. Default: out/aros
  --out-dir DIR       Package output directory. Default: out/aros/packages
  --revision N        Package revision. Default: 1
  --keep-stage        Keep validation staging directories for inspection.
  -h, --help          Show this help.

Input:
  out/aros/build-*/bin/*/AROS/Developer/

Outputs:
  FreeBASIC-Developer-VERSION-rREV-aros-ARCH.pkg
  FreeBASIC-Developer-VERSION-rREV-aros-ARCH.zip
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --target)
            require_value "$1" "${2-}"
            TARGET="$2"
            shift 2
            ;;
        --aros-root)
            require_value "$1" "${2-}"
            AROS_ROOT="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            PACKAGE_OUTDIR="$2"
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

case "$TARGET" in
    x86_64)
        AROS_BUILD_TARGET="pc-x86_64"
        AROS_SYSROOT_KEY="pc-x86_64"
        EXPECTED_MACHINE="x86-64"
        ;;
    m68k)
        AROS_BUILD_TARGET="amiga-m68k"
        AROS_SYSROOT_KEY="amiga-m68k"
        EXPECTED_MACHINE=""
        ;;
    arm)
        AROS_BUILD_TARGET="raspi-armhf"
        AROS_SYSROOT_KEY="raspi-arm"
        EXPECTED_MACHINE="ARM"
        ;;
    '')
        die "--target is required"
        ;;
    *)
        die "unsupported AROS architecture: $TARGET"
        ;;
esac

case "$PACKAGE_REVISION" in
    ''|*[!0-9]*) die "--revision must contain only digits" ;;
esac

##############################################################################
# Input validation
##############################################################################

VERSION_FILE="$ROOT/mk/version.mk"
PKG_TOOL="$SCRIPT_DIR/aros-pkg.py"
DEVELOPER_ROOT="$AROS_ROOT/build-$AROS_BUILD_TARGET/bin/$AROS_SYSROOT_KEY/AROS/Developer"
REQUIRED_TOOLS=(gcc as ld elfedit collect-aros)

[ -f "$VERSION_FILE" ] || die "version file not found: $VERSION_FILE"
[ -f "$PKG_TOOL" ] || die "AROS package tool not found: $PKG_TOOL"
[ -d "$DEVELOPER_ROOT" ] ||
    die "native AROS Developer tree is unavailable: $DEVELOPER_ROOT"

if [ "$TARGET" = "m68k" ]; then
    REQUIRED_TOOLS+=(elf2hunk)
fi

for tool in "${REQUIRED_TOOLS[@]}"; do
    [ -f "$DEVELOPER_ROOT/bin/$tool" ] ||
        die "native AROS $tool is unavailable: $DEVELOPER_ROOT/bin/$tool"
done

# AROS selects its libc header implementation through wrapper headers under
# aros/stdc and aros/posixc.  stdio.h is intentionally not exposed at the
# include root, so validate the actual public headers used by native GCC.
[ -f "$DEVELOPER_ROOT/include/aros/stdc/stdio.h" ] ||
    die "native AROS C stdio header is unavailable: $DEVELOPER_ROOT/include/aros/stdc/stdio.h"
[ -f "$DEVELOPER_ROOT/include/aros/posixc/stdlib.h" ] ||
    die "native AROS C stdlib header is unavailable: $DEVELOPER_ROOT/include/aros/posixc/stdlib.h"
[ -d "$DEVELOPER_ROOT/libexec/gcc" ] ||
    die "native AROS GCC support files are unavailable: $DEVELOPER_ROOT/libexec/gcc"

if [ "$TARGET" = "m68k" ]; then
    for tool in "${REQUIRED_TOOLS[@]}"; do
        description="$(file -b "$DEVELOPER_ROOT/bin/$tool")"
        [[ "$description" == *"AmigaOS loadseg()ble executable/binary"* ]] ||
            die "native AROS $tool is not m68k Hunk loadable: $description"
    done
else
    description="$(file -b "$DEVELOPER_ROOT/bin/gcc")"
    [[ "$description" == *"$EXPECTED_MACHINE"* ]] ||
        die "native AROS GCC machine does not match $TARGET: $description"
    [[ "$description" == *"AROS Research Operating System"* ]] ||
        die "native AROS GCC does not carry the AROS OSABI: $description"
fi

FBVERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$VERSION_FILE")"
[ -n "$FBVERSION" ] || die "could not read FBVERSION from $VERSION_FILE"

##############################################################################
# Package staging
##############################################################################

mkdir -p "$PACKAGE_OUTDIR"
STAGE_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.developer-stage.XXXXXX")"
EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.developer-extract.XXXXXX")"

PACKAGE_NAME="FreeBASIC-Developer-${FBVERSION}-r${PACKAGE_REVISION}-aros-${TARGET}"
PACKAGE_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.pkg"
ZIP_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.zip"
PACKAGE_ROOT="$STAGE_ROOT/package"
STAGED_DEVELOPER_ROOT="$PACKAGE_ROOT/Developer"
README_PATH="$STAGED_DEVELOPER_ROOT/Documentation/FreeBASIC-Developer-README.txt"

msg "staging $PACKAGE_NAME"
run mkdir -p "$PACKAGE_ROOT"
run cp -a "$DEVELOPER_ROOT" "$STAGED_DEVELOPER_ROOT"
run mkdir -p "$(dirname "$README_PATH")"

cat > "$README_PATH" <<EOF
FreeBASIC Developer support for AROS $TARGET

This package provides the AROS Developer: tree paired with FreeBASIC
$FBVERSION for AROS $TARGET. It contains GCC, binutils, headers, libraries,
and GCC support programs required when fbc compiles generated C source.

Install this package and the matching FreeBASIC package with:

    Unpack $PACKAGE_NAME.pkg TO SYS:
    Unpack FreeBASIC-${FBVERSION}-r${PACKAGE_REVISION}-aros-${TARGET}.pkg TO SYS:

Reboot AROS or execute SYS:FreeBASIC/S/FreeBASIC-Startup before invoking fbc.
The two packages must be installed on the matching AROS target and release.
For m68k, the Developer commands and the FreeBASIC compiler are Hunk files
for the classic Amiga LoadSeg loader.
EOF

# PKG1 records files only. Removing empty source-tree directories makes the
# extracted validation tree exactly comparable to the staged package content.
run find "$PACKAGE_ROOT" -depth -type d -empty -delete

##############################################################################
# Archive creation and validation
##############################################################################

msg "creating AROS PKG1 archive"
rm -f -- "$PACKAGE_PATH" "$ZIP_PATH"
run python3 "$PKG_TOOL" create "$PACKAGE_ROOT" "$PACKAGE_PATH"

[ -s "$PACKAGE_PATH" ] || die "package tool did not create $PACKAGE_PATH"
[ "$(head -c 3 "$PACKAGE_PATH")" = "PKG" ] ||
    die "package does not have a PKG1 header: $PACKAGE_PATH"

msg "verifying package contents"
run python3 "$PKG_TOOL" extract "$PACKAGE_PATH" "$EXTRACT_ROOT"
run diff -qr "$PACKAGE_ROOT" "$EXTRACT_ROOT"

for tool in "${REQUIRED_TOOLS[@]}"; do
    [ -f "$EXTRACT_ROOT/Developer/bin/$tool" ] ||
        die "validated package is missing Developer/bin/$tool"
done

DOWNLOAD_ROOT="$STAGE_ROOT/download"
run mkdir -p "$DOWNLOAD_ROOT"
run cp "$PACKAGE_PATH" "$DOWNLOAD_ROOT/"
run cp "$README_PATH" "$DOWNLOAD_ROOT/README.txt"
(
    cd "$DOWNLOAD_ROOT"
    run zip -9 "$ZIP_PATH" "$(basename "$PACKAGE_PATH")" README.txt
)

msg "package ready"
ls -lh "$PACKAGE_PATH" "$ZIP_PATH"

if [ "$KEEP_STAGE" -eq 1 ]; then
    echo "Staging tree: $STAGE_ROOT"
    echo "Extracted validation tree: $EXTRACT_ROOT"
fi

# end of aros-package-developer.sh
