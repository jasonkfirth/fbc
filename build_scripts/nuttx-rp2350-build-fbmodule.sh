#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX/RP2350-PiZero dropped program build
# ------------------------------------------------------------
#
# File: nuttx-rp2350-build-fbmodule.sh
#
# Purpose:
#
#     Build one generated-FreeBASIC program as a NuttX ELF module that can be
#     copied to the RP2350-PiZero SD card and started from NSH.
#
# Responsibilities:
#
#     - reuse the NuttX smoke harness generated-C adaptation path
#     - stage the selected program as a loadable module instead of a built-in
#     - collect the resulting module into a predictable output directory
#
# This file intentionally does NOT contain:
#
#     - UF2 image building or flashing
#     - serial file transfer logic
#     - SD card formatting or partitioning
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/build_scripts" ] &&
       { [ -f "$SEARCH_DIR/GNUmakefile" ] ||
         [ -f "$SEARCH_DIR/makefile" ] ||
         [ -f "$SEARCH_DIR/Makefile" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi

    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root" >&2; exit 1; }

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

usage() {
    cat <<EOF
Usage: ./build_scripts/nuttx-rp2350-build-fbmodule.sh [options]

Options:
  --nuttx-workdir DIR     Directory containing nuttx/ and apps/
  --bas FILE              FreeBASIC source, default: examples/nuttx/fbhello.bas
  --app-name NAME         Program/module name, default: basename of --bas
  --fbc FILE              fbc binary to use when --generated-c is not supplied
  --generated-c FILE      Use existing generated C instead of invoking fbc
  --generated-c-dir DIR   Look for basename(--bas).c in this directory
  --out-dir DIR           Output directory, default: build/nuttx-rp2350-drop
  --with-gfxlib           Link the selected gfxlib2 smoke-test objects
  --stack-size BYTES      NuttX task stack size, default: 32768
  --help                  Show this help text

The resulting module is meant to be copied to /mnt/sd0/bin on the board.
EOF
}

NUTTX_WORKDIR="${NUTTX_WORKDIR:-}"
BAS_SRC="$ROOT/examples/nuttx/fbhello.bas"
APP_NAME=""
FBC_BIN="${FBC:-}"
GENERATED_C="${FB_GENERATED_C:-}"
GENERATED_C_DIR="${FB_NUTTX_GENERATED_C_DIR:-}"
OUT_DIR="$ROOT/build/nuttx-rp2350-drop"
WITH_GFXLIB=0
APP_STACKSIZE="${APP_STACKSIZE:-32768}"

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
        --generated-c-dir)
            [ "$#" -ge 2 ] || die "--generated-c-dir requires a directory"
            GENERATED_C_DIR="$2"
            shift 2
            ;;
        --out-dir)
            [ "$#" -ge 2 ] || die "--out-dir requires a directory"
            OUT_DIR="$2"
            shift 2
            ;;
        --with-gfxlib)
            WITH_GFXLIB=1
            shift
            ;;
        --stack-size)
            [ "$#" -ge 2 ] || die "--stack-size requires a byte count"
            APP_STACKSIZE="$2"
            shift 2
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

[ -n "$NUTTX_WORKDIR" ] || die "set NUTTX_WORKDIR or pass --nuttx-workdir"
[ -d "$NUTTX_WORKDIR/nuttx" ] || die "missing NuttX tree: $NUTTX_WORKDIR/nuttx"
[ -d "$NUTTX_WORKDIR/apps" ] || die "missing NuttX apps tree: $NUTTX_WORKDIR/apps"
[ -f "$BAS_SRC" ] || die "missing FreeBASIC source: $BAS_SRC"

if [ -z "$APP_NAME" ]; then
    APP_NAME="$(basename "$BAS_SRC" .bas)"
fi

case "$APP_NAME" in
    *[!A-Za-z0-9_]*|'')
        die "app name must contain only letters, numbers, and underscores"
        ;;
esac

case "$APP_STACKSIZE" in
    *[!0-9]*|'')
        die "invalid --stack-size: $APP_STACKSIZE"
        ;;
esac

if [ -z "$GENERATED_C" ] && [ -n "$GENERATED_C_DIR" ]; then
    GENERATED_C="$GENERATED_C_DIR/$(basename "$BAS_SRC" .bas).c"
fi

SMOKE_ARGS=(
    "$ROOT/build_scripts/nuttx-riscv32-qemu-smoke.sh"
    --nuttx-workdir "$NUTTX_WORKDIR"
    --app-name "$APP_NAME"
    --keep-existing-apps
    --skip-nuttx-config
    --no-run
    --loadable-module
)

if [ "$WITH_GFXLIB" -eq 1 ]; then
    SMOKE_ARGS+=(--with-gfxlib)
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
    run env APP_STACKSIZE="$APP_STACKSIZE" bash "${SMOKE_ARGS[@]}"
)

MODULE_FILE="$NUTTX_WORKDIR/apps/bin/$APP_NAME"
[ -f "$MODULE_FILE" ] || die "module was not produced: $MODULE_FILE"

mkdir -p "$OUT_DIR"
cp "$MODULE_FILE" "$OUT_DIR/$APP_NAME"

(
    cd "$OUT_DIR"
    sha256sum "$APP_NAME" > "$APP_NAME.sha256"
)

echo "NUTTX_RP2350_FB_MODULE_OK"
echo "MODULE: $OUT_DIR/$APP_NAME"
echo "SHA256: $OUT_DIR/$APP_NAME.sha256"

# end of nuttx-rp2350-build-fbmodule.sh
