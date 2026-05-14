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
  --with-xbox       Include the freebasic-xbox sidecar package where supported
  --no-target-ports Disable the default JS/Android/Xbox target additions
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
    loong64
)

DEBIAN_SID_ARCHES=(
    "${LINUX_ARCHES[@]}"
    loong64
    powerpc
    ppc64
)

DISTRO_TARGETS=(
    "ubuntu|ubuntu:22.04|22.04|jammy"
    "ubuntu|ubuntu:24.04|24.04|noble"
    "ubuntu|ubuntu:24.10|24.10|oracular"
    "ubuntu|ubuntu:25.04|25.04|plucky"
    "ubuntu|ubuntu:25.10|25.10|questing"
    "ubuntu|ubuntu:26.04|26.04|resolute"
    "debian|debian:12|12|bookworm"
    "debian|debian:13|13|trixie"
    "debian|debian:sid|sid|sid"
    "raspbian|badaix/raspios-lite:trixie|trixie|trixie"
    "raspbian|badaix/raspios-lite:bookworm|bookworm|bookworm"
    "raspbian|badaix/raspios-buster-armhf-lite:latest|buster|buster"
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
        ubuntu/questing/amd64/js|ubuntu/questing/amd64/android|ubuntu/questing/amd64/xbox|\
        ubuntu/resolute/amd64/js|ubuntu/resolute/amd64/android|ubuntu/resolute/amd64/xbox|\
        debian/trixie/amd64/js|debian/trixie/amd64/xbox|\
        ubuntu/questing/arm64/js|ubuntu/questing/arm64/android|\
        ubuntu/resolute/arm64/js|ubuntu/resolute/arm64/android|\
        debian/trixie/arm64/js)
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

    [ "$NO_ANDROID" -eq 0 ] || target_default_port android "$distro" "$codename" "$arch"
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

##############################################################################
# Planned graph
##############################################################################

list_plan() {
    local entry
    local distro
    local image
    local tag
    local codename
    local arch
    local outdir

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r distro image tag codename <<EOF
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

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v apt-get >/dev/null 2>&1; then
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends \
            docker.io qemu-user-static binfmt-support ca-certificates \
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

selected_entries() {
    local entry
    local distro
    local image
    local tag
    local codename
    local arch
    local outdir

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r distro image tag codename <<EOF
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
    local xbox_args
    local build_cmd
    local xbox_cmd
    local js_enabled
    local android_enabled
    local xbox_enabled

    IFS="|" read -r distro codename arch image outdir <<EOF
$selected
EOF

    container_outdir="/work/out/linux/${distro}/${codename}/${arch}"
    buildroot="/work/.build-debianubuntu-cross/${distro}/${codename}/${arch}"
    docker_log="$outdir/docker_build.log"
    mkdir -p "$outdir"

    js_enabled=0
    android_enabled=0
    xbox_enabled=0
    target_builds_js "$distro" "$codename" "$arch" && js_enabled=1
    target_builds_android "$distro" "$codename" "$arch" && android_enabled=1
    target_builds_xbox "$distro" "$codename" "$arch" && xbox_enabled=1

    build_args=(/work/build_scripts/debianubuntu-build-freebasic.sh --host-arch "$arch" --no-build)
    if [ "$js_enabled" -eq 0 ]; then
        build_args+=(--no-js)
    fi
    if [ "$android_enabled" -eq 1 ]; then
        build_args+=(--android)
    else
        build_args+=(--no-android)
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
    echo "Make jobs: ${MAKE_JOBS}"
    echo "Output: ${outdir}"
    if [ "$js_enabled" -eq 1 ] || [ "$android_enabled" -eq 1 ] || [ "$xbox_enabled" -eq 1 ]; then
        echo -n "Extra ports:"
        [ "$js_enabled" -eq 0 ] || echo -n " js"
        [ "$android_enabled" -eq 0 ] || echo -n " android"
        [ "$xbox_enabled" -eq 0 ] || echo -n " xbox"
        echo
    else
        echo "Extra ports: none"
    fi
    echo "============================================================"

    if ! {
        run_root docker pull "$image" &&
        run_root docker run --rm \
            -e DEBIAN_FRONTEND=noninteractive \
            -e FBC_PACKAGE_DISTRO_ID="$distro" \
            -e FBC_PACKAGE_CODENAME="$codename" \
            -e FBC_PACKAGE_OUTDIR="$container_outdir" \
            -e FBC_PACKAGE_CONFIGURE_CROSS_APT=1 \
            -e BUILDROOT="$buildroot" \
            -e JOBS="$MAKE_JOBS" \
            -v "$ROOT:/work" \
            -w /work \
            "$image" \
            bash -lc "${build_cmd}${xbox_cmd}"
    } > "$docker_log" 2>&1; then
        echo "BUILD FAILED: ${distro}/${codename} (${arch})"
        echo "Log: $docker_log"
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

    run_root docker run --rm --privileged tonistiigi/binfmt --install all

    while IFS= read -r selected; do
        [ -n "$selected" ] || continue
        entries+=("$selected")
    done < <(selected_entries)

    [ "${#entries[@]}" -gt 0 ] || die "no targets matched the selected filters"

    if [ "$SKIP_BOOTSTRAP" -eq 0 ]; then
        ensure_build_bootstrap_tarball
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
