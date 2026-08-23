#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE ARM fbctests workflow
# --------------------------------------------------
#
# File: build_scripts/wince/run-fbctests.sh
#
# Purpose:
#
#     Cross-build and execute the complete FreeBASIC fbcunit suite in bounded
#     Windows CE ARM batches under CERF.
#
# Responsibilities:
#
#     - select all or named top-level fbctests directories
#     - build one aggregate ARM Windows CE test executable per directory
#     - stage tracked test resources into CERF shared storage
#     - launch each batch through the guest-side fbctests runner
#     - validate the child exit status and fbcunit XML report
#     - preserve per-directory build logs, guest logs, XML, and screenshots
#
# This file intentionally does NOT contain:
#
#     - Windows CE toolchain, CERF, or ROM installation
#     - compiler or target-runtime construction
#     - MIPS Windows CE support
#     - test-source baseline overrides
#
# Memory policy:
#
#     Each executable contains one top-level directory.  This bounds link-time
#     and guest memory while preserving the complete inventory, matching the
#     batching policy used for other memory-constrained FreeBASIC targets.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

WORK_ROOT="${WINCE_WORK_ROOT:-$ROOT/out/wince/work}"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf}"
OUTPUT_ROOT="${WINCE_FBCTESTS_OUTDIR:-$ROOT/out/wince/fbctests}"
TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
SELECTED_DIRS_TEXT=""
BOOT_SECONDS=15
TIMEOUT_SECONDS=120
BUILD_ONLY=0
RESUME=0
JOBS=""

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
        ""|*[!0-9]*|0) jobs=1 ;;
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
        die "required host tool not found: $1"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/wince/run-fbctests.sh [options]

Options:
  --dirs LIST          Comma-separated directories from tests/dirlist.mk.
  --boot-seconds N     Windows CE shell initialization wait. Default: 15
  --timeout N          Per-batch execution timeout. Default: 120
  --jobs N             Parallel cross-build jobs. Default: detected CPU count
  --build-only         Cross-build every selected batch without running CERF.
  --resume             Skip batches whose saved XML and status report success.
  --out-dir DIR        Result directory. Default: out/wince/fbctests
  -h, --help           Show this help.

The script expects the prepared source tree at out/wince/work. Guest execution
also requires the CERF installation at out/wince/emulator/cerf. Override those
paths with WINCE_WORK_ROOT and WINCE_CERF_ROOT.
EOF
}

report_passed() {
    local result_file="$1"
    local xml_file="$2"

    [ -f "$result_file" ] || return 1
    [ -f "$xml_file" ] || return 1
    grep -a -E -q '^[[:space:]]*0[[:space:]]*$' "$result_file" || return 1
    grep -a -q '</testsuites>' "$xml_file" || return 1
    ! grep -a -E -q '<testsuite[^>]+(errors|failures)="[1-9]' "$xml_file"
}

copy_if_present() {
    local source="$1"
    local destination="$2"

    if [ -f "$source" ]; then
        cp "$source" "$destination"
    fi
}

remove_batch_outputs() {
    local batch_id="$1"
    local candidate

    for candidate in \
        "$CERF_ROOT/share/fbctests-$batch_id.xml" \
        "$CERF_ROOT/share/fbctests-$batch_id.result"; do
        case "$candidate" in
            "$CERF_ROOT"/share/fbctests-*) ;;
            *) die "refusing to remove unexpected path: $candidate" ;;
        esac

        if [ -f "$candidate" ]; then
            rm -f -- "$candidate"
        fi
    done
}

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --dirs)
            require_value "$1" "${2-}"
            SELECTED_DIRS_TEXT="$2"
            shift 2
            ;;
        --boot-seconds)
            require_value "$1" "${2-}"
            BOOT_SECONDS="$2"
            shift 2
            ;;
        --timeout)
            require_value "$1" "${2-}"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        --build-only)
            BUILD_ONLY=1
            shift
            ;;
        --resume)
            RESUME=1
            shift
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

for numeric_value in "$BOOT_SECONDS" "$TIMEOUT_SECONDS" "$JOBS"; do
    case "$numeric_value" in
        ""|*[!0-9]*|0)
            die "boot wait, timeout, and jobs must be positive integers"
            ;;
    esac
done

##############################################################################
# Validation and directory selection
##############################################################################

for tool in awk cp docker git grep make mkdir; do
    require_command "$tool"
done

[ -d "$WORK_ROOT/tests" ] || die "prepared Windows CE work tree not found"
[ -x "$WORK_ROOT/bin/fbc" ] || die "prepared host compiler not found"
[ -f "$WORK_ROOT/lib/freebasic/wince-arm/libfbmt.a" ] ||
    die "Windows CE ARM multithreaded runtime not found"
[ -f "$WORK_ROOT/lib/freebasic/wince-arm/libfbgfxmt.a" ] ||
    die "Windows CE ARM gfxlib2 runtime not found"
[ -f "$WORK_ROOT/lib/freebasic/wince-arm/libffi.a" ] ||
    die "Windows CE ARM libffi runtime not found"
if [ "$BUILD_ONLY" -eq 0 ]; then
    [ -d "$CERF_ROOT/share" ] || die "CERF shared directory not found"
    [ -x "$SCRIPT_DIR/run-arm-emulator.sh" ] ||
        die "ARM emulator runner is not executable"
fi

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

[ "${#ALL_DIRS[@]}" -gt 0 ] || die "tests/dirlist.mk contains no directories"

if [ -n "$SELECTED_DIRS_TEXT" ]; then
    IFS=',' read -r -a SELECTED_DIRS <<<"$SELECTED_DIRS_TEXT"
else
    SELECTED_DIRS=("${ALL_DIRS[@]}")
fi

for test_directory in "${SELECTED_DIRS[@]}"; do
    case "$test_directory" in
        ""|*[!A-Za-z0-9._-]*)
            die "invalid fbctests directory name: $test_directory"
            ;;
    esac
    [[ " ${ALL_DIRS[*]} " == *" $test_directory "* ]] ||
        die "unknown fbctests directory: $test_directory"
done

mkdir -p "$OUTPUT_ROOT/bin" "$OUTPUT_ROOT/build-logs" \
    "$OUTPUT_ROOT/run-logs" "$OUTPUT_ROOT/xml" "$OUTPUT_ROOT/results" \
    "$OUTPUT_ROOT/screenshots"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"

##############################################################################
# Harness and resource preparation
##############################################################################

msg "synchronizing Windows CE fbctests harness"
cp "$ROOT/tests/common.mk" "$WORK_ROOT/tests/common.mk"
cp "$ROOT/tests/unit-tests.mk" "$WORK_ROOT/tests/unit-tests.mk"
cp "$ROOT/tests/fbcunit/GNUmakefile" \
    "$WORK_ROOT/tests/fbcunit/GNUmakefile"
mkdir -p "$WORK_ROOT/tests/wince"
cp -R "$ROOT/tests/wince/." "$WORK_ROOT/tests/wince/"
mkdir -p "$WORK_ROOT/inc/wince/crt"
cp "$ROOT/inc/wince/crt.bi" "$WORK_ROOT/inc/wince/crt.bi"
cp "$ROOT/inc/wince/crt/stdio.bi" "$WORK_ROOT/inc/wince/crt/stdio.bi"

docker run --rm \
    -v "$WORK_ROOT:/work" \
    -w /work \
    "$TOOLCHAIN_IMAGE" \
    /work/bin/fbc -target wince-arm -gen gcc -O 0 \
        tests/wince/fbctests_runner.bas \
        -x tests/wince/fbctests-runner.exe

if [ "$BUILD_ONLY" -eq 0 ]; then
    cp "$WORK_ROOT/tests/wince/fbctests-runner.exe" \
        "$CERF_ROOT/share/fbctests-runner.exe"

    msg "staging tracked fbctests resources"
    while IFS= read -r -d '' tracked_file; do
        relative_path="${tracked_file#tests/}"
        destination="$CERF_ROOT/share/$relative_path"

        # Intent: honor tracked deletions in the working tree and do not let a
        # stale resource in the isolated guest share mask the source checkout.
        if [ ! -e "$ROOT/$tracked_file" ]; then
            rm -f -- "$destination"
            continue
        fi

        mkdir -p "$(dirname "$destination")"
        cp "$ROOT/$tracked_file" "$destination"
    done < <(git -C "$ROOT" ls-files -z -- tests)
fi

##############################################################################
# Bounded build and guest execution
##############################################################################

passed_batches=0
built_batches=0

for test_directory in "${SELECTED_DIRS[@]}"; do
    build_log="$OUTPUT_ROOT/build-logs/$test_directory.log"
    saved_result="$OUTPUT_ROOT/results/$test_directory.result"
    saved_xml="$OUTPUT_ROOT/xml/$test_directory.xml"

    if [ "$RESUME" -eq 1 ] && report_passed "$saved_result" "$saved_xml"; then
        echo "==> PASS (saved): $test_directory"
        passed_batches=$((passed_batches + 1))
        continue
    fi

    msg "cross-building Windows CE ARM fbctests: $test_directory"
    if ! docker run --rm \
        -v "$WORK_ROOT:/work" \
        -w /work/tests \
        "$TOOLCHAIN_IMAGE" \
        bash -c '
            set -euo pipefail
            test_directory="$1"
            jobs="$2"
            make -f unit-tests.mk clean \
                FBC=/work/bin/fbc \
                TARGET=wince-arm \
                TARGET_TRIPLET=arm-mingw32ce \
                CC=arm-mingw32ce-gcc \
                DIRLIST_FB="$test_directory"
            make -f unit-tests.mk -j"$jobs" build_tests \
                FBC=/work/bin/fbc \
                TARGET=wince-arm \
                TARGET_TRIPLET=arm-mingw32ce \
                CC=arm-mingw32ce-gcc \
                DIRLIST_FB="$test_directory" \
                ENABLE_CONSOLE_OUTPUT=1
        ' bash "$test_directory" "$JOBS" >"$build_log" 2>&1; then
        die "build failed for $test_directory; see $build_log"
    fi

    [ -s "$WORK_ROOT/tests/fbc-tests.exe" ] ||
        die "$test_directory did not produce fbc-tests.exe"
    cp "$WORK_ROOT/tests/fbc-tests.exe" \
        "$OUTPUT_ROOT/bin/$test_directory.exe"
    built_batches=$((built_batches + 1))

    if [ "$BUILD_ONLY" -eq 1 ]; then
        echo "==> BUILT: $test_directory"
        continue
    fi

    cp "$WORK_ROOT/tests/fbc-tests.exe" "$CERF_ROOT/share/fbc-tests.exe"
    printf '%s\r\n' "$test_directory" \
        > "$CERF_ROOT/share/fbctests-current.txt"
    remove_batch_outputs "$test_directory"

    msg "running Windows CE ARM fbctests: $test_directory"
    "$SCRIPT_DIR/run-arm-emulator.sh" \
        --program fbctests-runner.exe \
        --completion "fbctests-$test_directory.result" \
        --log-stem "fbctests-arm-$test_directory" \
        --boot-seconds "$BOOT_SECONDS" \
        --run-seconds "$TIMEOUT_SECONDS"

    guest_result="$CERF_ROOT/share/fbctests-$test_directory.result"
    guest_xml="$CERF_ROOT/share/fbctests-$test_directory.xml"
    copy_if_present "$guest_result" "$saved_result"
    copy_if_present "$guest_xml" "$saved_xml"
    copy_if_present "$CERF_ROOT/logs/fbctests-arm-$test_directory.log" \
        "$OUTPUT_ROOT/run-logs/$test_directory.log"
    copy_if_present "$CERF_ROOT/logs/fbctests-arm-$test_directory.png" \
        "$OUTPUT_ROOT/screenshots/$test_directory.png"

    if ! report_passed "$saved_result" "$saved_xml"; then
        die "fbctests failed for $test_directory; see $saved_xml"
    fi

    test_cases="$(grep -a -o '<testcase ' "$saved_xml" | wc -l)"
    passed_batches=$((passed_batches + 1))
    echo "==> PASS: $test_directory ($test_cases cases)"
done

if [ "$BUILD_ONLY" -eq 1 ]; then
    msg "cross-built $built_batches Windows CE ARM fbctests batches"
else
    msg "Windows CE ARM fbctests passed: $passed_batches/${#SELECTED_DIRS[@]} directories"
fi

# end of build_scripts/wince/run-fbctests.sh
