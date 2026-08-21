#!/usr/bin/env bash
#
# Project: FreeBASIC MIPS Linux release workflow
# ----------------------------------------------
#
# File: mips-package-freebasic.sh
#
# Purpose:
#
#     Create an individually downloadable native FreeBASIC compiler package
#     for one MIPS Linux ABI.
#
# Responsibilities:
#
#     - validate the native compiler's ELF class and byte order
#     - stage the matching includes, rtlib, gfxlib2, and sfxlib
#     - bundle target libffi and libtinfo archives used by compiled programs
#     - create and inspect tar.xz and ZIP release archives
#     - compile and run a smoke program with the package under matching QEMU
#
# This file intentionally does NOT contain:
#
#     - compiler, runtime, or third-party library construction
#     - Docker image construction
#     - fbctests orchestration
#
# Package layout:
#
#     The compiler derives its installation prefix from bin/fbc. Keeping the
#     include and library trees beside that directory makes the archive
#     relocatable on a native MIPS Linux system.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

TARGET=""
NATIVE_ROOT="${MIPS_NATIVE_ROOT:-$ROOT/out/mips/native}"
PACKAGE_OUTDIR="${MIPS_PACKAGE_OUTDIR:-$ROOT/out/mips/packages}"
PACKAGE_REVISION="${MIPS_PACKAGE_REVISION:-1}"
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

require_command() {
    command -v "$1" >/dev/null 2>&1 ||
        die "required tool not found: $1"
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
Usage: ./build_scripts/mips-package-freebasic.sh --target TRIPLET [options]

Required:
  --target TRIPLET  One of the four supported MIPS GNU target triplets.

Options:
  --native-root DIR Native compiler root. Default: out/mips/native
  --out-dir DIR     Package output directory. Default: out/mips/packages
  --revision N      Package revision. Default: 1
  --keep-stage      Preserve temporary staging directories.
  -h, --help        Show this help.

Outputs:
  FreeBASIC-VERSION-rREV-linux-ARCH.tar.xz
  FreeBASIC-VERSION-rREV-linux-ARCH.zip
EOF
}

map_target() {
    case "$TARGET" in
        mips-linux-gnu)
            PACKAGE_ARCH=mips32
            RUNTIME_NAME=linux-mips32
            EXPECTED_CLASS=ELF32
            EXPECTED_ENDIAN="2's complement, big endian"
            QEMU=qemu-mips
            ;;
        mipsel-linux-gnu)
            PACKAGE_ARCH=mips32el
            RUNTIME_NAME=linux-mips32el
            EXPECTED_CLASS=ELF32
            EXPECTED_ENDIAN="2's complement, little endian"
            QEMU=qemu-mipsel
            ;;
        mips64-linux-gnuabi64)
            PACKAGE_ARCH=mips64
            RUNTIME_NAME=linux-mips64
            EXPECTED_CLASS=ELF64
            EXPECTED_ENDIAN="2's complement, big endian"
            QEMU=qemu-mips64
            ;;
        mips64el-linux-gnuabi64)
            PACKAGE_ARCH=mips64el
            RUNTIME_NAME=linux-mips64el
            EXPECTED_CLASS=ELF64
            EXPECTED_ENDIAN="2's complement, little endian"
            QEMU=qemu-mips64el
            ;;
        '')
            die "--target is required"
            ;;
        *)
            die "unsupported MIPS target: $TARGET"
            ;;
    esac

    SYSROOT="/usr/$TARGET"
}

validate_compiler_elf() {
    local compiler="$1"
    local header

    header="$(readelf -h "$compiler")"
    grep -q "Class:.*$EXPECTED_CLASS" <<< "$header" ||
        die "compiler has the wrong ELF class: $compiler"
    grep -Fq "Data:                              $EXPECTED_ENDIAN" <<< "$header" ||
        die "compiler has the wrong byte order: $compiler"
    grep -q 'Machine:.*MIPS' <<< "$header" ||
        die "compiler is not a MIPS executable: $compiler"
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
        --native-root)
            require_value "$1" "${2-}"
            NATIVE_ROOT="$2"
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

case "$PACKAGE_REVISION" in
    ''|*[!0-9]*) die "--revision must contain only digits" ;;
esac

map_target

##############################################################################
# Input validation
##############################################################################

for tool in readelf tar unzip zip "$QEMU"; do
    require_command "$tool"
done

VERSION_FILE="$ROOT/mk/version.mk"
COMPILER="$NATIVE_ROOT/$TARGET/fbc"
RUNTIME_DIR="$ROOT/lib/freebasic/$RUNTIME_NAME"

[ -f "$VERSION_FILE" ] || die "version file not found: $VERSION_FILE"
[ -x "$COMPILER" ] || die "native compiler not found: $COMPILER"
[ -d "$RUNTIME_DIR" ] || die "runtime directory not found: $RUNTIME_DIR"
[ -d "$ROOT/inc" ] || die "FreeBASIC include tree not found: $ROOT/inc"
[ -d "$SYSROOT" ] || die "target sysroot not found: $SYSROOT"
[ -f "$SYSROOT/lib/libffi.a" ] || die "target libffi archive not found"
[ -f "$SYSROOT/lib/libtinfo.a" ] || die "target libtinfo archive not found"

FBVERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$VERSION_FILE")"
[ -n "$FBVERSION" ] || die "could not read FBVERSION from $VERSION_FILE"

REQUIRED_RUNTIME_FILES=(
    fbrt0.o
    fbrt1.o
    fbrt2.o
    libfb.a
    libfbmt.a
    libfbgfx.a
    libfbgfxmt.a
    libsfx.a
    libsfxmt.a
)

for file in "${REQUIRED_RUNTIME_FILES[@]}"; do
    [ -f "$RUNTIME_DIR/$file" ] ||
        die "incomplete MIPS runtime: missing $RUNTIME_DIR/$file"
done

validate_compiler_elf "$COMPILER"

##############################################################################
# Package staging
##############################################################################

mkdir -p "$PACKAGE_OUTDIR"
STAGE_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-stage.XXXXXX")"
EXTRACT_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-extract.XXXXXX")"

PACKAGE_NAME="FreeBASIC-${FBVERSION}-r${PACKAGE_REVISION}-linux-${PACKAGE_ARCH}"
PACKAGE_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.tar.xz"
ZIP_PATH="$PACKAGE_OUTDIR/$PACKAGE_NAME.zip"
PACKAGE_ROOT="$STAGE_ROOT/$PACKAGE_NAME"
PACKAGE_RUNTIME="$PACKAGE_ROOT/lib/freebasic/$RUNTIME_NAME"

msg "staging $PACKAGE_NAME"
run mkdir -p \
    "$PACKAGE_ROOT/bin" \
    "$PACKAGE_ROOT/include/freebasic" \
    "$PACKAGE_RUNTIME" \
    "$PACKAGE_ROOT/doc"

run cp "$COMPILER" "$PACKAGE_ROOT/bin/fbc"
run chmod 755 "$PACKAGE_ROOT/bin/fbc"
run cp -a "$ROOT/inc/." "$PACKAGE_ROOT/include/freebasic/"
run cp -a "$RUNTIME_DIR/." "$PACKAGE_RUNTIME/"
run cp "$SYSROOT/lib/libffi.a" "$PACKAGE_RUNTIME/libffi.a"
run cp "$SYSROOT/lib/libtinfo.a" "$PACKAGE_RUNTIME/libtinfo.a"
run cp "$ROOT/readme.txt" "$PACKAGE_ROOT/doc/FreeBASIC-readme.txt"

if [ -f "$ROOT/debian/copyright" ]; then
    run cp "$ROOT/debian/copyright" "$PACKAGE_ROOT/doc/copyright"
fi

if [ -f "$ROOT/doc/libffi-license.txt" ]; then
    run cp "$ROOT/doc/libffi-license.txt" "$PACKAGE_ROOT/doc/libffi-license.txt"
fi

if [ -f /usr/share/doc/libncurses-dev/copyright ]; then
    run cp /usr/share/doc/libncurses-dev/copyright \
        "$PACKAGE_ROOT/doc/ncurses-copyright"
fi

cat > "$PACKAGE_ROOT/README.txt" <<EOF
FreeBASIC $FBVERSION for MIPS Linux $PACKAGE_ARCH

Extract this archive and either invoke bin/fbc directly or add bin to PATH.
The compiler finds this package's include and library directories relative to
its own executable.

The package contains the normal and multithreaded runtime, gfxlib2, sfxlib,
libffi, and libtinfo. It intentionally does not bundle GCC or binutils. Install
a native GNU C toolchain for this machine before compiling FreeBASIC programs.

Qualified GNU target triplet: $TARGET
EOF

##############################################################################
# Archive creation and validation
##############################################################################

msg "creating release archives"
run rm -f -- "$PACKAGE_PATH" "$ZIP_PATH"
run tar --sort=name --owner=0 --group=0 --numeric-owner \
    -C "$STAGE_ROOT" -cJf "$PACKAGE_PATH" "$PACKAGE_NAME"
(
    cd "$STAGE_ROOT"
    zip -X -q -r "$ZIP_PATH" "$PACKAGE_NAME"
)

tar -tf "$PACKAGE_PATH" > "$EXTRACT_ROOT/tar-contents.txt"
grep -Fqx "$PACKAGE_NAME/bin/fbc" "$EXTRACT_ROOT/tar-contents.txt" ||
    die "tar archive does not contain bin/fbc"
grep -Fqx "$PACKAGE_NAME/lib/freebasic/$RUNTIME_NAME/libfbgfx.a" \
    "$EXTRACT_ROOT/tar-contents.txt" ||
    die "tar archive does not contain gfxlib2"
grep -Fqx "$PACKAGE_NAME/lib/freebasic/$RUNTIME_NAME/libsfx.a" \
    "$EXTRACT_ROOT/tar-contents.txt" ||
    die "tar archive does not contain sfxlib"
unzip -tq "$ZIP_PATH" >/dev/null || die "ZIP archive validation failed"

run tar -xJf "$PACKAGE_PATH" -C "$EXTRACT_ROOT"
EXTRACTED_COMPILER="$EXTRACT_ROOT/$PACKAGE_NAME/bin/fbc"
validate_compiler_elf "$EXTRACTED_COMPILER"

# Version startup proves that the archived compiler is loadable with the
# selected ABI and that no host executable was accidentally packaged.
VERSION_OUTPUT="$("$QEMU" -L "$SYSROOT" "$EXTRACTED_COMPILER" -version)"
grep -q "FreeBASIC Compiler - Version $FBVERSION" <<< "$VERSION_OUTPUT" ||
    die "packaged compiler did not report the expected version"

# Compile from the extracted installation tree so archive validation covers
# the relocatable include and library layout, not only compiler startup. QEMU
# executes the packaged compiler while -buildprefix selects the matching GNU
# tools available in the release container.
PACKAGE_SMOKE="$EXTRACT_ROOT/package-smoke"
run "$QEMU" -L "$SYSROOT" "$EXTRACTED_COMPILER" \
    -buildprefix "$TARGET-" \
    "$ROOT/tests/mips/runtime-smoke.bas" \
    -x "$PACKAGE_SMOKE"
validate_compiler_elf "$PACKAGE_SMOKE"

SMOKE_OUTPUT="$("$QEMU" -L "$SYSROOT" "$PACKAGE_SMOKE")"
grep -q '^mips runtime ok$' <<< "$SMOKE_OUTPUT" ||
    die "packaged compiler smoke failed for $TARGET"

msg "created $PACKAGE_PATH"
echo "==> created $ZIP_PATH"

# end of build_scripts/mips-package-freebasic.sh
