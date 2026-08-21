#!/usr/bin/env bash
#
# Project: FreeBASIC AROS fbctests workflow
# ----------------------------------------
#
# File: aros-run-fbctests.sh
#
# Purpose:
#
#     Cross-build and run the FreeBASIC fbcunit suite in bounded AROS batches.
#
# Responsibilities:
#
#     - select all or named top-level fbcunit directories
#     - rebuild target libraries and one test executable per batch
#     - stage tracked test resources on architecture-appropriate boot media
#     - run x86_64 and ARM under QEMU and m68k under FS-UAE
#     - preserve serial reports and reject crashes, timeouts, and test failures
#
# This file intentionally does NOT contain:
#
#     - AROS SDK or FreeBASIC package construction
#     - compiler log-test orchestration
#     - Exampleageddon or OMA game orchestration
#
# Architecture policy:
#
#     Generic m68k compiler support remains independent of AROS.  This runner
#     applies the 68000 soft-float and Amiga Hunk requirements only while
#     preparing the AROS m68k executable and emulator transport.
#
# Memory policy:
#
#     x86_64 uses 2 GiB by default.  QEMU's Raspberry Pi 2 model exposes the
#     board's fixed 1 GiB layout.  The m68k machine uses 64 MiB of contiguous
#     motherboard RAM plus 256 MiB of Zorro III RAM and one-directory batches,
#     because current AROS m68k does not add the Zorro III region to its main
#     allocator early enough for large LoadSeg requests.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
OUTPUT_ROOT="${AROS_FBCTESTS_OUTDIR:-$AROS_ROOT/fbctests}"
TARGETS="${AROS_TARGETS:-x86_64,m68k,arm}"

BATCH_SIZE_OVERRIDE=""
COMPILE_ONLY=0
JOBS=""
RESUME=0
SELECTED_DIRS_TEXT=""
SKIP_LIBS=0
TIMEOUT_SECONDS=240
X86_MEMORY_MIB=2048

CURRENT_TARGET=""
M68K_READER_PID=""
M68K_EMULATOR_PID=""
QEMU_PID=""

##############################################################################
# General helpers
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
        die "required host tool not found: $1"
}

stop_m68k_processes() {
    local grace_seconds
    local process_id

    for process_id in "$M68K_EMULATOR_PID" "$M68K_READER_PID"; do
        [ -n "$process_id" ] || continue
        if kill -0 "$process_id" 2>/dev/null; then
            kill "$process_id" 2>/dev/null || true
        fi
        grace_seconds=0
        while kill -0 "$process_id" 2>/dev/null &&
              [ "$grace_seconds" -lt 5 ]; do
            sleep 1
            grace_seconds=$((grace_seconds + 1))
        done
        if kill -0 "$process_id" 2>/dev/null; then
            kill -KILL "$process_id" 2>/dev/null || true
        fi
        wait "$process_id" 2>/dev/null || true
    done

    M68K_EMULATOR_PID=""
    M68K_READER_PID=""
}

stop_qemu() {
    local grace_seconds=0

    [ -n "$QEMU_PID" ] || return 0
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        kill "$QEMU_PID" 2>/dev/null || true
    fi
    while kill -0 "$QEMU_PID" 2>/dev/null && [ "$grace_seconds" -lt 5 ]; do
        sleep 1
        grace_seconds=$((grace_seconds + 1))
    done
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        kill -KILL "$QEMU_PID" 2>/dev/null || true
    fi
    wait "$QEMU_PID" 2>/dev/null || true
    QEMU_PID=""
}

cleanup() {
    stop_qemu
    stop_m68k_processes
}

wait_for_terminal_marker() {
    local batch_id="$1"
    local elapsed=0
    local log_file="$2"
    local process_id="$3"
    local terminal_pattern

    terminal_pattern="FBC_AROS_TEST: $CURRENT_TARGET $batch_id (PASS|FAIL|INCOMPLETE)"
    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        if grep -a -E -q "$terminal_pattern" "$log_file" 2>/dev/null; then
            return 0
        fi
        if [ "$CURRENT_TARGET" = m68k ] &&
           grep -a -E -q "$terminal_pattern" \
               "$OUTPUT_ROOT/m68k/host/marker.log" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$process_id" 2>/dev/null; then
            return 1
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done

    return 1
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-run-fbctests.sh [options]

Options:
  --targets LIST     Comma-separated x86_64,m68k,arm list. Default: all
  --dirs LIST        Comma-separated directories from tests/dirlist.mk.
  --batch-size N     Override the default directories per executable.
                     Defaults: x86_64=4, arm=4, m68k=1
  --timeout SEC      Per-batch emulator timeout. Default: 240
  --memory MIB       x86_64 QEMU memory. Default: 2048; minimum: 256
  --compile-only     Build every selected executable without running AROS.
  --skip-libs        Reuse the existing target runtime and graphics libraries.
  --resume           Skip batches already recorded as passed.
  --jobs N           Parallel cross-build jobs. Default: detected CPU count
  -h, --help         Show this help.

Examples:
  ./build_scripts/aros-run-fbctests.sh --dirs boolean
  ./build_scripts/aros-run-fbctests.sh --targets m68k --resume
  ./build_scripts/aros-run-fbctests.sh --batch-size 1 --resume

Logs and completion records are saved below out/aros/fbctests.  An emulator
timeout is accepted only when the guest emitted the exact PASS marker first.
EOF
}

trap cleanup EXIT

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
        --batch-size)
            require_value "$1" "${2-}"
            BATCH_SIZE_OVERRIDE="$2"
            shift 2
            ;;
        --timeout)
            require_value "$1" "${2-}"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --memory)
            require_value "$1" "${2-}"
            X86_MEMORY_MIB="$2"
            shift 2
            ;;
        --compile-only)
            COMPILE_ONLY=1
            shift
            ;;
        --skip-libs)
            SKIP_LIBS=1
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

for positive_integer in "$TIMEOUT_SECONDS" "$X86_MEMORY_MIB" "$JOBS"; do
    case "$positive_integer" in
        ''|*[!0-9]*|0) die "timeout, memory, and jobs must be positive integers" ;;
    esac
done

if [ -n "$BATCH_SIZE_OVERRIDE" ]; then
    case "$BATCH_SIZE_OVERRIDE" in
        ''|*[!0-9]*|0) die "--batch-size must be a positive integer" ;;
    esac
fi

[ "$X86_MEMORY_MIB" -ge 256 ] || die "--memory must be at least 256 MiB"

IFS=',' read -r -a REQUESTED_TARGETS <<< "$TARGETS"
SELECTED_TARGETS=()
for target in "${REQUESTED_TARGETS[@]}"; do
    case "$target" in
        x86_64|m68k|arm) ;;
        '') continue ;;
        *) die "unsupported AROS target in --targets: $target" ;;
    esac

    if [[ " ${SELECTED_TARGETS[*]} " != *" $target "* ]]; then
        SELECTED_TARGETS+=("$target")
    fi
done
[ "${#SELECTED_TARGETS[@]}" -gt 0 ] || die "--targets selected no targets"

##############################################################################
# Target and test selection
##############################################################################

map_target() {
    local target="$1"

    case "$target" in
        x86_64)
            MAP_FBC_TARGET="aros-x86_64"
            MAP_TARGET_TRIPLET="x86_64-aros"
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-pc-x86_64"
            MAP_TOOL_PREFIX="x86_64-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-pc-x86_64"
            MAP_AROS_SYSTEM="pc-x86_64"
            ;;
        m68k)
            MAP_FBC_TARGET="aros-m68k"
            MAP_TARGET_TRIPLET="m68k-aros"
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-amiga-m68k"
            MAP_TOOL_PREFIX="m68k-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-amiga-m68k"
            MAP_AROS_SYSTEM="amiga-m68k"
            ;;
        arm)
            MAP_FBC_TARGET="aros-arm"
            MAP_TARGET_TRIPLET="arm-aros"
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-raspi-armhf"
            MAP_TOOL_PREFIX="arm-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-raspi-armhf"
            MAP_AROS_SYSTEM="raspi-arm"
            ;;
        *)
            die "internal unsupported target: $target"
            ;;
    esac
}

default_batch_size() {
    case "$1" in
        m68k) printf '%s\n' 1 ;;
        x86_64|arm) printf '%s\n' 4 ;;
    esac
}

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
    IFS=',' read -r -a SELECTED_DIRS <<< "$SELECTED_DIRS_TEXT"
else
    SELECTED_DIRS=("${ALL_DIRS[@]}")
fi

for directory in "${SELECTED_DIRS[@]}"; do
    [ -n "$directory" ] || die "--dirs contains an empty directory name"
    [ -n "${VALID_DIRS[$directory]:-}" ] ||
        die "unknown fbcunit directory: $directory"
    [ -d "$ROOT/tests/$directory" ] ||
        die "fbcunit directory does not exist: tests/$directory"
done

##############################################################################
# Paths and host prerequisites
##############################################################################

FBC="$ROOT/bin/fbc"
RESOURCE_ROOT="$OUTPUT_ROOT/resources"
CONTROL_ROOT="$OUTPUT_ROOT/control"

for tool in awk cp dd find git grep make mkfifo paste sed sort stat tar timeout; do
    require_command "$tool"
done

[ -x "$FBC" ] || die "host FreeBASIC compiler not found: $FBC"

if [ "$COMPILE_ONLY" -eq 0 ]; then
    require_command xorriso
    for target in "${SELECTED_TARGETS[@]}"; do
        case "$target" in
            x86_64) require_command qemu-system-x86_64 ;;
            arm)
                require_command qemu-system-arm
                require_command mcopy
                require_command mmd
                require_command mkfs.vfat
                require_command sfdisk
                ;;
            m68k) require_command fs-uae ;;
        esac
    done
fi

mkdir -p "$OUTPUT_ROOT" "$CONTROL_ROOT" "$OUTPUT_ROOT/state"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
RESOURCE_ROOT="$OUTPUT_ROOT/resources"
CONTROL_ROOT="$OUTPUT_ROOT/control"

##############################################################################
# Reproducible test resource staging
##############################################################################

prepare_resources() {
    local relative_path
    local source_file
    local temporary_root

    temporary_root="$(mktemp -d "$OUTPUT_ROOT/.resources.XXXXXX")"

    # Only tracked inputs are copied.  This excludes host executables, object
    # files, and temporary audio dumps left by earlier test runs while still
    # preserving current uncommitted edits to tracked fixtures.
    while IFS= read -r -d '' source_file; do
        relative_path="${source_file#tests/}"
        mkdir -p "$temporary_root/$(dirname "$relative_path")"
        cp "$ROOT/$source_file" "$temporary_root/$relative_path"
    done < <(git -C "$ROOT" ls-files -z -- tests)

    if [ -e "$RESOURCE_ROOT" ]; then
        [ "$RESOURCE_ROOT" = "$OUTPUT_ROOT/resources" ] ||
            die "refusing to replace unexpected resource directory"
        rm -rf -- "$RESOURCE_ROOT"
    fi
    mv "$temporary_root" "$RESOURCE_ROOT"
}

write_dirlist() {
    local destination="$1"
    shift

    {
        echo '# Generated by build_scripts/aros-run-fbctests.sh'
        printf 'DIRLIST_FB :='
        printf ' %s' "$@"
        printf '\n'
        echo '# end of generated directory list'
    } > "$destination"
}

##############################################################################
# Target library and helper construction
##############################################################################

refresh_target_libraries() {
    local ffi_include
    local target="$1"

    [ "$SKIP_LIBS" -eq 0 ] || return 0
    map_target "$target"
    ffi_include="$ROOT/lib/freebasic/aros-$target/include"
    [ -f "$ROOT/lib/freebasic/aros-$target/libffi.a" ] ||
        die "AROS $target libffi is missing; run the AROS build workflow first"
    [ -f "$ffi_include/ffi.h" ] ||
        die "AROS $target libffi headers are missing; run the AROS build workflow first"

    msg "refreshing AROS $target runtime, gfxlib2, and sfxlib"
    env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
        make -s -C "$ROOT" -j"$JOBS" \
        TARGET_TRIPLET="$MAP_TARGET_TRIPLET" \
        BUILD_FBC="$FBC" \
        libs
}

build_log_relay() {
    local helper_directory
    local helper_elf
    local target="$1"

    map_target "$target"
    helper_directory="$OUTPUT_ROOT/$target/helpers"
    helper_elf="$helper_directory/log-relay.elf"
    mkdir -p "$helper_directory"

    msg "building AROS $target serial report relay"
    case "$target" in
        m68k)
            "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                -O2 -fno-common -march=68000 -msoft-float \
                "$ROOT/tests/aros/log-relay.c" -o "$helper_elf" -ldebug
            "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" \
                "$helper_elf" "$helper_directory/log-relay"
            ;;
        x86_64|arm)
            if [ "$target" = arm ]; then
                # raspi-armhf is an ARMv7-A hard-float ABI.  arm-aros-gcc has
                # no matching default multilib, so every directly compiled C
                # helper must select the same ABI as FreeBASIC and AROS.
                "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                    -O2 -fno-common \
                    -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4 \
                    "$ROOT/tests/aros/log-relay.c" \
                    -o "$helper_directory/log-relay" -ldebug
            else
                "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                    -O2 -fno-common \
                    "$ROOT/tests/aros/log-relay.c" \
                    -o "$helper_directory/log-relay" -ldebug
            fi
            ;;
    esac
}

build_resource_unpacker() {
    local helper_directory
    local helper_elf
    local target="$1"

    map_target "$target"
    helper_directory="$OUTPUT_ROOT/$target/helpers"
    helper_elf="$helper_directory/resource-unpack.elf"
    mkdir -p "$helper_directory"

    msg "building AROS $target resource unpacker"
    case "$target" in
        m68k)
            "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                -O2 -fno-common -march=68000 -msoft-float \
                "$ROOT/tests/aros/resource-unpack.c" \
                -o "$helper_elf" -ldebug
            "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" \
                "$helper_elf" "$helper_directory/resource-unpack"
            ;;
        x86_64|arm)
            if [ "$target" = arm ]; then
                "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                    -O2 -fno-common \
                    -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4 \
                    "$ROOT/tests/aros/resource-unpack.c" \
                    -o "$helper_directory/resource-unpack" -ldebug
            else
                "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                    -O2 -fno-common \
                    "$ROOT/tests/aros/resource-unpack.c" \
                    -o "$helper_directory/resource-unpack" -ldebug
            fi
            ;;
    esac
}

build_batch() {
    local batch_id="$1"
    local dirlist="$2"
    local executable_directory
    local target="$3"

    map_target "$target"
    executable_directory="$OUTPUT_ROOT/$target/bin"
    mkdir -p "$executable_directory"

    msg "cross-building AROS $target $batch_id: ${BATCH_DIRS[*]}"
    write_dirlist "$dirlist" "${BATCH_DIRS[@]}"

    env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
        make -s -C "$ROOT/tests" -f unit-tests.mk clean \
        FBC="$FBC" \
        TARGET="$MAP_FBC_TARGET" \
        CC="$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
        AR="$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-ar" \
        DIRLIST_INC="$dirlist"

    env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
        make -s -C "$ROOT/tests" -f unit-tests.mk -j"$JOBS" build_tests \
        FBC="$FBC" \
        TARGET="$MAP_FBC_TARGET" \
        CC="$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
        AR="$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-ar" \
        DIRLIST_INC="$dirlist"

    [ -s "$ROOT/tests/fbc-tests" ] ||
        die "AROS $target $batch_id did not produce tests/fbc-tests"

    if [ "$target" = m68k ]; then
        cp "$ROOT/tests/fbc-tests" "$executable_directory/$batch_id.elf"
        "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" \
            "$executable_directory/$batch_id.elf" \
            "$executable_directory/$batch_id"
    else
        cp "$ROOT/tests/fbc-tests" "$executable_directory/$batch_id"
    fi
}

##############################################################################
# Guest startup scripts
##############################################################################

write_cd_startup() {
    local batch_id="$1"
    local destination="$2"
    local failure_repeats=""
    local incomplete_repeats=""
    local pass_repeats=""
    local serial_padding=""
    local target="$3"

    if [ "$target" = m68k ]; then
        cat > "$destination" <<EOF
FailAt 21

SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id directories: ${BATCH_DIRS[*]}"
MakeDir RAM:FreeBASIC
MakeDir RAM:FreeBASIC/T
MakeDir RAM:FreeBASIC/ENV
Assign T: RAM:FreeBASIC/T
Assign ENV: RAM:FreeBASIC/ENV
SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id unpacking resources"
SYS:Tests/FreeBASIC/resource-unpack FBCDrive:resources.tar RAM:FreeBASIC
SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id copying executable"
Copy FBCDrive:fbc-tests RAM:FreeBASIC/fbc-tests QUIET
Protect RAM:FreeBASIC/fbc-tests +e
CD RAM:FreeBASIC

# FS-UAE exposes the selected fixtures and executable as FBCDrive:.  Keeping
# this transport separate from AROS's CD boot avoids minutes of emulated ISO
# copying per low-memory batch while the program still runs entirely on AROS.
Stack 1048576
SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id running tests"
RAM:FreeBASIC/fbc-tests --verbose --brief-summary >FBCDrive:fbctests.log
Set FBCTestRC \$RC
SYS:Tests/FreeBASIC/log-relay FBCDrive:fbctests.log " Total "
Set RelayRC \$RC

If 0 EQ \$RelayRC VAL
    If 0 EQ \$FBCTestRC VAL
        Echo "FBC_AROS_TEST: m68k $batch_id PASS" >FBCDrive:marker.log
        SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id PASS"
        SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id PASS"
        SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id PASS"
    Else
        Echo "FBC_AROS_TEST: m68k $batch_id FAIL" >FBCDrive:marker.log
        SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id FAIL"
        SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id FAIL"
        SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id FAIL"
    EndIf
Else
    Echo "FBC_AROS_TEST: m68k $batch_id INCOMPLETE" >FBCDrive:marker.log
    SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id INCOMPLETE"
    SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id INCOMPLETE"
    SYS:Tests/FreeBASIC/KEcho ".FBC_AROS_TEST: m68k $batch_id INCOMPLETE"
EndIf

Wait 2
C:Shutdown
EOF
        return
    fi

    # The m68k serial device can lose the first byte of a debug write.  A
    # disposable prefix keeps the following automation marker intact whether
    # that byte is consumed or delivered.
    cat > "$destination" <<EOF
FailAt 21

SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id directories: ${BATCH_DIRS[*]}"

MakeDir RAM:FreeBASIC
SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id copying resources"
Copy SYS:Tests/FreeBASIC/resources.tar RAM:FreeBASIC/resources.tar QUIET
SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id unpacking resources"
SYS:Tests/FreeBASIC/resource-unpack RAM:FreeBASIC/resources.tar RAM:FreeBASIC
Delete RAM:FreeBASIC/resources.tar QUIET
SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id copying executable"
Copy SYS:Tests/FreeBASIC/fbc-tests RAM:FreeBASIC/fbc-tests QUIET
Protect RAM:FreeBASIC/fbc-tests +e
MakeDir RAM:FreeBASIC/T
MakeDir RAM:FreeBASIC/ENV
Assign T: RAM:FreeBASIC/T
Assign ENV: RAM:FreeBASIC/ENV
CD RAM:FreeBASIC

# AROS shells otherwise launch children with a roughly 40 KiB stack.  The
# recursion coverage in fbctests intentionally needs more than that, while a
# 1 MiB stack remains bounded even on the m68k test machine.
Stack 1048576
SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id running tests"
RAM:FreeBASIC/fbc-tests --brief-summary --hide-cases >RAM:FreeBASIC/fbctests.log
Set FBCTestRC \$RC
SYS:Tests/FreeBASIC/log-relay RAM:FreeBASIC/fbctests.log " Total "
Set RelayRC \$RC

If 0 EQ \$RelayRC VAL
    If 0 EQ \$FBCTestRC VAL
        SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id PASS"
$pass_repeats
    Else
        SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id FAIL"
$failure_repeats
    EndIf
Else
    SYS:Tests/FreeBASIC/KEcho "${serial_padding}FBC_AROS_TEST: $target $batch_id INCOMPLETE"
$incomplete_repeats
EndIf

Wait 2
C:Shutdown
EOF
}

write_arm_startup() {
    local batch_id="$1"
    local destination="$2"

    cat > "$destination" <<EOF
FailAt 21
Assign T: SYS:Tests/FreeBASIC/T
Assign ENV: SYS:Tests/FreeBASIC/ENV
Echo "FBC_AROS_TEST: arm $batch_id directories: ${BATCH_DIRS[*]}" >SYS:Tests/FreeBASIC/start.log
SYS:Tests/FreeBASIC/log-relay SYS:Tests/FreeBASIC/start.log
CD SYS:Tests/FreeBASIC/resources
Stack 1048576
SYS:Tests/FreeBASIC/fbc-tests --brief-summary --hide-cases >SYS:Tests/FreeBASIC/fbctests.log
Set FBCTestRC \$RC
SYS:Tests/FreeBASIC/log-relay SYS:Tests/FreeBASIC/fbctests.log " Total "
Set RelayRC \$RC
If 0 EQ \$RelayRC VAL
    If 0 EQ \$FBCTestRC VAL
        Echo "FBC_AROS_TEST: arm $batch_id PASS" >SYS:Tests/FreeBASIC/marker.log
    Else
        Echo "FBC_AROS_TEST: arm $batch_id FAIL" >SYS:Tests/FreeBASIC/marker.log
    EndIf
Else
    Echo "FBC_AROS_TEST: arm $batch_id INCOMPLETE" >SYS:Tests/FreeBASIC/marker.log
EndIf
SYS:Tests/FreeBASIC/log-relay SYS:Tests/FreeBASIC/marker.log
Wait 2
C:Shutdown
EOF
}

write_x86_grub_config() {
    local destination="$1"

    cat > "$destination" <<EOF
set timeout=0
set default=0

menuentry "FreeBASIC AROS fbctests" {
    multiboot2 /boot/pc/bootstrap.xz vesa=800x600x32 ATA=32bit debug=serial:0@115200 nomonitors
    module2 /boot/pc/kernel.xz
    module2 /boot/pc/aros-bsp.pkg.xz
    module2 /boot/pc/aros-acpi.pkg.xz
    module2 /boot/aros-base.pkg.xz
    module2 /boot/aros-fs.pkg.xz
    module2 /boot/poseidon.pkg.xz
}
EOF
}

##############################################################################
# x86_64 QEMU execution
##############################################################################

run_x86_batch() {
    local base_iso
    local batch_id="$1"
    local current_iso
    local executable
    local log_file="$2"
    local startup
    local resource_archive

    map_target x86_64
    base_iso="$MAP_AROS_BUILD/distfiles/aros-pc-x86_64.iso"
    executable="$OUTPUT_ROOT/x86_64/bin/$batch_id"
    current_iso="$OUTPUT_ROOT/x86_64/current.iso"
    startup="$CONTROL_ROOT/x86_64-$batch_id-Startup-Sequence"
    resource_archive="$CONTROL_ROOT/x86_64-$batch_id-resources.tar"

    [ -f "$base_iso" ] || die "x86_64 AROS ISO not found: $base_iso"
    [ -f "$executable" ] || die "x86_64 test executable not found: $executable"
    [ -f "$MAP_AROS_BUILD/bin/pc-x86_64/AROS/Developer/Debug/KEcho" ] ||
        die "x86_64 KEcho helper is missing"

    write_cd_startup "$batch_id" "$startup" x86_64
    write_x86_grub_config "$CONTROL_ROOT/x86_64-grub.cfg"
    # Several fbcunit directories consume fixtures from the shared data/
    # directory.  Keep those alongside the selected directories in one
    # sequential ISO file, avoiding recursive CD traversal without unpacking
    # unrelated test sources for every batch.
    # A batch containing the data directory names it twice.  GNU tar would
    # encode the second copy as hard-link entries, which the deliberately
    # small AROS unpacker does not support.  Dereferencing keeps every entry
    # within the regular-file and directory subset accepted by the guest.
    tar --format=ustar --owner=0 --group=0 --numeric-owner \
        --hard-dereference \
        -cf "$resource_archive" -C "$RESOURCE_ROOT" \
        data "${BATCH_DIRS[@]}"

    cp --reflink=auto --sparse=always "$base_iso" "$current_iso"
    if ! xorriso -indev "$current_iso" -outdev "$current_iso" \
        -boot_image any keep \
        -map "$startup" /S/Startup-Sequence \
        -map "$CONTROL_ROOT/x86_64-grub.cfg" /boot/grub/grub.cfg \
        -map "$MAP_AROS_BUILD/bin/pc-x86_64/AROS/Developer/Debug/KEcho" /Tests/FreeBASIC/KEcho \
        -map "$OUTPUT_ROOT/x86_64/helpers/log-relay" /Tests/FreeBASIC/log-relay \
        -map "$OUTPUT_ROOT/x86_64/helpers/resource-unpack" /Tests/FreeBASIC/resource-unpack \
        -map "$executable" /Tests/FreeBASIC/fbc-tests \
        -map "$resource_archive" /Tests/FreeBASIC/resources.tar \
        -commit -end > "$OUTPUT_ROOT/x86_64/xorriso.log" 2>&1; then
        tail -80 "$OUTPUT_ROOT/x86_64/xorriso.log" >&2
        die "could not construct the x86_64 AROS fbctests ISO"
    fi

    msg "running AROS x86_64 $batch_id at $X86_MEMORY_MIB MiB"
    qemu-system-x86_64 \
        -M pc \
        -m "$X86_MEMORY_MIB" \
        -cdrom "$current_iso" \
        -boot d \
        -serial stdio \
        -display none \
        -monitor none \
        -audiodev driver=none,id=fbctest_audio \
        -device AC97,audiodev=fbctest_audio \
        -no-reboot > "$log_file" 2>&1 &
    QEMU_PID="$!"
    wait_for_terminal_marker "$batch_id" "$log_file" "$QEMU_PID" || true
    stop_qemu
}

##############################################################################
# ARM QEMU execution
##############################################################################

prepare_arm_base_image() {
    local aros_system
    local base_image="$OUTPUT_ROOT/arm/base.img"
    local boot_architecture="$CONTROL_ROOT/AROS.boot"

    map_target arm
    aros_system="$MAP_AROS_BUILD/bin/raspi-arm/AROS"
    [ -d "$aros_system" ] || die "ARM AROS Workbench tree not found: $aros_system"

    msg "building reusable 512 MiB ARM AROS fbctests image"
    mkdir -p "$OUTPUT_ROOT/arm"
    # Reset an existing image before recreating its partition table.  Merely
    # extending or shrinking with truncate retains old filesystem signatures.
    truncate -s 0 "$base_image"
    truncate -s 512M "$base_image"
    printf '%s\n' 'label: dos' 'unit: sectors' 'first-lba: 2048' \
        '1 : start=2048, size=1046528, type=c, bootable' |
        sfdisk "$base_image" >/dev/null
    mkfs.vfat -F 32 -n AROS --offset=2048 "$base_image" 523264 >/dev/null

    mcopy -i "$base_image@@1M" -spm "$aros_system"/* ::
    # The raspi Workbench staging tree omits S because the ROM normally owns
    # the boot shell.  A writable S directory lets the FAT boot volume supply
    # the deterministic fbctests Startup-Sequence used by automation.  mmd has
    # no portable parents option, so create each path in dependency order.
    mmd -i "$base_image@@1M" ::/S
    mmd -i "$base_image@@1M" ::/Tests
    mmd -i "$base_image@@1M" ::/Tests/FreeBASIC
    mmd -i "$base_image@@1M" ::/Tests/FreeBASIC/T
    mmd -i "$base_image@@1M" ::/Tests/FreeBASIC/ENV
    mcopy -i "$base_image@@1M" -spm \
        "$RESOURCE_ROOT" ::/Tests/FreeBASIC
    mcopy -i "$base_image@@1M" \
        "$OUTPUT_ROOT/arm/helpers/log-relay" \
        ::/Tests/FreeBASIC/log-relay

    # AROS uses this architecture signature to accept ELF programs loaded
    # from the otherwise architecture-neutral FAT filesystem.
    printf 'arm\n' > "$boot_architecture"
    mcopy -i "$base_image@@1M" "$boot_architecture" ::/AROS.boot
}

run_arm_batch() {
    local aros_system
    local base_image="$OUTPUT_ROOT/arm/base.img"
    local batch_id="$1"
    local current_image="$OUTPUT_ROOT/arm/current.img"
    local executable="$OUTPUT_ROOT/arm/bin/$batch_id"
    local log_file="$2"
    local startup="$CONTROL_ROOT/arm-$batch_id-Startup-Sequence"

    map_target arm
    aros_system="$MAP_AROS_BUILD/bin/raspi-arm/AROS"
    [ -f "$base_image" ] || die "ARM base test image not found: $base_image"
    [ -f "$executable" ] || die "ARM test executable not found: $executable"

    write_arm_startup "$batch_id" "$startup"
    cp --reflink=auto --sparse=always "$base_image" "$current_image"
    mcopy -o -i "$current_image@@1M" \
        "$startup" ::/S/Startup-Sequence
    mcopy -o -i "$current_image@@1M" \
        "$executable" ::/Tests/FreeBASIC/fbc-tests

    msg "running AROS ARM $batch_id on the Raspberry Pi 2 1 GiB model"
    qemu-system-arm \
        -M raspi2b \
        -m 1G \
        -nographic \
        -monitor none \
        -serial stdio \
        -no-reboot \
        -kernel "$aros_system/aros-arm-raspi.img" \
        -initrd "$aros_system/aros-arm-bsp.rom" \
        -dtb "$aros_system/bcm2709-rpi-2-b.dtb" \
        -drive file="$current_image",format=raw,if=sd \
        > "$log_file" 2>&1 &
    QEMU_PID="$!"
    wait_for_terminal_marker "$batch_id" "$log_file" "$QEMU_PID" || true
    stop_qemu
}

##############################################################################
# m68k FS-UAE execution
##############################################################################

run_m68k_batch() {
    local base_iso
    local batch_id="$1"
    local config_file="$CONTROL_ROOT/m68k-$batch_id.fs-uae"
    local current_iso="$OUTPUT_ROOT/m68k/current.iso"
    local emulator_log="$OUTPUT_ROOT/m68k/logs/$batch_id.fs-uae.log"
    local executable="$OUTPUT_ROOT/m68k/bin/$batch_id"
    local fifo="$OUTPUT_ROOT/m68k/serial.fifo"
    local host_directory="$OUTPUT_ROOT/m68k/host"
    local log_file="$2"
    local resource_archive="$host_directory/resources.tar"
    local startup="$CONTROL_ROOT/m68k-$batch_id-Startup-Sequence"

    map_target m68k
    base_iso="$MAP_AROS_BUILD/distfiles/aros-amiga-m68k.iso"
    [ -f "$base_iso" ] || die "m68k AROS ISO not found: $base_iso"
    [ -f "$executable" ] || die "m68k Hunk test executable not found: $executable"
    [ -f "$MAP_AROS_BUILD/bin/amiga-m68k/AROS/Developer/Debug/KEcho" ] ||
        die "m68k KEcho helper is missing"

    write_cd_startup "$batch_id" "$startup" m68k

    [ "$host_directory" = "$OUTPUT_ROOT/m68k/host" ] ||
        die "refusing to replace unexpected m68k host-directory volume"
    rm -rf -- "$host_directory"
    mkdir -p "$host_directory"
    tar --format=ustar --owner=0 --group=0 --numeric-owner \
        --hard-dereference \
        -cf "$resource_archive" -C "$RESOURCE_ROOT" \
        data "${BATCH_DIRS[@]}"
    cp "$executable" "$host_directory/fbc-tests"
    chmod u+x "$host_directory/fbc-tests"

    cp --reflink=auto --sparse=always "$base_iso" "$current_iso"
    if ! xorriso -dev "$current_iso" \
        -boot_image any replay \
        -map "$startup" /S/Startup-Sequence \
        -map "$MAP_AROS_BUILD/bin/amiga-m68k/AROS/Developer/Debug/KEcho" /Tests/FreeBASIC/KEcho \
        -map "$OUTPUT_ROOT/m68k/helpers/log-relay" /Tests/FreeBASIC/log-relay \
        -map "$OUTPUT_ROOT/m68k/helpers/resource-unpack" /Tests/FreeBASIC/resource-unpack \
        -commit -end > "$OUTPUT_ROOT/m68k/xorriso.log" 2>&1; then
        tail -80 "$OUTPUT_ROOT/m68k/xorriso.log" >&2
        die "could not construct the m68k AROS fbctests ISO"
    fi

    cat > "$config_file" <<EOF
[fs-uae]
amiga_model = A4000/040
kickstart_file = $MAP_AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-rom.bin
kickstart_ext_file = $MAP_AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-ext.bin
cdrom_drive_0 = $current_iso
hard_drive_0 = $host_directory
hard_drive_0_label = FBCDrive
hard_drive_0_priority = -128
fast_memory = 8192
motherboard_ram = 65536
zorro_iii_memory = 1048576
graphics_card = uaegfx
graphics_card_memory = 32768
serial_port = $fifo
amiga_enable_serial_port = 1
fullscreen = 0
window_width = 800
window_height = 600
sound_output = none
EOF

    if [ ! -p "$fifo" ]; then
        [ ! -e "$fifo" ] || rm -f -- "$fifo"
        mkfifo "$fifo"
    fi
    : > "$log_file"
    : > "$emulator_log"

    msg "running AROS m68k $batch_id with bounded LoadSeg memory"
    timeout "$TIMEOUT_SECONDS" dd if="$fifo" of="$log_file" status=none &
    M68K_READER_PID="$!"
    timeout "$TIMEOUT_SECONDS" fs-uae "$config_file" --no-gui \
        > "$emulator_log" 2>&1 &
    M68K_EMULATOR_PID="$!"

    wait_for_terminal_marker \
        "$batch_id" "$log_file" "$M68K_EMULATOR_PID" || true

    if [ -f "$host_directory/marker.log" ]; then
        printf '\n' >> "$log_file"
        cat "$host_directory/marker.log" >> "$log_file"
    fi
    stop_m68k_processes

    # FS-UAE's serial FIFO can lose bytes during shutdown.  The host-directory
    # marker is written by AROS after the test process exits, so append it once
    # more after both readers stop.  This also makes a complete host marker the
    # authoritative transport when a serial PASS line was corrupted.
    if [ -f "$host_directory/marker.log" ]; then
        printf '\n' >> "$log_file"
        cat "$host_directory/marker.log" >> "$log_file"
    fi
}

##############################################################################
# Batch validation and orchestration
##############################################################################

validate_batch_log() {
    local batch_id="$1"
    local done_file="$2"
    local log_file="$3"
    local pass_marker="FBC_AROS_TEST: $CURRENT_TARGET $batch_id PASS"

    if ! grep -a -F -q "$pass_marker" "$log_file"; then
        tail -120 "$log_file" || true
        die "AROS $CURRENT_TARGET $batch_id failed or timed out; see $log_file"
    fi

    grep -a -E 'FBC_AROS_TEST:|Total[[:space:]]+[0-9]+|Asserts|Passed|Failed' \
        "$log_file" | tail -80 || true
    {
        printf 'target=%s\n' "$CURRENT_TARGET"
        printf 'batch=%s\n' "$batch_id"
        printf 'directories=%s\n' "${BATCH_DIRS[*]}"
        printf 'marker=%s\n' "$pass_marker"
    } > "$done_file"
}

batch_is_complete() {
    local batch_id="$1"
    local done_file="$2"

    [ "$RESUME" -eq 1 ] || return 1
    [ -f "$done_file" ] || return 1
    grep -F -x -q "target=$CURRENT_TARGET" "$done_file" || return 1
    grep -F -x -q "directories=${BATCH_DIRS[*]}" "$done_file"
}

run_target_batches() {
    local batch_id
    local batch_index
    local batch_number
    local batch_size
    local batch_start
    local dirlist
    local done_file
    local directory_key
    local log_file
    local passed_batches=0
    local target="$1"
    local total_batches

    CURRENT_TARGET="$target"
    batch_size="${BATCH_SIZE_OVERRIDE:-$(default_batch_size "$target")}"
    total_batches=$(( (${#SELECTED_DIRS[@]} + batch_size - 1) / batch_size ))

    mkdir -p \
        "$OUTPUT_ROOT/$target/bin" \
        "$OUTPUT_ROOT/$target/dirlists" \
        "$OUTPUT_ROOT/$target/logs"

    if [ "$COMPILE_ONLY" -eq 0 ]; then
        build_log_relay "$target"
        build_resource_unpacker "$target"
        if [ "$target" = arm ]; then
            prepare_arm_base_image
        fi
    fi

    for ((batch_index = 0; batch_index < total_batches; batch_index++)); do
        batch_number=$((batch_index + 1))
        batch_id="$(printf 'batch%03d' "$batch_number")"
        batch_start=$((batch_index * batch_size))
        BATCH_DIRS=("${SELECTED_DIRS[@]:batch_start:batch_size}")
        directory_key="${BATCH_DIRS[*]}"
        directory_key="${directory_key// /+}"
        dirlist="$OUTPUT_ROOT/$target/dirlists/$batch_id.mk"
        done_file="$OUTPUT_ROOT/state/$target-$directory_key.done"
        log_file="$OUTPUT_ROOT/$target/logs/$batch_id.serial.log"

        if batch_is_complete "$batch_id" "$done_file"; then
            echo "==> skipping passed AROS $target $batch_id: ${BATCH_DIRS[*]}"
            passed_batches=$((passed_batches + 1))
            continue
        fi

        build_batch "$batch_id" "$dirlist" "$target"
        if [ "$COMPILE_ONLY" -eq 1 ]; then
            continue
        fi

        case "$target" in
            x86_64) run_x86_batch "$batch_id" "$log_file" ;;
            arm) run_arm_batch "$batch_id" "$log_file" ;;
            m68k) run_m68k_batch "$batch_id" "$log_file" ;;
        esac
        validate_batch_log "$batch_id" "$done_file" "$log_file"
        passed_batches=$((passed_batches + 1))
    done

    if [ "$COMPILE_ONLY" -eq 1 ]; then
        echo "==> cross-built $total_batches AROS $target fbcunit batches"
    else
        echo "==> AROS $target fbcunit passed: $passed_batches/$total_batches batches"
    fi
}

##############################################################################
# Main workflow
##############################################################################

prepare_resources

for target in "${SELECTED_TARGETS[@]}"; do
    map_target "$target"
    [ -x "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" ] ||
        die "AROS $target cross compiler not found in $MAP_TOOLCHAIN"
    refresh_target_libraries "$target"
done

for target in "${SELECTED_TARGETS[@]}"; do
    run_target_batches "$target"
done

echo
if [ "$COMPILE_ONLY" -eq 1 ]; then
    echo "==> selected AROS fbcunit batches cross-built for: ${SELECTED_TARGETS[*]}"
else
    echo "==> selected AROS fbcunit batches passed for: ${SELECTED_TARGETS[*]}"
    echo "    Logs: $OUTPUT_ROOT/{x86_64,m68k,arm}/logs"
fi

# end of aros-run-fbctests.sh
