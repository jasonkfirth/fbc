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

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }
need_cmd() { command -v "$1" >/dev/null 2>&1 || die "missing command: $1"; }

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/linux-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit the matrix to one distro family (for now: debian, ubuntu)
  --arch ARCH       Limit the matrix to one Debian-style CPU arch (amd64, arm64, ...)
  --jobs N          Maximum parallel Docker jobs
  --serial          Build entries serially
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --skip-bootstrap  Reuse existing source bootstrap tarballs
  --list            Show the currently configured distro targets
  --help            Show this help text

This script is the Linux matrix driver. It currently wires Debian/Ubuntu
targets first, with per-distro script dispatch so additional Linux package
builders can be added beside them over time.
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
ARCH_FILTER=""
SERIAL=0
KEEP_GOING=0
SKIP_HOST_DEPS=0
SKIP_BOOTSTRAP=0
LIST_ONLY=0

if command -v nproc >/dev/null 2>&1; then
    MAX_JOBS="$(nproc)"
else
    MAX_JOBS=1
fi

while [ $# -gt 0 ]; do
    case "$1" in
        --distro) DISTRO_FILTER="$2"; shift 2 ;;
        --arch) ARCH_FILTER="$2"; shift 2 ;;
        --jobs) MAX_JOBS="$2"; shift 2 ;;
        --serial) SERIAL=1; shift ;;
        --keep-going) KEEP_GOING=1; shift ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        --skip-bootstrap) SKIP_BOOTSTRAP=1; shift ;;
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
# Host dependency installation
##############################################################################

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing host dependencies via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends \
            docker.io \
            qemu-user-static \
            binfmt-support \
            ca-certificates \
            curl wget git \
            build-essential make pkg-config rsync \
            tar xz-utils gzip zip unzip \
            dos2unix jq python3 perl bc
        return 0
    fi

    if command -v pacman >/dev/null 2>&1; then
        msg "installing host dependencies via pacman"
        run_root pacman -Sy --noconfirm \
            docker qemu-user-static binfmt-qemu-static \
            ca-certificates curl wget git \
            base-devel pkgconf rsync \
            tar xz gzip zip unzip \
            dos2unix jq python python-perl bc
        return 0
    fi

    if command -v dnf >/dev/null 2>&1; then
        msg "installing host dependencies via dnf"
        run_root dnf install -y \
            docker qemu-user-static \
            ca-certificates curl wget git \
            gcc gcc-c++ make pkgconf-pkg-config rsync \
            tar xz gzip zip unzip \
            dos2unix jq python3 perl bc
        return 0
    fi

    if command -v yum >/dev/null 2>&1; then
        msg "installing host dependencies via yum"
        run_root yum install -y \
            docker qemu-user-static \
            ca-certificates curl wget git \
            gcc gcc-c++ make pkgconfig rsync \
            tar xz gzip zip unzip \
            dos2unix jq python3 perl bc
        return 0
    fi

    if command -v zypper >/dev/null 2>&1; then
        msg "installing host dependencies via zypper"
        run_root zypper --non-interactive install \
            docker qemu-user-static \
            ca-certificates curl wget git \
            gcc gcc-c++ make pkgconf rsync \
            tar xz gzip zip unzip \
            dos2unix jq python3 perl bc
        return 0
    fi

    die "unsupported host package manager; install Docker, qemu-user-static, binfmt, make, rsync, tar, xz, jq, python3, perl, bc manually"
}

##############################################################################
# Bootstrap generation
##############################################################################

ensure_host_compiler() {
    if [ ! -x "./bin/fbc" ]; then
        msg "building host compiler for bootstrap emission"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$MAX_JOBS"
    fi

    [ -x "./bin/fbc" ] || die "host compiler not available"
}

bootstrap_mapping() {
    case "$1" in
        amd64)   echo "linux-x86_64 linux-amd64" ;;
        i386)    echo "linux-x86 linux-i386" ;;
        arm64)   echo "linux-aarch64 linux-arm64" ;;
        armhf)   echo "linux-arm linux-armhf" ;;
        armel)   echo "linux-arm linux-armel" ;;
        ppc64el) echo "linux-powerpc64le linux-ppc64el" ;;
        s390x)   echo "linux-s390x linux-s390x" ;;
        riscv64) echo "linux-riscv64 linux-riscv64" ;;
        loong64) echo "linux-loongarch64 linux-loongarch64" ;;
        *)
            die "unsupported bootstrap arch: $1"
            ;;
    esac
}

build_bootstrap_for_arch() {
    local debarch="$1"
    local fbc_target
    local dir_key
    local pkg

    read -r fbc_target dir_key <<EOF
$(bootstrap_mapping "$debarch")
EOF

    pkg="FreeBASIC-${VERSION}-source-bootstrap-${dir_key}.tar.xz"

    msg "building source bootstrap tarball for $debarch"

    rm -f "$pkg"
    rm -rf "bootstrap/${dir_key}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        FBC_TARGET="$fbc_target" \
        FBTARGET_DIR_OVERRIDE="$dir_key" \
        bootstrap-dist-target \
        -j"$MAX_JOBS"

    [ -f "$pkg" ] || die "missing bootstrap archive: $pkg"
}

##############################################################################
# Matrix definition
##############################################################################

LINUX_ARCHES=(
    amd64
    arm64
    armhf
    ppc64el
    s390x
    riscv64
)

DISTRO_TARGETS=(
    "ubuntu|22.04|jammy|debianubuntu-build-freebasic.sh"
    "ubuntu|24.04|noble|debianubuntu-build-freebasic.sh"
    "ubuntu|24.10|oracular|debianubuntu-build-freebasic.sh"
    "ubuntu|25.04|plucky|debianubuntu-build-freebasic.sh"
    "ubuntu|25.10|questing|debianubuntu-build-freebasic.sh"
    "ubuntu|26.04|resolute|debianubuntu-build-freebasic.sh"
    "debian|12|bookworm|debianubuntu-build-freebasic.sh"
    "debian|13|trixie|debianubuntu-build-freebasic.sh"
    "debian|sid|sid|debianubuntu-build-freebasic.sh"
)

docker_platform_for_arch() {
    case "$1" in
        amd64) echo "linux/amd64" ;;
        i386) echo "linux/386" ;;
        arm64) echo "linux/arm64" ;;
        armhf) echo "linux/arm/v7" ;;
        armel) echo "linux/arm/v6" ;;
        ppc64el) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        loong64) echo "linux/loong64" ;;
        *)
            die "unsupported Docker platform arch: $1"
            ;;
    esac
}

##############################################################################
# Listing
##############################################################################

if [ "$LIST_ONLY" -eq 1 ]; then
    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r distro tag codename script_name <<EOF
$entry
EOF
        echo "${distro}|${tag}|${codename}|${script_name}"
    done
    exit 0
fi

##############################################################################
# Build prep
##############################################################################

install_host_deps

need_cmd docker
need_cmd tar
need_cmd rsync
need_cmd "$MAKE_CMD"

run_root docker run --rm --privileged tonistiigi/binfmt --install all

mkdir -p out/linux

if [ "$SKIP_BOOTSTRAP" -eq 0 ]; then
    ensure_host_compiler

    if [ -n "$ARCH_FILTER" ]; then
        build_bootstrap_for_arch "$ARCH_FILTER"
    else
        for debarch in "${LINUX_ARCHES[@]}"; do
            build_bootstrap_for_arch "$debarch"
        done
    fi
fi

##############################################################################
# Build execution
##############################################################################

build_one() {
    local entry="$1"
    local distro
    local tag
    local codename
    local script_name
    local arch
    local platform
    local image
    local outdir

    IFS="|" read -r distro tag codename script_name arch <<EOF
$entry
EOF

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 0
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 0
    fi

    platform="$(docker_platform_for_arch "$arch")"
    image="${distro}:${tag}"
    outdir="$ROOT/out/linux/${distro}/${codename}/${arch}"

    mkdir -p "$outdir"

    echo
    echo "============================================================"
    echo "Building ${distro}/${codename} (${arch})"
    echo "Docker image: ${image}"
    echo "Docker platform: ${platform}"
    echo "Script: build_scripts/${script_name}"
    echo "============================================================"

    run_root docker pull --platform "$platform" "$image"

    if ! run_root docker run --rm \
        --platform "$platform" \
        -e DEBIAN_FRONTEND=noninteractive \
        -e JOBS="$MAX_JOBS" \
        -v "$ROOT:/work" \
        -w /work \
        "$image" \
        bash -lc "/work/build_scripts/${script_name} --no-build" \
        &> "$outdir/docker_build.log"; then

        echo "BUILD FAILED: ${distro}/${codename} (${arch})"

        if [ "$KEEP_GOING" -eq 0 ]; then
            exit 1
        fi

        return 0
    fi

    echo "SUCCESS: ${distro}/${codename} (${arch})"
}

BUILD_MATRIX=()

for distro_entry in "${DISTRO_TARGETS[@]}"; do
    IFS="|" read -r distro tag codename script_name <<EOF
$distro_entry
EOF

    for arch in "${LINUX_ARCHES[@]}"; do
        BUILD_MATRIX+=("${distro}|${tag}|${codename}|${script_name}|${arch}")
    done
done

running=0

for entry in "${BUILD_MATRIX[@]}"; do
    if [ "$SERIAL" -eq 1 ]; then
        build_one "$entry"
        continue
    fi

    build_one "$entry" &
    running=$((running + 1))

    if [ "$running" -ge "$MAX_JOBS" ]; then
        wait -n
        running=$((running - 1))
    fi
done

wait

echo
echo "============================================================"
echo "ALL LINUX BUILDS FINISHED"
echo "============================================================"

ls -R out/linux
