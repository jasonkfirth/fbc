#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS Exampleageddon workflow
# -------------------------------------------------
#
# File: riscos-run-exampleageddon.sh
#
# Purpose:
#
#     Compile the complete FreeBASIC example tree for RISC OS and execute every
#     self-contained example in bounded RPCEmu batches.
#
# Responsibilities:
#
#     - refresh the RISC OS runtime libraries used by example programs
#     - run the portable Exampleageddon compile/classification inventory
#     - convert self-contained ARM ELF programs to native RISC OS AIF files
#     - stage per-example resources through UnixLib suffix directories
#     - execute examples sequentially in 192 MiB TaskWindows at 256 MiB RAM
#     - preserve per-batch logs and per-example return codes
#     - merge compile and guest results into CSV and Markdown reports
#
# This file intentionally does NOT contain:
#
#     - GCCSDK, libffi, FreeBASIC, or RPCEmu construction
#     - policy for classifying interactive or externally dependent examples
#     - GUI automation for examples which need input or a display
#     - correctness assertions about the textual output of demonstration code
#
# Runtime ownership:
#
#     The Exampleageddon work tree and RunExamples boot task below the selected
#     RPCEmu runtime are replaced. Other RISC OS Choices and HostFS content are
#     preserved. Host-side compile binaries and reports remain under out/.
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
OUTPUT_ROOT="${RISCOS_EXAMPLEAGEDDON_OUTDIR:-$ROOT/out/riscos/exampleageddon}"

BATCH_SIZE=25
COMPILE_TIMEOUT=180
TIMEOUT_SECONDS=180
RPCEMU_MEMORY=256
COMPILE_ONLY=0
SKIP_COMPILE=0
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
Usage: ./build_scripts/riscos-run-exampleageddon.sh [options]

Options:
  --batch-size N    Self-contained programs per RPCEmu boot. Default: 25
  --compile-timeout SEC
                    Per-example cross-compile timeout. Default: 180
  --timeout SEC     Per-batch emulator timeout. Default: 180
  --memory MIB      RPCEmu memory. Default: 256; maximum: 256
  --compile-only    Compile and classify without launching RPCEmu.
  --skip-compile    Reuse an existing results.csv and target binaries.
  --resume          Reuse complete batch logs whose manifest key matches.
  --jobs N          Parallel cross-build jobs. Default: detected CPU count
  -h, --help        Show this help.

The full compile inventory, RISC OS result CSV, report, and guest logs are
written below out/riscos/exampleageddon. A timed-out batch can be isolated by
rerunning with --batch-size 1 --resume.
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
        --compile-timeout)
            require_value "$1" "${2-}"
            COMPILE_TIMEOUT="$2"
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
        --skip-compile)
            SKIP_COMPILE=1
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

for numeric_value in "$BATCH_SIZE" "$COMPILE_TIMEOUT" "$TIMEOUT_SECONDS" "$JOBS"; do
    case "$numeric_value" in
        ''|*[!0-9]*|0)
            die "batch size, timeouts, and jobs must be positive integers"
            ;;
    esac
done

case "$RPCEMU_MEMORY" in
    4|8|16|32|64|128|256) ;;
    *) die "--memory must be one of: 4, 8, 16, 32, 64, 128, 256" ;;
esac

# Leave one quarter of emulated RAM for RISC OS and the desktop. The default
# therefore grants each sequential example process a 192 MiB Wimp slot.
TASKWINDOW_MIB=$((RPCEMU_MEMORY * 3 / 4))
TASKWINDOW_KIB=$((TASKWINDOW_MIB * 1024))

##############################################################################
# Paths and tools
##############################################################################

for tool in make python3 strings awk sed grep find paste pgrep sha256sum file; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

FBC="$ROOT/bin/fbc"
ENV_FILE="$GCCSDK_ROOT/env.sh"
ELF2AIF="$GCCSDK_ROOT/cross/bin/elf2aif"
RPCEMU_SOURCE="$RPCEMU_WORKDIR/source"
RPCEMU_BINARY="$RPCEMU_SOURCE/rpcemu-recompiler"
RUNTIME_HOSTFS="$RPCEMU_SOURCE/hostfs"
EXAMPLE_WORK="$RUNTIME_HOSTFS/FreeBASIC/exampleageddon/work"
RUNTIME_LOGS="$RUNTIME_HOSTFS/FreeBASIC/exampleageddon/logs"
BOOT_TASK="$RUNTIME_HOSTFS/!Boot/Choices/Boot/Tasks/RunExamples,feb"

COMPILE_RESULTS="$OUTPUT_ROOT/results.csv"
MANIFEST="$OUTPUT_ROOT/riscos-manifest.tsv"
RUN_STATUS="$OUTPUT_ROOT/riscos-run-status.tsv"
RISCOS_RESULTS="$OUTPUT_ROOT/riscos-results.csv"
RISCOS_REPORT="$OUTPUT_ROOT/riscos-report.md"

[ -x "$FBC" ] || die "host FreeBASIC compiler not found: $FBC"
[ -f "$ENV_FILE" ] || die "GCCSDK environment not found: $ENV_FILE"
[ -x "$ELF2AIF" ] || die "GCCSDK elf2aif not found: $ELF2AIF"
[ -d "$HOSTFS_ROOT/FreeBASIC" ] ||
    die "native staging not found; run the Debian/Ubuntu RISC OS build script"

mkdir -p "$OUTPUT_ROOT/logs" "$OUTPUT_ROOT/aif"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"

# shellcheck disable=SC1090
source "$ENV_FILE"
command -v arm-unknown-riscos-gcc >/dev/null 2>&1 ||
    die "arm-unknown-riscos-gcc is unavailable after loading $ENV_FILE"

##############################################################################
# Compile inventory and execution manifest
##############################################################################

if [ "$SKIP_COMPILE" -eq 0 ]; then
    msg "refreshing RISC OS runtime libraries"
    make -s -C "$ROOT" -j"$JOBS" \
        TARGET_TRIPLET=arm-unknown-riscos \
        FBC="$FBC" \
        rtlib fbrt gfxlib2 sfxlib

    [ -s "$ROOT/lib/freebasic/riscos-arm/libffi.a" ] ||
        die "RISC OS libffi is missing from the FreeBASIC runtime directory"

    msg "compiling and classifying the complete example tree for RISC OS"
    python3 "$SCRIPT_DIR/exampleageddon-freebasic.py" \
        --root "$ROOT" \
        --outdir "$OUTPUT_ROOT" \
        --fbc "$FBC -target arm-unknown-riscos -static -i $ROOT/inc/riscos" \
        --prefix "$ROOT" \
        --include-dir "$ROOT/inc" \
        --jobs "$JOBS" \
        --compile-timeout "$COMPILE_TIMEOUT" \
        --no-run \
        --fail-on-self-contained
fi

[ -s "$COMPILE_RESULTS" ] ||
    die "Exampleageddon compile results not found: $COMPILE_RESULTS"

python3 - "$COMPILE_RESULTS" "$OUTPUT_ROOT" "$MANIFEST" <<'PY'
import csv
import sys
from pathlib import Path

results_path = Path(sys.argv[1])
output_root = Path(sys.argv[2])
manifest_path = Path(sys.argv[3])

with results_path.open(newline="", encoding="utf-8") as stream:
    rows = list(csv.DictReader(stream))

selected = [
    row
    for row in rows
    if row["group"] == "self-contained" and row["compile_status"] == "pass"
]

with manifest_path.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(("index", "path", "output", "compile_directory"))
    for index, row in enumerate(selected, start=1):
        output = Path(row["output"])
        stem = output.name
        compile_directory = output_root / "work" / stem / "compile"
        writer.writerow((f"{index:04d}", row["path"], output, compile_directory))
PY

mapfile -t MANIFEST_ROWS < <(tail -n +2 "$MANIFEST")
[ "${#MANIFEST_ROWS[@]}" -gt 0 ] ||
    die "the compile inventory contains no runnable self-contained examples"

if [ "$COMPILE_ONLY" -eq 1 ]; then
    echo
    echo "==> cross-compiled ${#MANIFEST_ROWS[@]} self-contained RISC OS examples"
    echo "    Compile report: $OUTPUT_ROOT/report.md"
    exit 0
fi

##############################################################################
# RISC OS resource staging
##############################################################################

declare -A BATCH_SUFFIXES=()

suffix_swap_tree() {
    local tree="$1"
    local changed=1
    local directory
    local destination
    local file
    local leaf
    local stem
    local suffix

    while [ "$changed" -eq 1 ]; do
        changed=0
        while IFS= read -r -d '' file; do
            directory="$(dirname "$file")"
            leaf="$(basename "$file")"
            stem="${leaf%.*}"
            suffix="${leaf##*.}"

            # Hidden Unix files are not resource suffixes and are unnecessary
            # in the isolated example run directory.
            [ -n "$stem" ] || continue
            case "$suffix" in
                ''|*[!A-Za-z0-9_+-]*)
                    die "unsupported resource suffix in $file"
                    ;;
            esac

            destination="$directory/$suffix/$stem"
            [ ! -e "$destination" ] ||
                die "resource suffix conversion collision: $destination"
            mkdir -p "$directory/$suffix"
            mv "$file" "$destination"
            BATCH_SUFFIXES["$suffix"]=1
            changed=1
        done < <(find "$tree" -type f -name '*.*' -print0)
    done
}

stage_case() {
    local case_id="$1"
    local source_path="$2"
    local binary="$3"
    local compile_directory="$4"
    local case_directory="$TEMP_STAGE/$case_id"
    local output_suffix

    [ -s "$binary" ] || die "compiled example is missing: $binary"
    [ -d "$compile_directory" ] ||
        die "example resource directory is missing: $compile_directory"
    [[ "$(file -b "$binary")" == *"ELF 32-bit"*"ARM"* ]] ||
        die "compiled example is not an ARM ELF file: $binary"

    mkdir -p "$case_directory"
    cp -a "$compile_directory/." "$case_directory/"

    # Example compilation can leave disposable backend intermediates after an
    # interrupted command. They are not runtime resources and can contain dots
    # whose suffix directories would only add noise to UnixLib's search list.
    find "$case_directory" -type f \
        \( -name '*.o' -o -name '*.obj' -o -name '*.asm' \) \
        -delete

    suffix_swap_tree "$case_directory"

    # Common output suffix directories let examples create files through
    # UnixLib even when no same-suffix input resource existed beforehand.
    for output_suffix in csv dat log tmp txt xml; do
        mkdir -p "$case_directory/$output_suffix"
        BATCH_SUFFIXES["$output_suffix"]=1
    done

    cp "$binary" "$case_directory/runner,ff8"
    "$ELF2AIF" "$case_directory/runner,ff8"

    printf '%s\t%s\n' "$case_id" "$source_path" >> "$TEMP_STAGE/cases.tsv"
}

install_batch_runtime() {
    local row
    local case_index
    local source_path
    local binary
    local compile_directory
    local -a fields

    TEMP_STAGE="$(mktemp -d "$OUTPUT_ROOT/.stage.XXXXXX")"
    : > "$TEMP_STAGE/cases.tsv"
    BATCH_SUFFIXES=()

    for row in "${BATCH_ROWS[@]}"; do
        IFS=$'\t' read -r -a fields <<<"$row"
        case_index="${fields[0]}"
        source_path="${fields[1]}"
        binary="${fields[2]}"
        compile_directory="${fields[3]}"
        stage_case "case$case_index" "$source_path" "$binary" "$compile_directory"
    done

    [ "$EXAMPLE_WORK" != "/" ] || die "refusing to replace the filesystem root"
    mkdir -p "$(dirname "$EXAMPLE_WORK")" "$RUNTIME_LOGS"
    rm -rf -- "$EXAMPLE_WORK"
    mv "$TEMP_STAGE" "$EXAMPLE_WORK"
    TEMP_STAGE=""
}

sorted_suffixes() {
    printf '%s\n' "${!BATCH_SUFFIXES[@]}" | sort | paste -sd: -
}

##############################################################################
# RISC OS execution
##############################################################################

write_boot_task() {
    local batch_id="$1"
    local batch_key="$2"
    local suffixes="$3"
    local run_task="$EXAMPLE_WORK/RunBatch,feb"
    local row
    local case_index
    local source_path
    local binary
    local compile_directory

    mkdir -p "$(dirname "$BOOT_TASK")" "$RUNTIME_LOGS" "$EXAMPLE_WORK"
    cat > "$run_task" <<EOF
| FreeBASIC RISC OS Exampleageddon execution task
| ------------------------------------------------
|
| File: RunBatch
|
| Purpose:
|
|     Run self-contained Exampleageddon batch $batch_id sequentially.
|
| Responsibilities:
|
|     - expose staged UnixLib suffix directories
|     - preserve one return-code marker for every attempted example
|     - capture a deterministic batch completion marker
|
| This generated task intentionally does NOT launch its own TaskWindow.

Set Sys\$RCLimit 2147483647
Spool HostFS:\$.FreeBASIC.exampleageddon.logs.$batch_id
Echo Starting FreeBASIC RISC OS Exampleageddon batch $batch_id.
Echo EXAMPLEAGEDDON_KEY_$batch_key
Set UnixEnv\$runner\$sfix "$suffixes"
EOF

    for row in "${BATCH_ROWS[@]}"; do
        IFS=$'\t' read -r case_index source_path binary compile_directory <<<"$row"
        cat >> "$run_task" <<EOF
Dir HostFS:\$.FreeBASIC.exampleageddon.work.case$case_index
Echo EXAMPLE_BEGIN_$case_index $source_path
Run runner
Set FreeBASIC\$ExampleStatus <Sys\$ReturnCode>
Echo EXAMPLE_RESULT_$case_index <FreeBASIC\$ExampleStatus>
EOF
    done

    cat >> "$run_task" <<EOF
Echo EXAMPLEAGEDDON_DONE_$batch_id
Spool

| end of RunBatch
EOF

    cat > "$BOOT_TASK" <<EOF
| FreeBASIC RISC OS Exampleageddon launch task
| --------------------------------------------
|
| File: RunExamples
|
| Purpose:
|
|     Launch Exampleageddon batch $batch_id without blocking the desktop.
|
| Responsibilities:
|
|     - allocate a $TASKWINDOW_MIB MiB TaskWindow for sequential examples
|     - allow Acorn VDU control sequences through the TaskWindow
|     - leave execution and completion logging to RunBatch
|
| This generated task intentionally does NOT wait for completion.

TaskWindow "Obey HostFS:\$.FreeBASIC.exampleageddon.work.RunBatch" -wimpslot ${TASKWINDOW_KIB}K -name "FreeBASIC examples" -ctrl -quit

| end of RunExamples
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

record_batch_results() {
    local batch_id="$1"
    local saved_log="$2"
    local default_status="$3"
    local row
    local case_index
    local source_path
    local binary
    local compile_directory
    local result_line
    local return_code
    local run_status

    for row in "${BATCH_ROWS[@]}"; do
        IFS=$'\t' read -r case_index source_path binary compile_directory <<<"$row"
        result_line=""
        if [ -f "$saved_log" ]; then
            # PRINT without a trailing separator leaves the cursor at the end
            # of the program's output. RISC OS then appends our Echo marker to
            # that same line, so search for the marker instead of requiring it
            # to begin the line.
            result_line="$(
                grep -a -o "EXAMPLE_RESULT_$case_index [^[:space:]]*" \
                    "$saved_log" | tail -n 1 || true
            )"
        fi

        if [ -n "$result_line" ]; then
            return_code="${result_line#EXAMPLE_RESULT_$case_index }"
            if [ "$return_code" = "0" ]; then
                run_status="pass"
            else
                run_status="fail"
            fi
        else
            return_code=""
            run_status="$default_status"
        fi

        printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
            "$source_path" "$run_status" "$return_code" "$batch_id" \
            "logs/$batch_id.log" "" >> "$RUN_STATUS"
    done
}

run_batch() {
    local batch_id="$1"
    local batch_key="$2"
    local suffixes
    local runtime_log=""
    local elapsed=0
    local saved_log="$OUTPUT_ROOT/logs/$batch_id.log"
    local completed=0

    suffixes="$(sorted_suffixes)"
    write_boot_task "$batch_id" "$batch_key" "$suffixes"

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
           grep -a -q "EXAMPLEAGEDDON_DONE_$batch_id" "$runtime_log"; then
            completed=1
            break
        fi

        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            break
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    stop_emulator

    if [ -n "$runtime_log" ] && [ -f "$runtime_log" ]; then
        strings -a "$runtime_log" > "$saved_log"
    else
        : > "$saved_log"
    fi

    if [ "$completed" -eq 1 ]; then
        record_batch_results "$batch_id" "$saved_log" "fail"
    else
        echo "RISC OS batch did not complete within $TIMEOUT_SECONDS seconds." >> "$saved_log"
        record_batch_results "$batch_id" "$saved_log" "timeout"
        echo "==> $batch_id timed out; continuing so later batches are covered" >&2
    fi
}

##############################################################################
# Main guest workflow
##############################################################################

if pgrep -x rpcemu-recompiler >/dev/null 2>&1 ||
   pgrep -x rpcemu-interpreter >/dev/null 2>&1; then
    die "an RPCEmu process is already running; close it before Exampleageddon"
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

printf 'path\trun_status\treturn_code\tbatch\trun_log\trun_seconds\n' > "$RUN_STATUS"

TOTAL_BATCHES=$(( (${#MANIFEST_ROWS[@]} + BATCH_SIZE - 1) / BATCH_SIZE ))

for ((batch_index = 0; batch_index < TOTAL_BATCHES; batch_index++)); do
    batch_number=$((batch_index + 1))
    batch_id="$(printf 'batch%03d' "$batch_number")"
    batch_start=$((batch_index * BATCH_SIZE))
    BATCH_ROWS=("${MANIFEST_ROWS[@]:batch_start:BATCH_SIZE}")
    batch_key="$(printf '%s\n' "${BATCH_ROWS[@]}" | sha256sum | awk '{print $1}')"
    saved_log="$OUTPUT_ROOT/logs/$batch_id.log"

    if [ "$RESUME" -eq 1 ] && [ -f "$saved_log" ] &&
       grep -Fqx "EXAMPLEAGEDDON_KEY_$batch_key" "$saved_log" &&
       grep -Fqx "EXAMPLEAGEDDON_DONE_$batch_id" "$saved_log"; then
        echo "==> reusing complete $batch_id"
        record_batch_results "$batch_id" "$saved_log" "fail"
        continue
    fi

    install_batch_runtime
    run_batch "$batch_id" "$batch_key"
done

if python3 "$SCRIPT_DIR/riscos-exampleageddon-report.py" \
    --compile-results "$COMPILE_RESULTS" \
    --run-status "$RUN_STATUS" \
    --output-csv "$RISCOS_RESULTS" \
    --report "$RISCOS_REPORT"; then
    echo
    echo "==> RISC OS Exampleageddon passed: ${#MANIFEST_ROWS[@]} examples"
    echo "    Report: $RISCOS_REPORT"
    echo "    Results: $RISCOS_RESULTS"
    echo "    Logs: $OUTPUT_ROOT/logs"
else
    echo
    echo "==> RISC OS Exampleageddon found self-contained problems" >&2
    echo "    Report: $RISCOS_REPORT" >&2
    echo "    Retry isolated batches with --batch-size 1 --resume" >&2
    exit 1
fi

# end of riscos-run-exampleageddon.sh
