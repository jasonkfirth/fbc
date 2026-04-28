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
# Ensure Debian / Ubuntu style environment
##############################################################################

if ! command -v apt-get >/dev/null 2>&1; then
    echo "ERROR: this script requires an APT-based distribution (Debian/Ubuntu)"
    exit 1
fi

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

assert_removable_tree() {
    local path="$1"
    [ -e "$path" ] || return 0
    if ! find "$path" ! -type l \( ! -user "$(id -u)" -o ! -writable \) -print -quit | grep -q .; then
        return 0
    fi
    die "build workspace is not writable: $path
Run: sudo chown -R $(id -un):$(id -gn) '$path'"
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
Usage: ./build_scripts/debianubuntu-build-freebasic.sh [options]

Options:
  --no-build      Reuse the existing source bootstrap tarball
  --no-js         Build packages with DEB_BUILD_PROFILES=nojs
  --no-package    Stop after ensuring the bootstrap tarball exists
  --skip-deps     Skip apt dependency installation
  --help          Show this help text

Environment:
  BUILDROOT       Temporary build root (default: <repo>/.build-debianubuntu)
  WORKDIR         Workspace for bootstrap/package preparation
  OUTBASE         Output root (default: <repo>/out)
  FBC_PACKAGE_OUTDIR
                  Full package output directory override
  FBC_PACKAGE_ARM_ARCH
                  ARM default arch override for package builds (armv6+fp)
  JOBS            Parallel make job count for bootstrap generation

Artifacts are written under:
  out/linux/<distro>/<codename>/<arch>/
EOF
}

##############################################################################
# Options
##############################################################################

NO_BUILD=0
NO_JS=0
NO_PACKAGE=0
SKIP_DEPS=0

for arg in "$@"; do
    case "$arg" in
        --no-build) NO_BUILD=1 ;;
        --no-js) NO_JS=1 ;;
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
# Tooling
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

##############################################################################
# Config
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-debianubuntu}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
OUTBASE="${OUTBASE:-$ROOT/out}"
BUILDDIR="${BUILDDIR:-$WORKDIR/package}"

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"

ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
[ -n "$ARCH" ] || die "could not detect Debian architecture"

case "$ARCH" in
    amd64)
        BOOTKEY="linux-amd64"
        FBC_TARGET="linux-x86_64"
        ;;
    i386)
        BOOTKEY="linux-i386"
        FBC_TARGET="linux-x86"
        ;;
    arm64)
        BOOTKEY="linux-arm64"
        FBC_TARGET="linux-aarch64"
        ;;
    armhf)
        BOOTKEY="linux-armhf"
        FBC_TARGET="linux-arm"
        ;;
    armel)
        BOOTKEY="linux-armel"
        FBC_TARGET="linux-arm"
        ;;
    ppc64el)
        BOOTKEY="linux-ppc64el"
        FBC_TARGET="linux-powerpc64le"
        ;;
    s390x)
        BOOTKEY="linux-s390x"
        FBC_TARGET="linux-s390x"
        ;;
    riscv64)
        BOOTKEY="linux-riscv64"
        FBC_TARGET="linux-riscv64"
        ;;
    loong64)
        BOOTKEY="linux-loongarch64"
        FBC_TARGET="linux-loongarch64"
        ;;
    *)
        die "unsupported Debian architecture: $ARCH"
        ;;
esac

BOOTSTRAP_TAR="FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}.tar.xz"

ARM_MAKE_ARGS=()
case "${FBC_PACKAGE_ARM_ARCH:-}" in
    "")
        ;;
    armv6+fp)
        ARM_MAKE_ARGS=(
            ARM_VER=v6
            ARM_FLOAT_ABI=hf
            DEFAULT_CPUTYPE_ARM=FB_CPUTYPE_ARMV6_FP
        )
        ;;
    *)
        die "unsupported FBC_PACKAGE_ARM_ARCH: $FBC_PACKAGE_ARM_ARCH"
        ;;
esac

DISTRO_ID=""
CODENAME=""
if [ -f /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    DISTRO_ID="${ID:-}"
    CODENAME="${VERSION_CODENAME:-}"
fi

if [ -z "$DISTRO_ID" ] && command -v lsb_release >/dev/null 2>&1; then
    DISTRO_ID="$(lsb_release -is 2>/dev/null | tr '[:upper:]' '[:lower:]' || true)"
fi

if [ -z "$CODENAME" ] && command -v lsb_release >/dev/null 2>&1; then
    CODENAME="$(lsb_release -sc 2>/dev/null || true)"
fi

[ -n "$DISTRO_ID" ] || DISTRO_ID="unknown"
[ -n "$CODENAME" ] || CODENAME="unknown"

if [ -n "${FBC_PACKAGE_DISTRO_ID:-}" ]; then
    DISTRO_ID="$FBC_PACKAGE_DISTRO_ID"
fi

if [ -n "${FBC_PACKAGE_CODENAME:-}" ]; then
    CODENAME="$FBC_PACKAGE_CODENAME"
fi

OUTDIR="${FBC_PACKAGE_OUTDIR:-${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}}"

mkdir -p "$WORKDIR" "$OUTDIR"

##############################################################################
# Dependency installation
##############################################################################

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Debian/Ubuntu build dependencies"

    export DEBIAN_FRONTEND=noninteractive
    export TERM=dumb
    export NCURSES_NO_UTF8_ACS=1
    export DEB_BUILD_MAINT_OPTIONS="hardening=+all"

    run_root apt-get update -y
    local js_deps=()
    if [ "$NO_JS" -eq 0 ]; then
        js_deps=(emscripten nodejs)
    fi

    run_root apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential gcc g++ binutils make \
        pkgconf rsync \
        debhelper dpkg-dev devscripts fakeroot lintian \
        quilt dos2unix \
        tar xz-utils \
        libncurses-dev libtinfo-dev libgpm-dev libffi-dev \
        libasound2-dev libpulse-dev \
        libx11-dev libxpm-dev libxext-dev libxrandr-dev libxrender-dev \
        libxcb1-dev libxau-dev libxdmcp-dev \
        libxi-dev libxinerama-dev libxxf86vm-dev \
        libgl1-mesa-dev libglu1-mesa-dev \
        "${js_deps[@]}" \
        perl python3 git
}

##############################################################################
# Bootstrap generation
##############################################################################

ensure_host_compiler() {
    if [ ! -x "./bin/fbc" ]; then
        msg "building host compiler"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$JOBS"
    fi

    [ -x "./bin/fbc" ] || die "host compiler not available"
}

build_bootstrap_tarball() {
    msg "building bootstrap tarball: $BOOTSTRAP_TAR"

    ensure_host_compiler

    rm -f "$BOOTSTRAP_TAR"
    rm -rf "bootstrap/${BOOTKEY}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        FBC_TARGET="$FBC_TARGET" \
        FBTARGET_DIR_OVERRIDE="$BOOTKEY" \
        "${ARM_MAKE_ARGS[@]}" \
        bootstrap-dist-target \
        -j"$JOBS"

    [ -f "$BOOTSTRAP_TAR" ] || die "bootstrap tarball was not created"
}

##############################################################################
# Debian packaging
##############################################################################

package_current_target() {
    local srcdir
    local pkgname
    local fullver
    local upver
    local origtar
    local rc
    local bootstrap_srcdir

    msg "preparing Debian package build"

    assert_removable_tree "$BUILDDIR"
    assert_removable_tree "$WORKDIR/bootstrap-from-tar"

    rm -rf "$BUILDDIR"
    run mkdir -p "$BUILDDIR"

    pkgname="$(dpkg-parsechangelog --file "$ROOT/debian/changelog" --show-field Source 2>/dev/null || true)"
    fullver="$(dpkg-parsechangelog --file "$ROOT/debian/changelog" --show-field Version 2>/dev/null || true)"

    [ -n "$pkgname" ] || die "could not parse package name"
    [ -n "$fullver" ] || die "could not parse package version"

    upver="${fullver%%-*}"
    srcdir="${pkgname}-${upver}"
    bootstrap_srcdir="$ROOT/bootstrap/$BOOTKEY"

    msg "staging Debian source tree"

    if [ ! -d "$bootstrap_srcdir" ]; then
        [ -f "$BOOTSTRAP_TAR" ] || die "missing bootstrap sources: $bootstrap_srcdir and $BOOTSTRAP_TAR"
        assert_removable_tree "$WORKDIR/bootstrap-from-tar"
        run mkdir -p "$WORKDIR/bootstrap-from-tar"
        run tar -xJf "$BOOTSTRAP_TAR" -C "$WORKDIR/bootstrap-from-tar" \
            "FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}/bootstrap/${BOOTKEY}"
        bootstrap_srcdir="$WORKDIR/bootstrap-from-tar/FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}/bootstrap/${BOOTKEY}"
    fi

    run mkdir -p "$BUILDDIR/$srcdir"

    run rsync -a --no-owner --no-group \
        --delete \
        --exclude '/.build-alpine/' \
        --exclude '/.build-debianubuntu/' \
        --exclude '/.codex/' \
        --exclude '/FreeBASIC-*-source-bootstrap-*.tar.*' \
        --exclude '/bin/' \
        --exclude '/bootstrap/' \
        --exclude '/lib/freebasic/' \
        --exclude '/obj/' \
        --exclude '/src/*/obj/' \
        --exclude '/out/' \
        --exclude '/stage/' \
        --exclude '/tmp/' \
        --exclude '/tests/*.log' \
        --exclude '/tests/*.tmp' \
        "$ROOT/" "$BUILDDIR/$srcdir/"

    run mkdir -p "$BUILDDIR/$srcdir/bootstrap/$BOOTKEY"
    run rsync -a --no-owner --no-group --delete "$bootstrap_srcdir/" "$BUILDDIR/$srcdir/bootstrap/$BOOTKEY/"

    cd "$BUILDDIR/$srcdir"

    [ -f debian/control ] || die "missing debian/control"
    [ -f debian/changelog ] || die "missing debian/changelog"
    [ -f GNUmakefile ] || [ -f makefile ] || [ -f Makefile ] || die "missing GNUmakefile/makefile/Makefile"

    echo "==> package name: $pkgname"
    echo "==> upstream version: $upver"
    echo "==> output dir: $OUTDIR"
    [ -z "${FBC_PACKAGE_ARM_ARCH:-}" ] || echo "==> ARM default arch: $FBC_PACKAGE_ARM_ARCH"
    [ "$NO_JS" -eq 0 ] || echo "==> build profile: nojs"

    cd "$BUILDDIR"

    origtar="${pkgname}_${upver}.orig.tar.xz"
    rm -f "$origtar"
    run tar -cJf "$origtar" \
        --exclude="$srcdir/debian" \
        --exclude="$srcdir/.build-alpine" \
        --exclude="$srcdir/.build-debianubuntu" \
        --exclude="$srcdir/out" \
        --exclude="$srcdir/stage" \
        "$srcdir"

    cd "$srcdir"

    if [ -f debian/rules ]; then
        chmod +x debian/rules || true
    fi

    rm -f contrib/swig/swig.exe || true

    msg "running dpkg-buildpackage"

    set +e
    if [ "$NO_JS" -eq 1 ]; then
        DEB_BUILD_PROFILES=nojs dpkg-buildpackage -us -uc 2>&1 | tee "$OUTDIR/build.log"
    else
        dpkg-buildpackage -us -uc 2>&1 | tee "$OUTDIR/build.log"
    fi
    rc=${PIPESTATUS[0]}
    set -e

    if [ "$rc" -ne 0 ]; then
        echo "ERROR: dpkg-buildpackage failed (exit=$rc)"
        tail -200 "$OUTDIR/build.log" || true
        exit "$rc"
    fi

    msg "collecting package artifacts"

    rm -f "$OUTDIR"/freebasic*.deb \
          "$OUTDIR"/freebasic*.ddeb \
          "$OUTDIR"/freebasic*.dsc \
          "$OUTDIR"/freebasic*.tar.* \
          "$OUTDIR"/freebasic*.buildinfo \
          "$OUTDIR"/freebasic*.changes \
          "$OUTDIR"/lintian-freebasic*.log

    shopt -s nullglob
    for f in ../*.deb ../*.ddeb ../*.dsc ../*.tar.* ../*.buildinfo ../*.changes; do
        [ -e "$f" ] || continue
        cp -av "$f" "$OUTDIR/" || true
    done
    shopt -u nullglob

    shopt -s nullglob
    for deb in "$OUTDIR"/*.deb; do
        [ -f "$deb" ] || continue
        lintian -IE --pedantic "$deb" | tee "$OUTDIR/lintian-$(basename "$deb").log" || true
    done
    shopt -u nullglob

    echo
    echo "==> build completed"
    echo "==> artifacts in $OUTDIR"
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
