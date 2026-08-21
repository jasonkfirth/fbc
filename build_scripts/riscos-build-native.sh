#!/usr/bin/env bash
#
# Project: FreeBASIC native RISC OS compiler workflow
# -------------------------------------------------
#
# File: riscos-build-native.sh
#
# Purpose:
#
#     Build and stage a native FreeBASIC compiler together with the GCCSDK
#     programs it invokes inside RISC OS.
#
# Responsibilities:
#
#     - validate the cross and native GCCSDK trees
#     - build the native FreeBASIC compiler with the stable GCCSDK settings
#     - build GCCSDK's open native ELF-to-AIF converter for UnixLib links
#     - bundle the target ARM libffi needed by FreeBASIC THREADCALL
#     - build and stage the target runtime and FreeBASIC include tree
#     - convert static ARM ELF programs to native RISC OS AIF files
#     - arrange GCC and FreeBASIC suffix directories for UnixLib
#     - create replaceable HostFS staging trees for !GCC and FreeBASIC
#     - stage the open runtime modules required by UnixLib and sfxlib
#
# This file intentionally does NOT contain:
#
#     - GCCSDK or RPCEmu construction
#     - guest desktop automation
#     - dynamic-library packaging
#     - native graphics, sound, or serial implementations
#     - RiscPkg archive construction
#
# Output ownership:
#
#     The selected output files and the !GCC and FreeBASIC directories below
#     --hostfs-root are replaced on subsequent runs.  Other HostFS content is
#     preserved.
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
NATIVE_ROOT="${GCCSDK_NATIVE_ROOT:-$ROOT/out/riscos/gccsdk/gcc4/release-area/full}"
HOSTFS_ROOT="$ROOT/out/riscos/hostfs"
OUT_DIR="$ROOT/out/riscos/programs"
BUILD_OPTIONAL_LIBS=0
DRENDERER_MODULE=""

TEMP_ROOT=""
ELF2AIF_TEMP=""

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

cleanup() {
    if [ -n "$ELF2AIF_TEMP" ] &&
       [[ "$ELF2AIF_TEMP" == "$OUT_DIR"/.elf2aif-source.* ]] &&
       [ -d "$ELF2AIF_TEMP" ]; then
        rm -rf -- "$ELF2AIF_TEMP"
    fi

    if [ -n "$TEMP_ROOT" ] &&
       [[ "$TEMP_ROOT" == "$HOSTFS_ROOT"/.native-stage.* ]] &&
       [ -d "$TEMP_ROOT" ]; then
        rm -rf -- "$TEMP_ROOT"
    fi
}

replace_directory() {
    local source="$1"
    local destination="$2"

    [ -d "$source" ] || die "staging source is not a directory: $source"
    [ "$destination" != "/" ] || die "refusing to replace the filesystem root"

    rm -rf -- "$destination"
    mv "$source" "$destination"
}

suffix_swap_tree() {
    local tree="$1"
    shift

    local directory
    local file
    local leaf
    local stem
    local suffix

    for suffix in "$@"; do
        while IFS= read -r -d '' file; do
            directory="$(dirname "$file")"
            leaf="$(basename "$file")"
            stem="${leaf%.$suffix}"
            mkdir -p "$directory/$suffix"
            mv "$file" "$directory/$suffix/$stem"
        done < <(find "$tree" -type f -name "*.$suffix" -print0)
    done
}

convert_static_programs() {
    local tree="$1"
    local file
    local description

    while IFS= read -r -d '' file; do
        description="$(file -b "$file")"
        if [[ "$description" == *"ELF 32-bit"* ]] &&
           [[ "$description" == *"ARM"* ]] &&
           [[ "$description" == *"statically linked"* ]]; then
            cp "$file" "$file,ff8"
            "$ELF2AIF" "$file,ff8"
            rm -f -- "$file"
        fi
    done < <(find "$tree" -type f -print0)
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-build-native.sh [options]

Options:
  --toolchain-bin DIR  Directory containing arm-unknown-riscos-gcc.
  --target-env DIR     GCCSDK target environment directory.
  --native-root DIR    GCCSDK native release root containing !GCC.
  --hostfs-root DIR    HostFS staging root. Default: out/riscos/hostfs
  --out-dir DIR        Native compiler ELF/AIF output directory.
  --with-libs          Also build native gfxlib2/sfxlib and stage DRenderer.
  --drenderer-module FILE
                       DigitalRenderer module to stage with --with-libs. The
                       GCCSDK cross module directory is searched by default.
  --jobs N             Parallel build jobs.
  -h, --help           Show this help.

Build the prerequisite native GCCSDK tree first with:
  ./build_scripts/riscos-gccsdk.sh --with-native
EOF
}

trap cleanup EXIT

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
        --native-root)
            require_value "$1" "${2-}"
            NATIVE_ROOT="$2"
            shift 2
            ;;
        --hostfs-root)
            require_value "$1" "${2-}"
            HOSTFS_ROOT="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUT_DIR="$2"
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

##############################################################################
# Toolchain and path validation
##############################################################################

for tool in file find make patch; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

TARGET_GCC="$TOOLCHAIN_BIN/arm-unknown-riscos-gcc"
READELF="$TOOLCHAIN_BIN/arm-unknown-riscos-readelf"
ELF2AIF="$TOOLCHAIN_BIN/elf2aif"
LIBFFI_LIBRARY="$TARGET_ENV/lib/libffi.a"
LIBFFI_LICENSE="$ROOT/out/riscos/gccsdk/gcc4/srcdir.orig/gcc-trunk/libffi/LICENSE"
NATIVE_GCC_TREE="$NATIVE_ROOT/!GCC"
ELF2AIF_SOURCE_ROOT="$NATIVE_ROOT/../../riscos/elf2aif"
SHARED_UNIX_LIBRARY="$NATIVE_GCC_TREE/bin/sul"

if [ -z "$DRENDERER_MODULE" ]; then
    DRENDERER_MODULE="$TOOLCHAIN_BIN/../module/DRenderer,ffa"
fi

[ -x "$TARGET_GCC" ] || die "GCCSDK compiler not found: $TARGET_GCC"
[ -x "$READELF" ] || die "GCCSDK readelf not found: $READELF"
[ -x "$ELF2AIF" ] || die "GCCSDK elf2aif not found: $ELF2AIF"
[ -s "$LIBFFI_LIBRARY" ] ||
    die "RISC OS libffi not found; rerun build_scripts/riscos-gccsdk.sh"
[ -s "$LIBFFI_LICENSE" ] ||
    die "RISC OS libffi licence not found: $LIBFFI_LICENSE"
[ -d "$TARGET_ENV/include" ] ||
    die "GCCSDK target environment not found: $TARGET_ENV"
[ -s "$NATIVE_GCC_TREE/bin/gcc" ] ||
    die "native GCC not found: $NATIVE_GCC_TREE/bin/gcc"
[ -s "$NATIVE_GCC_TREE/bin/as" ] ||
    die "native assembler not found: $NATIVE_GCC_TREE/bin/as"
[ -s "$NATIVE_GCC_TREE/bin/ld" ] ||
    die "native linker not found: $NATIVE_GCC_TREE/bin/ld"
[ -s "$SHARED_UNIX_LIBRARY" ] ||
    die "SharedUnixLibrary module not found: $SHARED_UNIX_LIBRARY"
[ "$BUILD_OPTIONAL_LIBS" -eq 0 ] || [ -s "$DRENDERER_MODULE" ] ||
    die "DigitalRenderer module not found: $DRENDERER_MODULE"
[ -s "$ELF2AIF_SOURCE_ROOT/src/elf2aif.c" ] ||
    die "GCCSDK elf2aif source not found: $ELF2AIF_SOURCE_ROOT"
[ "$HOSTFS_ROOT" != "/" ] || die "refusing to use the filesystem root as HostFS"
[ "$OUT_DIR" != "/" ] || die "refusing to use the filesystem root as --out-dir"

unset GCC CLANG
export GCCSDK_INSTALL_CROSSBIN="$TOOLCHAIN_BIN"
export GCCSDK_TARGET_ENV="$TARGET_ENV"
export PATH="$TOOLCHAIN_BIN:$PATH"

if [ ! -x "$ROOT/bin/fbc" ]; then
    make -C "$ROOT" -j"$JOBS" compiler
fi

##############################################################################
# Native FreeBASIC compiler and runtime
##############################################################################

mkdir -p "$OUT_DIR"
NATIVE_FBC_ELF="$OUT_DIR/fbc-native-elf"
NATIVE_FBC_AIF="$OUT_DIR/fbc-native-aif"
NATIVE_ELF2AIF_ELF="$OUT_DIR/elf2aif-native-elf"
NATIVE_ELF2AIF_AIF="$OUT_DIR/elf2aif-native-aif"
RISCOS_NATIVE_CFLAGS='-O0'

# GCCSDK GCC 4.7 fails while optimizing the generated ir-llvm translation.
# Keep the compiler itself at O0 until that legacy optimizer defect is isolated.
#
# RISC OS's UnixLib strtod() is not reliable in the native GCCSDK
# environment.  The RISC OS runtime provides its own bounded decimal parser,
# allowing the compiler and generated programs to retain GCCSDK's normal
# software-float ABI without depending on an FPA emulator.
make -C "$ROOT" -s clean-libs \
    TARGET_TRIPLET=arm-unknown-riscos \
    FBC="$ROOT/bin/fbc" \
    CFLAGS="$RISCOS_NATIVE_CFLAGS"
make -C "$ROOT" -s -j"$JOBS" rtlib \
    TARGET_TRIPLET=arm-unknown-riscos \
    FBC="$ROOT/bin/fbc" \
    CFLAGS="$RISCOS_NATIVE_CFLAGS"

make -C "$ROOT" -s clean-compiler \
    TARGET_TRIPLET=arm-unknown-riscos \
    BUILD_FBC="$ROOT/bin/fbc" \
    FBC_EXE="$NATIVE_FBC_ELF" \
    CFLAGS="$RISCOS_NATIVE_CFLAGS"
make -C "$ROOT" -s -j"$JOBS" compiler \
    TARGET_TRIPLET=arm-unknown-riscos \
    BUILD_FBC="$ROOT/bin/fbc" \
    FBC_EXE="$NATIVE_FBC_ELF" \
    CFLAGS="$RISCOS_NATIVE_CFLAGS"

[ -s "$NATIVE_FBC_ELF" ] ||
    die "native FreeBASIC compiler was not produced: $NATIVE_FBC_ELF"

ELF_HEADER="$("$READELF" -h "$NATIVE_FBC_ELF")"
grep -q 'Class:.*ELF32' <<<"$ELF_HEADER" ||
    die "$NATIVE_FBC_ELF is not an ELF32 file"
grep -q 'Machine:.*ARM' <<<"$ELF_HEADER" ||
    die "$NATIVE_FBC_ELF is not an ARM file"

cp "$NATIVE_FBC_ELF" "$NATIVE_FBC_AIF"
"$ELF2AIF" "$NATIVE_FBC_AIF"
[ -s "$NATIVE_FBC_AIF" ] || die "elf2aif did not produce $NATIVE_FBC_AIF"

# GCCSDK normally builds RISC OS helper applications for the
# SharedCLibrary.  Build only the converter needed by this UnixLib port, and
# keep it static so the staged compiler has no proprietary runtime dependency.
ELF2AIF_TEMP="$(mktemp -d "$OUT_DIR/.elf2aif-source.XXXXXX")"
cp -a "$ELF2AIF_SOURCE_ROOT/src/." "$ELF2AIF_TEMP/"
patch -s -d "$ELF2AIF_TEMP" -p1 < \
    "$ROOT/build_scripts/patches/gccsdk-elf2aif-unix-path.patch"

"$TARGET_GCC" \
    -std=gnu99 \
    -O2 \
    -static \
    '-DPACKAGE_STRING="elf2aif 0.06"' \
    -I"$ELF2AIF_TEMP" \
    -I"$ELF2AIF_TEMP/elf" \
    "$ELF2AIF_TEMP/elf2aif.c" \
    -o "$NATIVE_ELF2AIF_ELF"

cp "$NATIVE_ELF2AIF_ELF" "$NATIVE_ELF2AIF_AIF"
"$ELF2AIF" "$NATIVE_ELF2AIF_AIF"
[ -s "$NATIVE_ELF2AIF_AIF" ] ||
    die "elf2aif did not produce $NATIVE_ELF2AIF_AIF"
rm -rf -- "$ELF2AIF_TEMP"
ELF2AIF_TEMP=""

runtime_targets=(fbrt)
if [ "$BUILD_OPTIONAL_LIBS" -eq 1 ]; then
    runtime_targets+=(gfxlib2 sfxlib)
fi

make -C "$ROOT" -j"$JOBS" \
    TARGET_TRIPLET=arm-unknown-riscos \
    FBC="$ROOT/bin/fbc" \
    CFLAGS="$RISCOS_NATIVE_CFLAGS" \
    "${runtime_targets[@]}"

RUNTIME_DIR="$ROOT/lib/freebasic/riscos-arm"
[ -s "$RUNTIME_DIR/libfb.a" ] ||
    die "RISC OS runtime library not found: $RUNTIME_DIR/libfb.a"

# Keep libffi beside the FreeBASIC libraries. Native and cross fbc both add
# this directory to the library search path, making THREADCALL independent of
# whether a separately installed GCC4 package happens to include libffi.
cp "$LIBFFI_LIBRARY" "$RUNTIME_DIR/libffi.a"

##############################################################################
# HostFS staging
##############################################################################

mkdir -p "$HOSTFS_ROOT"
HOSTFS_ROOT="$(cd "$HOSTFS_ROOT" && pwd)"
TEMP_ROOT="$(mktemp -d "$HOSTFS_ROOT/.native-stage.XXXXXX")"
GCC_STAGE="$TEMP_ROOT/!GCC"
FREEBASIC_STAGE="$TEMP_ROOT/FreeBASIC"

mkdir -p "$GCC_STAGE" "$FREEBASIC_STAGE/bin"
cp -a "$NATIVE_GCC_TREE/." "$GCC_STAGE/"

GCC_SKELETON="$NATIVE_ROOT/../../riscos/dist/!GCC"
if [ ! -d "$GCC_SKELETON" ]; then
    GCC_SKELETON="$ROOT/out/riscos/gccsdk/gcc4/riscos/dist/!GCC"
fi
[ -d "$GCC_SKELETON" ] || die "GCCSDK !GCC skeleton not found"
cp -a "$GCC_SKELETON/." "$GCC_STAGE/"

convert_static_programs "$GCC_STAGE"
suffix_swap_tree "$GCC_STAGE" c cc tcc cmhg s h o adb ads ali
cp "$NATIVE_ELF2AIF_AIF" "$GCC_STAGE/bin/elf2aif,ff8"

cp "$NATIVE_FBC_AIF" "$FREEBASIC_STAGE/fbc,ff8"
cp "$NATIVE_FBC_AIF" "$FREEBASIC_STAGE/bin/fbc,ff8"
cp "$NATIVE_ELF2AIF_AIF" "$FREEBASIC_STAGE/bin/elf2aif,ff8"
cp "$NATIVE_FBC_ELF" "$FREEBASIC_STAGE/fbc-elf,e1f"
mkdir -p \
    "$FREEBASIC_STAGE/doc" \
    "$FREEBASIC_STAGE/include/freebasic" \
    "$FREEBASIC_STAGE/lib/freebasic/riscos-arm" \
    "$FREEBASIC_STAGE/examples"
cp -a "$ROOT/inc/." "$FREEBASIC_STAGE/include/freebasic/"
# The native compiler searches the complete RISC OS header overlay first.
# Keep the overlay directory in the installed include tree beside the shared
# headers it falls back to.
cp -a "$RUNTIME_DIR/." "$FREEBASIC_STAGE/lib/freebasic/riscos-arm/"
cp "$LIBFFI_LICENSE" "$FREEBASIC_STAGE/doc/libffi-license,fff"
cp "$ROOT/examples/riscos/hello.bas" "$FREEBASIC_STAGE/examples/hello.bas"
cp "$ROOT/examples/riscos/media-smoke.bas" \
    "$FREEBASIC_STAGE/examples/media-smoke.bas"
# media-smoke verifies UnixLib's case-insensitive directory handling.  Keep
# its tiny input tree with the installed examples so the packaged example can
# run without a HostFS-only test fixture.
mkdir -p "$FREEBASIC_STAGE/examples/dir-smoke/bmp/bmp"
cp "$ROOT/examples/riscos/hello.bas" \
    "$FREEBASIC_STAGE/examples/dir-smoke/bmp/bmp/UPPER,fff"
cp "$ROOT/examples/riscos/SetPaths" "$FREEBASIC_STAGE/SetPaths,feb"
cp "$ROOT/examples/riscos/Compile" "$FREEBASIC_STAGE/Compile,feb"
suffix_swap_tree "$FREEBASIC_STAGE/include/freebasic" bi h
suffix_swap_tree "$FREEBASIC_STAGE/lib/freebasic/riscos-arm" o a x
suffix_swap_tree "$FREEBASIC_STAGE/examples" bas

replace_directory "$GCC_STAGE" "$HOSTFS_ROOT/!GCC"
replace_directory "$FREEBASIC_STAGE" "$HOSTFS_ROOT/FreeBASIC"
rmdir "$TEMP_ROOT"
TEMP_ROOT=""

##############################################################################
# System module staging
##############################################################################

#
# UnixLib opens /dev/dsp through DigitalRenderer and loads the module from
# System:Modules.DRenderer on first use.  These modules are GCCSDK products,
# not FreeBASIC application files, so keep them in the normal !System module
# directory instead of hiding global platform dependencies inside the app.
#

SYSTEM_MODULES_DIR="$HOSTFS_ROOT/!Boot/Resources/!System/310/Modules"
mkdir -p "$SYSTEM_MODULES_DIR"
cp "$SHARED_UNIX_LIBRARY" "$SYSTEM_MODULES_DIR/SharedULib,ffa"

if [ "$BUILD_OPTIONAL_LIBS" -eq 1 ]; then
    cp "$DRENDERER_MODULE" "$SYSTEM_MODULES_DIR/DRenderer,ffa"
fi

echo "==> native RISC OS compiler staged"
echo "    FreeBASIC: $HOSTFS_ROOT/FreeBASIC/fbc,ff8"
echo "    GCC:       $HOSTFS_ROOT/!GCC"
echo "    UnixLib:   $SYSTEM_MODULES_DIR/SharedULib,ffa"
if [ "$BUILD_OPTIONAL_LIBS" -eq 1 ]; then
    echo "    Audio:     $SYSTEM_MODULES_DIR/DRenderer,ffa"
fi
echo "    In RISC OS, run !GCC and then FreeBASIC.SetPaths before using fbc."

# end of riscos-build-native.sh
