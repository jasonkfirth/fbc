#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS fbctests workflow
# --------------------------------------------
#
# File: riscos-run-fbctests.sh
#
# Purpose:
#
#     Cross-build and run the FreeBASIC unit tests in bounded RPCEmu batches.
#
# Responsibilities:
#
#     - select all or named top-level fbctests directories
#     - build one statically linked ARM test runner per bounded batch
#     - stage tracked test resources using UnixLib suffix directories
#     - run each AIF under RISC OS Open with 256 MiB by default
#     - preserve console logs and reject crashes, timeouts, and test failures
#
# This file intentionally does NOT contain:
#
#     - GCCSDK, FreeBASIC, or RPCEmu construction
#     - interactive desktop setup for a fresh RISC OS Choices directory
#     - compiler negative-test harness execution
#
# Runtime ownership:
#
#     The fbctests work tree and RunFBTests boot task below the selected RPCEmu
#     runtime are replaced. Other RISC OS Choices and HostFS content are kept.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

GCCSDK_ROOT="${GCCSDK_ROOT:-$ROOT/out/riscos/gccsdk}"
RPCEMU_WORKDIR="${RISCOS_RPCEMU_WORKDIR:-$ROOT/out/riscos/rpcemu}"
HOSTFS_ROOT="${RISCOS_HOSTFS_ROOT:-$ROOT/out/riscos/hostfs}"
OUTPUT_ROOT="${RISCOS_FBCTESTS_OUTDIR:-$ROOT/out/riscos/fbctests}"

BATCH_SIZE=4
TIMEOUT_SECONDS=240
RPCEMU_MEMORY=256
SELECTED_DIRS_TEXT=""
COMPILE_ONLY=0
RESUME=0

RPCEMU_PID=""
BOOT_TASK=""
TEMP_STAGE=""

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

stop_emulator() {
    local attempt

    [ -n "$RPCEMU_PID" ] || return 0
    if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
        RPCEMU_PID=""
        return 0
    fi

    kill "$RPCEMU_PID" 2>/dev/null || true
    for attempt in 1 2 3 4 5; do
        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            break
        fi
        sleep 1
    done

    if kill -0 "$RPCEMU_PID" 2>/dev/null; then
        kill -KILL "$RPCEMU_PID" 2>/dev/null || true
    fi
    wait "$RPCEMU_PID" 2>/dev/null || true
    RPCEMU_PID=""
}

cleanup() {
    stop_emulator

    if [ -n "$BOOT_TASK" ] &&
       [[ "$BOOT_TASK" == "$RPCEMU_WORKDIR"/source/hostfs/!Boot/Choices/Boot/Tasks/* ]] &&
       [ -f "$BOOT_TASK" ]; then
        rm -f -- "$BOOT_TASK"
    fi

    if [ -n "$TEMP_STAGE" ] &&
       [[ "$TEMP_STAGE" == "$OUTPUT_ROOT"/.stage.* ]] &&
       [ -d "$TEMP_STAGE" ]; then
        rm -rf -- "$TEMP_STAGE"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-run-fbctests.sh [options]

Options:
  --batch-size N    Top-level test directories per AIF. Default: 4
  --dirs LIST       Comma-separated directories from tests/dirlist.mk.
  --timeout SEC     Per-batch emulator timeout. Default: 240
  --memory MIB      RPCEmu memory. Default: 256; maximum: 256
  --compile-only    Build every selected AIF without launching RPCEmu.
  --resume          Skip batches whose saved log already reports success.
  --jobs N          Parallel cross-build jobs. Default: detected CPU count
  -h, --help        Show this help.

Examples:
  ./build_scripts/riscos-run-fbctests.sh --dirs boolean
  ./build_scripts/riscos-run-fbctests.sh --batch-size 1 --resume

Logs are saved below out/riscos/fbctests/logs. If a four-directory batch runs
out of memory, rerun with --batch-size 1 --resume.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --batch-size)
            require_value "$1" "${2-}"
            BATCH_SIZE="$2"
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
        --memory)
            require_value "$1" "${2-}"
            RPCEMU_MEMORY="$2"
            shift 2
            ;;
        --compile-only)
            COMPILE_ONLY=1
            shift
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
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

for numeric_value in "$BATCH_SIZE" "$TIMEOUT_SECONDS" "$JOBS"; do
    case "$numeric_value" in
        ''|*[!0-9]*|0) die "batch size, timeout, and jobs must be positive integers" ;;
    esac
done

case "$RPCEMU_MEMORY" in
    4|8|16|32|64|128|256) ;;
    *) die "--memory must be one of: 4, 8, 16, 32, 64, 128, 256" ;;
esac

# Leave one quarter of emulated RAM for RISC OS, the desktop, and filing
# systems. At the default 256 MiB this gives fbctests a 192 MiB Wimp slot.
TASKWINDOW_MIB=$((RPCEMU_MEMORY * 3 / 4))
TASKWINDOW_KIB=$((TASKWINDOW_MIB * 1024))

##############################################################################
# Paths, tools, and directory selection
##############################################################################

for tool in git make strings awk sed grep find paste pgrep; do
    command -v "$tool" >/dev/null 2>&1 || die "required host tool not found: $tool"
done

FBC="$ROOT/bin/fbc"
ENV_FILE="$GCCSDK_ROOT/env.sh"
ELF2AIF="$GCCSDK_ROOT/cross/bin/elf2aif"
RPCEMU_SOURCE="$RPCEMU_WORKDIR/source"
RPCEMU_BINARY="$RPCEMU_SOURCE/rpcemu-recompiler"
RUNTIME_HOSTFS="$RPCEMU_SOURCE/hostfs"
TEST_WORK="$RUNTIME_HOSTFS/FreeBASIC/fbctests/work"
RUNTIME_LOGS="$RUNTIME_HOSTFS/FreeBASIC/fbctests/logs"
BOOT_TASK="$RUNTIME_HOSTFS/!Boot/Choices/Boot/Tasks/RunFBTests,feb"

[ -x "$FBC" ] || die "host FreeBASIC compiler not found: $FBC"
[ -f "$ENV_FILE" ] || die "GCCSDK environment not found: $ENV_FILE"
[ -x "$ELF2AIF" ] || die "GCCSDK elf2aif not found: $ELF2AIF"
[ -d "$HOSTFS_ROOT/FreeBASIC" ] ||
    die "native FreeBASIC staging not found; run the Debian/Ubuntu RISC OS build script"

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

declare -A VALID_DIRS=()
for directory in "${ALL_DIRS[@]}"; do
    VALID_DIRS["$directory"]=1
done

if [ -n "$SELECTED_DIRS_TEXT" ]; then
    IFS=',' read -r -a SELECTED_DIRS <<<"$SELECTED_DIRS_TEXT"
else
    SELECTED_DIRS=("${ALL_DIRS[@]}")
fi

for directory in "${SELECTED_DIRS[@]}"; do
    [ -n "$directory" ] || die "--dirs contains an empty directory name"
    [ -n "${VALID_DIRS[$directory]:-}" ] ||
        die "unknown fbctests directory: $directory"
    [ -d "$ROOT/tests/$directory" ] ||
        die "fbctests directory does not exist: tests/$directory"
done

mkdir -p "$OUTPUT_ROOT/dirlists" "$OUTPUT_ROOT/logs" "$OUTPUT_ROOT/aif"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"

# shellcheck disable=SC1090
source "$ENV_FILE"
command -v arm-unknown-riscos-gcc >/dev/null 2>&1 ||
    die "arm-unknown-riscos-gcc is unavailable after loading $ENV_FILE"

##############################################################################
# Build and resource staging
##############################################################################

write_dirlist() {
    local destination="$1"
    shift

    {
        echo '# Generated by build_scripts/riscos-run-fbctests.sh'
        printf 'DIRLIST_FB :='
        printf ' %s' "$@"
        printf '\n'
        echo '# end of generated directory list'
    } > "$destination"
}

build_batch() {
    local batch_id="$1"
    local dirlist="$2"

    msg "cross-building $batch_id: ${BATCH_DIRS[*]}"
    write_dirlist "$dirlist" "${BATCH_DIRS[@]}"

    make -s -C "$ROOT/tests" -f unit-tests.mk clean \
        FBC="$FBC" \
        TARGET=arm-unknown-riscos \
        CC=arm-unknown-riscos-gcc \
        DIRLIST_INC="$dirlist"

    make -s -C "$ROOT/tests" -f unit-tests.mk -j"$JOBS" build_tests \
        FBC="$FBC" \
        TARGET=arm-unknown-riscos \
        CC=arm-unknown-riscos-gcc \
        ENABLE_CONSOLE_OUTPUT=1 \
        DIRLIST_INC="$dirlist"

    [ -s "$ROOT/tests/fbc-tests" ] ||
        die "$batch_id did not produce tests/fbc-tests"
    cp "$ROOT/tests/fbc-tests" "$OUTPUT_ROOT/aif/$batch_id,ff8"
    "$ELF2AIF" "$OUTPUT_ROOT/aif/$batch_id,ff8"
}

stage_tracked_resources() {
    local source_file
    local relative_path
    local relative_directory
    local leaf
    local stem
    local suffix
    local destination_directory
    local scope
    local -a scopes=("tests/data")

    declare -gA BATCH_SUFFIXES=()

    for scope in "${BATCH_DIRS[@]}"; do
        scopes+=("tests/$scope")
    done

    while IFS= read -r -d '' source_file; do
        relative_path="${source_file#tests/}"
        relative_directory="$(dirname "$relative_path")"
        leaf="$(basename "$relative_path")"

        case "$leaf" in
            .git*) continue ;;
        esac

        destination_directory="$TEMP_STAGE/$relative_directory"
        stem="$leaf"
        while [[ "$stem" == *.* ]]; do
            suffix="${stem##*.}"
            stem="${stem%.*}"
            case "$suffix" in
                ''|*[!A-Za-z0-9_-]*)
                    die "unsupported resource suffix in $source_file"
                    ;;
            esac
            BATCH_SUFFIXES["$suffix"]=1
            destination_directory="$destination_directory/$suffix"
        done

        mkdir -p "$destination_directory"
        cp "$ROOT/$source_file" "$destination_directory/$stem"
    done < <(git -C "$ROOT" ls-files -z -- "${scopes[@]}")

    for suffix in bas bi bmk bmp c cpp csv dat h inc ini ll log lst mk png \
        pp ps1 sh tmp txt xml; do
        BATCH_SUFFIXES["$suffix"]=1
    done

    # Tests create several output formats. Precreate their suffix directories
    # within every selected top-level directory so UnixLib can create files.
    for scope in "${BATCH_DIRS[@]}"; do
        for suffix in dat log tmp txt xml; do
            mkdir -p "$TEMP_STAGE/$scope/$suffix"
        done
    done
}

install_batch_runtime() {
    local batch_id="$1"

    TEMP_STAGE="$(mktemp -d "$OUTPUT_ROOT/.stage.XXXXXX")"
    stage_tracked_resources

    [ "$TEST_WORK" != "/" ] || die "refusing to replace the filesystem root"
    mkdir -p "$(dirname "$TEST_WORK")" "$RUNTIME_LOGS"
    rm -rf -- "$TEST_WORK"
    mv "$TEMP_STAGE" "$TEST_WORK"
    TEMP_STAGE=""
    cp "$OUTPUT_ROOT/aif/$batch_id,ff8" "$TEST_WORK/runner,ff8"
}

sorted_suffixes() {
    printf '%s\n' "${!BATCH_SUFFIXES[@]}" | sort | paste -sd: -
}

##############################################################################
# RISC OS execution
##############################################################################

write_boot_task() {
    local batch_id="$1"
    local suffixes="$2"
    local run_task="$TEST_WORK/RunBatch,feb"

    mkdir -p "$(dirname "$BOOT_TASK")" "$RUNTIME_LOGS" "$TEST_WORK"
    cat > "$run_task" <<EOF
| FreeBASIC RISC OS fbctests execution task
| -----------------------------------------
|
| File: RunBatch
|
| Purpose:
|
|     Run generated fbctests batch $batch_id inside a TaskWindow.
|
| Responsibilities:
|
|     - expose the staged UnixLib suffix directories
|     - capture all test output and a deterministic completion marker
|
| This generated task intentionally does NOT launch its own TaskWindow.

Set Sys\$RCLimit 2147483647
Spool HostFS:\$.FreeBASIC.fbctests.logs.$batch_id
Echo Starting FreeBASIC fbctests batch $batch_id.
Echo FreeBASIC fbctests directories: ${BATCH_DIRS[*]}
Set UnixEnv\$runner\$sfix "$suffixes"
Dir HostFS:\$.FreeBASIC.fbctests.work
Run runner --brief-summary
Set FreeBASIC\$TestStatus <Sys\$ReturnCode>
Echo FreeBASIC fbctests return code: <FreeBASIC\$TestStatus>
Echo FBCTESTS_DONE_$batch_id
Spool

| end of RunBatch
EOF

    cat > "$BOOT_TASK" <<EOF
| FreeBASIC RISC OS fbctests launch task
| --------------------------------------
|
| File: RunFBTests
|
| Purpose:
|
|     Launch fbctests batch $batch_id without blocking the desktop task.
|
| Responsibilities:
|
|     - allocate a $TASKWINDOW_MIB MiB TaskWindow slot for the test runner
|     - allow Acorn VDU control sequences through the TaskWindow
|     - leave execution and completion logging to RunBatch
|
| This generated task intentionally does NOT wait for test completion.

TaskWindow "Obey HostFS:\$.FreeBASIC.fbctests.work.RunBatch" -wimpslot ${TASKWINDOW_KIB}K -name "FreeBASIC fbctests" -ctrl -quit

| end of RunFBTests
EOF
}

find_runtime_log() {
    local batch_id="$1"

    if [ -f "$RUNTIME_LOGS/$batch_id,ffd" ]; then
        printf '%s\n' "$RUNTIME_LOGS/$batch_id,ffd"
    elif [ -f "$RUNTIME_LOGS/$batch_id" ]; then
        printf '%s\n' "$RUNTIME_LOGS/$batch_id"
    fi
}

run_batch() {
    local batch_id="$1"
    local suffixes
    local runtime_log=""
    local elapsed=0
    local saved_log="$OUTPUT_ROOT/logs/$batch_id.log"

    suffixes="$(sorted_suffixes)"
    write_boot_task "$batch_id" "$suffixes"

    runtime_log="$(find_runtime_log "$batch_id")"
    if [ -n "$runtime_log" ]; then
        rm -f -- "$runtime_log"
    fi

    msg "running $batch_id under RISC OS at $RPCEMU_MEMORY MiB"
    (
        cd "$RPCEMU_SOURCE"
        exec "$RPCEMU_BINARY"
    ) &
    RPCEMU_PID="$!"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        runtime_log="$(find_runtime_log "$batch_id")"
        if [ -n "$runtime_log" ] &&
           grep -a -q "FBCTESTS_DONE_$batch_id" "$runtime_log"; then
            break
        fi

        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            die "RPCEmu exited before $batch_id completed"
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    if [ -z "$runtime_log" ] ||
       ! grep -a -q "FBCTESTS_DONE_$batch_id" "$runtime_log"; then
        die "$batch_id timed out after $TIMEOUT_SECONDS seconds; try --batch-size 1"
    fi

    stop_emulator
    strings -a "$runtime_log" > "$saved_log"
    cat "$saved_log"

    if ! grep -q '^FreeBASIC fbctests return code: 0$' "$saved_log"; then
        die "$batch_id failed; see $saved_log"
    fi
}

##############################################################################
# Main workflow
##############################################################################

msg "refreshing RISC OS runtime libraries"
make -s -C "$ROOT" -j"$JOBS" \
    TARGET_TRIPLET=arm-unknown-riscos \
    FBC="$FBC" \
    rtlib fbrt gfxlib2

if [ "$COMPILE_ONLY" -eq 0 ]; then
    if pgrep -x rpcemu-recompiler >/dev/null 2>&1 ||
       pgrep -x rpcemu-interpreter >/dev/null 2>&1; then
        die "an RPCEmu process is already running; close it before fbctests"
    fi

    "$SCRIPT_DIR/riscos-rpcemu.sh" \
        --workdir "$RPCEMU_WORKDIR" \
        --hostfs "$HOSTFS_ROOT" \
        --memory "$RPCEMU_MEMORY" \
        --jobs "$JOBS"

    [ -x "$RPCEMU_BINARY" ] || die "RPCEmu binary not found: $RPCEMU_BINARY"
    [ -f "$RPCEMU_SOURCE/cmos.ram" ] ||
        die "RPCEmu CMOS state not found; launch RPCEmu once to configure RISC OS"
    [ -f "$RUNTIME_HOSTFS/!Boot/Choices/Boot/Tasks/PinSetup,feb" ] ||
        die "RISC OS Choices are incomplete; launch RPCEmu once and complete desktop setup"
fi

TOTAL_BATCHES=$(( (${#SELECTED_DIRS[@]} + BATCH_SIZE - 1) / BATCH_SIZE ))
PASSED_BATCHES=0

for ((batch_index = 0; batch_index < TOTAL_BATCHES; batch_index++)); do
    batch_number=$((batch_index + 1))
    batch_id="$(printf 'batch%03d' "$batch_number")"
    batch_start=$((batch_index * BATCH_SIZE))
    BATCH_DIRS=("${SELECTED_DIRS[@]:batch_start:BATCH_SIZE}")
    dirlist="$OUTPUT_ROOT/dirlists/$batch_id.mk"
    saved_log="$OUTPUT_ROOT/logs/$batch_id.log"

    if [ "$RESUME" -eq 1 ] && [ -f "$saved_log" ] &&
       grep -Fqx "FreeBASIC fbctests directories: ${BATCH_DIRS[*]}" "$saved_log" &&
       grep -q '^FreeBASIC fbctests return code: 0$' "$saved_log"; then
        echo "==> skipping passed $batch_id: ${BATCH_DIRS[*]}"
        PASSED_BATCHES=$((PASSED_BATCHES + 1))
        continue
    fi

    build_batch "$batch_id" "$dirlist"

    if [ "$COMPILE_ONLY" -eq 0 ]; then
        install_batch_runtime "$batch_id"
        run_batch "$batch_id"
        PASSED_BATCHES=$((PASSED_BATCHES + 1))
    fi
done

echo
if [ "$COMPILE_ONLY" -eq 1 ]; then
    echo "==> cross-built $TOTAL_BATCHES RISC OS fbctests batches"
    echo "    AIF files: $OUTPUT_ROOT/aif"
else
    echo "==> RISC OS fbctests passed: $PASSED_BATCHES/$TOTAL_BATCHES batches"
    echo "    Logs: $OUTPUT_ROOT/logs"
fi

# end of riscos-run-fbctests.sh
