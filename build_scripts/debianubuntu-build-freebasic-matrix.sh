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
Usage: ./build_scripts/debianubuntu-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit the matrix to one distro family (debian, ubuntu, raspbian)
  --arch ARCH       Limit the matrix to one Debian-style CPU arch (amd64, arm64, ...)
  --jobs N          Maximum make jobs for native Docker builds
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --skip-bootstrap  Reuse existing source bootstrap tarballs
  --no-android      Build packages without the freebasic-android profile
  --list            Show the currently configured distro targets
  --help            Show this help text

This script is the Debian/Ubuntu/Raspbian Linux package matrix driver.
Raspbian-only builds can be run with:
  ./build_scripts/debianubuntu-build-freebasic-matrix.sh --distro raspbian
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
NO_ANDROID=0
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
        --no-android) NO_ANDROID=1; shift ;;
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

if [ "$DISTRO_FILTER" = "raspbian" ] && [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "armhf" ]; then
    die "raspbian targets are armhf only"
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
        run "$MAKE_CMD" compiler -j"$MAKE_JOBS"
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
    local arm_arch="${2:-}"
    local fbc_target
    local dir_key
    local pkg
    local extra_make_args=()

    read -r fbc_target dir_key <<EOF
$(bootstrap_mapping "$debarch")
EOF

    case "$arm_arch" in
        "")
            ;;
        armv6+fp)
            extra_make_args=(
                ARM_VER=v6
                ARM_FLOAT_ABI=hf
                DEFAULT_CPUTYPE_ARM=FB_CPUTYPE_ARMV6_FP
            )
            ;;
        *)
            die "unsupported ARM bootstrap arch override: $arm_arch"
            ;;
    esac

    pkg="FreeBASIC-${VERSION}-source-bootstrap-${dir_key}.tar.xz"

    if [ -n "$arm_arch" ]; then
        msg "building source bootstrap tarball for $debarch ($arm_arch)"
    else
        msg "building source bootstrap tarball for $debarch"
    fi

    rm -f "$pkg"
    rm -rf "bootstrap/${dir_key}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        FBC_TARGET="$fbc_target" \
        FBTARGET_DIR_OVERRIDE="$dir_key" \
        "${extra_make_args[@]}" \
        bootstrap-dist-target \
        -j"$MAKE_JOBS"

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

RASPBIAN_ARCHES=(
    armhf
)

DISTRO_TARGETS=(
    "ubuntu|ubuntu:22.04|22.04|jammy|debianubuntu-build-freebasic.sh"
    "ubuntu|ubuntu:24.04|24.04|noble|debianubuntu-build-freebasic.sh"
    "ubuntu|ubuntu:24.10|24.10|oracular|debianubuntu-build-freebasic.sh"
    "ubuntu|ubuntu:25.04|25.04|plucky|debianubuntu-build-freebasic.sh"
    "ubuntu|ubuntu:25.10|25.10|questing|debianubuntu-build-freebasic.sh"
    "ubuntu|ubuntu:26.04|26.04|resolute|debianubuntu-build-freebasic.sh"
    "debian|debian:12|12|bookworm|debianubuntu-build-freebasic.sh"
    "debian|debian:13|13|trixie|debianubuntu-build-freebasic.sh"
    "debian|debian:sid|sid|sid|debianubuntu-build-freebasic.sh"
    "raspbian|badaix/raspios-lite:trixie|trixie|trixie|debianubuntu-build-freebasic.sh"
    "raspbian|badaix/raspios-lite:bookworm|bookworm|bookworm|debianubuntu-build-freebasic.sh"
    "raspbian|badaix/raspios-buster-armhf-lite:latest|buster|buster|debianubuntu-build-freebasic.sh"
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

host_docker_platform() {
    local machine

    machine="$(uname -m)"

    case "$machine" in
        x86_64|amd64) echo "linux/amd64" ;;
        i386|i686) echo "linux/386" ;;
        aarch64|arm64) echo "linux/arm64" ;;
        armv7l) echo "linux/arm/v7" ;;
        armv6l) echo "linux/arm/v6" ;;
        ppc64le) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        loongarch64) echo "linux/loong64" ;;
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

target_arches() {
    local distro="$1"

    case "$distro" in
        raspbian)
            printf '%s\n' "${RASPBIAN_ARCHES[@]}"
            ;;
        *)
            printf '%s\n' "${LINUX_ARCHES[@]}"
            ;;
    esac
}

host_outdir_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    case "$distro" in
        raspbian) echo "$ROOT/out/raspbian/${codename}/${arch}" ;;
        *) echo "$ROOT/out/linux/${distro}/${codename}/${arch}" ;;
    esac
}

container_outdir_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    case "$distro" in
        raspbian) echo "/work/out/raspbian/${codename}/${arch}" ;;
        *) echo "/work/out/linux/${distro}/${codename}/${arch}" ;;
    esac
}

arm_arch_for_target() {
    local distro="$1"
    local arch="$2"

    if [ "$distro" = "raspbian" ] && [ "$arch" = "armhf" ]; then
        echo "armv6+fp"
    fi
}

##############################################################################
# Listing
##############################################################################

if [ "$LIST_ONLY" -eq 1 ]; then
    for entry in "${DISTRO_TARGETS[@]}"; do
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

mkdir -p out/linux out/raspbian
HOST_PLATFORM="$(host_docker_platform)"
echo "==> host Docker platform: $HOST_PLATFORM"
echo "==> native make jobs: $MAKE_JOBS"

RASPBIAN_BOOTSTRAP_READY=0

if [ "$SKIP_BOOTSTRAP" -eq 0 ]; then
    ensure_host_compiler

    if [ -n "$ARCH_FILTER" ]; then
        if [ "$DISTRO_FILTER" = "raspbian" ]; then
            build_bootstrap_for_arch "$ARCH_FILTER" "$(arm_arch_for_target "$DISTRO_FILTER" "$ARCH_FILTER")"
            RASPBIAN_BOOTSTRAP_READY=1
        else
            build_bootstrap_for_arch "$ARCH_FILTER"
        fi
    elif [ "$DISTRO_FILTER" = "raspbian" ]; then
        for debarch in "${RASPBIAN_ARCHES[@]}"; do
            build_bootstrap_for_arch "$debarch" "$(arm_arch_for_target "$DISTRO_FILTER" "$debarch")"
        done
        RASPBIAN_BOOTSTRAP_READY=1
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
    local image
    local tag
    local codename
    local script_name
    local arch
    local platform
    local outdir
    local container_outdir
    local arm_arch
    local build_jobs
    local android_arg

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
    outdir="$(host_outdir_for_target "$distro" "$codename" "$arch")"
    container_outdir="$(container_outdir_for_target "$distro" "$codename" "$arch")"
    arm_arch="$(arm_arch_for_target "$distro" "$arch")"
    android_arg=""
    if [ "$NO_ANDROID" -eq 1 ]; then
        android_arg=" --no-android"
    fi

    mkdir -p "$outdir"

    if [ -n "$arm_arch" ] && [ "$SKIP_BOOTSTRAP" -eq 0 ] && [ "$RASPBIAN_BOOTSTRAP_READY" -eq 0 ]; then
        build_bootstrap_for_arch "$arch" "$arm_arch"
        RASPBIAN_BOOTSTRAP_READY=1
    fi

    echo
    echo "============================================================"
    echo "Building ${distro}/${codename} (${arch})"
    echo "Docker image: ${image}"
    echo "Docker platform: ${platform}"
    [ -z "$arm_arch" ] || echo "ARM default arch: ${arm_arch}"
    echo "Make jobs: ${build_jobs}"
    echo "Script: build_scripts/${script_name}"
    echo "============================================================"

    if ! {
        run_root docker pull --platform "$platform" "$image" &&
        run_root docker run --rm \
            --platform "$platform" \
            -e DEBIAN_FRONTEND=noninteractive \
            -e FBC_PACKAGE_DISTRO_ID="$distro" \
            -e FBC_PACKAGE_CODENAME="$codename" \
            -e FBC_PACKAGE_OUTDIR="$container_outdir" \
            -e FBC_PACKAGE_ARM_ARCH="$arm_arch" \
            -e BUILDROOT="/work/.build-debianubuntu/${distro}/${codename}/${arch}" \
            -e JOBS="$build_jobs" \
            -v "$ROOT:/work" \
            -w /work \
            "$image" \
            bash -lc "/work/build_scripts/${script_name} --no-build${android_arg}"
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
    local tag
    local codename
    local script_name
    local arch

    IFS="|" read -r distro _ tag codename script_name arch <<EOF
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

for distro_entry in "${DISTRO_TARGETS[@]}"; do
    IFS="|" read -r distro image tag codename script_name <<EOF
$distro_entry
EOF

    while IFS= read -r arch; do
        BUILD_MATRIX+=("${distro}|${image}|${tag}|${codename}|${script_name}|${arch}")
    done < <(target_arches "$distro")
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
    echo "LINUX BUILDS FINISHED WITH FAILURES: $failures"
    echo "============================================================"
    ls -R out/linux out/raspbian 2>/dev/null || true
    exit 1
fi

echo
echo "============================================================"
echo "ALL LINUX BUILDS FINISHED"
echo "============================================================"

ls -R out/linux out/raspbian 2>/dev/null || true
