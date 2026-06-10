#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: rpm-cross-build-freebasic-matrix.sh
#
# Purpose:
#
#     Build the RPM-family FreeBASIC package matrix.
#
# Responsibilities:
#
#     * define Fedora, Rocky, AlmaLinux, and openSUSE RPM targets
#     * prepare FreeBASIC bootstrap source tarballs for each target CPU
#     * run the one-package RPM builder in clean Docker containers
#     * write artifacts under out/linux/<distro>/<release>/<arch>
#
# This file intentionally does NOT contain:
#
#     * RPM spec policy for an individual package
#     * target package validation
#     * Debian/APK/Slackware package policy
#

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
CLEANUP_SUCCESS=0
CLEANUP_DIRS=("$ROOT/.build-rpm")

cleanup_build_roots() {
    local path

    [ "$CLEANUP_SUCCESS" -eq 1 ] || return 0

    for path in "${CLEANUP_DIRS[@]}"; do
        [ -n "$path" ] || continue
        rm -rf "$path" 2>/dev/null || true
    done
}

trap cleanup_build_roots EXIT

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }
need_cmd() { command -v "$1" >/dev/null 2>&1 || die "missing command: $1"; }

log_has_missing_manifest() {
    local log="$1"

    [ -f "$log" ] || return 1
    grep -Eq 'no matching manifest|manifest unknown|not found: manifest' "$log"
}

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif [ "${1:-}" = "docker" ] && docker ps >/dev/null 2>&1; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/rpm-cross-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit the matrix to one distro family
  --release NAME    Limit the matrix to one release
  --arch ARCH       Limit the target CPU architecture
  --jobs N          Maximum make jobs for Docker builds
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --skip-bootstrap  Reuse existing source bootstrap tarballs
  --execute         Attempt package builds
  --list            Show the configured RPM targets
  --help            Show this help text
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
RELEASE_FILTER=""
ARCH_FILTER=""
KEEP_GOING=0
SKIP_HOST_DEPS=0
SKIP_BOOTSTRAP=0
EXECUTE=0
LIST_ONLY=0

if command -v nproc >/dev/null 2>&1; then
    MAKE_JOBS="$(nproc)"
else
    MAKE_JOBS=1
fi

while [ $# -gt 0 ]; do
    case "$1" in
        --distro) DISTRO_FILTER="$2"; shift 2 ;;
        --release) RELEASE_FILTER="$2"; shift 2 ;;
        --arch) ARCH_FILTER="$2"; shift 2 ;;
        --jobs) MAKE_JOBS="$2"; shift 2 ;;
        --keep-going) KEEP_GOING=1; shift ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        --skip-bootstrap) SKIP_BOOTSTRAP=1; shift ;;
        --execute) EXECUTE=1; shift ;;
        --list) LIST_ONLY=1; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

case "$MAKE_JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

##############################################################################
# Tooling
##############################################################################

if command -v gmake >/dev/null 2>&1; then
    MAKE_CMD="gmake"
else
    MAKE_CMD="make"
fi

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"

##############################################################################
# Matrix definition
##############################################################################

RPM_COMMON_ARCHES=(
    x86_64
    aarch64
    ppc64le
    s390x
)

RPM_RISCV_ARCHES=(
    "${RPM_COMMON_ARCHES[@]}"
    riscv64
)

RPM_TARGETS=(
    "fedora|fedora:44|44"
    "fedora|fedora:rawhide|rawhide"
    "rocky|rockylinux:10|10"
    "rocky|rockylinux:9|9"
    "almalinux|almalinux:10|10"
    "almalinux|almalinux:9|9"
    "opensuse|opensuse/tumbleweed|tumbleweed"
)

target_arches() {
    local distro="$1"
    local release="$2"

    case "$distro/$release" in
        rocky/10)
            printf '%s\n' "${RPM_RISCV_ARCHES[@]}"
            ;;
        *)
            printf '%s\n' "${RPM_COMMON_ARCHES[@]}"
            ;;
    esac
}

docker_platform_for_arch() {
    case "$1" in
        x86_64) echo "linux/amd64" ;;
        aarch64) echo "linux/arm64" ;;
        ppc64le) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        *)
            die "unsupported Docker platform arch: $1"
            ;;
    esac
}

bootstrap_mapping() {
    case "$1" in
        x86_64)  echo "linux-x86_64 linux-x86_64" ;;
        aarch64) echo "linux-aarch64 linux-aarch64" ;;
        ppc64le) echo "linux-powerpc64le linux-powerpc64le" ;;
        s390x)   echo "linux-s390x linux-s390x" ;;
        riscv64) echo "linux-riscv64 linux-riscv64" ;;
        *)
            die "unsupported RPM bootstrap arch: $1"
            ;;
    esac
}

target_matches_filters() {
    local distro="$1"
    local release="$2"
    local arch="$3"

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 1
    fi

    if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$release" ]; then
        return 1
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 1
    fi

    return 0
}

##############################################################################
# Listing
##############################################################################

list_plan() {
    local entry
    local distro
    local image
    local release
    local arch
    local outdir

    for entry in "${RPM_TARGETS[@]}"; do
        IFS="|" read -r distro image release <<EOF
$entry
EOF
        while IFS= read -r arch; do
            target_matches_filters "$distro" "$release" "$arch" || continue
            outdir="$ROOT/out/linux/${distro}/${release}/${arch}"
            printf 'rpm|%s|%s|%s|%s|%s\n' "$distro" "$release" "$arch" "$image" "$outdir"
        done < <(target_arches "$distro" "$release")
    done
}

if [ "$EXECUTE" -eq 0 ] || [ "$LIST_ONLY" -eq 1 ]; then
    list_plan
    if [ "$LIST_ONLY" -eq 0 ]; then
        echo
        echo "==> plan only"
        echo "==> pass --execute to build RPM packages"
    fi
    exit 0
fi

##############################################################################
# Host dependency installation
##############################################################################

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing host dependencies via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends \
            docker.io qemu-user-static binfmt-support ca-certificates \
            build-essential make rsync tar xz-utils
        return 0
    fi

    if command -v pacman >/dev/null 2>&1; then
        msg "installing host dependencies via pacman"
        run_root pacman -Sy --noconfirm \
            docker qemu-user-static binfmt-qemu-static ca-certificates \
            base-devel rsync tar xz
        return 0
    fi

    if command -v dnf >/dev/null 2>&1; then
        msg "installing host dependencies via dnf"
        run_root dnf install -y \
            docker qemu-user-static ca-certificates \
            gcc gcc-c++ make rsync tar xz
        return 0
    fi

    die "unsupported host package manager; install Docker, qemu-user-static, make, rsync, tar, and xz manually"
}

##############################################################################
# Bootstrap generation
##############################################################################

ensure_host_compiler() {
    if ! ./bin/fbc -version >/dev/null 2>&1; then
        msg "building host compiler for bootstrap emission"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$MAKE_JOBS"
    fi

    ./bin/fbc -version >/dev/null 2>&1 || die "host compiler not available"
}

build_bootstrap_for_arch() {
    local arch="$1"
    local fbc_target
    local dir_key
    local pkg

    read -r fbc_target dir_key <<EOF
$(bootstrap_mapping "$arch")
EOF

    pkg="FreeBASIC-${VERSION}-source-bootstrap-${dir_key}.tar.xz"

    msg "building source bootstrap tarball for RPM $arch"

    rm -f "$pkg"
    rm -rf "bootstrap/${dir_key}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        FBC_TARGET="$fbc_target" \
        FBTARGET_DIR_OVERRIDE="$dir_key" \
        bootstrap-dist-target \
        -j"$MAKE_JOBS"

    [ -f "$pkg" ] || die "missing bootstrap archive: $pkg"
}

prepare_bootstraps() {
    local seen=""
    local row
    local distro
    local release
    local arch
    local image
    local outdir

    [ "$SKIP_BOOTSTRAP" -eq 0 ] || return 0

    ensure_host_compiler

    while IFS= read -r row; do
        IFS="|" read -r _ distro release arch image outdir <<EOF
$row
EOF
        case " $seen " in
            *" $arch "*) continue ;;
        esac
        seen="$seen $arch"
        build_bootstrap_for_arch "$arch"
    done < <(list_plan)
}

##############################################################################
# Build execution
##############################################################################

build_one() {
    local row="$1"
    local family
    local distro
    local release
    local arch
    local image
    local outdir
    local platform

    IFS="|" read -r family distro release arch image outdir <<EOF
$row
EOF

    platform="$(docker_platform_for_arch "$arch")"
    mkdir -p "$outdir"

    echo
    echo "============================================================"
    echo "Building ${distro}/${release} (${arch})"
    echo "Docker image: ${image}"
    echo "Docker platform: ${platform}"
    echo "Make jobs: ${MAKE_JOBS}"
    echo "============================================================"

    if ! {
        run_root docker pull --platform "$platform" "$image" &&
        run_root docker run --rm \
            --platform "$platform" \
            -e FBC_PACKAGE_DISTRO_ID="$distro" \
            -e FBC_PACKAGE_CODENAME="$release" \
            -e BUILDROOT="/work/.build-rpm/${distro}/${release}/${arch}" \
            -e JOBS="$MAKE_JOBS" \
            -v "$ROOT:/work" \
            -w /work \
            "$image" \
            bash -lc "/work/build_scripts/rpm-build-freebasic.sh --no-build"
    } &> "$outdir/docker_build.log"; then
        if log_has_missing_manifest "$outdir/docker_build.log"; then
            echo "SKIPPED: ${distro}/${release} (${arch}) has no Docker image for ${platform}"
            echo "Log: $outdir/docker_build.log"
            return 0
        fi

        echo "BUILD FAILED: ${distro}/${release} (${arch})"
        echo "Log: $outdir/docker_build.log"
        return 1
    fi

    echo "SUCCESS: ${distro}/${release} (${arch})"
}

##############################################################################
# Main
##############################################################################

install_host_deps

need_cmd docker
need_cmd "$MAKE_CMD"
run_root docker run --rm --privileged tonistiigi/binfmt --install all

prepare_bootstraps

failures=0

while IFS= read -r row; do
    [ -n "$row" ] || continue
    if ! build_one "$row"; then
        failures=$((failures + 1))
        if [ "$KEEP_GOING" -eq 0 ]; then
            break
        fi
    fi
done < <(list_plan)

if [ "$failures" -ne 0 ]; then
    echo
    echo "============================================================"
    echo "RPM BUILDS FINISHED WITH FAILURES: $failures"
    echo "============================================================"
    exit 1
fi

CLEANUP_SUCCESS=1

echo
echo "============================================================"
echo "ALL RPM BUILDS FINISHED"
echo "============================================================"

##############################################################################
# end of rpm-cross-build-freebasic-matrix.sh
##############################################################################
