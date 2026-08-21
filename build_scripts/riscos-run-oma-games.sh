#!/usr/bin/env bash
#
# Project: FreeBASIC OMA game acceptance on RISC OS
# -------------------------------------------------
#
# File: riscos-run-oma-games.sh
#
# Purpose:
#
#     Launch every built OMA game under RISC OS and record evidence that its
#     interactive graphics process initializes and remains alive.
#
# Responsibilities:
#
#     - call the shared OMA build and packaging workflow unless asked not to
#     - synchronize the current HostFS tree into RPCEmu
#     - launch each game as a foreground non-Wimp process
#     - keep gfxlib2 and sfxlib diagnostics off the graphics display
#     - qualify foreground process liveness from launch and return markers
#     - preserve one RPCEmu screenshot and text log per game
#     - write TSV and Markdown result reports
#
# This file intentionally does NOT contain:
#
#     - game compilation or asset selection rules
#     - scripted gameplay input sequences
#     - visual reference-image comparison
#     - emulator, GCCSDK, or FreeBASIC construction logic
#
# Test contract:
#
#     These games are interactive and are not expected to terminate. A game
#     passes when its foreground Run command does not return during the
#     stability interval. Padding makes both the pre-Run and unexpected-return
#     markers cross RISC OS Spool's block boundary. Foreground execution
#     prevents Wimp or TaskWindow redraws from competing with gfxlib2's direct
#     ownership of the physical framebuffer. Backend diagnostics remain off
#     because terminal output itself alters the active graphics display.
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
OUTPUT_ROOT="${RISCOS_OMA_OUTDIR:-$ROOT/out/riscos/oma}"

TIMEOUT_SECONDS=120
STABILITY_SECONDS=8
RPCEMU_MEMORY=256
SKIP_BUILD=0
JOBS=""
GAME_FILTER=""

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

find_runtime_log() {
    local key="$1"
    local log_root="$2"

    if [ -f "$log_root/$key,ffd" ]; then
        printf '%s\n' "$log_root/$key,ffd"
    elif [ -f "$log_root/$key" ]; then
        printf '%s\n' "$log_root/$key"
    fi
}

capture_rpcemu() {
    local destination="$1"
    local window_id=""

    window_id="$(xdotool search --name '^RPCEmu' 2>/dev/null | tail -n 1 || true)"
    [ -n "$window_id" ] || return 1

    import -window "$window_id" "$destination"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-run-oma-games.sh [options]

Options:
  --timeout SEC     Per-game emulator timeout. Default: 120
  --stability SEC   Seconds a graphics process must remain active. Default: 8
  --memory MIB      RPCEmu memory. Default: 256; maximum: 256
  --skip-build      Reuse the current OMA manifest and HostFS applications.
  --game KEY        Test only the manifest entry named KEY.
  --jobs N          Parallel host build jobs. Default: detected CPU count
  -h, --help        Show this help.

Logs, screenshots, results.tsv, and report.md are written below
out/riscos/oma/runtime by default.
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
        --stability)
            require_value "$1" "${2-}"
            STABILITY_SECONDS="$2"
            shift 2
            ;;
        --memory)
            require_value "$1" "${2-}"
            RPCEMU_MEMORY="$2"
            shift 2
            ;;
        --skip-build)
            SKIP_BUILD=1
            shift
            ;;
        --game)
            require_value "$1" "${2-}"
            GAME_FILTER="$2"
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

for numeric_value in "$TIMEOUT_SECONDS" "$STABILITY_SECONDS" "$JOBS"; do
    case "$numeric_value" in
        ''|*[!0-9]*|0)
            die "timeout, stability, and jobs must be positive integers"
            ;;
    esac
done

case "$RPCEMU_MEMORY" in
    4|8|16|32|64|128|256) ;;
    *) die "--memory must be one of: 4, 8, 16, 32, 64, 128, 256" ;;
esac

GAME_SLOT_MIB=$((RPCEMU_MEMORY * 3 / 4))
GAME_SLOT_KIB=$((GAME_SLOT_MIB * 1024))

##############################################################################
# Build and runtime synchronization
##############################################################################

for tool in grep import pgrep strings xdotool; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

if pgrep -f '(^|/)rpcemu-(recompiler|interpreter)( |$)' >/dev/null 2>&1; then
    die "an RPCEmu process is already running; close it before OMA tests"
fi

if [ "$SKIP_BUILD" -eq 0 ]; then
    msg "building and packaging the available OMA games"
    "$SCRIPT_DIR/riscos-build-oma-games.sh" --jobs "$JOBS"
fi

MANIFEST="$OUTPUT_ROOT/manifest.tsv"
require_manifest_header=$'key\tdisplay\tkind\tstatus\tsource\tapp\tsuffixes\tpackage\tnote'
[ -f "$MANIFEST" ] || die "OMA build manifest not found: $MANIFEST"
[ "$(head -n 1 "$MANIFEST")" = "$require_manifest_header" ] ||
    die "OMA build manifest has an unexpected schema: $MANIFEST"

msg "synchronizing the RPCEmu runtime"
"$SCRIPT_DIR/riscos-rpcemu.sh" \
    --workdir "$RPCEMU_WORKDIR" \
    --hostfs "$HOSTFS_ROOT" \
    --memory "$RPCEMU_MEMORY" \
    --jobs "$JOBS"

RPCEMU_SOURCE="$RPCEMU_WORKDIR/source"
RPCEMU_BINARY="$RPCEMU_SOURCE/rpcemu-recompiler"
RUNTIME_HOSTFS="$RPCEMU_SOURCE/hostfs"
RUNTIME_GAMES="$RUNTIME_HOSTFS/OMAGames"
RUNTIME_LOGS="$RUNTIME_GAMES/logs"
BOOT_TASK="$RUNTIME_HOSTFS/!Boot/Choices/Boot/Tasks/RunOMA,feb"
RESULT_ROOT="$OUTPUT_ROOT/runtime"
RESULT_TSV="$RESULT_ROOT/results.tsv"
REPORT="$RESULT_ROOT/report.md"

[ -x "$RPCEMU_BINARY" ] || die "RPCEmu binary not found: $RPCEMU_BINARY"
[ -f "$RPCEMU_SOURCE/cmos.ram" ] ||
    die "RPCEmu CMOS state not found; launch RPCEmu once to configure RISC OS"
[ -f "$RUNTIME_HOSTFS/!Boot/Choices/Boot/Tasks/PinSetup,feb" ] ||
    die "RISC OS Choices are incomplete; launch RPCEmu once and finish setup"

mkdir -p "$RUNTIME_LOGS" "$RESULT_ROOT/logs" "$RESULT_ROOT/screenshots"
printf '%s\n' $'key\tdisplay\tstatus\tdetail\tlog\tscreenshot' > "$RESULT_TSV"

##############################################################################
# Per-game guest execution
##############################################################################

run_game() {
    local key="$1"
    local display_name="$2"
    local app_name="$3"
    local suffixes="$4"
    local game_root="$RUNTIME_GAMES/$app_name"
    local run_task="$game_root/RunTest,feb"
    local runtime_log=""
    local saved_log="$RESULT_ROOT/logs/$key.log"
    local screenshot="$RESULT_ROOT/screenshots/$key.png"
    local elapsed=0
    local started_at=-1
    local status="fail"
    local detail=""

    [ -s "$game_root/game,ff8" ] ||
        die "staged game AIF not found for $display_name: $game_root/game,ff8"

    rm -f -- "$RUNTIME_LOGS/$key,ffd" "$RUNTIME_LOGS/$key" \
        "$saved_log" "$screenshot"

    cat > "$run_task" <<EOF
| $display_name RISC OS acceptance task
| --------------------------------------
|
| File: RunTest
|
| Purpose:
|
|     Launch $display_name without writing diagnostics over its display.
|
| Responsibilities:
|
|     - expose this application's UnixLib suffix directories
|     - capture launch evidence and an unexpected return code
|     - cross a Spool buffer boundary before entering the interactive process
|
| This generated task intentionally does NOT stop an interactive game.

Set Sys\$RCLimit 2147483647
Set UnixEnv\$game\$sfix "$suffixes"
Unset UnixEnv\$GFXLIB_DEBUG
Unset UnixEnv\$SFXLIB_DEBUG
Spool HostFS:\$.OMAGames.logs.$key
Echo OMA_GAME_BEGIN_$key
Echo OMA_LOG_PAD_1_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_LOG_PAD_2_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_LOG_PAD_3_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_LOG_PAD_4_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_LOG_PAD_5_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Dir HostFS:\$.OMAGames.$app_name
Run game
Set OMA\$GameStatus <Sys\$ReturnCode>
Echo OMA_GAME_RETURN_$key <OMA\$GameStatus>
Echo OMA_RETURN_PAD_1_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_RETURN_PAD_2_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_RETURN_PAD_3_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_RETURN_PAD_4_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo OMA_RETURN_PAD_5_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Spool

| end of RunTest
EOF

    mkdir -p "$(dirname "$BOOT_TASK")"
    cat > "$BOOT_TASK" <<EOF
| $display_name RISC OS acceptance launch task
| ---------------------------------------------
|
| File: RunOMA
|
| Purpose:
|
|     Start one OMA game as a foreground non-Wimp process.
|
| Responsibilities:
|
|     - reserve a $GAME_SLOT_MIB MiB application slot
|     - prevent desktop redraws while gfxlib2 owns the framebuffer
|     - leave launch evidence collection to the per-game task
|
| This generated task intentionally does NOT run more than one game.

WimpSlot -min ${GAME_SLOT_KIB}K
Obey HostFS:\$.OMAGames.$app_name.RunTest

| end of RunOMA
EOF

    msg "testing $display_name under RISC OS"
    (
        cd "$RPCEMU_SOURCE"
        exec "$RPCEMU_BINARY"
    ) &
    RPCEMU_PID="$!"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        runtime_log="$(find_runtime_log "$key" "$RUNTIME_LOGS")"

        if [ -n "$runtime_log" ]; then
            if grep -a -q "OMA_GAME_RETURN_$key" "$runtime_log"; then
                detail="game returned before the stability interval"
                break
            fi

            # The begin marker and its padding are written before the
            # foreground Run command. If the game returns, the return marker
            # and its padding become visible. Otherwise the blocked command
            # is the liveness evidence, without enabling diagnostics that
            # would write through the VDU over the framebuffer.
            if grep -a -q "OMA_GAME_BEGIN_$key" "$runtime_log"; then
                if [ "$started_at" -lt 0 ]; then
                    started_at="$elapsed"
                fi

                if [ $((elapsed - started_at)) -ge "$STABILITY_SECONDS" ]; then
                    status="pass"
                    detail="foreground game remained active for ${STABILITY_SECONDS}s; display diagnostics disabled"
                    break
                fi
            fi
        fi

        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            detail="RPCEmu exited before the game qualified"
            break
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    if [ "$status" != "pass" ] && [ -z "$detail" ]; then
        detail="timed out after ${TIMEOUT_SECONDS}s without a launch marker"
    fi

    capture_rpcemu "$screenshot" || screenshot=""
    stop_emulator

    if [ -n "$runtime_log" ] && [ -f "$runtime_log" ]; then
        strings -a "$runtime_log" > "$saved_log"
    else
        : > "$saved_log"
    fi

    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$key" "$display_name" "$status" "$detail" \
        "logs/$key.log" \
        "${screenshot#"$RESULT_ROOT"/}" >> "$RESULT_TSV"

    if [ "$status" = "pass" ]; then
        echo "    PASS: $detail"
    else
        echo "    FAIL: $detail" >&2
    fi
}

while IFS=$'\t' read -r key display_name kind status source app_name suffixes \
    package note; do
    [ "$key" != "key" ] || continue
    [ "$status" = "built" ] || continue
    if [ -n "$GAME_FILTER" ] && [ "$key" != "$GAME_FILTER" ]; then
        continue
    fi
    run_game "$key" "$display_name" "$app_name" "$suffixes"
done < "$MANIFEST"

rm -f -- "$BOOT_TASK"
BOOT_TASK=""

##############################################################################
# Result reporting
##############################################################################

pass_count="$(awk -F '\t' 'NR > 1 && $3 == "pass" { count++ } END { print count + 0 }' "$RESULT_TSV")"
fail_count="$(awk -F '\t' 'NR > 1 && $3 == "fail" { count++ } END { print count + 0 }' "$RESULT_TSV")"
missing_count="$(awk -F '\t' 'NR > 1 && $4 == "missing" { count++ } END { print count + 0 }' "$MANIFEST")"

if [ -n "$GAME_FILTER" ] && [ $((pass_count + fail_count)) -eq 0 ]; then
    die "no built OMA manifest entry matched --game $GAME_FILTER"
fi

{
    printf '%s\n' \
        '# RISC OS OMA game acceptance' \
        '' \
        "- Passed: $pass_count" \
        "- Failed: $fail_count" \
        "- Missing source trees: $missing_count" \
        "- RPCEmu memory: ${RPCEMU_MEMORY} MiB" \
        "- Stability interval: ${STABILITY_SECONDS} seconds" \
        '' \
        '| Game | Result | Evidence |' \
        '| --- | --- | --- |'

    while IFS=$'\t' read -r key display_name status detail log screenshot; do
        [ "$key" != "key" ] || continue
        printf '| %s | %s | %s |\n' "$display_name" "$status" "$detail"
    done < "$RESULT_TSV"

    printf '%s\n' \
        '' \
        'Missing sources are listed in `../manifest.tsv` and are not reported' \
        'as runtime failures because no program could be built.'
} > "$REPORT"

echo
echo "==> RISC OS OMA game acceptance completed"
echo "    Passed: $pass_count"
echo "    Failed: $fail_count"
echo "    Report: $REPORT"

[ "$fail_count" -eq 0 ] || exit 1

# end of riscos-run-oma-games.sh
