#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS smoke-build workflow
# ------------------------------------------------
#
# File: riscos-build-smoke.sh
#
# Purpose:
#
#     Build the RISC OS runtime and produce a small executable for emulator or
#     physical-machine testing.
#
# Responsibilities:
#
#     - validate the selected GCCSDK installation
#     - build the requested FreeBASIC target libraries
#     - link a static ARM executable and inspect its ELF metadata
#     - stage non-conflicting ELF and, when possible, AIF output for HostFS
#     - stage the SharedUnixLibrary module required by UnixLib programs
#     - stage DigitalRenderer when the native sfxlib backend is requested
#
# This file intentionally does NOT contain:
#
#     - GCCSDK construction
#     - RPCEmu construction or launch
#     - RISC OS guest automation
#     - guest-side graphics or sound correctness assertions
#
# Output ownership:
#
#     Files with the selected program name in --out-dir and --hostfs-dir, and
#     SharedULib in --system-modules-dir, are replaced on subsequent runs.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

TOOLCHAIN_BIN="${GCCSDK_INSTALL_CROSSBIN:-$ROOT/out/riscos/gccsdk/cross/bin}"
TARGET_ENV="${GCCSDK_TARGET_ENV:-$ROOT/out/riscos/gccsdk/cross/arm-unknown-riscos}"
SOURCE="$ROOT/examples/riscos/hello.bas"
OUT_DIR="$ROOT/out/riscos/programs"
HOSTFS_DIR="$ROOT/out/riscos/hostfs/FreeBASIC"
PROGRAM_NAME="fbhello"
SHARED_UNIX_LIBRARY=""
DRENDERER_MODULE=""
SYSTEM_MODULES_DIR=""
BUILD_OPTIONAL_LIBS=0

##############################################################################
# Helpers
##############################################################################

die() {
    echo "ERROR: $*" >&2
    exit 1
}

detect_jobs() {
    local jobs=1

    if command -v nproc >/dev/null 2>&1; then
        jobs="$(nproc)"
    elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
        jobs="$(getconf _NPROCESSORS_ONLN)"
    fi

    case "$jobs" in
        ''|*[!0-9]*|0) jobs=1 ;;
    esac

    printf '%s\n' "$jobs"
}

require_value() {
    local option="$1"
    local value="${2-}"

    [ -n "$value" ] || die "$option requires a value"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-build-smoke.sh [options]

Options:
  --toolchain-bin DIR  Directory containing arm-unknown-riscos-gcc.
  --target-env DIR     GCCSDK target environment directory.
  --source FILE        BASIC source. Default: examples/riscos/hello.bas
  --out-dir DIR        ELF/AIF output directory. Default: out/riscos/programs
  --hostfs-dir DIR     RPCEmu HostFS staging directory.
  --name NAME          Output leaf using letters, digits, dot, underscore, or
                       hyphen. Default: fbhello
  --shared-unix-library FILE
                       SharedUnixLibrary module to include in HostFS. The
                       GCCSDK cross-bin directory is searched by default.
  --system-modules-dir DIR
                       HostFS !System.310.Modules staging directory. Default:
                       !Boot/Resources/!System/310/Modules beside FreeBASIC
  --with-libs          Also build native gfxlib2/sfxlib and stage DRenderer.
  --drenderer-module FILE
                       DigitalRenderer module to include with --with-libs.
                       GCCSDK's cross module directory is searched by default.
  --jobs N             Parallel runtime build jobs.
  -h, --help           Show this help.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

JOBS="$(detect_jobs)"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --toolchain-bin)
            require_value "$1" "${2-}"
            TOOLCHAIN_BIN="$2"
            shift 2
            ;;
        --target-env)
            require_value "$1" "${2-}"
            TARGET_ENV="$2"
            shift 2
            ;;
        --source)
            require_value "$1" "${2-}"
            SOURCE="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUT_DIR="$2"
            shift 2
            ;;
        --hostfs-dir)
            require_value "$1" "${2-}"
            HOSTFS_DIR="$2"
            shift 2
            ;;
        --name)
            require_value "$1" "${2-}"
            PROGRAM_NAME="$2"
            shift 2
            ;;
        --shared-unix-library)
            require_value "$1" "${2-}"
            SHARED_UNIX_LIBRARY="$2"
            shift 2
            ;;
        --system-modules-dir)
            require_value "$1" "${2-}"
            SYSTEM_MODULES_DIR="$2"
            shift 2
            ;;
        --with-libs)
            BUILD_OPTIONAL_LIBS=1
            shift
            ;;
        --drenderer-module)
            require_value "$1" "${2-}"
            DRENDERER_MODULE="$2"
            shift 2
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
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

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

case "$PROGRAM_NAME" in
    ''|.|..|*/*|*,*|*[!A-Za-z0-9._-]*)
        die "--name contains characters unsafe for a staged RISC OS leaf"
        ;;
esac

##############################################################################
# Toolchain validation
##############################################################################

TARGET_GCC="$TOOLCHAIN_BIN/arm-unknown-riscos-gcc"
READELF="$TOOLCHAIN_BIN/arm-unknown-riscos-readelf"

[ -f "$SOURCE" ] || die "BASIC source not found: $SOURCE"
[ -d "$TARGET_ENV/include" ] ||
    die "GCCSDK target environment not found: $TARGET_ENV"
[ -x "$TARGET_GCC" ] || die "GCCSDK compiler not found: $TARGET_GCC"
[ -x "$READELF" ] || die "GCCSDK readelf not found: $READELF"

if [ -z "$SYSTEM_MODULES_DIR" ]; then
    HOSTFS_ROOT="$(dirname "$HOSTFS_DIR")"
    SYSTEM_MODULES_DIR="$HOSTFS_ROOT/!Boot/Resources/!System/310/Modules"
fi

[ "$SYSTEM_MODULES_DIR" != "/" ] ||
    die "refusing to use the filesystem root as --system-modules-dir"

if [ -n "$SHARED_UNIX_LIBRARY" ]; then
    [ -f "$SHARED_UNIX_LIBRARY" ] ||
        die "SharedUnixLibrary module not found: $SHARED_UNIX_LIBRARY"
else
    for candidate in \
        "$TOOLCHAIN_BIN/sul" \
        "$TOOLCHAIN_BIN/arm-unknown-riscos-sul" \
        "$TARGET_ENV/bin/sul"; do
        if [ -f "$candidate" ]; then
            SHARED_UNIX_LIBRARY="$candidate"
            break
        fi
    done
fi

if [ -z "$DRENDERER_MODULE" ]; then
    DRENDERER_MODULE="$TOOLCHAIN_BIN/../module/DRenderer,ffa"
fi

if [ "$BUILD_OPTIONAL_LIBS" -eq 1 ]; then
    [ -f "$DRENDERER_MODULE" ] ||
        die "DigitalRenderer module not found: $DRENDERER_MODULE"
    grep -a -q 'DigitalRenderer' "$DRENDERER_MODULE" ||
        die "$DRENDERER_MODULE is not a DigitalRenderer module"
fi

# The FreeBASIC makefiles use these environment variable names for compiler
# overrides.  Host values would silently bypass TARGET_TRIPLET selection.
unset GCC CLANG

if [ ! -x "$ROOT/bin/fbc" ]; then
    make -C "$ROOT" -j"$JOBS" compiler
fi

export GCCSDK_INSTALL_CROSSBIN="$TOOLCHAIN_BIN"
export GCCSDK_TARGET_ENV="$TARGET_ENV"
export PATH="$TOOLCHAIN_BIN:$PATH"

##############################################################################
# Runtime and program build
##############################################################################

runtime_targets=(rtlib fbrt)
if [ "$BUILD_OPTIONAL_LIBS" -eq 1 ]; then
    runtime_targets+=(gfxlib2 sfxlib)
fi

make -C "$ROOT" -j"$JOBS" \
    TARGET_TRIPLET=arm-unknown-riscos \
    FBC="$ROOT/bin/fbc" \
    "${runtime_targets[@]}"

mkdir -p "$OUT_DIR" "$HOSTFS_DIR"
ELF_FILE="$OUT_DIR/$PROGRAM_NAME"

"$ROOT/bin/fbc" \
    -target arm-unknown-riscos \
    -static \
    -i "$ROOT/inc" \
    "$SOURCE" \
    -x "$ELF_FILE"

[ -s "$ELF_FILE" ] || die "FreeBASIC did not produce $ELF_FILE"

##############################################################################
# ELF validation and RISC OS staging
##############################################################################

ELF_HEADER="$("$READELF" -h "$ELF_FILE")"
grep -q 'Class:.*ELF32' <<<"$ELF_HEADER" ||
    die "$ELF_FILE is not an ELF32 file"
grep -q 'Data:.*little endian' <<<"$ELF_HEADER" ||
    die "$ELF_FILE is not little-endian"
grep -q 'Machine:.*ARM' <<<"$ELF_HEADER" ||
    die "$ELF_FILE is not an ARM ELF file"
grep -q 'Flags:.*software FP' <<<"$ELF_HEADER" ||
    die "$ELF_FILE does not use the GCCSDK software floating-point ABI"

ELF2AIF=""
for candidate in \
    "$TOOLCHAIN_BIN/elf2aif" \
    "$TOOLCHAIN_BIN/arm-unknown-riscos-elf2aif"; do
    if [ -x "$candidate" ]; then
        ELF2AIF="$candidate"
        break
    fi
done

if [ -n "$ELF2AIF" ]; then
    AIF_FILE="$OUT_DIR/$PROGRAM_NAME-aif"
    cp "$ELF_FILE" "$AIF_FILE"
    "$ELF2AIF" "$AIF_FILE"
    [ -s "$AIF_FILE" ] || die "elf2aif did not produce $AIF_FILE"

    # HostFS removes the comma suffix when presenting the leaf to RISC OS.
    # Give the diagnostic ELF a different leaf so it cannot hide the AIF.
    cp "$ELF_FILE" "$HOSTFS_DIR/$PROGRAM_NAME-elf,e1f"

    # &FF8 identifies an absolute, self-contained RISC OS application image.
    cp "$AIF_FILE" "$HOSTFS_DIR/$PROGRAM_NAME,ff8"
    rm -f "$HOSTFS_DIR/$PROGRAM_NAME,e1f"
    echo "==> staged native AIF: $HOSTFS_DIR/$PROGRAM_NAME,ff8"
else
    # &E1F is the RISC OS filetype used for ELF images.  HostFS represents the
    # filetype as a comma suffix on the host pathname.
    cp "$ELF_FILE" "$HOSTFS_DIR/$PROGRAM_NAME,e1f"
    rm -f \
        "$HOSTFS_DIR/$PROGRAM_NAME-elf,e1f" \
        "$HOSTFS_DIR/$PROGRAM_NAME,ff8"
    echo "==> elf2aif not found; staged ELF: $HOSTFS_DIR/$PROGRAM_NAME,e1f"
    echo "    Install ELFLoader in RISC OS before running the ELF file."
fi

if [ -n "$SHARED_UNIX_LIBRARY" ]; then
    grep -a -q 'SharedUnixLibrary' "$SHARED_UNIX_LIBRARY" ||
        die "$SHARED_UNIX_LIBRARY is not a SharedUnixLibrary module"
    mkdir -p "$SYSTEM_MODULES_DIR"
    cp "$SHARED_UNIX_LIBRARY" "$SYSTEM_MODULES_DIR/SharedULib,ffa"
    echo "==> staged SharedUnixLibrary: $SYSTEM_MODULES_DIR/SharedULib,ffa"
else
    echo "==> SharedUnixLibrary not found; the staged program cannot run yet"
    echo "    Pass --shared-unix-library with the GCCSDK sul module."
fi

if [ "$BUILD_OPTIONAL_LIBS" -eq 1 ]; then
    mkdir -p "$SYSTEM_MODULES_DIR"
    cp "$DRENDERER_MODULE" "$SYSTEM_MODULES_DIR/DRenderer,ffa"
    echo "==> staged DigitalRenderer: $SYSTEM_MODULES_DIR/DRenderer,ffa"
fi

echo "==> RISC OS smoke build complete"
echo "    ELF:    $ELF_FILE"
echo "    HostFS: $HOSTFS_DIR"

# end of riscos-build-smoke.sh
