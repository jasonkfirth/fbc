#!/usr/bin/env bash
#
# Project: FreeBASIC MIPS Linux release workflow
# ----------------------------------------------
#
# File: build_scripts/mips/container-build.sh
#
# Purpose:
#
#     Perform the FreeBASIC MIPS compiler, library, test, and package build
#     inside the pinned toolchain container.
#
# Responsibilities:
#
#     - bootstrap a current host compiler from repository sources
#     - build rtlib, gfxlib2, and sfxlib for every selected MIPS ABI
#     - build and exercise a native compiler for every selected ABI
#     - invoke bounded QEMU fbctests
#     - invoke package creation after qualification succeeds
#
# This file intentionally does NOT contain:
#
#     - Docker image construction or host source copying
#     - host package installation
#     - QEMU system-machine configuration
#
# Environment contract:
#
#     /work is an isolated FreeBASIC source tree. /mips-output is a persistent
#     host directory for compilers, logs, test reports, and packages.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

ROOT=/work
OUTPUT_ROOT=/mips-output
TARGETS="${MIPS_TARGETS:-mips-linux-gnu,mipsel-linux-gnu,mips64-linux-gnuabi64,mips64el-linux-gnuabi64}"
JOBS="${JOBS:-2}"
RUN_TESTS=1
RUN_PACKAGES=1
RESUME_TESTS=0
INCREMENTAL=0

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

map_target() {
    local target="$1"

    case "$target" in
        mips-linux-gnu)
            MAP_QEMU=qemu-mips
            MAP_RUNTIME=linux-mips32
            ;;
        mipsel-linux-gnu)
            MAP_QEMU=qemu-mipsel
            MAP_RUNTIME=linux-mips32el
            ;;
        mips64-linux-gnuabi64)
            MAP_QEMU=qemu-mips64
            MAP_RUNTIME=linux-mips64
            ;;
        mips64el-linux-gnuabi64)
            MAP_QEMU=qemu-mips64el
            MAP_RUNTIME=linux-mips64el
            ;;
        *)
            die "unsupported MIPS target: $target"
            ;;
    esac

    MAP_SYSROOT="/usr/$target"
}

usage() {
    cat <<EOF
Usage: build_scripts/mips/container-build.sh [options]

Options:
  --targets LIST  Comma-separated GNU target triplets. Default: all four
  --jobs N        Parallel build jobs. Default: 2
  --skip-tests    Build and smoke-test, but do not run full fbctests.
  --skip-package  Do not create release archives.
  --resume        Reuse saved successful per-directory fbctests logs.
  --incremental   Preserve compatible compiler and library objects.
  -h, --help      Show this help.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --targets)
            require_value "$1" "${2-}"
            TARGETS="$2"
            shift 2
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        --skip-tests)
            RUN_TESTS=0
            shift
            ;;
        --skip-package)
            RUN_PACKAGES=0
            shift
            ;;
        --resume)
            RESUME_TESTS=1
            shift
            ;;
        --incremental)
            INCREMENTAL=1
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

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

IFS=',' read -r -a REQUESTED_TARGETS <<< "$TARGETS"
SELECTED_TARGETS=()
for target in "${REQUESTED_TARGETS[@]}"; do
    [ -n "$target" ] || continue
    map_target "$target"
    command -v "$target-gcc" >/dev/null 2>&1 ||
        die "cross compiler not found: $target-gcc"
    command -v "$MAP_QEMU" >/dev/null 2>&1 ||
        die "QEMU interpreter not found: $MAP_QEMU"
    [ -d "$MAP_SYSROOT" ] || die "target sysroot not found: $MAP_SYSROOT"

    if [[ " ${SELECTED_TARGETS[*]} " != *" $target "* ]]; then
        SELECTED_TARGETS+=("$target")
    fi
done
[ "${#SELECTED_TARGETS[@]}" -gt 0 ] || die "--targets selected no targets"

cd "$ROOT"
mkdir -p "$OUTPUT_ROOT/build-logs" "$OUTPUT_ROOT/native"

HEADLESS_FEATURE_ARGS=(
    DISABLE_GPM=YesPlease
    DISABLE_X11=YesPlease
    DISABLE_OPENGL=YesPlease
    DISABLE_ALSA=YesPlease
    DISABLE_PULSE=YesPlease
)

##############################################################################
# Host compiler
##############################################################################

msg "building the host compiler from bootstrap sources"
if [ "$INCREMENTAL" -eq 0 ] || [ ! -x "$ROOT/bin/fbc" ]; then
    make -j"$JOBS" bootstrap-minimal \
        > "$OUTPUT_ROOT/build-logs/host-bootstrap.log" 2>&1
    make clean-libs
fi
make -j"$JOBS" rtlib \
    BUILD_FBC="$ROOT/bin/fbc" \
    "${HEADLESS_FEATURE_ARGS[@]}" \
    > "$OUTPUT_ROOT/build-logs/host-runtime.log" 2>&1

# The release bootstrap compiler predates the type alias syntax in the
# current headers.  Build the current compiler once through the compatibility
# definitions, then use that compiler for the ordinary self-hosted build.
make -j"$JOBS" -o libs compiler \
    BUILD_FBC="$ROOT/bin/fbc" \
    BUILD_FBCFLAGS="-d __FB_BOOTSTRAP_COMPAT__" \
    "${HEADLESS_FEATURE_ARGS[@]}" \
    > "$OUTPUT_ROOT/build-logs/host-compiler-bootstrap.log" 2>&1
make clean-compiler
make -j"$JOBS" -o libs compiler \
    BUILD_FBC="$ROOT/bin/fbc" \
    "${HEADLESS_FEATURE_ARGS[@]}" \
    > "$OUTPUT_ROOT/build-logs/host-compiler.log" 2>&1
"$ROOT/bin/fbc" -version

make compiler-mips-smoke BUILD_FBC="$ROOT/bin/fbc" \
    > "$OUTPUT_ROOT/build-logs/compiler-mips-smoke.log" 2>&1

##############################################################################
# Target libraries and native compilers
##############################################################################

for target in "${SELECTED_TARGETS[@]}"; do
    map_target "$target"
    runtime_dir="$ROOT/lib/freebasic/$MAP_RUNTIME"
    native_dir="$OUTPUT_ROOT/native/$target"
    native_compiler="$native_dir/fbc"
    smoke_binary="$native_dir/runtime-smoke"

    msg "building complete target libraries for $target"
    if [ "$INCREMENTAL" -eq 0 ]; then
        make clean-libs TARGET_TRIPLET="$target"
    fi
    make -j"$JOBS" rtlib fbrt gfxlib2 sfxlib \
        TARGET_TRIPLET="$target" \
        BUILD_FBC="$ROOT/bin/fbc" \
        DISABLE_GPM=YesPlease \
        DISABLE_X11=YesPlease \
        DISABLE_OPENGL=YesPlease \
        DISABLE_ALSA=YesPlease \
        DISABLE_PULSE=YesPlease \
        > "$OUTPUT_ROOT/build-logs/$target-libraries.log" 2>&1

    msg "building the native FreeBASIC compiler for $target"
    mkdir -p "$native_dir"
    if [ "$INCREMENTAL" -eq 0 ]; then
        make clean-compiler \
            TARGET_TRIPLET="$target" \
            FBC_EXE="$native_compiler"
    fi
    make -j"$JOBS" -o libs compiler \
        TARGET_TRIPLET="$target" \
        BUILD_FBC="$ROOT/bin/fbc" \
        FBC_EXE="$native_compiler" \
        TERM_LIB=-ltinfo \
        DISABLE_GPM=YesPlease \
        DISABLE_X11=YesPlease \
        DISABLE_OPENGL=YesPlease \
        DISABLE_ALSA=YesPlease \
        DISABLE_PULSE=YesPlease \
        > "$OUTPUT_ROOT/build-logs/$target-native-compiler.log" 2>&1

    "$MAP_QEMU" -L "$MAP_SYSROOT" "$native_compiler" -version \
        > "$OUTPUT_ROOT/build-logs/$target-native-version.log" 2>&1

    # A native compiler package is useful only when it can drive GCC. Under
    # QEMU user mode the build prefix selects the container's matching GNU
    # supply chain while the compiler itself executes target machine code.
    "$MAP_QEMU" -L "$MAP_SYSROOT" "$native_compiler" \
        -prefix "$ROOT" \
        -i "$ROOT/inc" \
        -buildprefix "$target-" \
        "$ROOT/tests/mips/runtime-smoke.bas" \
        -x "$smoke_binary" \
        > "$OUTPUT_ROOT/build-logs/$target-native-build-smoke.log" 2>&1
    "$MAP_QEMU" -L "$MAP_SYSROOT" "$smoke_binary" \
        > "$OUTPUT_ROOT/build-logs/$target-native-run-smoke.log" 2>&1
    grep -q '^mips runtime ok$' \
        "$OUTPUT_ROOT/build-logs/$target-native-run-smoke.log" ||
        die "native compiler smoke failed for $target"

    # The target archives are staged by the package script. Check the complete
    # media libraries here so a headless test environment cannot hide them.
    for library in libfb.a libfbmt.a libfbgfx.a libfbgfxmt.a libsfx.a libsfxmt.a; do
        [ -f "$runtime_dir/$library" ] ||
            die "missing $library after the $target library build"
    done
done

##############################################################################
# Full test and package workflows
##############################################################################

if [ "$RUN_TESTS" -eq 1 ]; then
    test_args=(
        --targets "$TARGETS"
        --jobs "$JOBS"
        --fbc "$ROOT/bin/fbc"
        --out-dir "$OUTPUT_ROOT/fbctests-batches"
    )
    if [ "$RESUME_TESTS" -eq 1 ]; then
        test_args+=(--resume)
    fi

    "$ROOT/build_scripts/mips-run-fbctests.sh" "${test_args[@]}"
fi

if [ "$RUN_PACKAGES" -eq 1 ]; then
    for target in "${SELECTED_TARGETS[@]}"; do
        MIPS_NATIVE_ROOT="$OUTPUT_ROOT/native" \
        MIPS_PACKAGE_OUTDIR="$OUTPUT_ROOT/packages" \
            "$ROOT/build_scripts/mips-package-freebasic.sh" \
                --target "$target"
    done
fi

msg "MIPS compiler, library, test, and package workflow completed"

# end of build_scripts/mips/container-build.sh
