#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: debianubuntu-cross-build-freebasic-matrix.sh
#
# Purpose:
#
#     Build the Debian-family FreeBASIC package matrix from native distro
#     containers while targeting multiple package CPU architectures.
#
# Responsibilities:
#
#     * define Debian, Ubuntu, and Raspbian release/architecture targets
#     * prepare the bootstrap source tarball used by package containers
#     * run the one-package Debian/Ubuntu builder in Docker containers
#     * write artifacts under out/linux/<distro>/<release>/<arch>
#
# This file intentionally does NOT contain:
#
#     * Debian package policy or debian/rules implementation details
#     * target package validation
#     * APK/RPM/Slackware package policy
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
. "$ROOT/build_scripts/build-success-cleanup.sh"
. "$ROOT/build_scripts/qemu-binfmt.sh"

#
# Release worktrees may share a large artifact directory through an out
# symlink.  The source bind mount alone cannot expose a sibling symlink target
# inside Docker, so mount the resolved output directory explicitly at
# /work/out.  This also keeps ordinary in-tree output directories working.
#
mkdir -p "$ROOT/out"
HOST_OUTPUT_ROOT="$(readlink -f "$ROOT/out")"
[ -n "$HOST_OUTPUT_ROOT" ] && [ -d "$HOST_OUTPUT_ROOT" ] || {
    echo "ERROR: could not resolve output directory: $ROOT/out" >&2
    exit 1
}

CLEANUP_SUCCESS=0
CLEANUP_DIRS=(
    "$ROOT/.build-debianubuntu-cross"
    "$ROOT/.build-debianubuntu-wii"
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

die() { echo "ERROR: $*" >&2; exit 1; }

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-cross-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit the matrix to one distro name
  --release NAME    Limit the matrix to one release/codename
  --arch ARCH       Limit the target CPU architecture
  --jobs N          Parallel make jobs inside package builds
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --skip-bootstrap  Reuse existing build-architecture bootstrap tarball
  --with-js         Include the freebasic-js package in cross-builds
  --with-android    Include the freebasic-android package in cross-builds
  --with-wii        Include the freebasic-wii package where supported
  --with-xbox       Include the freebasic-xbox sidecar package where supported
  --no-target-ports Disable the default JS/Android/Wii/Xbox target additions
  --execute         Attempt the cross-build execution path
  --list            Show the planned native-container cross-build matrix
  --help            Show this help text

This is the replacement Debian/Ubuntu package matrix.

The default mode lists the intended build graph. The execute path opens the
selected distro/release container and asks debian/rules to build the selected
host architecture using the build-compiler/host-compiler split.
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
RELEASE_FILTER=""
ARCH_FILTER=""
EXECUTE=0
LIST_ONLY=0
NO_JS=1
NO_ANDROID=1
WITH_WII=0
WITH_XBOX=0
TARGET_PORTS=1
KEEP_GOING=0
SKIP_HOST_DEPS=0
SKIP_BOOTSTRAP=0

if command -v nproc >/dev/null 2>&1; then
    MAKE_JOBS="${JOBS:-$(nproc)}"
else
    MAKE_JOBS="${JOBS:-1}"
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
        --with-js) NO_JS=0; shift ;;
        --with-android) NO_ANDROID=0; shift ;;
        --with-wii) WITH_WII=1; shift ;;
        --with-xbox) WITH_XBOX=1; shift ;;
        --no-target-ports) TARGET_PORTS=0; shift ;;
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
)

DEBIAN_SID_ARCHES=(
    "${LINUX_ARCHES[@]}"
    loong64
    powerpc
    ppc64
)

DISTRO_TARGETS=(
    "ubuntu|ubuntu:26.04|26.04|resolute"
    "debian|debian:13|13|trixie"
    "debian|debian:sid|sid|sid"
    "raspbian|badaix/raspios-lite:trixie|trixie|trixie"
)

target_arches() {
    local distro="$1"
    local codename="$2"

    case "$distro/$codename" in
        debian/trixie)
            printf '%s\n' "${DEBIAN_TRIXIE_ARCHES[@]}"
            ;;
        debian/sid)
            printf '%s\n' "${DEBIAN_SID_ARCHES[@]}"
            ;;
        raspbian/*)
            printf '%s\n' armhf
            ;;
        *)
            printf '%s\n' "${LINUX_ARCHES[@]}"
            ;;
    esac
}

target_default_port() {
    local port="$1"
    local distro="$2"
    local codename="$3"
    local arch="$4"

    [ "$TARGET_PORTS" -eq 1 ] || return 1

    case "$distro/$codename/$arch/$port" in
        ubuntu/resolute/amd64/js|ubuntu/resolute/amd64/android|ubuntu/resolute/amd64/wii|ubuntu/resolute/amd64/xbox|\
        debian/trixie/amd64/js|debian/trixie/amd64/xbox|\
        ubuntu/resolute/arm64/js|\
        debian/trixie/arm64/js)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

android_supported_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    : "$codename"

    # The Android target runtimes include ARM, but Google's Linux NDK host
    # programs and Ubuntu's installer package are only available on amd64.
    case "$distro/$arch" in
        ubuntu/amd64)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

target_builds_js() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    [ "$NO_JS" -eq 0 ] || target_default_port js "$distro" "$codename" "$arch"
}

target_builds_android() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    android_supported_for_target "$distro" "$codename" "$arch" || return 1

    [ "$NO_ANDROID" -eq 0 ] || target_default_port android "$distro" "$codename" "$arch"
}

wii_supported_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    [ "$distro/$codename/$arch" = "ubuntu/resolute/amd64" ]
}

target_builds_wii() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    wii_supported_for_target "$distro" "$codename" "$arch" || return 1
    [ "$WITH_WII" -eq 1 ] || target_default_port wii "$distro" "$codename" "$arch"
}

target_builds_xbox() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    [ "$WITH_XBOX" -eq 1 ] || target_default_port xbox "$distro" "$codename" "$arch"
}

target_matches_filters() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 1
    fi

    if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$codename" ]; then
        return 1
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 1
    fi

    return 0
}

host_outdir_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    echo "$ROOT/out/linux/${distro}/${codename}/${arch}"
}

docker_platform_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    case "$distro/$arch" in
        raspbian/armhf)
            case "$codename" in
                bookworm)
                    echo "linux/arm/v8"
                    ;;
                buster)
                    echo ""
                    ;;
                *)
                    echo "linux/arm/v7"
                    ;;
            esac
            ;;
        raspbian/arm64)
            echo "linux/arm64"
            ;;
        *)
            echo ""
            ;;
    esac
}

arm_arch_for_target() {
    local distro="$1"
    local arch="$2"

    if [ "$distro" = "raspbian" ] && [ "$arch" = "armhf" ]; then
        echo "armv6+fp"
        return 0
    fi

    echo ""
}

##############################################################################
# Planned graph
##############################################################################

list_plan() {
    local entry
    local distro
    local image
    local _tag
    local codename
    local arch
    local outdir

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r distro image _tag codename <<EOF
$entry
EOF

        while IFS= read -r arch; do
            target_matches_filters "$distro" "$codename" "$arch" || continue
            outdir="$(host_outdir_for_target "$distro" "$codename" "$arch")"
            printf '%s|%s|%s|%s|%s\n' "$distro" "$codename" "$arch" "$image" "$outdir"
        done < <(target_arches "$distro" "$codename")
    done
}

##############################################################################
# Execution helpers
##############################################################################

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

run() { echo "==> $*"; "$@"; }

if command -v gmake >/dev/null 2>&1; then
    MAKE_CMD="gmake"
else
    MAKE_CMD="make"
fi

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

WII_TOOLCHAIN_IMAGE="${FBC_WII_TOOLCHAIN_IMAGE:-devkitpro/devkitppc@sha256:44cb1a920e1ec3ec7c06767493c3b85f8d643d6137cc4661f0201895ac6e4967}"

wii_toolchain_available() {
    local devkitpro="$ROOT/.build-debianubuntu-wii/devkitpro"

    [ -x "$devkitpro/devkitPPC/bin/powerpc-eabi-gcc" ] || return 1
    [ -x "$devkitpro/devkitPPC/bin/powerpc-eabi-ar" ] || return 1
    [ -x "$devkitpro/devkitPPC/bin/powerpc-eabi-ranlib" ] || return 1
    [ -x "$devkitpro/tools/bin/elf2dol" ] || return 1
    [ -d "$devkitpro/libogc/include" ] || return 1
    [ -d "$devkitpro/libogc/lib/wii" ] || return 1
}

prepare_wii_toolchain() {
    local devkitpro="$ROOT/.build-debianubuntu-wii/devkitpro"

    if wii_toolchain_available; then
        echo "==> reusing staged Wii toolchain: $devkitpro"
        return 0
    fi

    fb_remove_build_tree "$ROOT" "$devkitpro" || die "could not reset Wii toolchain staging directory"
    mkdir -p "$devkitpro"

    echo "==> staging the pinned official devkitPPC toolchain"
    run_root docker pull --platform linux/amd64 "$WII_TOOLCHAIN_IMAGE"
    run_root docker run --rm --platform linux/amd64 \
        --user "$(id -u):$(id -g)" \
        -v "$devkitpro:/export" \
        "$WII_TOOLCHAIN_IMAGE" \
        bash -lc 'set -e; tar -C /opt/devkitpro -cf - devkitPPC libogc tools | tar -C /export -xf -'

    wii_toolchain_available || die "the staged devkitPPC image is missing required Wii tools or libogc files"
}

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v apt-get >/dev/null 2>&1; then
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends \
            docker.io qemu-user-binfmt  binfmt-support ca-certificates \
            dpkg-dev
        return 0
    fi

    if command -v dnf >/dev/null 2>&1; then
        run_root dnf install -y docker qemu-user-static ca-certificates dpkg
        return 0
    fi

    if command -v pacman >/dev/null 2>&1; then
        run_root pacman -Sy --noconfirm docker qemu-user-static binfmt-qemu-static ca-certificates dpkg
        return 0
    fi

    die "unsupported host package manager; install Docker, qemu-user-static, binfmt, and dpkg manually"
}

ensure_build_bootstrap_tarball() {
    local version
    local arch
    local bootkey
    local tarball

    version="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
    [ -n "$version" ] || die "could not determine FBVERSION"

    arch="$(dpkg --print-architecture 2>/dev/null || true)"
    [ -n "$arch" ] || die "could not detect build architecture"

    case "$arch" in
        amd64) bootkey="linux-amd64" ;;
        i386) bootkey="linux-i386" ;;
        arm64) bootkey="linux-arm64" ;;
        armhf) bootkey="linux-armhf" ;;
        armel) bootkey="linux-armel" ;;
        ppc64el) bootkey="linux-ppc64el" ;;
        s390x) bootkey="linux-s390x" ;;
        riscv64) bootkey="linux-riscv64" ;;
        loong64) bootkey="linux-loongarch64" ;;
        *) die "unsupported build architecture for bootstrap tarball: $arch" ;;
    esac

    tarball="$ROOT/FreeBASIC-${version}-source-bootstrap-${bootkey}.tar.xz"
    if [ -f "$tarball" ]; then
        return 0
    fi

    echo "==> building build-architecture bootstrap tarball: $(basename "$tarball")"
    BUILDROOT="$ROOT/.build-debianubuntu-cross/bootstrap-host" \
    "$ROOT/build_scripts/debianubuntu-build-freebasic.sh" \
        --no-package \
        --skip-deps \
        --no-js \
        --no-android
}

ensure_host_compiler() {
    if [ ! -x "$ROOT/bin/fbc" ]; then
        echo "==> building host compiler for bootstrap emission"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$MAKE_JOBS"
    fi

    [ -x "$ROOT/bin/fbc" ] || die "host compiler not available"
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
    local version
    local fbc_target
    local dir_key
    local target_triplet
    local pkg
    local extra_make_args=()

    version="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
    [ -n "$version" ] || die "could not determine FBVERSION"

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

    pkg="$ROOT/FreeBASIC-${version}-source-bootstrap-${dir_key}.tar.xz"

    if [ -n "$arm_arch" ]; then
        echo "==> building source bootstrap tarball for $debarch ($arm_arch)"
    else
        echo "==> building source bootstrap tarball for $debarch"
    fi

    ensure_host_compiler
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

ensure_raspbian_bootstrap_tarballs() {
    local selected
    local distro
    local codename
    local arch
    local image
    local outdir
    local arm_arch
    local key
    local prepared=""

    [ "$SKIP_BOOTSTRAP" -eq 0 ] || return 0

    for selected in "$@"; do
        IFS="|" read -r distro codename arch image outdir <<EOF
$selected
EOF
        [ "$distro" = "raspbian" ] || continue

        arm_arch="$(arm_arch_for_target "$distro" "$arch")"
        key="${arch}:${arm_arch}"
        case " $prepared " in
            *" $key "*)
                continue
                ;;
        esac

        build_bootstrap_for_arch "$arch" "$arm_arch"
        prepared="${prepared:+$prepared }$key"
    done
}

selected_entries() {
    local entry
    local distro
    local image
    local _tag
    local codename
    local arch
    local outdir

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r distro image _tag codename <<EOF
$entry
EOF

        while IFS= read -r arch; do
            target_matches_filters "$distro" "$codename" "$arch" || continue
            outdir="$(host_outdir_for_target "$distro" "$codename" "$arch")"
            printf '%s|%s|%s|%s|%s\n' "$distro" "$codename" "$arch" "$image" "$outdir"
        done < <(target_arches "$distro" "$codename")
    done
}

build_one() {
    local selected="$1"
    local distro
    local codename
    local arch
    local image
    local outdir
    local container_outdir
    local buildroot
    local docker_log
    local build_args
    local docker_platform
    local docker_platform_args=()
    local docker_env_args=()
    local arm_arch
    local xbox_args
    local build_cmd
    local xbox_cmd
    local js_enabled
    local android_enabled
    local wii_enabled
    local xbox_enabled

    IFS="|" read -r distro codename arch image outdir <<EOF
$selected
EOF

    container_outdir="/work/out/linux/${distro}/${codename}/${arch}"
    buildroot="/work/.build-debianubuntu-cross/${distro}/${codename}/${arch}"
    docker_log="$outdir/docker_build.log"
    docker_platform="$(docker_platform_for_target "$distro" "$codename" "$arch")"
    arm_arch="$(arm_arch_for_target "$distro" "$arch")"
    mkdir -p "$outdir"

    if [ -n "$docker_platform" ]; then
        docker_platform_args=(--platform "$docker_platform")
    fi

    if [ -n "$arm_arch" ]; then
        docker_env_args+=(-e FBC_PACKAGE_ARM_ARCH="$arm_arch")
    fi

    js_enabled=0
    android_enabled=0
    wii_enabled=0
    xbox_enabled=0
    target_builds_js "$distro" "$codename" "$arch" && js_enabled=1
    target_builds_android "$distro" "$codename" "$arch" && android_enabled=1
    target_builds_wii "$distro" "$codename" "$arch" && wii_enabled=1
    target_builds_xbox "$distro" "$codename" "$arch" && xbox_enabled=1

    if [ "$wii_enabled" -eq 1 ]; then
        prepare_wii_toolchain
        docker_env_args+=(
            -e DEVKITPRO=/work/.build-debianubuntu-wii/devkitpro
            -e DEVKITPPC=/work/.build-debianubuntu-wii/devkitpro/devkitPPC
        )
    fi

    build_args=(/work/build_scripts/debianubuntu-build-freebasic.sh --host-arch "$arch" --no-build)
    if [ "$js_enabled" -eq 0 ]; then
        build_args+=(--no-js)
    fi
    if [ "$android_enabled" -eq 1 ]; then
        build_args+=(--android)
    else
        build_args+=(--no-android)
    fi
    if [ "$wii_enabled" -eq 1 ]; then
        build_args+=(--wii)
    else
        build_args+=(--no-wii)
    fi

    printf -v build_cmd '%q ' "${build_args[@]}"

    xbox_cmd=""
    if [ "$xbox_enabled" -eq 1 ]; then
        xbox_args=(
            env
            FBC_PACKAGE_OUTDIR="${container_outdir}/xbox"
            BUILDROOT="/work/.build-debianubuntu-xbox/${distro}/${codename}/${arch}"
            NXDK_DIR="/work/.build-debianubuntu-xbox/nxdk"
            /work/build_scripts/debianubuntu-build-freebasic-xbox.sh
            --nxdk-dir
            /work/.build-debianubuntu-xbox/nxdk
        )
        printf -v xbox_cmd '%q ' "${xbox_args[@]}"
        xbox_cmd=" && ${xbox_cmd}"
    fi

    echo
    echo "============================================================"
    echo "Cross-building ${distro}/${codename} (${arch})"
    echo "Docker image: ${image}"
    [ -z "$docker_platform" ] || echo "Docker platform: ${docker_platform}"
    echo "Make jobs: ${MAKE_JOBS}"
    echo "Output: ${outdir}"
    [ -z "$arm_arch" ] || echo "ARM default arch: ${arm_arch}"
    if [ "$js_enabled" -eq 1 ] || [ "$android_enabled" -eq 1 ] || [ "$wii_enabled" -eq 1 ] || [ "$xbox_enabled" -eq 1 ]; then
        echo -n "Extra ports:"
        [ "$js_enabled" -eq 0 ] || echo -n " js"
        [ "$android_enabled" -eq 0 ] || echo -n " android"
        [ "$wii_enabled" -eq 0 ] || echo -n " wii"
        [ "$xbox_enabled" -eq 0 ] || echo -n " xbox"
        echo
    else
        echo "Extra ports: none"
    fi
    echo "============================================================"

    if ! {
        run_root docker pull "${docker_platform_args[@]}" "$image" &&
        run_root docker run --rm \
            "${docker_platform_args[@]}" \
            -e DEBIAN_FRONTEND=noninteractive \
            -e FBC_PACKAGE_DISTRO_ID="$distro" \
            -e FBC_PACKAGE_CODENAME="$codename" \
            -e FBC_PACKAGE_OUTDIR="$container_outdir" \
            -e FBC_PACKAGE_CONFIGURE_CROSS_APT=1 \
            -e BUILDROOT="$buildroot" \
            -e JOBS="$MAKE_JOBS" \
            "${docker_env_args[@]}" \
            -v "$ROOT:/work" \
            -v "$HOST_OUTPUT_ROOT:/work/out" \
            -w /work \
            "$image" \
            bash -lc "${build_cmd}${xbox_cmd}"
    } > "$docker_log" 2>&1; then
        echo "BUILD FAILED: ${distro}/${codename} (${arch})"
        echo "Log: $docker_log"
        return 1
    fi

    compgen -G "$outdir/freebasic-nuttx_*.deb" >/dev/null || {
        echo "BUILD FAILED: ${distro}/${codename} (${arch}) did not produce freebasic-nuttx"
        return 1
    }
    if [ "$wii_enabled" -eq 1 ] && ! compgen -G "$outdir/freebasic-wii_*.deb" >/dev/null; then
        echo "BUILD FAILED: ${distro}/${codename} (${arch}) did not produce freebasic-wii"
        return 1
    fi
    if [ "$xbox_enabled" -eq 1 ] && ! compgen -G "$outdir/xbox/freebasic-xbox_*.deb" >/dev/null; then
        echo "BUILD FAILED: ${distro}/${codename} (${arch}) did not produce freebasic-xbox"
        return 1
    fi

    echo "SUCCESS: ${distro}/${codename} (${arch})"
}

execute_plan() {
    local entries=()
    local selected
    local failures

    install_host_deps
    need_cmd docker
    need_cmd dpkg

    fb_install_qemu_binfmt

    while IFS= read -r selected; do
        [ -n "$selected" ] || continue
        entries+=("$selected")
    done < <(selected_entries)

    [ "${#entries[@]}" -gt 0 ] || die "no targets matched the selected filters"

    if [ "$SKIP_BOOTSTRAP" -eq 0 ]; then
        ensure_build_bootstrap_tarball
        ensure_raspbian_bootstrap_tarballs "${entries[@]}"
    fi

    failures=0
    for selected in "${entries[@]}"; do
        if ! build_one "$selected"; then
            failures=$((failures + 1))
            if [ "$KEEP_GOING" -eq 0 ]; then
                break
            fi
        fi
    done

    [ "$failures" -eq 0 ] || exit 1
    CLEANUP_SUCCESS=1
}

##############################################################################
# Main
##############################################################################

if [ "$EXECUTE" -eq 0 ] || [ "$LIST_ONLY" -eq 1 ]; then
    list_plan
    if [ "$LIST_ONLY" -eq 0 ]; then
        echo
        echo "==> plan only"
        echo "==> pass --execute to build Debian-family packages"
    fi
    exit 0
fi

execute_plan

##############################################################################
# end of debianubuntu-cross-build-freebasic-matrix.sh
##############################################################################
