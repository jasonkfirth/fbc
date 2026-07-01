#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX RP2350-PiZero firmware workflow
# --------------------------------------------------------
#
# File: fbc-nuttx-rp2350-pizero-firmware.sh
#
# Purpose:
#
#     Fetch Apache NuttX, build the Waveshare RP2350-PiZero firmware image
#     used by the FreeBASIC NuttX bring-up work, and optionally copy the UF2
#     to a mounted BOOTSEL/write-mode volume.
#
# Responsibilities:
#
#     - clone or update the external NuttX and apps repositories
#     - run the existing RP2350-PiZero image builder from this SDK
#     - keep the board-specific DVI, USB, MicroSD, and BOOTSEL work included
#     - copy the finished UF2 only when a destination is explicitly supplied
#
# This file intentionally does NOT contain:
#
#     - a vendored copy of Apache NuttX
#     - a second copy of the RP2350 image configuration logic
#     - FreeBASIC program upload over YMODEM or network protocols
#     - automatic partitioning or formatting of SD cards
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project or installed SDK root
##############################################################################

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

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC NuttX SDK root" >&2; exit 1; }

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

default_workdir() {
    if [ -n "${FB_NUTTX_RP2350_PIZERO_WORKDIR:-}" ]; then
        printf '%s\n' "$FB_NUTTX_RP2350_PIZERO_WORKDIR"
    elif [ -n "${NUTTX_WORKDIR:-}" ]; then
        printf '%s\n' "$NUTTX_WORKDIR"
    else
        printf '%s\n' "${HOME:-/tmp}/fbxl-nuttx-rp2350-pizero"
    fi
}

default_out_dir() {
    if [ -w "$ROOT" ]; then
        printf '%s\n' "$ROOT/build/nuttx-rp2350-pizero"
        return 0
    fi

    if [ -n "${XDG_CACHE_HOME:-}" ]; then
        printf '%s\n' "$XDG_CACHE_HOME/freebasic/nuttx-rp2350-pizero"
    else
        printf '%s\n' "${HOME:-/tmp}/.cache/freebasic/nuttx-rp2350-pizero"
    fi
}

safe_remove_temp_dir() {
    local target="$1"
    local parent="$2"

    case "$target" in
        "$parent"/.clone-*)
            rm -rf "$target"
            ;;
        *)
            die "refusing to remove unexpected temporary directory: $target"
            ;;
    esac
}

clone_repo() {
    local url="$1"
    local ref="$2"
    local target="$3"
    local parent
    local temp_target

    if [ -d "$target/.git" ]; then
        echo "==> repository exists: $target"
        return 0
    fi

    if [ -e "$target" ]; then
        die "$target exists but is not a git repository"
    fi

    parent="$(dirname "$target")"
    mkdir -p "$parent"
    temp_target="$parent/.clone-$(basename "$target").$$"

    safe_remove_temp_dir "$temp_target" "$parent" >/dev/null 2>&1 || true

    if git clone --depth 1 --branch "$ref" "$url" "$temp_target"; then
        mv "$temp_target" "$target"
        return 0
    fi

    safe_remove_temp_dir "$temp_target" "$parent"

    #
    # A commit hash is not accepted by "git clone --branch" on every git
    # version.  Fall back to a full clone and then check out the requested ref.
    #
    run git clone "$url" "$temp_target"
    (
        cd "$temp_target"
        run git checkout "$ref"
    )
    mv "$temp_target" "$target"
}

update_repo() {
    local target="$1"
    local ref="$2"

    [ -d "$target/.git" ] || die "$target is not a git repository"

    (
        cd "$target"
        run git fetch --tags --prune origin
        run git checkout "$ref"

        if git rev-parse --abbrev-ref --symbolic-full-name '@{u}' >/dev/null 2>&1; then
            run git pull --ff-only
        fi
    )
}

copy_uf2_to_volume() {
    local source_uf2="$1"
    local dest_dir="$2"
    local dest_file

    [ -f "$source_uf2" ] || die "UF2 output was not found: $source_uf2"
    [ -d "$dest_dir" ] || die "UF2 destination is not a directory: $dest_dir"
    [ -w "$dest_dir" ] || die "UF2 destination is not writable: $dest_dir"

    dest_file="$dest_dir/$(basename "$source_uf2")"
    run cp "$source_uf2" "$dest_file"

    #
    # BOOTSEL volumes normally disappear shortly after a valid UF2 is copied.
    # A plain sync gives the host a chance to flush data before that detach.
    #
    sync || true
    echo "UF2_COPIED: $dest_file"
}

usage() {
    cat <<EOF
Usage: fbc-nuttx-rp2350-pizero-firmware [options] [image options]

Fetch/build options:
  --workdir DIR          Directory that will contain nuttx/ and apps/
  --nuttx-workdir DIR    Alias for --workdir
  --nuttx-url URL        NuttX repository URL
  --apps-url URL         NuttX apps repository URL
  --nuttx-ref REF        NuttX branch/tag/commit, default: master
  --apps-ref REF         apps branch/tag/commit, default: same as --nuttx-ref
  --update              Fetch and fast-forward existing repositories
  --out-dir DIR          Output directory for the RP2350 image
  --copy-uf2 DIR         Copy the finished UF2 to this mounted BOOTSEL volume
  --uf2-dir DIR          Alias for --copy-uf2
  --help                 Show this help text

Common image options passed through to nuttx-rp2350-pizero-image.sh:
  --with-i2s-audio
  --with-usb-host-hub
  --with-usb-ethernet
  --without-dvi-smoke
  --autorun-gfx-demo --allow-blind-autorun
  --autorun-dvi-smoke --allow-blind-autorun

Environment:
  FB_NUTTX_RP2350_PIZERO_WORKDIR
                         Default workdir when --workdir is omitted
  NUTTX_WORKDIR          Alternate default workdir
  FBC                    Host fbc used by the image builder
  JOBS                   Parallel make jobs

Typical board preparation without writing the board:

  fbc-nuttx-rp2350-pizero-firmware \\
    --workdir \$HOME/fbxl-nuttx-rp2350-pizero

Typical BOOTSEL/write-mode copy:

  fbc-nuttx-rp2350-pizero-firmware \\
    --workdir \$HOME/fbxl-nuttx-rp2350-pizero \\
    --copy-uf2 /media/\$USER/RPI-RP2
EOF
}

##############################################################################
# Options
##############################################################################

WORKDIR="$(default_workdir)"
NUTTX_URL="${NUTTX_URL:-https://github.com/apache/nuttx.git}"
APPS_URL="${APPS_URL:-https://github.com/apache/nuttx-apps.git}"
NUTTX_REF="${NUTTX_REF:-master}"
APPS_REF="${APPS_REF:-}"
OUT_DIR="${FB_NUTTX_RP2350_PIZERO_OUT_DIR:-$(default_out_dir)}"
COPY_UF2_DIR=""
UPDATE_REPOS=0
IMAGE_ARGS=()

while [ "$#" -gt 0 ]; do
    case "$1" in
        --workdir|--nuttx-workdir)
            [ "$#" -ge 2 ] || die "$1 requires a directory"
            WORKDIR="$2"
            shift 2
            ;;
        --nuttx-url)
            [ "$#" -ge 2 ] || die "--nuttx-url requires a URL"
            NUTTX_URL="$2"
            shift 2
            ;;
        --apps-url)
            [ "$#" -ge 2 ] || die "--apps-url requires a URL"
            APPS_URL="$2"
            shift 2
            ;;
        --nuttx-ref)
            [ "$#" -ge 2 ] || die "--nuttx-ref requires a ref"
            NUTTX_REF="$2"
            shift 2
            ;;
        --apps-ref)
            [ "$#" -ge 2 ] || die "--apps-ref requires a ref"
            APPS_REF="$2"
            shift 2
            ;;
        --update)
            UPDATE_REPOS=1
            shift
            ;;
        --out-dir)
            [ "$#" -ge 2 ] || die "--out-dir requires a directory"
            OUT_DIR="$2"
            shift 2
            ;;
        --copy-uf2|--uf2-dir)
            [ "$#" -ge 2 ] || die "$1 requires a directory"
            COPY_UF2_DIR="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        --)
            shift
            while [ "$#" -gt 0 ]; do
                IMAGE_ARGS+=("$1")
                shift
            done
            ;;
        *)
            IMAGE_ARGS+=("$1")
            shift
            ;;
    esac
done

[ -n "$APPS_REF" ] || APPS_REF="$NUTTX_REF"

case "$WORKDIR" in
    "") die "workdir must not be empty" ;;
esac

NUTTX_DIR="$WORKDIR/nuttx"
APPS_DIR="$WORKDIR/apps"
IMAGE_SCRIPT="$ROOT/build_scripts/nuttx-rp2350-pizero-image.sh"

[ -f "$IMAGE_SCRIPT" ] || die "missing RP2350 image script: $IMAGE_SCRIPT"

##############################################################################
# Fetch NuttX
##############################################################################

clone_repo "$NUTTX_URL" "$NUTTX_REF" "$NUTTX_DIR"
clone_repo "$APPS_URL" "$APPS_REF" "$APPS_DIR"

if [ "$UPDATE_REPOS" -eq 1 ]; then
    update_repo "$NUTTX_DIR" "$NUTTX_REF"
    update_repo "$APPS_DIR" "$APPS_REF"
fi

##############################################################################
# Build image
##############################################################################

run bash "$IMAGE_SCRIPT" \
    --nuttx-workdir "$WORKDIR" \
    --out-dir "$OUT_DIR" \
    "${IMAGE_ARGS[@]}"

UF2="$OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.uf2"

if [ -n "$COPY_UF2_DIR" ]; then
    copy_uf2_to_volume "$UF2" "$COPY_UF2_DIR"
fi

echo "FREEBASIC_NUTTX_RP2350_PIZERO_FIRMWARE_READY"
echo "WORKDIR: $WORKDIR"
echo "NUTTX:   $NUTTX_DIR"
echo "APPS:    $APPS_DIR"
echo "OUT:     $OUT_DIR"
if [ -f "$UF2" ]; then
    echo "UF2:     $UF2"
fi

# end of fbc-nuttx-rp2350-pizero-firmware.sh
