#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE ARM emulator runner
# -------------------------------------------------
#
# File: build_scripts/wince/run-arm-emulator.sh
#
# Purpose:
#
#     Boot the pinned ARM Windows CE image in CERF, optionally launching one
#     executable from the guest-additions shared folder.
#
# Responsibilities:
#
#     - start CERF in the pinned Wine/Xvfb container
#     - wait for the Windows CE shell before interacting with it
#     - optionally open CERF's guest Task Manager and launch the requested
#       program
#     - hide the Task Manager as soon as the launch request is submitted
#     - preserve an emulator log and final screenshot
#     - stop Wine and Xvfb before the container exits
#
# This file intentionally does NOT contain:
#
#     - Windows CE ROM or CERF download logic
#     - FreeBASIC compilation rules
#     - guest test-result interpretation
#     - MIPS Windows CE emulator support
#
# Automation model:
#
#     CERF guest additions expose the host share as "\Storage Card".  Their
#     Task Manager starts a guest process without relying on a desktop shell
#     command line, which keeps this runner useful across Windows CE images
#     with different shell configurations.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

EMULATOR_IMAGE="${WINCE_EMULATOR_IMAGE:-freebasic-wince-emulator:noble}"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf}"
PROGRAM="wince-emulator-runner.exe"
PROGRAM_ARGUMENTS=""
COMPLETION_FILE=""
LOG_STEM="wm6-arm-program"
BOOT_SECONDS=24
RUN_SECONDS=12
BOOT_ONLY=0

##############################################################################
# Helpers
##############################################################################

die() {
    echo "ERROR: $*" >&2
    exit 1
}

require_value() {
    local option="$1"
    local value="${2-}"

    [ -n "$value" ] || die "$option requires a value"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/wince/run-arm-emulator.sh [options]

Options:
  --program NAME     Executable in CERF's share folder.
                     Default: wince-emulator-runner.exe
  --arguments TEXT   Command-line arguments passed to the guest executable.
  --completion NAME  Shared file whose appearance ends the run wait early.
  --log-stem NAME    Base name for the CERF log and screenshot.
                     Default: wm6-arm-program
  --boot-seconds N   Shell initialization wait. Default: 24
  --run-seconds N    Guest execution wait. Default: 12
  --boot-only        Capture the booted shell and exit without launching a
                     guest executable.
  -h, --help         Show this help.

The CERF tree defaults to out/wince/emulator/cerf. Override it with
WINCE_CERF_ROOT and override the container image with WINCE_EMULATOR_IMAGE.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --program)
            require_value "$1" "${2-}"
            PROGRAM="$2"
            shift 2
            ;;
        --arguments)
            require_value "$1" "${2-}"
            PROGRAM_ARGUMENTS="$2"
            shift 2
            ;;
        --completion)
            require_value "$1" "${2-}"
            COMPLETION_FILE="$2"
            shift 2
            ;;
        --log-stem)
            require_value "$1" "${2-}"
            LOG_STEM="$2"
            shift 2
            ;;
        --boot-seconds)
            require_value "$1" "${2-}"
            BOOT_SECONDS="$2"
            shift 2
            ;;
        --run-seconds)
            require_value "$1" "${2-}"
            RUN_SECONDS="$2"
            shift 2
            ;;
        --boot-only)
            BOOT_ONLY=1
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

case "$PROGRAM" in
    ""|*/*|*\\*|.|..)
        die "--program must be one file name from the CERF share folder"
        ;;
esac

case "$PROGRAM_ARGUMENTS" in
    *$'\n'*|*$'\r'*)
        die "--arguments must not contain line breaks"
        ;;
esac

case "$COMPLETION_FILE" in
    "")
        ;;
    */*|*\\*|.|..)
        die "--completion must be one file name from the CERF share folder"
        ;;
esac

case "$LOG_STEM" in
    ""|*[!A-Za-z0-9._-]*)
        die "--log-stem may contain only letters, digits, dot, underscore, and hyphen"
        ;;
esac

for seconds in "$BOOT_SECONDS" "$RUN_SECONDS"; do
    case "$seconds" in
        ""|*[!0-9]*|0)
            die "wait durations must be positive integers"
            ;;
    esac
done

##############################################################################
# Host-side validation
##############################################################################

command -v docker >/dev/null 2>&1 || die "docker is required"
[ -d "$CERF_ROOT" ] || die "CERF directory not found: $CERF_ROOT"
[ -f "$CERF_ROOT/cerf.exe" ] || die "CERF executable not found"
[ -f "$CERF_ROOT/roms-wince-arm.bin" ] || die "Windows CE ARM ROM not found"
[ -d "$CERF_ROOT/share" ] || die "CERF guest-additions share is not prepared"
if [ "$BOOT_ONLY" -eq 0 ]; then
	[ -f "$CERF_ROOT/share/$PROGRAM" ] ||
		die "guest executable not found: $CERF_ROOT/share/$PROGRAM"
fi

mkdir -p "$CERF_ROOT/logs"

##############################################################################
# Containerized emulator session
##############################################################################

docker run --rm -i \
    -v "$CERF_ROOT:/cerf" \
    -e DISPLAY=:99 \
    "$EMULATOR_IMAGE" \
    bash -s -- "$PROGRAM" "$LOG_STEM" "$BOOT_SECONDS" "$RUN_SECONDS" \
	"$PROGRAM_ARGUMENTS" "$COMPLETION_FILE" "$BOOT_ONLY" <<'CONTAINER_SCRIPT'
set -euo pipefail

program="$1"
log_stem="$2"
boot_seconds="$3"
run_seconds="$4"
program_arguments="$5"
completion_file="$6"
boot_only="$7"
xvfb_pid=""
wine_pid=""

cleanup() {
    local attempt

    if [ -n "$wine_pid" ] && kill -0 "$wine_pid" 2>/dev/null; then
        kill "$wine_pid" 2>/dev/null || true
        for attempt in 1 2 3 4 5; do
            if ! kill -0 "$wine_pid" 2>/dev/null; then
                break
            fi
            sleep 1
        done
        if kill -0 "$wine_pid" 2>/dev/null; then
            kill -KILL "$wine_pid" 2>/dev/null || true
        fi
        wait "$wine_pid" 2>/dev/null || true
    fi

    if [ -n "$xvfb_pid" ] && kill -0 "$xvfb_pid" 2>/dev/null; then
        kill "$xvfb_pid" 2>/dev/null || true
        wait "$xvfb_pid" 2>/dev/null || true
    fi
}

trap cleanup EXIT

Xvfb :99 -screen 0 1280x1024x24 >/tmp/wince-xvfb.log 2>&1 &
xvfb_pid="$!"
export WINEPREFIX=/wine

wine 'Z:\cerf\cerf.exe' \
    --board-id=devemu \
    '--rom-primary=Z:\cerf\roms-wince-arm.bin' \
    --guest-additions \
    '--share-folder=Z:\cerf\share' \
    --boot=cold \
    --log=CERF,CAUTION,NKDBG,GUEST,GUEST_HOST \
    "--log-file=Z:\cerf\logs\\${log_stem}.log" \
    >/tmp/wince-wine.log 2>&1 &
wine_pid="$!"

main_window=""
for attempt in $(seq 1 45); do
    main_window="$(xdotool search --name 'CERF' 2>/dev/null | tail -n 1 || true)"
    if [ -n "$main_window" ]; then
        break
    fi
    sleep 1
done

if [ -z "$main_window" ]; then
    cat /tmp/wince-wine.log >&2
    echo "ERROR: CERF window did not appear" >&2
    exit 1
fi

sleep "$boot_seconds"

# A boot-only run deliberately stops here.  It is useful for validating a ROM
# and board pairing without allowing a guest executable to affect the result.
if [ "$boot_only" -eq 1 ]; then
    import -window "$main_window" "/cerf/logs/${log_stem}.png"
    exit 0
fi

# Open Actions, hover Guest Additions, and select Task Manager.
xdotool mousemove --window "$main_window" 27 11 click 1 2>/dev/null
sleep 1
xdotool mousemove --window "$main_window" 150 150 2>/dev/null
sleep 1
xdotool mousemove --window "$main_window" 386 176 click 1 2>/dev/null
sleep 3

task_window="$(xdotool search --name 'Task Manager' 2>/dev/null | tail -n 1 || true)"
if [ -z "$task_window" ]; then
    echo "ERROR: CERF guest Task Manager did not appear" >&2
    exit 1
fi

task_height="$(xdotool getwindowgeometry --shell "$task_window" |
    sed -n 's/^HEIGHT=//p')"
case "$task_height" in
    ""|*[!0-9]*)
        echo "ERROR: unable to read Task Manager geometry" >&2
        exit 1
        ;;
esac

# The Run edit control occupies the bottom row of the guest Task Manager.
run_y=$((task_height - 31))
guest_command="\\Storage Card\\${program}"
if [ -n "$program_arguments" ]; then
    guest_command="$guest_command $program_arguments"
fi
xdotool mousemove --window "$task_window" 160 "$run_y" click 1 2>/dev/null
xdotool key --window "$task_window" ctrl+a 2>/dev/null
xdotool type --window "$task_window" --delay 15 \
    "$guest_command" 2>/dev/null
xdotool key --window "$task_window" Return 2>/dev/null
sleep 1

# The host-side Task Manager is only a launch bridge.  Leaving it mapped
# obscures the guest display and can flash through framebuffer updates.
xdotool windowunmap "$task_window" 2>/dev/null
xdotool windowactivate "$main_window" 2>/dev/null || true

if [ -n "$completion_file" ]; then
    for attempt in $(seq 1 "$run_seconds"); do
        if [ -f "/cerf/share/$completion_file" ]; then
            break
        fi
        sleep 1
    done

	if [ ! -f "/cerf/share/$completion_file" ]; then
		import -window "$main_window" "/cerf/logs/${log_stem}.png" || true
		if [ -f "/cerf/share/oma.trace" ]; then
			echo "INFO: guest checkpoint: $(tr -d '\r\n' < /cerf/share/oma.trace)" >&2
		fi
		echo "ERROR: guest completion file did not appear: $completion_file" >&2
		exit 1
	fi
else
    sleep "$run_seconds"
fi
import -window "$main_window" "/cerf/logs/${log_stem}.png"
CONTAINER_SCRIPT

# end of build_scripts/wince/run-arm-emulator.sh
