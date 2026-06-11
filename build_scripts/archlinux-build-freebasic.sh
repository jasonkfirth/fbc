#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: archlinux-build-freebasic.sh
#
# Purpose:
#
#     Build one Arch Linux package from source using the distro-native
#     PKGBUILD flow.
#
# Responsibilities:
#
#     * validate that an Arch-like environment is available
#     * install required build toolchain dependencies
#     * stage a clean source tree with bootstrap binaries
#     * run a PKGBUILD build in an isolated work directory
#     * write artifacts under out/linux/<distro>/<release>/<arch>/
#
# This file intentionally does not contain:
#
#     * matrix discovery logic
#     * package family aggregation logic
#     * container orchestration
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

CLEANUP_SUCCESS=0
CLEANUP_DIRS=()

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

disable_pacman_download_sandbox() {
    [ -f /etc/pacman.conf ] || return 0

    if grep -Eq '^[[:space:]]*DisableSandbox([[:space:]]|$)' /etc/pacman.conf; then
        return 0
    fi

    sed -i '/^\[options\]/a DisableSandbox' /etc/pacman.conf
}

usage() {
    cat <<EOF
Usage: ./build_scripts/archlinux-build-freebasic.sh [options]

Options:
  --no-build      Reuse the existing source bootstrap tarball
  --no-package    Stop after ensuring the bootstrap tarball exists
  --skip-deps     Skip pacman dependency installation
  --skip-host-deps Same as --skip-deps
  --skip-bootstrap Skip host bootstrap regeneration (same as --no-build)
  --jobs N        Set make parallelism
  --distro NAME   Override distro label (same as FBC_PACKAGE_DISTRO_ID)
  --release NAME  Override release label (same as FBC_PACKAGE_CODENAME)
  --help          Show this help text

Environment:
  BUILDROOT       Temporary build root (default: <repo>/.build-archlinux)
  WORKDIR         Workspace for source/package preparation
  OUTBASE         Output root (default: <repo>/out)
  FBC_PACKAGE_DISTRO_ID
                  Override distro label (default: archlinux)
  FBC_PACKAGE_CODENAME
                  Override release label (default: current)
  FBC_PACKAGE_HOST_ARCH
                  Override architecture for bootstrap generation
  JOBS            Parallel make job count for bootstrap generation

Artifacts are written under:
  out/linux/<distro>/<codename>/<arch>/
EOF
}

##############################################################################
# Options
##############################################################################

NO_BUILD=0
NO_PACKAGE=0
SKIP_DEPS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) NO_BUILD=1; shift ;;
        --no-package) NO_PACKAGE=1; shift ;;
        --skip-deps) SKIP_DEPS=1; shift ;;
        --skip-host-deps) SKIP_DEPS=1; shift ;;
        --skip-bootstrap) NO_BUILD=1; shift ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --distro)
            export FBC_PACKAGE_DISTRO_ID="$2"
            shift 2
            ;;
        --release)
            export FBC_PACKAGE_CODENAME="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

##############################################################################
# Tooling and config
##############################################################################

if command -v gmake >/dev/null 2>&1; then
    MAKE_CMD="gmake"
else
    MAKE_CMD="make"
fi

if command -v nproc >/dev/null 2>&1; then
    JOBS="${JOBS:-$(nproc)}"
else
    JOBS="${JOBS:-1}"
fi

BUILDROOT="${BUILDROOT:-$ROOT/.build-archlinux}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
OUTBASE="${OUTBASE:-$ROOT/out}"
PKGROOT="${PKGROOT:-$WORKDIR/pkgbuild}"
BUILDDIR="${BUILDDIR:-$WORKDIR/package}"
SOURCE_COPY_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"
CLEANUP_DIRS=("$BUILDROOT")

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
REV="$(sed -n 's/^REV[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"
[ -n "$REV" ] || REV=1
[ -f "$SOURCE_COPY_EXCLUDES" ] || die "missing source copy excludes: $SOURCE_COPY_EXCLUDES"

ARCH_REQUEST="${FBC_PACKAGE_HOST_ARCH:-$(uname -m)}"
ARCH=""
BOOTKEY=""
FBC_TARGET=""
TARGET_TRIPLET=""
PKGNAME="freebasic"
PKGFILE_ROOT="$PKGNAME-$VERSION-$REV"

case "$ARCH_REQUEST" in
    x86_64|amd64)
        ARCH="x86_64"
        BOOTKEY="linux-x86_64"
        FBC_TARGET="linux-x86_64"
        TARGET_TRIPLET="x86_64-pc-linux-gnu"
        ;;
    aarch64|arm64)
        ARCH="aarch64"
        BOOTKEY="linux-aarch64"
        FBC_TARGET="linux-aarch64"
        TARGET_TRIPLET="aarch64-linux-gnu"
        ;;
    armv7h|armv7l|armv7*|armv6*)
        ARCH="armv7h"
        BOOTKEY="linux-arm"
        FBC_TARGET="linux-arm"
        TARGET_TRIPLET="arm-linux-gnueabihf"
        ;;
    riscv64)
        ARCH="riscv64"
        BOOTKEY="linux-riscv64"
        FBC_TARGET="linux-riscv64"
        TARGET_TRIPLET="riscv64-linux-gnu"
        ;;
    i386|i486|i586|i686)
        die "32-bit x86 architecture is not supported in Arch Linux package flow yet: $ARCH_REQUEST"
        ;;
    *)
        die "unsupported Arch architecture: $ARCH_REQUEST"
        ;;
esac

BUILD_ARCH_REQUEST="$(uname -m)"
BUILD_ARCH=""
BUILD_BOOTKEY=""
BUILD_FBC_TARGET=""
BUILD_TARGET_TRIPLET=""
case "$BUILD_ARCH_REQUEST" in
    x86_64|amd64)
        BUILD_ARCH="x86_64"
        BUILD_BOOTKEY="linux-x86_64"
        BUILD_FBC_TARGET="linux-x86_64"
        BUILD_TARGET_TRIPLET="x86_64-pc-linux-gnu"
        ;;
    aarch64|arm64)
        BUILD_ARCH="aarch64"
        BUILD_BOOTKEY="linux-aarch64"
        BUILD_FBC_TARGET="linux-aarch64"
        BUILD_TARGET_TRIPLET="aarch64-linux-gnu"
        ;;
    armv7h|armv7l|armv7*|armv6*)
        BUILD_ARCH="armv7h"
        BUILD_BOOTKEY="linux-arm"
        BUILD_FBC_TARGET="linux-arm"
        BUILD_TARGET_TRIPLET="arm-linux-gnueabihf"
        ;;
    riscv64)
        BUILD_ARCH="riscv64"
        BUILD_BOOTKEY="linux-riscv64"
        BUILD_FBC_TARGET="linux-riscv64"
        BUILD_TARGET_TRIPLET="riscv64-linux-gnu"
        ;;
    *)
        die "unsupported Arch build architecture: $BUILD_ARCH_REQUEST"
        ;;
esac

CROSS_PACKAGE_BUILD=0
if [ "$ARCH" != "$BUILD_ARCH" ]; then
    CROSS_PACKAGE_BUILD=1
fi

if [ "$ARCH" = "armv7h" ] && [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
    die "Arch x86_64 repositories do not provide an official armv7h Linux cross GCC package"
fi

BOOTSTRAP_TAR="$ROOT/FreeBASIC-${VERSION}-source-bootstrap-${BUILD_BOOTKEY}.tar.xz"
SYSROOT="$WORKDIR/sysroot/$ARCH"

DISTRO_ID="${FBC_PACKAGE_DISTRO_ID:-archlinux}"
CODENAME="${FBC_PACKAGE_CODENAME:-current}"
[ -n "$CODENAME" ] || CODENAME="current"

OUTDIR="${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}"
mkdir -p "$WORKDIR" "$PKGROOT" "$OUTDIR"

##############################################################################
# Environment checks
##############################################################################

if ! command -v pacman >/dev/null 2>&1; then
    echo "ERROR: this script requires pacman and an Arch-style environment"
    exit 1
fi

if ! command -v makepkg >/dev/null 2>&1; then
    die "this script requires makepkg (install with base-devel)"
fi

##############################################################################
# Dependencies
##############################################################################

write_cross_pacman_config() {
    local conf="$1"

    case "$ARCH" in
        aarch64)
            cat > "$conf" <<EOF
[options]
Architecture = aarch64
SigLevel = Never
LocalFileSigLevel = Optional
DisableSandbox

[core]
Server = http://mirror.archlinuxarm.org/aarch64/\$repo

[extra]
Server = http://mirror.archlinuxarm.org/aarch64/\$repo

[alarm]
Server = http://mirror.archlinuxarm.org/aarch64/\$repo

[aur]
Server = http://mirror.archlinuxarm.org/aarch64/\$repo
EOF
            ;;
        riscv64)
            cat > "$conf" <<EOF
[options]
Architecture = riscv64
SigLevel = Never
LocalFileSigLevel = Optional
DisableSandbox

[core]
Server = https://riscv.mirror.pkgbuild.com/repo/\$repo

[extra]
Server = https://riscv.mirror.pkgbuild.com/repo/\$repo
EOF
            ;;
        *)
            die "no Arch cross sysroot repository mapping for $ARCH"
            ;;
    esac
}

prepare_cross_sysroot() {
    local conf="$WORKDIR/pacman-$ARCH.conf"
    local dbpath="$WORKDIR/pacman-db-$ARCH"
    local cache="$WORKDIR/pacman-cache-$ARCH"
    local packages=(
        glibc
        gcc-libs
        ncurses
        gpm
        libffi
        alsa-lib
        libpulse
        libx11
        libxext
        libxpm
        libxrandr
        libxrender
        mesa
        glu
    )

    [ "$CROSS_PACKAGE_BUILD" -eq 1 ] || return 0

    msg "preparing Arch $ARCH target sysroot"
    rm -rf "$SYSROOT" "$dbpath" "$cache"
    mkdir -p "$SYSROOT" "$dbpath" "$cache"
    write_cross_pacman_config "$conf"

    run pacman \
        --config "$conf" \
        --dbpath "$dbpath" \
        --cachedir "$cache" \
        -Syw --noconfirm \
        "${packages[@]}"

    while IFS= read -r pkg; do
        run bsdtar -xpf "$pkg" -C "$SYSROOT" \
            --exclude .BUILDINFO \
            --exclude .MTREE \
            --exclude .PKGINFO
    done < <(find "$cache" -type f -name '*.pkg.tar.*' | sort)
}

install_deps() {
    local deps=(
        base-devel
        pkgconf
        rsync
        tar
        xz
        dos2unix
        gcc
        binutils
        libarchive
        ncurses
        gpm
        libffi
        alsa-lib
        pulseaudio
        libx11
        libxext
        libxpm
        libxrandr
        libxrender
        mesa
        glu
    )

    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Arch build dependencies via pacman"
    disable_pacman_download_sandbox
    run pacman -Syu --noconfirm

    if [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
        deps+=(
            "${TARGET_TRIPLET}-binutils"
            "${TARGET_TRIPLET}-gcc"
            "${TARGET_TRIPLET}-glibc"
        )
    fi

    run pacman -S --noconfirm --needed "${deps[@]}"
    prepare_cross_sysroot
}

run_makepkg() {
    local makepkg_uid="${FBC_MAKEPKG_UID:-1000}"
    local makepkg_gid="${FBC_MAKEPKG_GID:-1000}"
    local makepkg_user
    local makepkg_opts="MAKEFLAGS=\"-j$JOBS\" CFLAGS= CXXFLAGS= CPPFLAGS= LDFLAGS= LTOFLAGS="
    makepkg_opts="$makepkg_opts CARCH=\"$ARCH\" CHOST=\"$TARGET_TRIPLET\""

    if [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
        makepkg_opts="$makepkg_opts FBC_ARCH_CROSS=1"
        makepkg_opts="$makepkg_opts FBC_ARCH_TARGET_TRIPLET=\"$TARGET_TRIPLET\""
        makepkg_opts="$makepkg_opts FBC_ARCH_BOOTKEY=\"$BOOTKEY\""
        makepkg_opts="$makepkg_opts FBC_ARCH_FBC_TARGET=\"$FBC_TARGET\""
        makepkg_opts="$makepkg_opts FBC_ARCH_BUILD_TRIPLET=\"$BUILD_TARGET_TRIPLET\""
        makepkg_opts="$makepkg_opts FBC_ARCH_BUILD_BOOTKEY=\"$BUILD_BOOTKEY\""
        makepkg_opts="$makepkg_opts FBC_ARCH_BUILD_FBC_TARGET=\"$BUILD_FBC_TARGET\""
        makepkg_opts="$makepkg_opts FBC_ARCH_SYSROOT=\"$SYSROOT\""
    fi

    if [ "$(id -u)" -ne 0 ]; then
        run bash -lc "cd \"$PKGROOT\" && $makepkg_opts makepkg --force --noconfirm"
        return 0
    fi

    if ! command -v su >/dev/null 2>&1; then
        run bash -lc "cd \"$PKGROOT\" && $makepkg_opts makepkg --force --noconfirm"
        return 0
    fi

    if ! getent passwd "$makepkg_uid" >/dev/null 2>&1; then
        if ! getent group "$makepkg_gid" >/dev/null 2>&1; then
            run groupadd -g "$makepkg_gid" fbcbuild
        fi
        run useradd -m -u "$makepkg_uid" -g "$makepkg_gid" -s /bin/bash fbcbuild || true
    fi

    makepkg_user="$(getent passwd "$makepkg_uid" | cut -d: -f1)"
    [ -n "$makepkg_user" ] || makepkg_user="root"

    run chown -R "${makepkg_uid}:${makepkg_gid}" "$BUILDDIR" "$PKGROOT" "$OUTDIR"
    run su -s /bin/bash "$makepkg_user" -c "cd \"$PKGROOT\" && ${makepkg_opts} makepkg --force --noconfirm"
}

##############################################################################
# Bootstrap generation
##############################################################################

ensure_host_compiler() {
    if ! ./bin/fbc -version >/dev/null 2>&1; then
        msg "building host compiler"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$JOBS"
    fi

    ./bin/fbc -version >/dev/null 2>&1 || die "host compiler not available"
}

build_bootstrap_tarball() {
    msg "building bootstrap tarball: $(basename "$BOOTSTRAP_TAR")"

    ensure_host_compiler

    rm -f "$BOOTSTRAP_TAR"
    fb_remove_build_tree "$ROOT" "$ROOT/bootstrap/${BUILD_BOOTKEY}" || die "could not remove bootstrap/${BUILD_BOOTKEY}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        FBC_TARGET="$BUILD_FBC_TARGET" \
        FBTARGET_DIR_OVERRIDE="$BUILD_BOOTKEY" \
        bootstrap-dist-target \
        -j"$JOBS"

    [ -f "$BOOTSTRAP_TAR" ] || die "bootstrap tarball was not created"
}

##############################################################################
# Source staging and package preparation
##############################################################################

stage_source_tree() {
    local bootstrap_srcdir

    [ -f "$SOURCE_COPY_EXCLUDES" ] || die "missing source copy excludes: $SOURCE_COPY_EXCLUDES"

    rm -rf "$BUILDDIR" "$WORKDIR/bootstrap-from-tar"
    mkdir -p "$BUILDDIR"

    bootstrap_srcdir="$ROOT/bootstrap/$BUILD_BOOTKEY"
    if [ ! -d "$bootstrap_srcdir" ]; then
        [ -f "$BOOTSTRAP_TAR" ] || die "missing bootstrap source: $bootstrap_srcdir and $BOOTSTRAP_TAR"
        mkdir -p "$WORKDIR/bootstrap-from-tar"
        run tar -xJf "$BOOTSTRAP_TAR" -C "$WORKDIR/bootstrap-from-tar" \
            "FreeBASIC-${VERSION}-source-bootstrap-${BUILD_BOOTKEY}/bootstrap/${BUILD_BOOTKEY}"
        bootstrap_srcdir="$WORKDIR/bootstrap-from-tar/FreeBASIC-${VERSION}-source-bootstrap-${BUILD_BOOTKEY}/bootstrap/${BUILD_BOOTKEY}"
    fi

    run rsync -a --no-owner --no-group \
        --delete \
        --exclude-from="$SOURCE_COPY_EXCLUDES" \
        --exclude '/bin/' \
        --exclude '/bootstrap/' \
        "$ROOT/" "$BUILDDIR/"

    mkdir -p "$BUILDDIR/bootstrap/$BUILD_BOOTKEY"
    run rsync -a --no-owner --no-group --delete "$bootstrap_srcdir/" "$BUILDDIR/bootstrap/$BUILD_BOOTKEY/"
}

prepare_pkgbuild_dir() {
    local src_tar="$WORKDIR/${PKGNAME}-${VERSION}.tar.xz"

    rm -rf "$PKGROOT"
    mkdir -p "$PKGROOT"

    run tar -C "$WORKDIR" -cJf "$src_tar" --transform "s#^package#${PKGNAME}-${VERSION}#" package
    [ -f "$src_tar" ] || die "source tarball was not created: $src_tar"

    cp "$ROOT/contrib/pkg/arch/PKGBUILD" "$PKGROOT/PKGBUILD"
    sed -i \
        -e "s/^pkgver=.*/pkgver=${VERSION}/" \
        -e "s/^pkgrel=.*/pkgrel=${REV}/" \
        -e "s|^source=.*|source=(\"${PKGNAME}-${VERSION}.tar.xz\")|" \
        "$PKGROOT/PKGBUILD"

    cp "$src_tar" "$PKGROOT/"
}

package_current_target() {
    local pkg_file
    local built_pkgs=()

    msg "preparing Arch Linux package build"

    stage_source_tree
    prepare_pkgbuild_dir

    msg "building freebasic ${VERSION}-${REV}"
    run_makepkg

    shopt -s nullglob
    built_pkgs=("$PKGROOT"/*.pkg.tar.* "$PKGROOT"/*.pkg.tar)
    shopt -u nullglob
    [ "${#built_pkgs[@]}" -gt 0 ] || die "makepkg did not create a package in $PKGROOT"

    rm -f "$OUTDIR/$PKGNAME"-*
    for pkg_file in "${built_pkgs[@]}"; do
        run cp "$pkg_file" "$OUTDIR/"
    done

    msg "build completed"
    msg "artifacts:"
    ls -lh "$OUTDIR"/"$PKGNAME"-* 2>/dev/null || true
}

##############################################################################
# Main
##############################################################################

install_deps

if [ "$NO_BUILD" -eq 0 ]; then
    build_bootstrap_tarball
else
    [ -f "$BOOTSTRAP_TAR" ] || die "missing bootstrap tarball: $BOOTSTRAP_TAR"
fi

if [ "$NO_PACKAGE" -eq 1 ]; then
    msg "bootstrap tarball ready"
    CLEANUP_SUCCESS=1
    echo "==> $BOOTSTRAP_TAR"
    exit 0
fi

package_current_target
CLEANUP_SUCCESS=1
