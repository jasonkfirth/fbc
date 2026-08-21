#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS media acceptance workflow
# ----------------------------------------------------
#
# File: riscos-run-media-smoke.sh
#
# Purpose:
#
#     Cross-build and run the native gfxlib2 and sfxlib acceptance program.
#
# Responsibilities:
#
#     - build the combined graphics and sound program with RISC OS libraries
#     - synchronize its program and runtime modules into RPCEmu HostFS
#     - launch the program so its default Wimp graphics mode can register
#     - preserve its console log and verify deterministic success markers
#
# This file intentionally does NOT contain:
#
#     - graphics, input, or sound implementation code
#     - RPCEmu or GCCSDK construction logic
#     - subjective display or audio-quality assessment
#
# Runtime ownership:
#
#     The RunMedia boot task, media-smoke log, and generated program below the
#     selected RPCEmu runtime are replaced. Other Choices and HostFS content
#     are preserved.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RPCEMU_WORKDIR="${RISCOS_RPCEMU_WORKDIR:-$ROOT/out/riscos/rpcemu}"
HOSTFS_ROOT="${RISCOS_HOSTFS_ROOT:-$ROOT/out/riscos/hostfs}"
OUTPUT_ROOT="${RISCOS_MEDIA_OUTDIR:-$ROOT/out/riscos/media-smoke}"

TIMEOUT_SECONDS=120
RPCEMU_MEMORY=256
JOBS=""

RPCEMU_PID=""
BOOT_TASK=""

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
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-run-media-smoke.sh [options]

Options:
  --timeout SEC     Emulator timeout. Default: 120
  --memory MIB      RPCEmu memory. Default: 256; maximum: 256
  --jobs N          Parallel cross-build jobs. Default: detected CPU count
  -h, --help        Show this help.

The saved log is out/riscos/media-smoke/media-smoke.log by default.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
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

for numeric_value in "$TIMEOUT_SECONDS" "$JOBS"; do
    case "$numeric_value" in
        ''|*[!0-9]*|0) die "timeout and jobs must be positive integers" ;;
    esac
done

case "$RPCEMU_MEMORY" in
    4|8|16|32|64|128|256) ;;
    *) die "--memory must be one of: 4, 8, 16, 32, 64, 128, 256" ;;
esac

PROGRAM_SLOT_MIB=$((RPCEMU_MEMORY * 3 / 4))
PROGRAM_SLOT_KIB=$((PROGRAM_SLOT_MIB * 1024))

##############################################################################
# Paths and prerequisites
##############################################################################

for tool in grep pgrep strings; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

RPCEMU_SOURCE="$RPCEMU_WORKDIR/source"
RPCEMU_BINARY="$RPCEMU_SOURCE/rpcemu-recompiler"
RUNTIME_HOSTFS="$RPCEMU_SOURCE/hostfs"
RUNTIME_LOGS="$RUNTIME_HOSTFS/FreeBASIC/media-smoke"
RUNTIME_PROGRAM="$RUNTIME_HOSTFS/FreeBASIC/fbmedia,ff8"
BOOT_TASK="$RUNTIME_HOSTFS/!Boot/Choices/Boot/Tasks/RunMedia,feb"
SAVED_LOG="$OUTPUT_ROOT/media-smoke.log"
SAVED_GFX_LOG="$OUTPUT_ROOT/gfx-debug.log"
SAVED_SFX_LOG="$OUTPUT_ROOT/sfx-debug.log"

if pgrep -f '(^|/)rpcemu-(recompiler|interpreter)( |$)' >/dev/null 2>&1; then
    die "an RPCEmu process is already running; close it before media smoke"
fi

mkdir -p "$OUTPUT_ROOT"

##############################################################################
# Build and runtime synchronization
##############################################################################

msg "cross-building the RISC OS media acceptance program"
"$SCRIPT_DIR/riscos-build-smoke.sh" \
    --source examples/riscos/media-smoke.bas \
    --name fbmedia \
    --with-libs \
    --jobs "$JOBS"

msg "synchronizing the RPCEmu runtime"
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
[ -s "$RUNTIME_PROGRAM" ] ||
    die "media acceptance program was not synchronized: $RUNTIME_PROGRAM"

##############################################################################
# Guest execution
##############################################################################

mkdir -p "$(dirname "$BOOT_TASK")" "$RUNTIME_LOGS"
rm -f -- \
    "$RUNTIME_LOGS/log,ffd" \
    "$RUNTIME_LOGS/log" \
    "$RUNTIME_LOGS/gfx-debug" \
    "$RUNTIME_LOGS/sfx-debug" \
    "$SAVED_LOG" \
    "$SAVED_GFX_LOG" \
    "$SAVED_SFX_LOG"

# Reproduce the physical layout used for a suffix-swapped file below a real
# parent directory that happens to have the same name as the suffix.
mkdir -p "$RUNTIME_HOSTFS/FreeBASIC/dir-smoke/bmp/bmp"
cp "$ROOT/examples/riscos/hello.bas" \
    "$RUNTIME_HOSTFS/FreeBASIC/dir-smoke/bmp/bmp/UPPER"

cat > "$RUNTIME_HOSTFS/FreeBASIC/RunMedia,feb" <<EOF
| FreeBASIC RISC OS native media acceptance task
| ------------------------------------------------
|
| File: RunMedia
|
| Purpose:
|
|     Run the native gfxlib2 and sfxlib acceptance program.
|
| Responsibilities:
|
|     - capture program output and its return code
|     - emit a deterministic completion marker
|
| The program registers its own Wimp task when it selects windowed graphics.

Set Sys\$RCLimit 2147483647
Set UnixEnv\$fbmedia\$sfix "BMP:bmp"
Set UnixEnv\$GFXLIB_DEBUG 1
Set UnixEnv\$GFXLIB_DEBUG_LOG media-smoke/gfx-debug
Set UnixEnv\$SFXLIB_DEBUG 1
Set UnixEnv\$SFXLIB_DEBUG_LOG media-smoke/sfx-debug
Spool HostFS:\$.FreeBASIC.media-smoke.log
Echo Starting FreeBASIC native media smoke.
Dir HostFS:\$.FreeBASIC
Run HostFS:\$.FreeBASIC.fbmedia
Set FreeBASIC\$MediaStatus <Sys\$ReturnCode>
Echo FreeBASIC media smoke return code: <FreeBASIC\$MediaStatus>
Echo FB_RISCOS_MEDIA_DONE
Spool

| end of RunMedia
EOF

cat > "$BOOT_TASK" <<EOF
| FreeBASIC RISC OS native media launch task
| -------------------------------------------
|
| File: RunMedia
|
| Purpose:
|
|     Launch the media acceptance program for native Wimp and fullscreen tests.
|
| Responsibilities:
|
|     - reserve a $PROGRAM_SLOT_MIB MiB application slot
|     - leave Wimp mode selection and window ownership to gfxlib2
|     - leave execution and completion logging to the program task
|
| This generated task intentionally does NOT wait for program completion.

WimpSlot -min ${PROGRAM_SLOT_KIB}K
Obey HostFS:\$.FreeBASIC.RunMedia

| end of RunMedia
EOF

msg "running the native media acceptance program under RISC OS"
(
    cd "$RPCEMU_SOURCE"
    exec "$RPCEMU_BINARY"
) &
RPCEMU_PID="$!"

elapsed=0
runtime_log=""
while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
    if [ -f "$RUNTIME_LOGS/log,ffd" ]; then
        runtime_log="$RUNTIME_LOGS/log,ffd"
    elif [ -f "$RUNTIME_LOGS/log" ]; then
        runtime_log="$RUNTIME_LOGS/log"
    fi

    if [ -n "$runtime_log" ] &&
       grep -a -q 'FB_RISCOS_MEDIA_DONE' "$runtime_log"; then
        break
    fi

    if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
        die "RPCEmu exited before media smoke completed"
    fi

    sleep 2
    elapsed=$((elapsed + 2))
done

if [ -z "$runtime_log" ] ||
   ! grep -a -q 'FB_RISCOS_MEDIA_DONE' "$runtime_log"; then
    die "media smoke timed out after $TIMEOUT_SECONDS seconds"
fi

stop_emulator
[ -s "$RUNTIME_LOGS/gfx-debug" ] ||
    die "gfxlib2 did not create its file-only diagnostic log"
[ -s "$RUNTIME_LOGS/sfx-debug" ] ||
    die "sfxlib did not create its file-only diagnostic log"
strings -a "$runtime_log" > "$SAVED_LOG"
strings -a "$RUNTIME_LOGS/gfx-debug" > "$SAVED_GFX_LOG"
strings -a "$RUNTIME_LOGS/sfx-debug" > "$SAVED_SFX_LOG"
cat "$SAVED_LOG"

grep -q '^FreeBASIC media smoke return code: 0$' "$SAVED_LOG" ||
    die "media smoke returned an error; see $SAVED_LOG"
grep -q '^FB_RISCOS_GFX_DRIVER=RISC OS$' "$SAVED_LOG" ||
    die "native gfxlib2 marker missing; see $SAVED_LOG"
grep -q '^FB_RISCOS_GFX_WINDOWED_OK$' "$SAVED_LOG" ||
    die "native Wimp window marker missing; see $SAVED_LOG"
grep -q '^FB_RISCOS_GFX_FULLSCREEN_OK$' "$SAVED_LOG" ||
    die "native fullscreen marker missing; see $SAVED_LOG"
grep -q '^FB_RISCOS_SFX_DRIVER=RISC OS DigitalRenderer$' "$SAVED_LOG" ||
    die "native sfxlib marker missing; see $SAVED_LOG"
grep -q '^FB_RISCOS_MEDIA_SMOKE_OK$' "$SAVED_LOG" ||
    die "media acceptance marker missing; see $SAVED_LOG"
grep -q '^gfxlib2: RISC OS: initializing ' "$SAVED_GFX_LOG" ||
    die "gfxlib2 diagnostic file is incomplete; see $SAVED_GFX_LOG"
grep -q '^SFX: sfx_core: initialization complete$' "$SAVED_SFX_LOG" ||
    die "sfxlib diagnostic file is incomplete; see $SAVED_SFX_LOG"

echo
echo "==> RISC OS native gfxlib2 and sfxlib acceptance passed"
echo "    Log: $SAVED_LOG"
echo "    Graphics diagnostics: $SAVED_GFX_LOG"
echo "    Sound diagnostics:    $SAVED_SFX_LOG"

# end of riscos-run-media-smoke.sh
