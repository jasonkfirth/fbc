#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/build_scripts" ] && { [ -f "$SEARCH_DIR/GNUmakefile" ] || [ -f "$SEARCH_DIR/makefile" ] || [ -f "$SEARCH_DIR/Makefile" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }

cd "$ROOT"
. "$ROOT/build_scripts/build-success-cleanup.sh"

CLEANUP_SUCCESS=0
CLEANUP_DIRS=(
    "$ROOT/.build-docs"
    "$ROOT/.build-debianubuntu"
    "$ROOT/.build-debianubuntu-xbox"
)

cleanup_build_roots() {
    local path

    [ "$CLEANUP_SUCCESS" -eq 1 ] || return 0

    for path in "${CLEANUP_DIRS[@]}"; do
        [ -n "$path" ] || continue
        fb_remove_build_tree "$ROOT" "$path" || true
    done
}

trap cleanup_build_roots EXIT

die() { echo "ERROR: $*" >&2; exit 1; }

usage() {
    cat <<EOF
Usage: ./build_scripts/linux-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit to one distro family (debian, ubuntu, alpine, postmarketos)
  --arch ARCH       Limit to one architecture
  --jobs N          Maximum make jobs for native Docker builds
  --keep-going      Continue after per-matrix failures
  --skip-host-deps  Skip host dependency installation in child matrices
  --skip-bootstrap  Reuse existing source bootstrap tarballs
  --skip-docs       Skip the offline documentation tool build preflight
  --list            Show the configured Linux distro targets
  --help            Show this help text

This is the top-level Linux matrix driver. It delegates to the distro-family
matrix scripts under build_scripts/.
EOF
}

DISTRO_FILTER=""
PASSTHRU=()
KEEP_GOING=0
LIST_ONLY=0
SKIP_DOCS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --distro)
            DISTRO_FILTER="$2"
            PASSTHRU+=("$1" "$2")
            shift 2
            ;;
        --arch|--jobs)
            PASSTHRU+=("$1" "$2")
            shift 2
            ;;
        --keep-going)
            KEEP_GOING=1
            PASSTHRU+=("$1")
            shift
            ;;
        --skip-host-deps|--skip-bootstrap|--serial)
            PASSTHRU+=("$1")
            shift
            ;;
        --skip-docs)
            SKIP_DOCS=1
            shift
            ;;
        --list)
            LIST_ONLY=1
            PASSTHRU+=("$1")
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

run_matrix() {
    local script="$1"

    echo
    echo "============================================================"
    echo "Running $script"
    echo "============================================================"

    "$ROOT/build_scripts/$script" "${PASSTHRU[@]}"
}

SCRIPT_LIST=()

case "$DISTRO_FILTER" in
    "")
        SCRIPT_LIST=(
            debianubuntu-build-freebasic-matrix.sh
            alpine-freebasic-matrix-build.sh
        )
        ;;
    debian|ubuntu)
        SCRIPT_LIST=(debianubuntu-build-freebasic-matrix.sh)
        ;;
    alpine|postmarketos)
        SCRIPT_LIST=(alpine-freebasic-matrix-build.sh)
        ;;
    *)
        die "unsupported Linux distro family: $DISTRO_FILTER"
        ;;
esac

doc_fbc() {
    if [ -x "$ROOT/bin/fbc" ]; then
        echo "$ROOT/bin/fbc -i $ROOT/inc -p $ROOT/.build-docs/lib"
        return 0
    fi

    command -v fbc >/dev/null 2>&1 || die "cannot build documentation tools without fbc"
    command -v fbc
}

find_runtime_libcurl() {
    local candidate

    for candidate in \
        /lib/x86_64-linux-gnu/libcurl.so.4 \
        /usr/lib/x86_64-linux-gnu/libcurl.so.4 \
        /lib/aarch64-linux-gnu/libcurl.so.4 \
        /usr/lib/aarch64-linux-gnu/libcurl.so.4 \
        /usr/lib64/libcurl.so.4 \
        /usr/lib/libcurl.so.4 \
        /lib/libcurl.so.4
    do
        if [ -e "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done

    if command -v ldconfig >/dev/null 2>&1; then
        ldconfig -p 2>/dev/null | awk '/libcurl\.so\.4[[:space:]]/ { print $NF; exit }'
    fi
}

prepare_doc_link_libs() {
    local libcurl

    mkdir -p "$ROOT/.build-docs/lib"

    if [ ! -e "$ROOT/.build-docs/lib/libcurl.so" ]; then
        libcurl="$(find_runtime_libcurl || true)"
        if [ -n "$libcurl" ]; then
            ln -sfn "$libcurl" "$ROOT/.build-docs/lib/libcurl.so"
        fi
    fi
}

build_doc_tools() {
    local fbc

    [ "$SKIP_DOCS" -eq 0 ] || return 0

    prepare_doc_link_libs
    fbc="$(doc_fbc)"

    echo
    echo "============================================================"
    echo "Building documentation tools"
    echo "============================================================"
    echo "Command: make -C doc FBC=$fbc"
    echo "Note: this compiles the offline doc tools only; it does not refresh doc/manual/cache."

    MAKEFLAGS= make -C "$ROOT/doc" "FBC=$fbc"
}

failures=0

if [ "$LIST_ONLY" -eq 0 ]; then
    build_doc_tools
fi

for script in "${SCRIPT_LIST[@]}"; do
    if ! run_matrix "$script"; then
        failures=$((failures + 1))
        if [ "$KEEP_GOING" -eq 0 ] && [ "$LIST_ONLY" -eq 0 ]; then
            break
        fi
    fi
done

if [ "$failures" -ne 0 ]; then
    echo
    echo "============================================================"
    echo "LINUX MATRIX FINISHED WITH FAILURES: $failures"
    echo "============================================================"
    exit 1
fi

if [ "$LIST_ONLY" -eq 0 ]; then
    CLEANUP_SUCCESS=1

    echo
    echo "============================================================"
    echo "ALL LINUX MATRIX BUILDS FINISHED"
    echo "============================================================"
    ls -R out/linux 2>/dev/null || true
fi
