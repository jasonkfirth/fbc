#!/usr/bin/env bash
#
# Project: FreeBASIC MIPS Linux fbctests workflow
# ------------------------------------------------
#
# File: mips-run-fbctests.sh
#
# Purpose:
#
#     Cross-build and execute the complete FreeBASIC fbcunit suite for the
#     supported MIPS Linux ABIs under QEMU user-mode emulation.
#
# Responsibilities:
#
#     - select all or named top-level fbcunit directories
#     - build one bounded test executable at a time
#     - select the matching QEMU interpreter and target sysroot
#     - preserve build and execution logs for every ABI and directory
#     - reject timeouts, emulator failures, and non-zero fbcunit summaries
#
# This file intentionally does NOT contain:
#
#     - Docker image construction
#     - FreeBASIC compiler or target library construction
#     - native compiler packaging
#
# Memory policy:
#
#     QEMU user mode does not reserve a fixed guest RAM allocation. Running
#     one top-level directory per executable keeps compiler and linker peaks
#     bounded while preserving the complete test inventory.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

OUTPUT_ROOT="${MIPS_FBCTESTS_OUTDIR:-$ROOT/out/mips/fbctests-batches}"
TARGETS="${MIPS_TARGETS:-mips-linux-gnu,mipsel-linux-gnu,mips64-linux-gnuabi64,mips64el-linux-gnuabi64}"
FBC="${MIPS_BUILD_FBC:-$ROOT/bin/fbc}"
SELECTED_DIRS_TEXT=""
RESUME=0
TIMEOUT_SECONDS=600

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

require_command() {
    command -v "$1" >/dev/null 2>&1 ||
        die "required tool not found: $1"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/mips-run-fbctests.sh [options]

Options:
  --targets LIST   Comma-separated GNU target triplets. Default: all four
  --dirs LIST      Comma-separated directories from tests/dirlist.mk.
  --timeout SEC    Per-directory QEMU timeout. Default: 600
  --resume         Skip directories whose saved log already reports success.
  --jobs N         Parallel cross-build jobs. Default: detected CPU count
  --fbc FILE       Host FreeBASIC compiler. Default: bin/fbc
  --out-dir DIR    Result directory. Default: out/mips/fbctests-batches
  -h, --help       Show this help.

This script expects the cross compilers and QEMU interpreters supplied by
build_scripts/mips/Dockerfile. The main Debian/Ubuntu MIPS build script starts
that environment automatically.
EOF
}

map_target() {
    local target="$1"

    case "$target" in
        mips-linux-gnu)
            MAP_QEMU=qemu-mips
            ;;
        mipsel-linux-gnu)
            MAP_QEMU=qemu-mipsel
            ;;
        mips64-linux-gnuabi64)
            MAP_QEMU=qemu-mips64
            ;;
        mips64el-linux-gnuabi64)
            MAP_QEMU=qemu-mips64el
            ;;
        *)
            die "unsupported MIPS target: $target"
            ;;
    esac

    MAP_SYSROOT="/usr/$target"
}

log_reports_success() {
    local log_file="$1"

    [ -f "$log_file" ] || return 1
    grep -a -E -q \
        '^[[:space:]]*[0-9]+[[:space:]]+[0-9]+[[:space:]]+0[[:space:]]+Total' \
        "$log_file"
}

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --targets)
            require_value "$1" "${2-}"
            TARGETS="$2"
            shift 2
            ;;
        --dirs)
            require_value "$1" "${2-}"
            SELECTED_DIRS_TEXT="$2"
            shift 2
            ;;
        --timeout)
            require_value "$1" "${2-}"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --resume)
            RESUME=1
            shift
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        --fbc)
            require_value "$1" "${2-}"
            FBC="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
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

for numeric_value in "$JOBS" "$TIMEOUT_SECONDS"; do
    case "$numeric_value" in
        ''|*[!0-9]*|0) die "jobs and timeout must be positive integers" ;;
    esac
done

##############################################################################
# Target and test selection
##############################################################################

require_command awk
require_command grep
require_command make
require_command timeout

[ -x "$FBC" ] || die "host FreeBASIC compiler not found: $FBC"
[ -f "$ROOT/tests/dirlist.mk" ] || die "test directory list not found"

IFS=',' read -r -a REQUESTED_TARGETS <<< "$TARGETS"
SELECTED_TARGETS=()
for target in "${REQUESTED_TARGETS[@]}"; do
    [ -n "$target" ] || continue
    map_target "$target"
    require_command "$target-gcc"
    require_command "$MAP_QEMU"
    [ -d "$MAP_SYSROOT" ] || die "target sysroot not found: $MAP_SYSROOT"

    if [[ " ${SELECTED_TARGETS[*]} " != *" $target "* ]]; then
        SELECTED_TARGETS+=("$target")
    fi
done
[ "${#SELECTED_TARGETS[@]}" -gt 0 ] || die "--targets selected no targets"

mapfile -t ALL_DIRS < <(
    awk '
        /^DIRLIST_FB[[:space:]]*:=/ { capture = 1; next }
        capture {
            sub(/[[:space:]]*\\[[:space:]]*$/, "")
            if ($0 ~ /^[[:space:]]*$/) exit
            for (field = 1; field <= NF; field++) print $field
        }
    ' "$ROOT/tests/dirlist.mk"
)
[ "${#ALL_DIRS[@]}" -gt 0 ] || die "tests/dirlist.mk contains no DIRLIST_FB entries"

if [ -n "$SELECTED_DIRS_TEXT" ]; then
    IFS=',' read -r -a REQUESTED_DIRS <<< "$SELECTED_DIRS_TEXT"
    SELECTED_DIRS=()
    for test_dir in "${REQUESTED_DIRS[@]}"; do
        [[ " ${ALL_DIRS[*]} " == *" $test_dir "* ]] ||
            die "unknown fbctests directory: $test_dir"
        if [[ " ${SELECTED_DIRS[*]} " != *" $test_dir "* ]]; then
            SELECTED_DIRS+=("$test_dir")
        fi
    done
else
    SELECTED_DIRS=("${ALL_DIRS[@]}")
fi
[ "${#SELECTED_DIRS[@]}" -gt 0 ] || die "--dirs selected no directories"

##############################################################################
# Bounded build and emulation
##############################################################################

mkdir -p "$OUTPUT_ROOT"

for target in "${SELECTED_TARGETS[@]}"; do
    map_target "$target"
    target_output="$OUTPUT_ROOT/$target"
    dirlist_file="$target_output/dirlist.mk"
    mkdir -p "$target_output"

    msg "running ${#SELECTED_DIRS[@]} fbctests directories for $target"

    for test_dir in "${SELECTED_DIRS[@]}"; do
        build_log="$target_output/$test_dir-build.log"
        run_log="$target_output/$test_dir-run.log"

        if [ "$RESUME" -eq 1 ] && log_reports_success "$run_log"; then
            echo "==> PASS (saved): $target $test_dir"
            continue
        fi

        # Rewriting the include makes unit-tests.mk regenerate its source list.
        # Cleaning with that same list prevents objects from another ABI or
        # directory from entering the next aggregate executable.
        printf 'DIRLIST_FB := %s\n' "$test_dir" > "$dirlist_file"
        make -C "$ROOT/tests" -f unit-tests.mk clean \
            DIRLIST_INC="$dirlist_file" \
            FBC="$FBC" \
            TARGET="$target" \
            > "$build_log" 2>&1

        if ! make -C "$ROOT/tests" -f unit-tests.mk -j"$JOBS" build_tests \
            DIRLIST_INC="$dirlist_file" \
            FBC="$FBC" \
            TARGET="$target" \
            >> "$build_log" 2>&1; then
            echo "ERROR: build failed for $target $test_dir; see $build_log" >&2
            exit 1
        fi

        # Several file and data tests intentionally use paths relative to the
        # tests directory. Preserve that native harness working directory.
        if ! (
            cd "$ROOT/tests"
            timeout --signal=TERM --kill-after=10 "$TIMEOUT_SECONDS" \
                "$MAP_QEMU" -L "$MAP_SYSROOT" ./fbc-tests
        ) > "$run_log" 2>&1; then
            echo "ERROR: QEMU run failed for $target $test_dir; see $run_log" >&2
            exit 1
        fi

        if ! log_reports_success "$run_log"; then
            echo "ERROR: fbcunit did not report a clean total for $target $test_dir" >&2
            echo "See $run_log" >&2
            exit 1
        fi

        echo "==> PASS: $target $test_dir"
    done
done

msg "all selected MIPS fbctests passed"

# end of build_scripts/mips-run-fbctests.sh
