#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: slackware-build-freebasic.sh
#
# Purpose:
#
#     Build one Slackware FreeBASIC binary package from inside a Slackware
#     container.
#
# Responsibilities:
#
#     * install or verify Slackware build tools
#     * stage a clean FreeBASIC source tree with bootstrap sources
#     * build and install FreeBASIC into a package root
#     * emit a Slackware .txz package under out/linux/slackware/<release>/<arch>
#
# This file intentionally does NOT contain:
#
#     * the top-level Linux package matrix
#     * target package validation
#     * Debian/APK/RPM package policy
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

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

usage() {
    cat <<EOF
Usage: ./build_scripts/slackware-build-freebasic.sh [options]

Options:
  --no-build      Reuse the existing source bootstrap tarball
  --no-package    Stop after ensuring the bootstrap tarball exists
  --skip-deps     Skip Slackware dependency installation
  --help          Show this help text

Environment:
  BUILDROOT       Temporary build root (default: <repo>/.build-slackware)
  WORKDIR         Workspace for source/package preparation
  OUTBASE         Output root (default: <repo>/out)
  JOBS            Parallel make job count

Artifacts are written under:
  out/linux/slackware/<release>/<arch>/
EOF
}

##############################################################################
# Options
##############################################################################

NO_BUILD=0
NO_PACKAGE=0
SKIP_DEPS=0

for arg in "$@"; do
    case "$arg" in
        --no-build) NO_BUILD=1 ;;
        --no-package) NO_PACKAGE=1 ;;
        --skip-deps) SKIP_DEPS=1 ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $arg"
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

BUILDROOT="${BUILDROOT:-$ROOT/.build-slackware}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
BUILDDIR="${BUILDDIR:-$WORKDIR/package}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
OUTBASE="${OUTBASE:-$ROOT/out}"
PREFIX="${PREFIX:-/usr}"
SOURCE_COPY_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
REV="$(sed -n 's/^REV[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"
[ -n "$REV" ] || REV=1

MACHINE="$(uname -m)"
case "$MACHINE" in
    x86_64|amd64)
        ARCH="x86_64"
        BOOTKEY="linux-x86_64"
        FBC_TARGET="linux-x86_64"
        ;;
    i386|i486|i586|i686)
        ARCH="i586"
        BOOTKEY="linux-x86"
        FBC_TARGET="linux-x86"
        ;;
    aarch64|arm64)
        ARCH="aarch64"
        BOOTKEY="linux-aarch64"
        FBC_TARGET="linux-aarch64"
        ;;
    *)
        die "unsupported Slackware architecture: $MACHINE"
        ;;
esac

TARGET_TRIPLET="$(gcc -dumpmachine 2>/dev/null || true)"
[ -n "$TARGET_TRIPLET" ] || TARGET_TRIPLET="${ARCH}-slackware-linux"

BOOTSTRAP_TAR="$ROOT/FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}.tar.xz"

DISTRO_ID="${FBC_PACKAGE_DISTRO_ID:-slackware}"
CODENAME="${FBC_PACKAGE_CODENAME:-unknown}"
if [ "$CODENAME" = "unknown" ] && [ -f /etc/slackware-version ]; then
    CODENAME="$(sed 's/^Slackware[[:space:]]*//' /etc/slackware-version | tr ' ' '-')"
fi

OUTDIR="${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}"
PKGFILE="$OUTDIR/freebasic-${VERSION}-${ARCH}-${REV}.txz"

mkdir -p "$WORKDIR" "$OUTDIR"

##############################################################################
# Dependencies
##############################################################################

configure_slackpkg_mirror() {
    local release="$1"
    local mirror

    [ -f /etc/slackpkg/mirrors ] || return 0

    if grep -Eq '^[[:space:]]*(http|https|ftp)://' /etc/slackpkg/mirrors; then
        return 0
    fi

    case "$release" in
        current)
            mirror="https://mirrors.slackware.com/slackware/slackware64-current/"
            ;;
        15.0)
            mirror="https://mirrors.slackware.com/slackware/slackware64-15.0/"
            ;;
        *)
            return 0
            ;;
    esac

    msg "configuring Slackware mirror: $mirror"
    printf '%s\n' "$mirror" >> /etc/slackpkg/mirrors
}

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    if command -v gcc >/dev/null 2>&1 && command -v "$MAKE_CMD" >/dev/null 2>&1; then
        return 0
    fi

    command -v slackpkg >/dev/null 2>&1 || die "missing gcc/make and slackpkg is not available"

    configure_slackpkg_mirror "$CODENAME"

    msg "installing Slackware build dependencies via slackpkg"
    local packages=(
        binutils
        glibc
        glibc-solibs
        kernel-headers
        'gcc-[0-9]*'
        gcc-g++
        gc
        guile
        libunistring
        make
        pkgconf
        pkg-config
        rsync
        tar
        xz
        gzip
        findutils
        flex
        file
        dos2unix
        diffutils
        which
        coreutils
        ncurses
        gpm
        libffi
        alsa-lib
        'pulseaudio-[0-9]*'
        dbus
        libsndfile
        flac
        libvorbis
        libogg
        opus
        mpg123
        lame
        libid3tag
        libasyncns
        lz4
        xxHash
        xorgproto
        libxcb
        libXau
        libXdmcp
        libX11
        libXext
        libXpm
        libXrandr
        libXrender
        mesa
        libglvnd
        glu
    )

    slackpkg -batch=on -default_answer=y update gpg || true
    run slackpkg -batch=on -default_answer=y update
    run slackpkg -batch=on -default_answer=y install "${packages[@]}"
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
    rm -rf "bootstrap/${BOOTKEY}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        FBC_TARGET="$FBC_TARGET" \
        FBTARGET_DIR_OVERRIDE="$BOOTKEY" \
        bootstrap-dist-target \
        -j"$JOBS"

    [ -f "$BOOTSTRAP_TAR" ] || die "bootstrap tarball was not created"
}

##############################################################################
# Source staging
##############################################################################

stage_source_tree() {
    local bootstrap_srcdir

    [ -f "$SOURCE_COPY_EXCLUDES" ] || die "missing source copy excludes: $SOURCE_COPY_EXCLUDES"

    rm -rf "$BUILDDIR" "$WORKDIR/bootstrap-from-tar"
    mkdir -p "$BUILDDIR"

    bootstrap_srcdir="$ROOT/bootstrap/$BOOTKEY"
    if [ ! -d "$bootstrap_srcdir" ]; then
        [ -f "$BOOTSTRAP_TAR" ] || die "missing bootstrap sources: $bootstrap_srcdir and $BOOTSTRAP_TAR"
        mkdir -p "$WORKDIR/bootstrap-from-tar"
        run tar -xJf "$BOOTSTRAP_TAR" -C "$WORKDIR/bootstrap-from-tar" \
            "FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}/bootstrap/${BOOTKEY}"
        bootstrap_srcdir="$WORKDIR/bootstrap-from-tar/FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}/bootstrap/${BOOTKEY}"
    fi

    run rsync -a --no-owner --no-group \
        --delete \
        --exclude-from="$SOURCE_COPY_EXCLUDES" \
        "$ROOT/" "$BUILDDIR/"

    mkdir -p "$BUILDDIR/bootstrap/$BOOTKEY"
    run rsync -a --no-owner --no-group --delete "$bootstrap_srcdir/" "$BUILDDIR/bootstrap/$BOOTKEY/"
}

##############################################################################
# Packaging
##############################################################################

write_slack_desc() {
    mkdir -p "$PKGROOT/install"
    cat > "$PKGROOT/install/slack-desc" <<'EOF'
         |-----handy-ruler------------------------------------------------------|
freebasic: freebasic (FreeBASIC compiler)
freebasic:
freebasic: FreeBASIC is a free, open source BASIC compiler for modern
freebasic: platforms. This package includes the compiler, runtime libraries,
freebasic: headers, examples, and documentation needed to build FreeBASIC
freebasic: programs on Slackware.
freebasic:
freebasic: Homepage: https://www.freebasic.net/
freebasic:
freebasic:
freebasic:
EOF
}

package_current_target() {
    msg "preparing Slackware package build"

    rm -rf "$PKGROOT"
    stage_source_tree

    msg "building FreeBASIC"
    (
        cd "$BUILDDIR"
        run "$MAKE_CMD" TARGET_TRIPLET="$TARGET_TRIPLET" FBC_TARGET="$FBC_TARGET" FBTARGET_DIR_OVERRIDE="$BOOTKEY" bootstrap-minimal -j"$JOBS"
        run "$MAKE_CMD" TARGET_TRIPLET="$TARGET_TRIPLET" FBC_TARGET="$FBC_TARGET" FBTARGET_DIR_OVERRIDE="$BOOTKEY" all FBC=bootstrap/fbc BUILD_FBC_TARGET="$FBC_TARGET" -j"$JOBS"
        mkdir -p .package-smoke
        cat > .package-smoke/console.bas <<'EOF'
print "Hello world"
EOF
        cat > .package-smoke/gfx.bas <<'EOF'
screenres 160, 100, 32
screenset 0, 0
color rgb(255, 255, 255), rgb(0, 0, 0)
cls
draw string (8, 8), "Hello world"
line (8, 28)-(120, 70), rgb(0, 200, 255), bf
print "Hello world"
sleep 50
screen 0
EOF
        cat > .package-smoke/gfx-screen13.bas <<'EOF'
screen 13
screenset 0, 0
pset (8, 8), 12
print "gfx screen 13"
screen 0
EOF
        cat > .package-smoke/sfx.bas <<'EOF'
print "sfx-start"
play "ABCDEFG"
print "sfx-end"
EOF
        run bin/fbc -v -prefix "$BUILDDIR" .package-smoke/console.bas -x .package-smoke/console
        run bin/fbc -v -prefix "$BUILDDIR" .package-smoke/gfx.bas -x .package-smoke/gfx
        run bin/fbc -v -prefix "$BUILDDIR" .package-smoke/gfx-screen13.bas -x .package-smoke/gfx-screen13
        run bin/fbc -v -prefix "$BUILDDIR" .package-smoke/sfx.bas -x .package-smoke/sfx
        (
            cd examples/sfxlib
            run ../../bin/fbc -v -prefix "$BUILDDIR" showcase.bas -x "$BUILDDIR/.package-smoke/sfx-showcase"
        )
        run "$MAKE_CMD" TARGET_TRIPLET="$TARGET_TRIPLET" FBC_TARGET="$FBC_TARGET" FBTARGET_DIR_OVERRIDE="$BOOTKEY" install DESTDIR="$PKGROOT" prefix="$PREFIX" FBC=bootstrap/fbc BUILD_FBC_TARGET="$FBC_TARGET"
        mkdir -p "$PKGROOT/usr/share/freebasic/examples"
        cp -a examples/. "$PKGROOT/usr/share/freebasic/examples/"
        mkdir -p "$PKGROOT/usr/doc/freebasic-$VERSION"
        cp -a readme.txt changelog.txt doc/gpl.txt doc/lgpl.txt "$PKGROOT/usr/doc/freebasic-$VERSION/"
        cp -a src/compiler/license.txt "$PKGROOT/usr/doc/freebasic-$VERSION/compiler-license.txt"
        cp -a src/rtlib/license.txt "$PKGROOT/usr/doc/freebasic-$VERSION/rtlib-license.txt"
        cp -a src/gfxlib2/license.txt "$PKGROOT/usr/doc/freebasic-$VERSION/gfxlib2-license.txt"
    )

    write_slack_desc

    rm -f "$PKGFILE"
    msg "running makepkg"
    (
        cd "$PKGROOT"
        run /sbin/makepkg -l y -c n "$PKGFILE"
    )

    [ -f "$PKGFILE" ] || die "Slackware package was not created: $PKGFILE"

    echo
    echo "==> build completed"
    echo "==> artifact: $PKGFILE"
    ls -lh "$OUTDIR"
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
    echo "==> $BOOTSTRAP_TAR"
    exit 0
fi

package_current_target

##############################################################################
# end of slackware-build-freebasic.sh
##############################################################################
