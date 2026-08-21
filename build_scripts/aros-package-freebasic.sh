#!/usr/bin/env bash
#
# Project: FreeBASIC AROS release workflow
# ----------------------------------------
#
# File: aros-package-freebasic.sh
#
# Purpose:
#
#     Stage and package a native FreeBASIC compiler for one AROS architecture.
#
# Responsibilities:
#
#     - validate the native compiler and complete runtime library set
#     - stage a self-contained SYS:FreeBASIC installation
#     - register that installation with the AROS package startup mechanism
#     - create and verify an AROS PKG1 archive
#     - create a ZIP containing the individually downloadable package
#
# This file intentionally does NOT contain:
#
#     - AROS source or toolchain construction
#     - emulator startup
#     - fbctests, Exampleageddon, or OMA orchestration
#
# Package layout:
#
#     The compiler derives its prefix from its own executable path.  Keeping
#     bin, include/freebasic, and lib/freebasic together below SYS:FreeBASIC
#     therefore makes the package relocatable without mixing its files into
#     the operating system's Developer package.
#

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
           [[ "$STAGE_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-stage.* ]] &&
           [ -d "$STAGE_ROOT" ]; then
            rm -rf -- "$STAGE_ROOT"
        fi

        if [ -n "$EXTRACT_ROOT" ] &&
           [[ "$EXTRACT_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-extract.* ]] &&
           [ -d "$EXTRACT_ROOT" ]; then
            rm -rf -- "$EXTRACT_ROOT"
        fi
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-package-freebasic.sh --target ARCH [options]

Required:
  --target ARCH       AROS architecture: x86_64, m68k, or arm.

Options:
  --aros-root DIR     AROS workspace. Default: out/aros
  --out-dir DIR       Package output directory. Default: out/aros/packages
  --revision N        Package revision. Default: 1
  --keep-stage        Keep validation staging directories for inspection.
  -h, --help          Show this help.

Inputs:
  out/aros/freebasic/ARCH/fbc
  lib/freebasic/aros-ARCH/
  inc/

Outputs:
  FreeBASIC-${FBVERSION}-rREV-aros-ARCH.pkg
  FreeBASIC-${FBVERSION}-rREV-aros-ARCH.zip
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
    x86_64|m68k|arm) ;;
    '') die "--target is required" ;;
    *) die "unsupported AROS architecture: $TARGET" ;;
esac

case "$PACKAGE_REVISION" in
    ''|*[!0-9]*) die "--revision must contain only digits" ;;
esac

##############################################################################
# Input validation
##############################################################################

VERSION_FILE="$ROOT/mk/version.mk"
PKG_TOOL="$SCRIPT_DIR/aros-pkg.py"
COMPILER="$AROS_ROOT/freebasic/$TARGET/fbc"
RUNTIME_DIR="$ROOT/lib/freebasic/aros-$TARGET"

[ -f "$VERSION_FILE" ] || die "version file not found: $VERSION_FILE"
[ -f "$PKG_TOOL" ] || die "AROS package tool not found: $PKG_TOOL"
[ -f "$COMPILER" ] || die "native compiler not found: $COMPILER"
[ -d "$RUNTIME_DIR" ] || die "runtime directory not found: $RUNTIME_DIR"
[ -d "$ROOT/inc" ] || die "FreeBASIC include tree not found: $ROOT/inc"

FBVERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$VERSION_FILE")"
[ -n "$FBVERSION" ] || die "could not read FBVERSION from $VERSION_FILE"

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

for file in "${REQUIRED_RUNTIME_FILES[@]}"; do
    [ -f "$RUNTIME_DIR/$file" ] ||
        die "incomplete AROS runtime: missing $RUNTIME_DIR/$file"
done

[ -f "$RUNTIME_DIR/include/ffi.h" ] ||
    die "incomplete AROS runtime: missing $RUNTIME_DIR/include/ffi.h"
[ -f "$RUNTIME_DIR/include/ffitarget.h" ] ||
    die "incomplete AROS runtime: missing $RUNTIME_DIR/include/ffitarget.h"

COMPILER_DESCRIPTION="$(file -b "$COMPILER")"
if [ "$TARGET" = "m68k" ]; then
    [[ "$COMPILER_DESCRIPTION" == *"AmigaOS loadseg()ble executable/binary"* ]] ||
        die "compiler is not an AROS m68k-loadable Hunk file: $COMPILER_DESCRIPTION"
else
    case "$TARGET" in
        x86_64) EXPECTED_MACHINE='x86-64' ;;
        arm) EXPECTED_MACHINE='ARM' ;;
    esac

    [[ "$COMPILER_DESCRIPTION" == *"$EXPECTED_MACHINE"* ]] ||
        die "compiler machine does not match $TARGET: $COMPILER_DESCRIPTION"
    [[ "$COMPILER_DESCRIPTION" == *"AROS Research Operating System"* ]] ||
        die "compiler does not carry the AROS OSABI: $COMPILER_DESCRIPTION"
fi

##############################################################################
# Package staging
##############################################################################

mkdir -p "$PACKAGE_OUTDIR"
STAGE_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-stage.XXXXXX")"
EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-extract.XXXXXX")"

PACKAGE_NAME="FreeBASIC-${FBVERSION}-r${PACKAGE_REVISION}-aros-${TARGET}"
PACKAGE_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.pkg"
ZIP_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.zip"
PACKAGE_ROOT="$STAGE_ROOT/package"
FREEBASIC_ROOT="$PACKAGE_ROOT/FreeBASIC"

msg "staging $PACKAGE_NAME"
run mkdir -p \
    "$FREEBASIC_ROOT/bin" \
    "$FREEBASIC_ROOT/include/freebasic" \
    "$FREEBASIC_ROOT/lib/freebasic/aros-$TARGET" \
    "$FREEBASIC_ROOT/S" \
    "$FREEBASIC_ROOT/Documentation" \
    "$PACKAGE_ROOT/Prefs/Env-Archive/SYS/Packages"

run cp "$COMPILER" "$FREEBASIC_ROOT/bin/fbc"
run cp -a "$ROOT/inc/." "$FREEBASIC_ROOT/include/freebasic/"
run cp -a "$RUNTIME_DIR/." "$FREEBASIC_ROOT/lib/freebasic/aros-$TARGET/"
run cp "$ROOT/readme.txt" "$FREEBASIC_ROOT/Documentation/FreeBASIC-readme.txt"
run cp "$ROOT/debian/copyright" "$FREEBASIC_ROOT/Documentation/copyright"

if [ -f "$ROOT/changelog.txt" ]; then
    run cp "$ROOT/changelog.txt" "$FREEBASIC_ROOT/Documentation/changelog.txt"
fi

if [ -f "$ROOT/doc/libffi-license.txt" ]; then
    run cp "$ROOT/doc/libffi-license.txt" \
        "$FREEBASIC_ROOT/Documentation/libffi-license.txt"
fi

printf '%s\n' 'SYS:FreeBASIC' \
    > "$PACKAGE_ROOT/Prefs/Env-Archive/SYS/Packages/FreeBASIC"

printf '%s\n' \
    'Assign EXISTS "FreeBASIC:" >NIL:' \
    'If WARN' \
    '    Assign "FreeBASIC:" "SYS:FreeBASIC" >NIL:' \
	'EndIf' \
	'Path ADD FreeBASIC:bin' \
	'SetEnv GCC Developer:bin/gcc' \
	'SetEnv ELF2HUNK Developer:bin/elf2hunk' \
	> "$FREEBASIC_ROOT/S/FreeBASIC-Startup"

cat > "$FREEBASIC_ROOT/README" <<EOF
FreeBASIC $FBVERSION for AROS $TARGET

Install this package with:

    Unpack $PACKAGE_NAME.pkg TO SYS:

Reboot AROS or execute SYS:FreeBASIC/S/FreeBASIC-Startup before invoking fbc.
The compiler requires the matching AROS development supply chain. Install the
matching FreeBASIC-Developer-${FBVERSION}-r${PACKAGE_REVISION}-aros-${TARGET}
package first when it is supplied. GCC and binutils remain in that separate
SYS:Developer package rather than being duplicated under SYS:FreeBASIC.

The normal native fbc compile-and-link path is qualified inside x86_64, ARM,
and m68k AROS guests. The m68k qualification compiles generated C at -O0,
links with the AROS-hosted GCC supply chain, converts the result to Hunk, and
runs that Hunk inside AROS. Larger m68k-native work may still need to be split
into batches because the current AROS allocator exposes less memory than the
full Zorro III board configured in FS-UAE.

This package includes the normal and multithreaded FreeBASIC runtime, gfxlib2,
sfxlib, and the target libffi used by THREADCALL. AROS graphics use Intuition
and CyberGraphX. Sound uses ahi.device and falls back to the null driver when
AHI is unavailable.
EOF

# PKG1 records files only.  Removing empty source-tree directories makes the
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

DOWNLOAD_ROOT="$STAGE_ROOT/download"
run mkdir -p "$DOWNLOAD_ROOT"
run cp "$PACKAGE_PATH" "$DOWNLOAD_ROOT/"
run cp "$FREEBASIC_ROOT/README" "$DOWNLOAD_ROOT/README.txt"
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

# end of aros-package-freebasic.sh
