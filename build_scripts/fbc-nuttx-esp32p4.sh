#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX ESP32-P4 remote workflow
# --------------------------------------------------
#
# File: fbc-nuttx-esp32p4.sh
#
# Purpose:
#
#     Build a FreeBASIC program as a NuttX RISC-V ELF module and deploy it to
#     an Ethernet-connected ESP32-P4 NuttX board.
#
# Responsibilities:
#
#     - reuse the existing generated-FreeBASIC NuttX module build path
#     - create and configure the default NuttX workspace on first use
#     - serve the module and optional data files over a temporary HTTP server
#     - use serial only for first-contact DHCP/IP discovery when needed
#     - use telnet for normal remote NSH command execution once available
#     - fetch ELF programs onto board storage and optionally execute them
#
# This file intentionally does NOT contain:
#
#     - firmware image configuration
#     - firmware flashing logic
#     - board-specific LCD, camera, USB host, or audio initialization
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

START_DIR="$(pwd)"
SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT=""

find_root_from() {
    local search_dir="$1"

    while :; do
        if [ -d "$search_dir/build_scripts" ] &&
           { [ -f "$search_dir/GNUmakefile" ] ||
             [ -f "$search_dir/makefile" ] ||
             [ -f "$search_dir/Makefile" ] ||
             [ -f "$search_dir/.nuttx-sdk-root" ]; }; then
            ROOT="$search_dir"
            return 0
        fi

        [ "$search_dir" = "/" ] && break
        search_dir="$(dirname "$search_dir")"
    done

    return 1
}

for root_candidate in \
    "${FB_NUTTX_SDK_ROOT:-}" \
    "$START_DIR" \
    "$SCRIPT_DIR" \
    "$(dirname "$SCRIPT_DIR")" \
    /usr/share/freebasic/nuttx-sdk; do
    [ -n "$root_candidate" ] || continue
    find_root_from "$root_candidate" && break
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root" >&2; exit 1; }

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

default_out_dir() {
    if [ -w "$ROOT" ]; then
        printf '%s\n' "$ROOT/build/nuttx-esp32p4-remote"
        return 0
    fi

    if [ -n "${XDG_CACHE_HOME:-}" ]; then
        printf '%s\n' "$XDG_CACHE_HOME/freebasic/nuttx-esp32p4-remote"
    else
        printf '%s\n' "${HOME:-/tmp}/.cache/freebasic/nuttx-esp32p4-remote"
    fi
}

default_nuttx_workdir() {
    if [ -n "${FB_NUTTX_ESP32P4_WORKDIR:-}" ]; then
        printf '%s\n' "$FB_NUTTX_ESP32P4_WORKDIR"
    elif [ -n "${NUTTX_WORKDIR:-}" ]; then
        printf '%s\n' "$NUTTX_WORKDIR"
    elif [ -n "${XDG_CACHE_HOME:-}" ]; then
        printf '%s\n' "$XDG_CACHE_HOME/freebasic/nuttx-esp32p4"
    else
        printf '%s\n' "${HOME:-/tmp}/.cache/freebasic/nuttx-esp32p4"
    fi
}

max_jobs() {
    local n=1

    if command -v nproc >/dev/null 2>&1; then
        n="$(nproc)"
    elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
        n="$(getconf _NPROCESSORS_ONLN)"
    fi

    case "$n" in
        ''|*[!0-9]*) n=1 ;;
    esac

    [ "$n" -ge 1 ] || n=1
    echo "$n"
}

find_riscv_tool() {
    local tool="$1"
    local candidate

    for candidate in \
        "riscv-none-elf-$tool" \
        "riscv64-unknown-elf-$tool" \
        "riscv32-unknown-elf-$tool"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

usage() {
    cat <<EOF
Usage: ./build_scripts/fbc-nuttx-esp32p4.sh [options]

Options:
  --nuttx-workdir DIR   Directory containing nuttx/ and apps/
                        Default: \$XDG_CACHE_HOME/freebasic/nuttx-esp32p4
                        or \$HOME/.cache/freebasic/nuttx-esp32p4
  --bas FILE            FreeBASIC source, default: examples/nuttx/fbhello.bas
  --app-name NAME       Module name, default: basename of --bas
  --fbc FILE            fbc binary used when generating C
  --generated-c FILE    Use an existing generated C file instead of fbc
  --out-dir DIR         Output directory, default: build/nuttx-esp32p4-remote
  --stack-size BYTES    NuttX task stack size, default: 65536
  --with-gfxlib         Link the NuttX gfxlib support objects
  --sfx-no-media-decoders
                        Link generated-tone sfxlib without WAV/MP3/Ogg decoders
  --asset FILE[:DEST]   Also upload a data file. DEST is relative to the
                        selected board directory.
  --storage data        Store under /data/fb
  --storage sd          Store under /mnt/sd0/fb
  --storage auto        Prefer /mnt/sd0/fb when a telnet probe sees it,
                        otherwise use /data/fb. This is the default.
  --remote-dir DIR      Explicit board directory for module and relative assets
  --server-host IP      Host IP used in board wget URLs
  --server-bind IP      Host bind address, default: 0.0.0.0
  --server-port PORT    Temporary HTTP server port, default: 8008
  --board-ip IP         Board IP address for telnet control
  --telnet-port PORT    Board telnet port, default: 23
  --serial-port DEVICE  Serial console used for first-contact DHCP/IP discovery
  --control MODE        auto, telnet, serial, or none. Default: auto
  --run                 Run the uploaded ELF program from board storage
  --no-upload           Build and stage the module only
  --prepare-only        Create and configure the workspace, then stop
  --no-bootstrap        Require an already configured workspace
  --help                Show this help text

Environment:
  NUTTX_WORKDIR         Same as --nuttx-workdir
  FB_NUTTX_ESP32P4_WORKDIR
                        Alternate default workdir
  FB_NUTTX_AUTO_SETUP   Set to 0 to disable workspace setup
  FBC                   Same as --fbc
  FB_HOST_FBC           Host fbc used to build the helper tools
  FB_GENERATED_C        Same as --generated-c
  FB_NUTTX_BOARD_IP     Same as --board-ip
  FB_NUTTX_CONSOLE_TIMEOUT
                        Console command timeout in seconds, default: 120
  FB_NUTTX_SERIAL_NO_SYNC
                        Set to 0 to force the initial serial prompt sync
  FB_NUTTX_RENEW_ETH0   Set to 1 to run renew eth0 before network uploads
  ESPTOOL_PORT          Default serial port when --serial-port is omitted

Typical first contact:

  ./build_scripts/fbc-nuttx-esp32p4.sh \\
    --serial-port /dev/ttyACM0 --run

Typical later upload once telnet is available:

  ./build_scripts/fbc-nuttx-esp32p4.sh \\
    --board-ip 192.168.250.162 --bas examples/nuttx/fbgfx_sfx_smoke.bas \\
    --with-gfxlib --sfx-no-media-decoders --run

The first normal run creates and configures the workspace when needed. It
does not flash firmware; use fbc-nuttx-esp32p4-firmware for that step.
EOF
}

NUTTX_WORKDIR="${NUTTX_WORKDIR:-$(default_nuttx_workdir)}"
BAS_SRC="$ROOT/examples/nuttx/fbhello.bas"
APP_NAME=""
FBC_BIN="${FBC:-}"
HOST_FBC="${FB_HOST_FBC:-}"
GENERATED_C="${FB_GENERATED_C:-}"
OUT_DIR="${FB_NUTTX_OUT_DIR:-$(default_out_dir)}"
APP_STACKSIZE="${APP_STACKSIZE:-65536}"
WITH_GFXLIB=0
SFX_NO_MEDIA_DECODERS=0
ASSETS=()
STORAGE="auto"
REMOTE_DIR=""
SERVER_HOST="${FB_NUTTX_SERVER_HOST:-}"
SERVER_BIND="${FB_NUTTX_SERVER_BIND:-0.0.0.0}"
SERVER_PORT="${FB_NUTTX_SERVER_PORT:-8008}"
BOARD_IP="${FB_NUTTX_BOARD_IP:-}"
TELNET_PORT="${FB_NUTTX_TELNET_PORT:-23}"
SERIAL_PORT="${FB_NUTTX_SERIAL_PORT:-${ESPTOOL_PORT:-}}"
CONSOLE_TIMEOUT="${FB_NUTTX_CONSOLE_TIMEOUT:-120}"
SERIAL_NO_SYNC="${FB_NUTTX_SERIAL_NO_SYNC:-1}"
RENEW_ETH0="${FB_NUTTX_RENEW_ETH0:-0}"
CONTROL="${FB_NUTTX_CONTROL:-auto}"
DO_RUN=0
DO_UPLOAD=1
AUTO_SETUP="${FB_NUTTX_AUTO_SETUP:-1}"
PREPARE_ONLY=0
JOBS="${JOBS:-$(max_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --nuttx-workdir)
            [ "$#" -ge 2 ] || die "--nuttx-workdir requires a directory"
            NUTTX_WORKDIR="$2"
            shift 2
            ;;
        --bas)
            [ "$#" -ge 2 ] || die "--bas requires a file"
            BAS_SRC="$2"
            shift 2
            ;;
        --app-name)
            [ "$#" -ge 2 ] || die "--app-name requires a name"
            APP_NAME="$2"
            shift 2
            ;;
        --fbc)
            [ "$#" -ge 2 ] || die "--fbc requires a file"
            FBC_BIN="$2"
            shift 2
            ;;
        --generated-c)
            [ "$#" -ge 2 ] || die "--generated-c requires a file"
            GENERATED_C="$2"
            shift 2
            ;;
        --out-dir)
            [ "$#" -ge 2 ] || die "--out-dir requires a directory"
            OUT_DIR="$2"
            shift 2
            ;;
        --stack-size)
            [ "$#" -ge 2 ] || die "--stack-size requires a byte count"
            APP_STACKSIZE="$2"
            shift 2
            ;;
        --with-gfxlib)
            WITH_GFXLIB=1
            shift
            ;;
        --sfx-no-media-decoders)
            SFX_NO_MEDIA_DECODERS=1
            shift
            ;;
        --asset)
            [ "$#" -ge 2 ] || die "--asset requires FILE[:DEST]"
            ASSETS+=("$2")
            shift 2
            ;;
        --storage)
            [ "$#" -ge 2 ] || die "--storage requires data, sd, or auto"
            STORAGE="$2"
            shift 2
            ;;
        --remote-dir)
            [ "$#" -ge 2 ] || die "--remote-dir requires a board path"
            REMOTE_DIR="$2"
            shift 2
            ;;
        --server-host)
            [ "$#" -ge 2 ] || die "--server-host requires an address"
            SERVER_HOST="$2"
            shift 2
            ;;
        --server-bind)
            [ "$#" -ge 2 ] || die "--server-bind requires an address"
            SERVER_BIND="$2"
            shift 2
            ;;
        --server-port)
            [ "$#" -ge 2 ] || die "--server-port requires a port"
            SERVER_PORT="$2"
            shift 2
            ;;
        --board-ip)
            [ "$#" -ge 2 ] || die "--board-ip requires an address"
            BOARD_IP="$2"
            shift 2
            ;;
        --telnet-port)
            [ "$#" -ge 2 ] || die "--telnet-port requires a port"
            TELNET_PORT="$2"
            shift 2
            ;;
        --serial-port)
            [ "$#" -ge 2 ] || die "--serial-port requires a device"
            SERIAL_PORT="$2"
            shift 2
            ;;
        --control)
            [ "$#" -ge 2 ] || die "--control requires auto, telnet, serial, or none"
            CONTROL="$2"
            shift 2
            ;;
        --run)
            DO_RUN=1
            shift
            ;;
        --no-upload)
            DO_UPLOAD=0
            shift
            ;;
        --prepare-only)
            PREPARE_ONLY=1
            DO_UPLOAD=0
            shift
            ;;
        --no-bootstrap)
            AUTO_SETUP=0
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

case "$AUTO_SETUP" in
    0|1) ;;
    *) die "FB_NUTTX_AUTO_SETUP must be 0 or 1" ;;
esac

ensure_nuttx_workspace() {
    local firmware_script="$ROOT/build_scripts/fbc-nuttx-esp32p4-firmware.sh"

    if [ "$AUTO_SETUP" -eq 0 ]; then
        [ -d "$NUTTX_WORKDIR/nuttx" ] || die "missing NuttX tree: $NUTTX_WORKDIR/nuttx"
        [ -d "$NUTTX_WORKDIR/apps" ] || die "missing NuttX apps tree: $NUTTX_WORKDIR/apps"
        [ -f "$NUTTX_WORKDIR/nuttx/.config" ] ||
            die "missing NuttX configuration: $NUTTX_WORKDIR/nuttx/.config"
        return 0
    fi

    [ -f "$firmware_script" ] || die "missing NuttX firmware setup script: $firmware_script"

    if [ -d "$NUTTX_WORKDIR/nuttx" ] &&
       [ -d "$NUTTX_WORKDIR/apps" ] &&
       [ -f "$NUTTX_WORKDIR/nuttx/.config" ]; then
        echo "==> using configured NuttX workspace: $NUTTX_WORKDIR"
        return 0
    fi

    echo "==> preparing NuttX workspace: $NUTTX_WORKDIR"
    run bash "$firmware_script" --workdir "$NUTTX_WORKDIR" --no-build
}

ensure_nuttx_workspace

if [ "$PREPARE_ONLY" -eq 1 ]; then
    echo "NUTTX_WORKSPACE_READY: $NUTTX_WORKDIR"
    exit 0
fi

[ -f "$BAS_SRC" ] || [ -n "$GENERATED_C" ] || die "missing FreeBASIC source: $BAS_SRC"

if [ -z "$APP_NAME" ]; then
    APP_NAME="$(basename "$BAS_SRC" .bas)"
fi

case "$APP_NAME" in
    *[!A-Za-z0-9_]*|'')
        die "app name must contain only letters, numbers, and underscores"
        ;;
esac

case "$APP_STACKSIZE" in
    ''|*[!0-9]*) die "invalid --stack-size: $APP_STACKSIZE" ;;
esac

case "$SERVER_PORT" in
    ''|*[!0-9]*) die "invalid --server-port: $SERVER_PORT" ;;
esac

case "$TELNET_PORT" in
    ''|*[!0-9]*) die "invalid --telnet-port: $TELNET_PORT" ;;
esac

case "$CONSOLE_TIMEOUT" in
    ''|*[!0-9]*) die "invalid FB_NUTTX_CONSOLE_TIMEOUT: $CONSOLE_TIMEOUT" ;;
esac

case "$SERIAL_NO_SYNC" in
    0|1) ;;
    *) die "FB_NUTTX_SERIAL_NO_SYNC must be 0 or 1" ;;
esac

case "$RENEW_ETH0" in
    0|1) ;;
    *) die "FB_NUTTX_RENEW_ETH0 must be 0 or 1" ;;
esac

case "$JOBS" in
    ''|*[!0-9]*) die "invalid JOBS value: $JOBS" ;;
esac

case "$STORAGE" in
    auto|data|sd) ;;
    *) die "--storage must be auto, data, or sd" ;;
esac

case "$CONTROL" in
    auto|telnet|serial|none) ;;
    *) die "--control must be auto, telnet, serial, or none" ;;
esac

validate_board_path() {
    local path="$1"

    case "$path" in
        /*) ;;
        *) die "board path must be absolute: $path" ;;
    esac

    case "$path" in
        *"'"* | *" "* | *";"* | *"&"* | *"|"* | *"<"* | *">"* | *"$"* | *"("* | *")"*)
            die "board path contains unsupported shell characters: $path"
            ;;
    esac
}

validate_relative_path() {
    local path="$1"

    case "$path" in
        ""|/*|*..*|*"'"*|*" "*|*";"*|*"&"*|*"|"*|*"<"*|*">"*|*"$"*|*"("*|*")"*)
            die "asset destination is not a safe relative path: $path"
            ;;
    esac
}

if [ -n "$REMOTE_DIR" ]; then
    validate_board_path "$REMOTE_DIR"
fi

##############################################################################
# Build the FreeBASIC module
##############################################################################

BUILD_DIR="$OUT_DIR/build"
SERVE_DIR="$OUT_DIR/http"
TOOLS_DIR="$OUT_DIR/tools"
HTTP_LOG="$OUT_DIR/http-server.log"
MODULE_PATH="$NUTTX_WORKDIR/apps/bin/$APP_NAME"

mkdir -p "$BUILD_DIR" "$SERVE_DIR" "$TOOLS_DIR"
rm -f "$SERVE_DIR/$APP_NAME"

SMOKE_ARGS=(
    "$ROOT/build_scripts/nuttx-riscv32-qemu-smoke.sh"
    --nuttx-workdir "$NUTTX_WORKDIR"
    --skip-nuttx-config
    --reuse-config
    --keep-existing-apps
    --loadable-module
    --app-name "$APP_NAME"
)

if [ "$WITH_GFXLIB" -eq 1 ]; then
    SMOKE_ARGS+=(--with-gfxlib)
fi

if [ "$SFX_NO_MEDIA_DECODERS" -eq 1 ]; then
    SMOKE_ARGS+=(--sfx-no-media-decoders)
fi

if [ -n "$GENERATED_C" ]; then
    [ -f "$GENERATED_C" ] || die "missing generated C: $GENERATED_C"
    SMOKE_ARGS+=(--generated-c "$GENERATED_C")
else
    SMOKE_ARGS+=(--bas "$BAS_SRC")

    if [ -n "$FBC_BIN" ]; then
        SMOKE_ARGS+=(--fbc "$FBC_BIN")
    fi
fi

(
    cd "$ROOT"
    run env APP_STACKSIZE="$APP_STACKSIZE" JOBS="$JOBS" bash "${SMOKE_ARGS[@]}"
)

[ -f "$MODULE_PATH" ] || die "module was not produced: $MODULE_PATH"
cp "$MODULE_PATH" "$SERVE_DIR/$APP_NAME"

if STRIP_TOOL="$(find_riscv_tool strip)"; then
    run "$STRIP_TOOL" --strip-unneeded "$SERVE_DIR/$APP_NAME"
fi

(
    cd "$SERVE_DIR"
    sha256sum "$APP_NAME" > "$APP_NAME.sha256"
)

for asset in "${ASSETS[@]}"; do
    src="$asset"
    dst=""

    case "$asset" in
        *:*)
            src="${asset%%:*}"
            dst="${asset#*:}"
            ;;
    esac

    [ -f "$src" ] || die "missing asset: $src"

    if [ -z "$dst" ]; then
        dst="$(basename "$src")"
    fi

    validate_relative_path "$dst"

    mkdir -p "$SERVE_DIR/assets/$(dirname "$dst")"
    cp "$src" "$SERVE_DIR/assets/$dst"
done

echo "NUTTX_ESP32P4_FB_MODULE_OK"
echo "MODULE: $SERVE_DIR/$APP_NAME"
echo "SHA256: $SERVE_DIR/$APP_NAME.sha256"

if [ "$DO_UPLOAD" -eq 0 ]; then
    exit 0
fi

##############################################################################
# Board control and upload
##############################################################################

host_exe_suffix() {
    case "$(uname -s 2>/dev/null || echo unknown)" in
        MINGW*|MSYS*|CYGWIN*) printf '%s\n' ".exe" ;;
        *) printf '%s\n' "" ;;
    esac
}

find_host_fbc() {
    local host_os

    if [ -n "$HOST_FBC" ]; then
        printf '%s\n' "$HOST_FBC"
        return 0
    fi

    host_os="$(uname -s 2>/dev/null || echo unknown)"

	case "$host_os" in
		MINGW*|MSYS*|CYGWIN*)
			if [ -x "$ROOT/bin/fbc.exe" ]; then
				printf '%s\n' "$ROOT/bin/fbc.exe"
				return 0
			fi
			;;
	esac

	if [ -x "$ROOT/bin/fbc" ]; then
		printf '%s\n' "$ROOT/bin/fbc"
		return 0
	fi

	if [ -x "$ROOT/bin/fbc.exe" ]; then
		printf '%s\n' "$ROOT/bin/fbc.exe"
		return 0
	fi

	if command -v fbc >/dev/null 2>&1; then
		command -v fbc
		return 0
	fi

    return 1
}

CONSOLE_HELPER_SRC="$ROOT/build_scripts/nuttx-remote-console.bas"
HTTP_SERVER_SRC="$ROOT/build_scripts/nuttx-http-server.bas"
HOST_TOOL_SUFFIX="$(host_exe_suffix)"
CONSOLE_HELPER="$TOOLS_DIR/nuttx-remote-console$HOST_TOOL_SUFFIX"
HTTP_SERVER="$TOOLS_DIR/nuttx-http-server$HOST_TOOL_SUFFIX"

[ -f "$CONSOLE_HELPER_SRC" ] || die "missing console helper source: $CONSOLE_HELPER_SRC"
[ -f "$HTTP_SERVER_SRC" ] || die "missing HTTP server source: $HTTP_SERVER_SRC"

HOST_FBC="$(find_host_fbc)" || die "could not find a host fbc for $CONSOLE_HELPER_SRC"

build_fb_host_tool() {
    local src="$1"
    local exe="$2"
    local work_src_dir="$TOOLS_DIR/src"
    local work_src="$work_src_dir/$(basename "$src")"

    mkdir -p "$work_src_dir"

    if [ ! -f "$work_src" ] || [ "$src" -nt "$work_src" ]; then
        cp -p "$src" "$work_src"
    fi

    if [ ! -x "$exe" ] || [ "$src" -nt "$exe" ] || [ "$work_src" -nt "$exe" ]; then
        run "$HOST_FBC" -noobjinfo "$work_src" -x "$exe"
    fi
}

build_fb_host_tool "$CONSOLE_HELPER_SRC" "$CONSOLE_HELPER"
build_fb_host_tool "$HTTP_SERVER_SRC" "$HTTP_SERVER"

if [ -z "$SERVER_HOST" ]; then
    if command -v hostname >/dev/null 2>&1; then
        SERVER_HOST="$(hostname -I 2>/dev/null | awk '{ print $1; exit }')"
    fi

    if [ -z "$SERVER_HOST" ] && command -v ip >/dev/null 2>&1; then
        SERVER_HOST="$(ip route get 1.1.1.1 2>/dev/null |
            sed -n 's/.* src \([0-9.][0-9.]*\).*/\1/p' |
            head -n 1)"
    fi

    [ -n "$SERVER_HOST" ] || die "could not infer --server-host"
fi

console_args() {
    case "$1" in
        telnet)
            [ -n "$BOARD_IP" ] || die "telnet control requires --board-ip"
            printf '%s\n' --telnet "$BOARD_IP" --telnet-port "$TELNET_PORT" \
                --timeout "$CONSOLE_TIMEOUT"
            ;;
        serial)
            [ -n "$SERIAL_PORT" ] || die "serial control requires --serial-port"
            printf '%s\n' --serial "$SERIAL_PORT" --timeout "$CONSOLE_TIMEOUT"
            if [ "$SERIAL_NO_SYNC" -eq 1 ]; then
                printf '%s\n' --no-sync
            fi
            ;;
        *)
            die "internal error: unsupported console mode $1"
            ;;
    esac
}

console_command() {
    local mode="$1"
    shift

    local args=()
    local cmd

    while IFS= read -r arg; do
        args+=("$arg")
    done < <(console_args "$mode")

    for cmd in "$@"; do
        args+=(--cmd "$cmd")
    done

    "$CONSOLE_HELPER" "${args[@]}"
}

telnet_is_open() {
    local host="$1"
    local port="$2"

    timeout 2 bash -c "</dev/tcp/$host/$port" >/dev/null 2>&1
}

CONTROL_MODE="$CONTROL"

if [ "$CONTROL_MODE" = "auto" ]; then
    if [ -n "$BOARD_IP" ]; then
        CONTROL_MODE="telnet"
    elif [ -n "$SERIAL_PORT" ]; then
        CONTROL_MODE="serial"
    else
        CONTROL_MODE="none"
    fi
fi

if [ "$CONTROL_MODE" = "none" ]; then
    die "upload requires --board-ip or --serial-port"
fi

if [ "$CONTROL_MODE" = "serial" ] && [ -z "$BOARD_IP" ]; then
    echo "==> discovering board address over serial"
    SERIAL_DISCOVERY_OUTPUT="$(console_command serial "ifconfig" || true)"
    printf '%s\n' "$SERIAL_DISCOVERY_OUTPUT"

    BOARD_IP="$(
        printf '%s\n' "$SERIAL_DISCOVERY_OUTPUT" |
        sed -n 's/.*inet addr:\([0-9][0-9.]*\).*/\1/p' |
        awk '!/^127\./ { address = $0 } END { if (address != "") print address }'
    )"

    if [ -z "$BOARD_IP" ] && [ "$RENEW_ETH0" -eq 1 ]; then
        echo "==> renewing eth0 over serial"
        SERIAL_DISCOVERY_OUTPUT="$(
            console_command serial \
                "renew eth0" \
                "ifconfig" || true
        )"
        printf '%s\n' "$SERIAL_DISCOVERY_OUTPUT"

        BOARD_IP="$(
            printf '%s\n' "$SERIAL_DISCOVERY_OUTPUT" |
            sed -n 's/.*inet addr:\([0-9][0-9.]*\).*/\1/p' |
            awk '!/^127\./ { address = $0 } END { if (address != "") print address }'
        )"
    fi

    if [ -n "$BOARD_IP" ]; then
        echo "==> board address: $BOARD_IP"

        if telnet_is_open "$BOARD_IP" "$TELNET_PORT"; then
            echo "==> telnet is reachable at $BOARD_IP:$TELNET_PORT"
            CONTROL_MODE="telnet"
        else
            echo "==> telnet is not reachable; continuing over serial"
        fi
    else
        echo "==> board address was not found; continuing over serial"
    fi
fi

if [ -z "$REMOTE_DIR" ]; then
    case "$STORAGE" in
        data)
            REMOTE_DIR="/data/fb"
            ;;
        sd)
            REMOTE_DIR="/mnt/sd0/fb"
            ;;
        auto)
            REMOTE_DIR="/data/fb"

            if [ "$CONTROL_MODE" = "telnet" ]; then
                SD_PROBE_OUTPUT="$(console_command telnet "ls /mnt/sd0" || true)"
                if ! printf '%s\n' "$SD_PROBE_OUTPUT" |
                    grep -Eiq 'no such|failed|error|not found'; then
                    REMOTE_DIR="/mnt/sd0/fb"
                fi
            fi
            ;;
    esac
fi

validate_board_path "$REMOTE_DIR"

REMOTE_FILE="$REMOTE_DIR/$APP_NAME"

HTTP_PID=""

cleanup() {
    if [ -n "$HTTP_PID" ]; then
        kill "$HTTP_PID" >/dev/null 2>&1 || true
    fi
}

trap cleanup EXIT

start_http_server() {
    local first_port="$SERVER_PORT"
    local attempt=0
    local max_attempts=20
    local port

    while [ "$attempt" -lt "$max_attempts" ]; do
        port=$((10#$first_port + attempt))
        if [ "$port" -gt 65535 ]; then
            break
        fi

        : > "$HTTP_LOG"
        echo "==> serving $SERVE_DIR at http://$SERVER_HOST:$port/"
        "$HTTP_SERVER" \
            --bind "$SERVER_BIND" \
            --port "$port" \
            --directory "$SERVE_DIR" > "$HTTP_LOG" 2>&1 &
        HTTP_PID=$!

        sleep 1

        if kill -0 "$HTTP_PID" >/dev/null 2>&1; then
            SERVER_PORT="$port"
            return 0
        fi

        cat "$HTTP_LOG" >&2 || true
        HTTP_PID=""
        attempt=$((attempt + 1))
    done

    die "temporary HTTP server failed to start"
}

start_http_server

MODULE_URL="http://$SERVER_HOST:$SERVER_PORT/$APP_NAME"

UPLOAD_COMMANDS=()

if [ "$RENEW_ETH0" -eq 1 ]; then
    UPLOAD_COMMANDS+=("renew eth0")
fi

UPLOAD_COMMANDS+=(
    "mkdir $REMOTE_DIR"
    "rm $REMOTE_FILE"
    "wget -o $REMOTE_FILE $MODULE_URL"
    "ls -l $REMOTE_FILE"
)

for asset in "${ASSETS[@]}"; do
    src="$asset"
    dst=""

    case "$asset" in
        *:*)
            src="${asset%%:*}"
            dst="${asset#*:}"
            ;;
    esac

    if [ -z "$dst" ]; then
        dst="$(basename "$src")"
    fi

    asset_remote="$REMOTE_DIR/$dst"
    asset_url_path="assets/$dst"

    asset_remote_dir="$(dirname "$asset_remote")"
    UPLOAD_COMMANDS+=(
        "mkdir $asset_remote_dir"
        "rm $asset_remote"
        "wget -o $asset_remote http://$SERVER_HOST:$SERVER_PORT/$asset_url_path"
        "ls -l $asset_remote"
    )
done

if [ "$DO_RUN" -eq 1 ]; then
    UPLOAD_COMMANDS+=("$REMOTE_FILE")
fi

echo "==> uploading to $REMOTE_FILE using $CONTROL_MODE"
console_command "$CONTROL_MODE" "${UPLOAD_COMMANDS[@]}"

echo "NUTTX_ESP32P4_REMOTE_UPLOAD_OK"
echo "BOARD: $BOARD_IP"
echo "REMOTE: $REMOTE_FILE"

# end of fbc-nuttx-esp32p4.sh
