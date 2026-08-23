#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS emulator bootstrap
# ---------------------------------------------
#
# File: riscos-bootstrap-rpcemu-boot.sh
#
# Purpose:
#
#     Configure a fresh RPCEmu CMOS image to boot the HostFS RISC OS desktop.
#
# Responsibilities:
#
#     - locate the already-running RPCEmu X11 window
#     - enter RISC OS Configure commands through the emulated keyboard
#     - hard-reset the guest so its installed Choices boot tasks are executed
#     - retain screenshots around the one-time bootstrap for CI diagnosis
#
# This file intentionally does NOT contain:
#
#     - RPCEmu, ROM, or HardDisc4 construction
#     - fbctests or Exampleageddon execution
#     - success-marker policy owned by the workload runner
#
# Bootstrap model:
#
#     RPCEmu 0.9.5 ships a generic CMOS image.  It can start a RISC OS 5 ROM,
#     but it does not select HostFS or enable booting from the installed !Boot
#     tree.  RISC OS owns the CMOS layout, so use its documented Configure
#     commands instead of patching undocumented bytes on the host.
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
EVIDENCE_DIR="${RISCOS_BOOT_EVIDENCE_DIR:-$ROOT/out/riscos/boot-evidence}"
RPCEMU_PID=""
WINDOW_TIMEOUT=30
GUEST_STARTUP_SECONDS=12

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

capture_screen() {
    local name="$1"

    scrot "$EVIDENCE_DIR/$name.png"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-bootstrap-rpcemu-boot.sh [options]

Options:
  --pid PID           Process ID of an already-running RPCEmu instance.
  --workdir DIR       Managed RPCEmu directory. Default: out/riscos/rpcemu
  --evidence-dir DIR  Screenshot destination. Default: out/riscos/boot-evidence
  --window-timeout N  Seconds to wait for the RPCEmu window. Default: 30
  -h, --help          Show this help.

The caller must run this script inside an X11 session such as xvfb-run.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --pid)
            require_value "$1" "${2-}"
            RPCEMU_PID="$2"
            shift 2
            ;;
        --workdir)
            require_value "$1" "${2-}"
            RPCEMU_WORKDIR="$2"
            shift 2
            ;;
        --evidence-dir)
            require_value "$1" "${2-}"
            EVIDENCE_DIR="$2"
            shift 2
            ;;
        --window-timeout)
            require_value "$1" "${2-}"
            WINDOW_TIMEOUT="$2"
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

case "$RPCEMU_PID" in
    ''|*[!0-9]*|0) die "--pid must be a positive process identifier" ;;
esac

case "$WINDOW_TIMEOUT" in
    ''|*[!0-9]*|0) die "--window-timeout must be a positive integer" ;;
esac

##############################################################################
# Preconditions and window discovery
##############################################################################

for tool in xdotool scrot; do
    command -v "$tool" >/dev/null 2>&1 || die "required X11 tool not found: $tool"
done

[ -n "${DISPLAY:-}" ] || die "DISPLAY is not set"
[ -d "$RPCEMU_WORKDIR/source" ] ||
    die "RPCEmu runtime not found below $RPCEMU_WORKDIR"
kill -0 "$RPCEMU_PID" 2>/dev/null ||
    die "RPCEmu process $RPCEMU_PID is not running"

mkdir -p "$EVIDENCE_DIR"

window_id=""
elapsed=0
while [ "$elapsed" -lt "$WINDOW_TIMEOUT" ]; do
    window_id="$({
        xdotool search --onlyvisible --pid "$RPCEMU_PID" --name 'RPCEmu' || true
    } | head -n 1)"

    if [ -n "$window_id" ]; then
        break
    fi

    kill -0 "$RPCEMU_PID" 2>/dev/null ||
        die "RPCEmu exited before its X11 window appeared"
    sleep 1
    elapsed=$((elapsed + 1))
done

[ -n "$window_id" ] ||
    die "RPCEmu window did not appear within $WINDOW_TIMEOUT seconds"

##############################################################################
# RISC OS CMOS configuration
##############################################################################

# The window can exist while the ROM is still initialising.  Wait for the
# failed default ADFS desktop attempt to leave RISC OS at its supervisor prompt.
sleep "$GUEST_STARTUP_SECONDS"
kill -0 "$RPCEMU_PID" 2>/dev/null ||
    die "RPCEmu exited during RISC OS startup"

capture_screen "before-hostfs-configuration"
xdotool windowfocus --sync "$window_id"

xdotool type --delay 20 --clearmodifiers 'Configure FileSystem HostFS'
xdotool key Return
sleep 1
xdotool type --delay 20 --clearmodifiers 'Configure Boot'
xdotool key Return
sleep 1

capture_screen "after-hostfs-configuration"

# X11 keycode 127 is Pause/Break.  RPCEmu maps Ctrl-Break to the RISC OS hard
# reset required for new Configure values to take effect.
xdotool key ctrl+Pause
sleep 8
kill -0 "$RPCEMU_PID" 2>/dev/null ||
    die "RPCEmu exited after the HostFS boot reset"
capture_screen "after-hostfs-reset"

echo "==> requested RISC OS HostFS boot configuration and hard reset"
echo "    Evidence: $EVIDENCE_DIR"

# end of riscos-bootstrap-rpcemu-boot.sh
