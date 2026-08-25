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

show_failure_log() {
    local log="$1"

    [ -f "$log" ] || return 0

    echo
    echo "Last 200 lines of $log:"
    tail -n 200 "$log" || true
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
Usage: ./build_scripts/debianubuntu-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit the matrix to one distro family (debian, ubuntu, raspbian)
  --release NAME    Limit the matrix to one distro release or codename
  --arch ARCH       Limit the matrix to one Debian-style CPU arch (amd64, arm64, ...)
  --jobs N          Maximum make jobs for native Docker builds
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --skip-bootstrap  Reuse existing source bootstrap tarballs
  --no-android      Build packages without the freebasic-android profile
  --no-xbox         Do not auto-build freebasic-xbox even if nxdk is present
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
RELEASE_FILTER=""
ARCH_FILTER=""
KEEP_GOING=0
SKIP_HOST_DEPS=0
SKIP_BOOTSTRAP=0
NO_ANDROID=0
NO_XBOX=0
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
        --serial) shift ;;
        --keep-going) KEEP_GOING=1; shift ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        --skip-bootstrap) SKIP_BOOTSTRAP=1; shift ;;
        --no-android) NO_ANDROID=1; shift ;;
        --no-xbox) NO_XBOX=1; shift ;;
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
        amd64)   echo "linux-x86_64 linux-amd64 x86_64-linux-gnu" ;;
        i386)    echo "linux-x86 linux-i386 i686-linux-gnu" ;;
        arm64)   echo "linux-aarch64 linux-arm64 aarch64-linux-gnu" ;;
        armhf)   echo "linux-arm linux-armhf arm-linux-gnueabihf" ;;
        armel)   echo "linux-arm linux-armel arm-linux-gnueabi" ;;
        ppc64el) echo "linux-powerpc64le linux-ppc64el powerpc64le-linux-gnu" ;;
        s390x)   echo "linux-s390x linux-s390x s390x-linux-gnu" ;;
        riscv64) echo "linux-riscv64 linux-riscv64 riscv64-linux-gnu" ;;
        loong64) echo "linux-loongarch64 linux-loongarch64 loongarch64-linux-gnu" ;;
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
    local target_triplet
    local pkg
    local extra_make_args=()

    read -r fbc_target dir_key target_triplet <<EOF
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
    fb_remove_build_tree "$ROOT" "$ROOT/bootstrap/${dir_key}" || die "could not remove bootstrap/${dir_key}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        TARGET_TRIPLET="$target_triplet" \
        FBC_TARGET="$fbc_target" \
        FBTARGET_DIR_OVERRIDE="$dir_key" \
        BOOTSTRAP_DIST_WORKTREE=1 \
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

DEBIAN_TRIXIE_ARCHES=(
    "${LINUX_ARCHES[@]}"
    armel
    loong64
)

RASPBIAN_ARCHES=(
    armhf
)

DISTRO_TARGETS=(
    "ubuntu|ubuntu:26.04|26.04|resolute|debianubuntu-build-freebasic.sh"
    "debian|debian:13|13|trixie|debianubuntu-build-freebasic.sh"
    "debian|debian:sid|sid|sid|debianubuntu-build-freebasic.sh"
    "raspbian|badaix/raspios-lite:trixie|trixie|trixie|debianubuntu-build-freebasic.sh"
)

docker_platform_for_arch() {
    case "$1" in
        amd64) echo "linux/amd64" ;;
        i386) echo "linux/386" ;;
        arm64) echo "linux/arm64" ;;
        armhf) echo "linux/arm/v7" ;;
        armel) echo "linux/arm/v5" ;;
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
    local codename="$2"

    case "$distro" in
        raspbian)
            printf '%s\n' "${RASPBIAN_ARCHES[@]}"
            ;;
        debian)
            if [ "$codename" = "trixie" ]; then
                printf '%s\n' "${DEBIAN_TRIXIE_ARCHES[@]}"
            else
                printf '%s\n' "${LINUX_ARCHES[@]}"
            fi
            ;;
        *)
            printf '%s\n' "${LINUX_ARCHES[@]}"
            ;;
    esac
}

docker_image_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"
    local default_image="$4"

    case "${distro}/${codename}/${arch}" in
        debian/trixie/armel)
            echo "arm32v5/debian:trixie"
            ;;
        debian/trixie/loong64)
            echo "ghcr.io/loong64/debian:trixie"
            ;;
        *)
            echo "$default_image"
            ;;
    esac
}

bootstrap_arches_for_filters() {
    local entry
    local distro
    local image
    local tag
    local codename
    local script_name
    local arch
    local seen=" "

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r distro image tag codename script_name <<EOF
$entry
EOF
        if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
            continue
        fi
        if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$codename" ] && [ "$RELEASE_FILTER" != "$tag" ]; then
            continue
        fi

        while IFS= read -r arch; do
            if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
                continue
            fi
            case "$seen" in
                *" $arch "*)
                    ;;
                *)
                    seen="${seen}${arch} "
                    printf '%s\n' "$arch"
                    ;;
            esac
        done < <(target_arches "$distro" "$codename")
    done
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

android_supported_for_target() {
    local distro="$1"
    local arch="$2"

    [ "$distro" = "ubuntu" ] || return 1

    case "$arch" in
        amd64)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

xbox_supported_for_arch() {
    case "$1" in
        amd64|i386)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

path_is_under_root() {
    local path="$1"

    case "$path" in
        "$ROOT"/*|"$ROOT")
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

canonical_dir() {
    local path="$1"

    [ -d "$path" ] || return 1
    ( cd "$path" && pwd -P )
}

find_host_nxdk() {
    local candidate
    local candidates=()

    if [ -n "${NXDK_DIR:-}" ]; then
        candidates+=("$NXDK_DIR")
    fi

    candidates+=(
        "$ROOT/.build-debianubuntu-xbox/nxdk"
        "$ROOT/nxdk"
        "/opt/nxdk"
        "/usr/local/share/nxdk"
    )

    for candidate in "${candidates[@]}"; do
        [ -n "$candidate" ] || continue
        [ -f "$candidate/bin/activate" ] || continue
        [ -f "$candidate/bin/nxdk-cc" ] || continue
        [ -f "$candidate/bin/nxdk-link" ] || continue
        [ -f "$candidate/tools/cxbe/cxbe" ] || [ -f "$candidate/Makefile" ] || continue
        canonical_dir "$candidate"
        return 0
    done

    return 1
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
        if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$codename" ] && [ "$RELEASE_FILTER" != "$tag" ]; then
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
XBOX_NXDK_HOST=""
XBOX_NXDK_CONTAINER=""
XBOX_NXDK_NEEDS_MOUNT=0

if [ "$NO_XBOX" -eq 0 ]; then
    if XBOX_NXDK_HOST="$(find_host_nxdk)"; then
        if path_is_under_root "$XBOX_NXDK_HOST"; then
            XBOX_NXDK_CONTAINER="/work${XBOX_NXDK_HOST#"$ROOT"}"
        else
            XBOX_NXDK_CONTAINER="/xbox-nxdk"
            XBOX_NXDK_NEEDS_MOUNT=1
        fi
        echo "==> Xbox package auto-build: enabled when arch is supported"
        echo "==> nxdk: $XBOX_NXDK_HOST"
    else
        echo "==> Xbox package auto-build: skipped (nxdk not found)"
    fi
else
    echo "==> Xbox package auto-build: disabled by --no-xbox"
fi

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
        while IFS= read -r debarch; do
            build_bootstrap_for_arch "$debarch"
        done < <(bootstrap_arches_for_filters)
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
    local native_build_cmd
    local xbox_build_cmd
    local xbox_enabled
    local docker_extra_mounts=()

    IFS="|" read -r distro image tag codename script_name arch <<EOF
$entry
EOF

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 0
    fi

    if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$codename" ] && [ "$RELEASE_FILTER" != "$tag" ]; then
        return 0
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 0
    fi

    image="$(docker_image_for_target "$distro" "$codename" "$arch" "$image")"
    platform="$(docker_platform_for_arch "$arch")"
    build_jobs="$(make_jobs_for_platform "$platform" "$HOST_PLATFORM")"
    outdir="$(host_outdir_for_target "$distro" "$codename" "$arch")"
    container_outdir="$(container_outdir_for_target "$distro" "$codename" "$arch")"
    arm_arch="$(arm_arch_for_target "$distro" "$arch")"
    android_arg=""
    if [ "$NO_ANDROID" -eq 1 ] || ! android_supported_for_target "$distro" "$arch"; then
        android_arg=" --no-android"
    fi
    native_build_cmd="/work/build_scripts/${script_name} --no-build${android_arg}"
    xbox_build_cmd=""
    xbox_enabled=0
    if [ -n "$XBOX_NXDK_HOST" ] && xbox_supported_for_arch "$arch"; then
        xbox_enabled=1
        xbox_build_cmd=" && FBC_PACKAGE_OUTDIR=${container_outdir}/xbox BUILDROOT=/work/.build-debianubuntu-xbox/${distro}/${codename}/${arch} NXDK_DIR=${XBOX_NXDK_CONTAINER} /work/build_scripts/debianubuntu-build-freebasic-xbox.sh --nxdk-dir ${XBOX_NXDK_CONTAINER}"
        if [ "$XBOX_NXDK_NEEDS_MOUNT" -eq 1 ]; then
            docker_extra_mounts+=(-v "$XBOX_NXDK_HOST:$XBOX_NXDK_CONTAINER")
        fi
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
    if [ "$xbox_enabled" -eq 1 ]; then
        echo "Xbox package: enabled with nxdk at ${XBOX_NXDK_HOST}"
    elif [ -n "$XBOX_NXDK_HOST" ]; then
        echo "Xbox package: skipped for unsupported arch ${arch}"
    else
        echo "Xbox package: skipped because nxdk was not found"
    fi
    echo "============================================================"

    if ! : > "$outdir/docker_build.log"; then
        die "cannot write build log: $outdir/docker_build.log"
    fi

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
            "${docker_extra_mounts[@]}" \
            -w /work \
            "$image" \
            bash -lc "${native_build_cmd}${xbox_build_cmd}"
    } > "$outdir/docker_build.log" 2>&1; then
        if log_has_missing_manifest "$outdir/docker_build.log"; then
            echo "SKIPPED: ${distro}/${codename} (${arch}) has no Docker image for ${platform}"
            echo "Log: $outdir/docker_build.log"
            return 0
        fi

        echo "BUILD FAILED: ${distro}/${codename} (${arch})"
        echo "Log: $outdir/docker_build.log"
        show_failure_log "$outdir/docker_build.log"

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

    if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$codename" ] && [ "$RELEASE_FILTER" != "$tag" ]; then
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
    done < <(target_arches "$distro" "$codename")
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

CLEANUP_SUCCESS=1
