#!/usr/bin/env bash
#
# Project: FreeBASIC MIPS Linux release workflow
# ----------------------------------------------
#
# File: debianubuntu-build-freebasic-mips.sh
#
# Purpose:
#
#     Build and qualify FreeBASIC for all supported MIPS Linux ABIs from a
#     Debian or Ubuntu host without depending on host cross-tool packages.
#
# Responsibilities:
#
#     - build the pinned MIPS toolchain and QEMU Docker image
#     - prepare an isolated, current source copy below out/mips
#     - run compiler, library, native smoke, fbctests, and package workflows
#     - leave only durable logs, compilers, and archives in out/mips
#
# This file intentionally does NOT contain:
#
#     - MIPS compiler or runtime build commands
#     - test batching implementation
#     - package layout implementation
#     - host APT repository modification
#
# Host dependency policy:
#
#     Ubuntu releases do not expose every MIPS cross package through every
#     repository configuration. The complete target supply chain therefore
#     lives in build_scripts/mips/Dockerfile. The host needs only Docker and
#     rsync.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

OUTPUT_ROOT="${MIPS_OUTPUT_ROOT:-$ROOT/out/mips}"
WORK_ROOT="${MIPS_WORK_ROOT:-$ROOT/out/mips/work}"
IMAGE="${MIPS_DOCKER_IMAGE:-freebasic-mips-toolchain:noble}"
TARGETS="${MIPS_TARGETS:-mips-linux-gnu,mipsel-linux-gnu,mips64-linux-gnuabi64,mips64el-linux-gnuabi64}"
SKIP_IMAGE=0
REUSE_WORKTREE=0
SKIP_TESTS=0
SKIP_PACKAGE=0
RESUME_TESTS=0

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

run() {
    echo "==> $*"
    "$@"
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

    # Four parallel compiler processes keep the default useful on large hosts
    # without making a memory-constrained workstation unnecessarily fragile.
    if [ "$jobs" -gt 4 ]; then
        jobs=4
    fi

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

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic-mips.sh [options]

Options:
  --targets LIST    Comma-separated GNU target triplets. Default: all four
  --jobs N          Parallel build jobs. Default: detected, capped at 4
  --skip-image      Reuse the existing pinned Docker image.
  --reuse-worktree  Reuse out/mips/work without refreshing the source copy.
  --skip-tests      Skip the complete QEMU fbctests pass.
  --skip-package    Skip tar.xz and ZIP package creation.
  --resume          Reuse saved successful per-directory fbctests logs.
  --work-dir DIR    Isolated source copy below out/mips.
  --out-dir DIR     Persistent output directory. Default: out/mips
  --image NAME      Docker image name. Default: $IMAGE
  -h, --help        Show this help.

Outputs:
  out/mips/native/TRIPLET/fbc
  out/mips/fbctests-batches/TRIPLET/*-{build,run}.log
  out/mips/packages/FreeBASIC-*-linux-mips*.{tar.xz,zip}

Host prerequisites, if they are not already installed:

  sudo apt-get update
  sudo apt-get install -y docker.io rsync
EOF
}

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
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        --skip-image)
            SKIP_IMAGE=1
            shift
            ;;
        --reuse-worktree)
            REUSE_WORKTREE=1
            shift
            ;;
        --skip-tests)
            SKIP_TESTS=1
            shift
            ;;
        --skip-package)
            SKIP_PACKAGE=1
            shift
            ;;
        --resume)
            RESUME_TESTS=1
            shift
            ;;
        --work-dir)
            require_value "$1" "${2-}"
            WORK_ROOT="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --image)
            require_value "$1" "${2-}"
            IMAGE="$2"
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

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

require_command docker
require_command realpath
require_command rsync

SOURCE_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"
[ -f "$SOURCE_EXCLUDES" ] || die "source copy exclusions not found: $SOURCE_EXCLUDES"

##############################################################################
# Guarded workspace preparation
##############################################################################

ROOT_REAL="$(realpath -m "$ROOT")"
OUTPUT_REAL="$(realpath -m "$OUTPUT_ROOT")"
WORK_REAL="$(realpath -m "$WORK_ROOT")"

[ "$OUTPUT_REAL" != / ] || die "refusing to use / as the output directory"
[ "$WORK_REAL" != / ] || die "refusing to use / as the work directory"
[ "$WORK_REAL" != "$ROOT_REAL" ] || die "the isolated work directory cannot be the source root"
[[ "$WORK_REAL" == "$OUTPUT_REAL"/* ]] ||
    die "the work directory must be below the selected MIPS output directory"

mkdir -p "$OUTPUT_REAL" "$WORK_REAL"
[ -w "$OUTPUT_REAL" ] || die "output directory is not writable: $OUTPUT_REAL"
[ -w "$WORK_REAL" ] || die "work directory is not writable: $WORK_REAL"

if [ "$REUSE_WORKTREE" -eq 0 ]; then
    msg "refreshing the isolated MIPS source tree"
    run rsync -a --delete --delete-excluded \
        --exclude-from="$SOURCE_EXCLUDES" \
        "$ROOT_REAL/" "$WORK_REAL/"
fi

##############################################################################
# Pinned supply chain and contained build
##############################################################################

if [ "$SKIP_IMAGE" -eq 0 ]; then
    msg "building the pinned MIPS compiler and QEMU image"
    run docker build \
        -f "$ROOT_REAL/build_scripts/mips/Dockerfile" \
        -t "$IMAGE" \
        "$ROOT_REAL/build_scripts/mips"
else
    docker image inspect "$IMAGE" >/dev/null 2>&1 ||
        die "Docker image does not exist: $IMAGE"
fi

container_args=(
    --targets "$TARGETS"
    --jobs "$JOBS"
)
if [ "$SKIP_TESTS" -eq 1 ]; then
    container_args+=(--skip-tests)
fi
if [ "$SKIP_PACKAGE" -eq 1 ]; then
    container_args+=(--skip-package)
fi
if [ "$RESUME_TESTS" -eq 1 ]; then
    container_args+=(--resume)
fi
if [ "$REUSE_WORKTREE" -eq 1 ]; then
    container_args+=(--incremental)
fi

msg "running the contained MIPS build and qualification workflow"
run docker run --rm \
    --user "$(id -u):$(id -g)" \
    --mount "type=bind,src=$WORK_REAL,dst=/work" \
    --mount "type=bind,src=$OUTPUT_REAL,dst=/mips-output" \
    "$IMAGE" \
    /work/build_scripts/mips/container-build.sh "${container_args[@]}"

msg "MIPS outputs are available below $OUTPUT_REAL"

# end of build_scripts/debianubuntu-build-freebasic-mips.sh
