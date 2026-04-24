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

log_has_missing_manifest() {
    local log="$1"

    [ -f "$log" ] || return 1
    grep -Eq 'no matching manifest|manifest unknown|not found: manifest' "$log"
}

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
Usage: ./build_scripts/alpine-freebasic-matrix-build.sh [options]

Options:
  --distro NAME     Limit the matrix to one distro family (alpine, postmarketos)
  --arch ARCH       Limit the matrix to one Alpine arch
  --jobs N          Maximum make jobs for native Docker builds
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --skip-bootstrap  Reuse existing source bootstrap tarballs
  --list            Show the configured Alpine targets
  --help            Show this help text
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
ARCH_FILTER=""
KEEP_GOING=0
SKIP_HOST_DEPS=0
SKIP_BOOTSTRAP=0
LIST_ONLY=0

if command -v nproc >/dev/null 2>&1; then
    MAKE_JOBS="$(nproc)"
else
    MAKE_JOBS=1
fi

while [ $# -gt 0 ]; do
    case "$1" in
        --distro) DISTRO_FILTER="$2"; shift 2 ;;
        --arch) ARCH_FILTER="$2"; shift 2 ;;
        --jobs) MAKE_JOBS="$2"; shift 2 ;;
        --serial) shift ;;
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

case "$MAKE_JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "alpine" ] && [ "$DISTRO_FILTER" != "postmarketos" ]; then
    if [ "$LIST_ONLY" -eq 1 ]; then
        exit 0
    fi
    die "unsupported distro for apk-family matrix: $DISTRO_FILTER"
fi

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

    die "unsupported host package manager; install Docker, qemu-user-static, binfmt, make, rsync, tar, xz, jq, python3, perl, bc manually"
}

##############################################################################
# Bootstrap generation
##############################################################################

ensure_host_compiler() {
    if [ ! -x "./bin/fbc" ]; then
        msg "building host compiler for bootstrap emission"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$MAKE_JOBS"
    fi

    [ -x "./bin/fbc" ] || die "host compiler not available"
}

bootstrap_mapping() {
    case "$1" in
        x86_64)  echo "linux-x86_64 linux-x86_64" ;;
        x86)     echo "linux-x86 linux-x86" ;;
        aarch64) echo "linux-aarch64 linux-aarch64" ;;
        armv7)   echo "linux-arm linux-arm" ;;
        ppc64le) echo "linux-powerpc64le linux-powerpc64le" ;;
        s390x)   echo "linux-s390x linux-s390x" ;;
        riscv64) echo "linux-riscv64 linux-riscv64" ;;
        *)
            die "unsupported Alpine bootstrap arch: $1"
            ;;
    esac
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

    msg "building source bootstrap tarball for Alpine $arch"

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

##############################################################################
# Matrix definition
##############################################################################

ALPINE_ARCHES=(
    x86_64
    aarch64
    armv7
    ppc64le
    s390x
    riscv64
)

APK_TARGETS=(
    "alpine|alpine:3.23|3.23|3.23|alpine-freebasic-build.sh"
    "alpine|alpine:3.22|3.22|3.22|alpine-freebasic-build.sh"
    "alpine|alpine:3.21|3.21|3.21|alpine-freebasic-build.sh"
    "alpine|alpine:edge|edge|edge|alpine-freebasic-build.sh"
    "postmarketos|adamthiede/postmarketos:edge|edge|edge|alpine-freebasic-build.sh"
)

docker_platform_for_arch() {
    case "$1" in
        x86_64) echo "linux/amd64" ;;
        x86) echo "linux/386" ;;
        aarch64) echo "linux/arm64" ;;
        armv7) echo "linux/arm/v7" ;;
        ppc64le) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        *)
            die "unsupported Docker platform arch: $1"
            ;;
    esac
}

host_docker_platform() {
    local machine

    machine="$(uname -m)"

    case "$machine" in
        x86_64|amd64) echo "linux/amd64" ;;
        i386|i686) echo "linux/386" ;;
        aarch64|arm64) echo "linux/arm64" ;;
        armv7l) echo "linux/arm/v7" ;;
        ppc64le) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        *)
            die "unsupported host machine for Docker platform detection: $machine"
            ;;
    esac
}

make_jobs_for_platform() {
    local platform="$1"
    local host_platform="$2"

    if [ "$platform" = "$host_platform" ]; then
        echo "$MAKE_JOBS"
    else
        echo 1
    fi
}

##############################################################################
# Listing
##############################################################################

if [ "$LIST_ONLY" -eq 1 ]; then
    for entry in "${APK_TARGETS[@]}"; do
        IFS="|" read -r distro image tag codename script_name <<EOF
$entry
EOF
        if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
            continue
        fi
        echo "${distro}|${image}|${tag}|${codename}|${script_name}"
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
HOST_PLATFORM="$(host_docker_platform)"
echo "==> host Docker platform: $HOST_PLATFORM"
echo "==> native make jobs: $MAKE_JOBS"

if [ "$SKIP_BOOTSTRAP" -eq 0 ]; then
    ensure_host_compiler

    if [ -n "$ARCH_FILTER" ]; then
        build_bootstrap_for_arch "$ARCH_FILTER"
    else
        for arch in "${ALPINE_ARCHES[@]}"; do
            build_bootstrap_for_arch "$arch"
        done
    fi
fi

##############################################################################
# Build execution
##############################################################################

build_one() {
    local entry="$1"
    local distro
    local image
    local tag
    local codename
    local script_name
    local arch
    local platform
    local outdir
    local build_jobs

    IFS="|" read -r distro image tag codename script_name arch <<EOF
$entry
EOF

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 0
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 0
    fi

    platform="$(docker_platform_for_arch "$arch")"
    build_jobs="$(make_jobs_for_platform "$platform" "$HOST_PLATFORM")"
    outdir="$ROOT/out/linux/${distro}/${codename}/${arch}"

    mkdir -p "$outdir"

    echo
    echo "============================================================"
    echo "Building ${distro}/${codename} (${arch})"
    echo "Docker image: ${image}"
    echo "Docker platform: ${platform}"
    echo "Make jobs: ${build_jobs}"
    echo "Script: build_scripts/${script_name}"
    echo "============================================================"

    if ! {
        run_root docker pull --platform "$platform" "$image" &&
        run_root docker run --rm \
            --platform "$platform" \
            -e FBC_PACKAGE_DISTRO_ID="$distro" \
            -e FBC_PACKAGE_CODENAME="$codename" \
            -e BUILDROOT="/work/.build-alpine/${distro}/${codename}/${arch}" \
            -e JOBS="$build_jobs" \
            -v "$ROOT:/work" \
            -w /work \
            "$image" \
            sh -lc "apk add --no-cache bash && /work/build_scripts/${script_name} --no-build"
    } &> "$outdir/docker_build.log"; then
        if log_has_missing_manifest "$outdir/docker_build.log"; then
            echo "SKIPPED: ${distro}/${codename} (${arch}) has no Docker image for ${platform}"
            echo "Log: $outdir/docker_build.log"
            return 0
        fi

        echo "BUILD FAILED: ${distro}/${codename} (${arch})"
        echo "Log: $outdir/docker_build.log"

        return 1
    fi

    echo "SUCCESS: ${distro}/${codename} (${arch})"
}

entry_matches_filters() {
    local entry="$1"
    local distro
    local arch

    IFS="|" read -r distro _ _ _ _ arch <<EOF
$entry
EOF

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 1
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 1
    fi

    return 0
}

BUILD_MATRIX=()

for distro_entry in "${APK_TARGETS[@]}"; do
    IFS="|" read -r distro image tag codename script_name <<EOF
$distro_entry
EOF

    for arch in "${ALPINE_ARCHES[@]}"; do
        BUILD_MATRIX+=("${distro}|${image}|${tag}|${codename}|${script_name}|${arch}")
    done
done

failures=0

for entry in "${BUILD_MATRIX[@]}"; do
    entry_matches_filters "$entry" || continue

    if ! build_one "$entry"; then
        failures=$((failures + 1))
        if [ "$failures" -ne 0 ] && [ "$KEEP_GOING" -eq 0 ]; then
            break
        fi
    fi
done

if [ "$failures" -ne 0 ]; then
    echo
    echo "============================================================"
    echo "APK-FAMILY BUILDS FINISHED WITH FAILURES: $failures"
    echo "============================================================"
    ls -R out/linux/alpine out/linux/postmarketos 2>/dev/null || true
    exit 1
fi

echo
echo "============================================================"
echo "ALL APK-FAMILY BUILDS FINISHED"
echo "============================================================"

ls -R out/linux/alpine out/linux/postmarketos 2>/dev/null || true
