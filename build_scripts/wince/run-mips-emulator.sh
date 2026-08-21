#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE MIPS emulator runner
# --------------------------------------------------
#
# File: build_scripts/wince/run-mips-emulator.sh
#
# Purpose:
#
#     Boot a user-supplied MIPS Windows CE image in CERF, optionally launching
#     one executable from the guest-additions shared folder.
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
#     - ARM Windows CE emulator support
#
# Automation model:
#
#     CERF guest additions expose the host share as "\Storage Card".  Their
#     Task Manager starts a guest process without relying on a desktop shell
#     command line, which keeps this runner useful across Windows CE images
#     with different shell configurations. If CERF does not expose the host Task
#     Manager window, an optional keyboard fallback sends a direct launch command
#     to the guest framebuffer.
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
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf-mips}"
BOARD_ID="${WINCE_MIPS_BOARD_ID:-casio_cassiopeia_em500}"
ROM_PATH="${WINCE_MIPS_ROM:-$CERF_ROOT/roms-wince-mips.bin}"
PROGRAM="wince-emulator-runner.exe"
PROGRAM_ARGUMENTS=""
COMPLETION_FILE=""
LOG_STEM="em500-mips-program"
BOOT_SECONDS=30
RUN_SECONDS=12
BOOT_ONLY=0
TASK_MANAGER_PATTERN="Task Manager"
TASK_MANAGER_FALLBACK=1

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
Usage: ./build_scripts/wince/run-mips-emulator.sh [options]

Options:
  --board-id ID     CERF board id. Default: casio_cassiopeia_em500
  --rom PATH        MIPS ROM below the CERF tree.
                    Default: roms-wince-mips.bin
  --program NAME     Executable in CERF's share folder.
                     Default: wince-emulator-runner.exe
  --arguments TEXT   Command-line arguments passed to the guest executable.
  --completion NAME  Shared file whose appearance ends the run wait early.
  --log-stem NAME    Base name for the CERF log and screenshot.
                     Default: em500-mips-program
  --boot-seconds N   Shell initialization wait. Default: 30
  --run-seconds N    Guest execution wait. Default: 12
  --boot-only        Capture the booted shell and exit without launching a
                     guest executable.
  --task-manager TEXT
                     Window name/regex used to locate CERF Task Manager.
                     Default: Task Manager
  --no-fallback      Disable direct keyboard fallback when Task Manager is absent.
  -h, --help         Show this help.

The CERF tree defaults to out/wince/emulator/cerf-mips. Override it with
WINCE_CERF_ROOT, WINCE_MIPS_ROM, WINCE_MIPS_BOARD_ID, and
WINCE_EMULATOR_IMAGE. The ROM remains user-supplied and is not redistributed.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --board-id)
            require_value "$1" "${2-}"
            BOARD_ID="$2"
            shift 2
            ;;
        --rom)
            require_value "$1" "${2-}"
            ROM_PATH="$2"
            shift 2
            ;;
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
        --task-manager)
            require_value "$1" "${2-}"
            TASK_MANAGER_PATTERN="$2"
            shift 2
            ;;
        --no-fallback)
            TASK_MANAGER_FALLBACK=0
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

case "$BOARD_ID" in
    ""|*[!A-Za-z0-9_-]*)
        die "--board-id may contain only letters, digits, underscore, and hyphen"
        ;;
esac

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

case "$TASK_MANAGER_PATTERN" in
    ""|*$'\n'*|*$'\r'*)
        die "--task-manager must be one non-empty text pattern"
        ;;
esac

case "$TASK_MANAGER_FALLBACK" in
    0|1) ;;
    *)
        die "--no-fallback is a boolean flag; it does not take a value"
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
command -v realpath >/dev/null 2>&1 || die "realpath is required"
[ -d "$CERF_ROOT" ] || die "CERF directory not found: $CERF_ROOT"
[ -f "$CERF_ROOT/cerf.exe" ] || die "CERF executable not found"
[ -d "$CERF_ROOT/share" ] || die "CERF guest-additions share is not prepared"
if [ "$BOOT_ONLY" -eq 0 ]; then
    [ -f "$CERF_ROOT/share/$PROGRAM" ] ||
        die "guest executable not found: $CERF_ROOT/share/$PROGRAM"
fi

CERF_REAL="$(realpath -m "$CERF_ROOT")"
ROM_REAL="$(realpath -m "$ROM_PATH")"
case "$ROM_REAL" in
    "$CERF_REAL"/*) ;;
    *) die "the MIPS ROM must be stored below the selected CERF tree" ;;
esac
[ -f "$ROM_REAL" ] || die "Windows CE MIPS ROM not found: $ROM_REAL"
ROM_RELATIVE="${ROM_REAL#"$CERF_REAL"/}"

mkdir -p "$CERF_ROOT/logs"

##############################################################################
# Containerized emulator session
##############################################################################

docker run --rm -i \
    -v "$CERF_ROOT:/cerf" \
    -e DISPLAY=:99 \
    "$EMULATOR_IMAGE" \
    bash -s -- "$PROGRAM" "$LOG_STEM" "$BOOT_SECONDS" "$RUN_SECONDS" \
        "$PROGRAM_ARGUMENTS" "$COMPLETION_FILE" "$BOARD_ID" \
        "$ROM_RELATIVE" "$BOOT_ONLY" "$TASK_MANAGER_PATTERN" \
        "$TASK_MANAGER_FALLBACK" <<'CONTAINER_SCRIPT'
set -euo pipefail

program="$1"
log_stem="$2"
boot_seconds="$3"
run_seconds="$4"
program_arguments="$5"
completion_file="$6"
board_id="$7"
rom_relative="$8"
boot_only="$9"
task_manager_name="${10}"
task_manager_fallback="${11}"
xvfb_pid=""
wine_pid=""

dump_visible_windows() {
    local marker="$1"

    echo "INFO: $marker (host-visible X11 windows)"
    while IFS= read -r window_id; do
        [ -z "$window_id" ] && continue
        name="$(xdotool getwindowname "$window_id" 2>/dev/null || printf '<unknown>')"
        geometry="$(xdotool getwindowgeometry --shell "$window_id" 2>/dev/null || true)"
        [ -n "$geometry" ] || continue
        x="$(printf '%s\n' "$geometry" | sed -n 's/^X=//p')"
        y="$(printf '%s\n' "$geometry" | sed -n 's/^Y=//p')"
        width="$(printf '%s\n' "$geometry" | sed -n 's/^WIDTH=//p')"
        height="$(printf '%s\n' "$geometry" | sed -n 's/^HEIGHT=//p')"
        printf '  %s | %s | %sx%s+%s+%s\n' \
            "$window_id" "$name" "$width" "$height" "$x" "$y"
    done < <(xdotool search --onlyvisible --name "." 2>/dev/null || true)
}

launch_via_task_manager() {
    local task_window=""
    local main_geometry
    local main_x
    local main_y

    xdotool windowactivate "$main_window" 2>/dev/null || true
    xdotool windowraise "$main_window" 2>/dev/null || true

    main_geometry="$(xdotool getwindowgeometry --shell "$main_window" 2>/dev/null || true)"
    main_x="$(printf '%s\n' "$main_geometry" | sed -n 's/^X=//p')"
    main_y="$(printf '%s\n' "$main_geometry" | sed -n 's/^Y=//p')"
    [ -n "$main_x" ] || return 1
    [ -n "$main_y" ] || return 1

    #
    # CERF 6.7 exposes Task Manager through Actions, Guest Additions.  Wine's
    # native menu is outside the client coordinate space, so these measured
    # offsets must be applied to the absolute X11 window origin.
    #
    xdotool mousemove $((main_x + 27)) $((main_y + 11)) click 1 2>/dev/null
    sleep 1
    xdotool mousemove $((main_x + 80)) $((main_y + 150)) 2>/dev/null
    sleep 1
    xdotool mousemove $((main_x + 305)) $((main_y + 176)) click 1 2>/dev/null
    sleep 3

    for attempt in $(seq 1 12); do
        task_window="$(xdotool search --name "$task_manager_name" \
            2>/dev/null | tail -n 1 || true)"
        if [ -n "$task_window" ]; then
            return 0
        fi
        sleep 1
    done

    return 1
}

launch_via_keyboard_fallback() {
    # Keep this conservative: focus the guest frame, place a caret if possible,
    # and send the command as a direct keystroke burst.
    xdotool windowactivate "$main_window" 2>/dev/null || true
    xdotool mousemove --window "$main_window" 200 200 click 1 2>/dev/null
    sleep 1
    xdotool key --window "$main_window" ctrl+l 2>/dev/null || true
    xdotool key --window "$main_window" ctrl+a 2>/dev/null || true
    xdotool type --window "$main_window" --delay 15 "$guest_command" 2>/dev/null
    xdotool key --window "$main_window" Return 2>/dev/null
    sleep 1
    if [ -n "$completion_file" ] && [ -f "/cerf/share/$completion_file" ]; then
        return 0
    fi

    # Alternate launch path that can work with older shell shortcuts.
    xdotool key --window "$main_window" ctrl+esc 2>/dev/null || true
    sleep 1
    xdotool type --window "$main_window" --delay 15 "$guest_command" 2>/dev/null
    xdotool key --window "$main_window" Return 2>/dev/null
    return 0
}

complete_rockhopper_setup() {
    local main_geometry
    local main_height
    local main_width
    local main_x
    local main_y
    local point
    local warning_window

    [ "$board_id" = "nec_rockhopper" ] || return 0

    main_geometry="$(xdotool getwindowgeometry --shell "$main_window" 2>/dev/null || true)"
    main_width="$(printf '%s\n' "$main_geometry" | sed -n 's/^WIDTH=//p')"
    main_height="$(printf '%s\n' "$main_geometry" | sed -n 's/^HEIGHT=//p')"
    main_x="$(printf '%s\n' "$main_geometry" | sed -n 's/^X=//p')"
    main_y="$(printf '%s\n' "$main_geometry" | sed -n 's/^Y=//p')"
    [ -n "$main_width" ] || return 0
    [ -n "$main_height" ] || return 0
    [ -n "$main_x" ] || return 0
    [ -n "$main_y" ] || return 0

    #
    # Rockhopper's cold-boot image starts the Pocket PC calibration wizard.
    # Right-clicking the Guest Additions widget makes CERF expose a warning
    # only while that wizard is active, so it is also a safe setup probe.
    #
    xdotool mousemove \
        $((main_x + main_width - 65)) \
        $((main_y + main_height - 12)) \
        click 3 2>/dev/null
    sleep 1

    warning_window="$(xdotool search --name 'Guest Additions Warning' \
        2>/dev/null | tail -n 1 || true)"
    if [ -z "$warning_window" ]; then
        xdotool key --window "$main_window" Escape 2>/dev/null || true
        return 0
    fi

    # The modal blocks all guest input.  Closing it is sufficient because the
    # stock stylus path accepts the held calibration taps below.
    xdotool windowclose "$warning_window" 2>/dev/null || true
    for attempt in $(seq 1 10); do
        warning_window="$(xdotool search --name 'Guest Additions Warning' \
            2>/dev/null | tail -n 1 || true)"
        [ -z "$warning_window" ] && break
        sleep 1
    done
    if [ -n "$warning_window" ]; then
        echo "ERROR: unable to close Guest Additions calibration warning" >&2
        return 1
    fi

    # Switch from the enhanced pointer to the stock stylus.  The calibration
    # app does not accept Guest Additions pointer events.
    xdotool mousemove \
        $((main_x + main_width - 38)) \
        $((main_y + main_height - 12)) \
        click 1 2>/dev/null
    sleep 2

    # The calibration coordinates include CERF's 20-pixel host menu row.
    for point in \
        "320 260" \
        "64 116" \
        "64 404" \
        "576 404" \
        "576 116"; do
        set -- $point
        xdotool mousemove --window "$main_window" "$1" "$2" mousedown 1
        sleep 0.35
        xdotool mouseup 1
        sleep 0.7
    done
    sleep 4

    # Stylus page: Next.
    xdotool mousemove --window "$main_window" 203 323 mousedown 1
    sleep 0.2
    xdotool mouseup 1
    sleep 2

    # Popup-menu tutorial: cut the appointment, then paste it at 11 A.M.
    xdotool mousemove --window "$main_window" 110 231 mousedown 1
    sleep 1.2
    xdotool mouseup 1
    sleep 1
    xdotool mousemove --window "$main_window" 49 239 click 1
    sleep 2
    xdotool mousemove --window "$main_window" 110 245 mousedown 1
    sleep 1.2
    xdotool mouseup 1
    sleep 1
    xdotool mousemove --window "$main_window" 51 283 click 1
    sleep 2
    xdotool mousemove --window "$main_window" 203 323 click 1
    sleep 2

    # Location uses the same Next-button position as the tutorial pages.  The
    # final center tap dismisses setup-complete and is harmless if it advanced.
    xdotool mousemove --window "$main_window" 203 323 click 1
    sleep 2
    xdotool mousemove --window "$main_window" 320 260 click 1
    sleep 4
}

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
rom_windows="${rom_relative//\//\\}"

wine 'Z:\cerf\cerf.exe' \
    "--board-id=$board_id" \
    "--rom-primary=Z:\cerf\\${rom_windows}" \
    --guest-additions \
    '--share-folder=Z:\cerf\share' \
    --screen-width=640 \
    --screen-height=480 \
    --boot=cold \
    --log=CERF,CAUTION,NKDBG,GUEST,GUEST_HOST \
    "--log-file=Z:\cerf\logs\\${log_stem}.log" \
    >/tmp/wince-wine.log 2>&1 &
wine_pid="$!"

main_window=""
launch_mode="none"
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
complete_rockhopper_setup
dump_visible_windows "post-boot window snapshot"

# A boot-only run deliberately stops here.  It is useful for validating a ROM
# and board pairing without allowing a guest executable to affect the result.
if [ "$boot_only" -eq 1 ]; then
    import -window "$main_window" "/cerf/logs/${log_stem}.png"
    exit 0
fi

guest_command="\"\\Storage Card\\${program}\""
if [ -n "$program_arguments" ]; then
    guest_command="$guest_command $program_arguments"
fi

task_window=""
if launch_via_task_manager; then
    task_window="$(xdotool search --name "$task_manager_name" 2>/dev/null | tail -n 1 || true)"
    launch_mode="task-manager"
fi

if [ -z "$task_window" ] && [ "$task_manager_fallback" -eq 1 ]; then
    echo "WARN: CERF guest Task Manager did not appear; trying direct command fallback" >&2
    dump_visible_windows "window snapshot before fallback"
    launch_via_keyboard_fallback
    launch_mode="fallback"
fi

if [ "$launch_mode" = "none" ]; then
    if [ "$task_manager_fallback" -eq 1 ]; then
        echo "ERROR: host Task Manager was not found and fallback launch did not create output" >&2
    else
        echo "ERROR: CERF guest Task Manager did not appear" >&2
    fi
    dump_visible_windows "final snapshot before failing"
    exit 1
fi

if [ "$launch_mode" = "task-manager" ]; then
    task_geometry="$(xdotool getwindowgeometry --shell "$task_window")"
    task_x="$(printf '%s\n' "$task_geometry" | sed -n 's/^X=//p')"
    task_y="$(printf '%s\n' "$task_geometry" | sed -n 's/^Y=//p')"
    task_height="$(printf '%s\n' "$task_geometry" | sed -n 's/^HEIGHT=//p')"
    case "$task_height" in
        ""|*[!0-9]*)
            echo "ERROR: unable to read Task Manager geometry" >&2
            exit 1
            ;;
    esac
    [ -n "$task_x" ] || die "unable to read Task Manager X coordinate"
    [ -n "$task_y" ] || die "unable to read Task Manager Y coordinate"
else
    task_x=""
    task_y=""
    task_height=""
fi

# The Run edit control occupies the bottom row of the guest Task Manager.
if [ "$launch_mode" = "task-manager" ] && [ -n "$task_height" ]; then
    run_x=$((task_x + 160))
    run_y=$((task_y + task_height - 31))
    xdotool mousemove "$run_x" "$run_y" click 1 2>/dev/null
    xdotool key ctrl+a 2>/dev/null
    xdotool type --delay 15 "$guest_command" 2>/dev/null
    xdotool key Return 2>/dev/null
fi
sleep 1

# Keep the host-side Task Manager available while automation runs.  Its Run
# status and process list are valuable when a guest executable fails before it
# can create a shared result file.
if [ "$launch_mode" = "task-manager" ] && [ -n "$task_window" ]; then
    xdotool windowactivate "$main_window" 2>/dev/null || true
fi

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

# end of build_scripts/wince/run-mips-emulator.sh
