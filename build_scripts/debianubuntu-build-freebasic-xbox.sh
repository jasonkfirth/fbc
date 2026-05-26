#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

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

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

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
Usage: ./build_scripts/debianubuntu-build-freebasic-xbox.sh [options]

Options:
  --no-build       Reuse existing nxdk checkout and build outputs
  --no-package     Stop after building the Xbox target
  --skip-deps      Skip apt dependency installation
  --nxdk-dir DIR   Existing or desired nxdk checkout (default: <buildroot>/nxdk)
  --help           Show this help text

Environment:
  BUILDROOT        Temporary build root (default: <repo>/.build-debianubuntu-xbox)
  OUTBASE          Output root (default: <repo>/out)
  FBC_PACKAGE_OUTDIR
                   Full package output directory override
  JOBS             Parallel make job count
  NXDK_REPO        nxdk Git URL (default: https://github.com/XboxDev/nxdk.git)
  NXDK_REF         Optional nxdk branch/tag/commit to checkout

Artifacts are written under:
  out/linux/<distro>/<codename>/<arch>/xbox/

This is an experimental nxdk-based revival path for the existing Xbox target.
It packages the FreeBASIC Xbox runtime, a fbc-xbox wrapper, and the nxdk tree
used to build it.
EOF
}

NO_BUILD=0
NO_PACKAGE=0
SKIP_DEPS=0
NXDK_DIR_ARG=""

while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) NO_BUILD=1; shift ;;
        --no-package) NO_PACKAGE=1; shift ;;
        --skip-deps) SKIP_DEPS=1; shift ;;
        --nxdk-dir) NXDK_DIR_ARG="$2"; shift 2 ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

if ! command -v apt-get >/dev/null 2>&1; then
    die "this script requires an APT-based distribution (Debian/Ubuntu)"
fi

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

BUILDROOT="${BUILDROOT:-$ROOT/.build-debianubuntu-xbox}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
OUTBASE="${OUTBASE:-$ROOT/out}"
PKGROOT="$WORKDIR/package-root"
NXDK_REPO="${NXDK_REPO:-https://github.com/XboxDev/nxdk.git}"
NXDK_DIR="${NXDK_DIR_ARG:-${NXDK_DIR:-$BUILDROOT/nxdk}}"
XBOX_TARGET_TRIPLET="${XBOX_TARGET_TRIPLET:-i686-pc-xbox}"
XBOX_TARGET_KEY="${XBOX_TARGET_KEY:-xbox}"

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"
PACKAGE_VERSION="$(dpkg-parsechangelog --file "$ROOT/debian/changelog" --show-field Version 2>/dev/null || true)"
[ -n "$PACKAGE_VERSION" ] || PACKAGE_VERSION="${VERSION}-1"

ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
[ -n "$ARCH" ] || die "could not detect Debian architecture"

DISTRO_ID="unknown"
CODENAME="unknown"
if [ -f /etc/os-release ]; then
    DISTRO_ID="$(
        # shellcheck disable=SC1091
        . /etc/os-release
        printf '%s' "${ID:-unknown}"
    )"
    CODENAME="$(
        # shellcheck disable=SC1091
        . /etc/os-release
        printf '%s' "${VERSION_CODENAME:-unknown}"
    )"
fi

if [ -n "${FBC_PACKAGE_DISTRO_ID:-}" ]; then
    DISTRO_ID="$FBC_PACKAGE_DISTRO_ID"
fi

if [ -n "${FBC_PACKAGE_CODENAME:-}" ]; then
    CODENAME="$FBC_PACKAGE_CODENAME"
fi

OUTDIR="${FBC_PACKAGE_OUTDIR:-${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}/xbox}"

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Debian/Ubuntu nxdk build dependencies"
    export DEBIAN_FRONTEND=noninteractive
    run_root apt-get update -y
    run_root apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        make \
        cmake \
        flex \
        bison \
        clang \
        llvm \
        lld \
        git \
        dpkg-dev \
        fakeroot \
        rsync \
        tar \
        xz-utils \
        dos2unix \
        pkgconf \
        libncurses-dev \
        libtinfo-dev \
        libgpm-dev \
        libffi-dev
}

ensure_nxdk() {
    msg "preparing nxdk"
    mkdir -p "$(dirname "$NXDK_DIR")"

    if [ -d "$NXDK_DIR/.git" ]; then
        run git -C "$NXDK_DIR" submodule update --init --recursive
    elif [ -f "$NXDK_DIR/bin/activate" ] && [ -f "$NXDK_DIR/bin/nxdk-cc" ]; then
        echo "==> using installed nxdk tree: $NXDK_DIR"
    else
        run git clone --recursive "$NXDK_REPO" "$NXDK_DIR"
    fi

    if [ -n "${NXDK_REF:-}" ]; then
        [ -d "$NXDK_DIR/.git" ] || die "NXDK_REF requires an nxdk git checkout: $NXDK_DIR"
        run git -C "$NXDK_DIR" fetch --tags origin
        run git -C "$NXDK_DIR" checkout "$NXDK_REF"
        run git -C "$NXDK_DIR" submodule update --init --recursive
    fi

    [ -x "$NXDK_DIR/bin/activate" ] || die "nxdk activation script not found: $NXDK_DIR/bin/activate"
}

ensure_host_compiler() {
    if [ ! -x "./bin/fbc" ]; then
        msg "building host compiler"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" compiler -j"$JOBS"
    fi

    [ -x "./bin/fbc" ] || die "host compiler not available"
}

clean_xbox_target_outputs() {
    msg "cleaning stale Xbox target outputs"

    rm -rf \
        "lib/freebasic/$XBOX_TARGET_KEY" \
        "src/rtlib/obj/$XBOX_TARGET_KEY" \
        "src/fbrt/obj/$XBOX_TARGET_KEY" \
        "src/gfxlib2/obj/$XBOX_TARGET_KEY" \
        "src/sfxlib/obj/$XBOX_TARGET_KEY"
}

build_xbox_target() {
    msg "building FreeBASIC Xbox target with nxdk"

    ensure_host_compiler
    clean_xbox_target_outputs

    # nxdk exposes nxdk-cc/nxdk-cxx/nxdk-link/nxdk-lib after activation.
    # The existing FreeBASIC Xbox sources still carry OpenXDK-era assumptions,
    # so this is deliberately an experimental build step.
    eval "$("$NXDK_DIR/bin/activate" -s)"
    export PATH="$NXDK_DIR/bin:$PATH"

    command -v nxdk-cc >/dev/null 2>&1 || die "nxdk-cc not found after nxdk activation"
    command -v nxdk-cxx >/dev/null 2>&1 || die "nxdk-cxx not found after nxdk activation"
    command -v llvm-ar >/dev/null 2>&1 || die "llvm-ar not found"
    command -v llvm-ranlib >/dev/null 2>&1 || die "llvm-ranlib not found"

    run "$MAKE_CMD" \
        TARGET_TRIPLET="$XBOX_TARGET_TRIPLET" \
        TARGET_OS=xbox \
        TARGET_ARCH=x86 \
        DISABLE_MT=YesPlease \
        FBTARGET_DIR_OVERRIDE="$XBOX_TARGET_KEY" \
        BUILD_PREFIX= \
        CC=nxdk-cc \
        CXX=nxdk-cxx \
        LD=nxdk-link \
        AR=llvm-ar \
        RANLIB=llvm-ranlib \
        CXBE="$NXDK_DIR/tools/cxbe/cxbe" \
        FBC="./bin/fbc -i $ROOT/inc" \
        BUILD_FBC_TARGET=xbox \
        BUILD_FBC_BUILDPREFIX= \
		CPPFLAGS="-DHOST_XBOX -DDISABLE_FFI -DDISABLE_OPENGL -I$NXDK_DIR/lib -I$NXDK_DIR/lib/net/lwip/src/include -I$NXDK_DIR/lib/net/nforceif/include -I$NXDK_DIR/lib/net/nvnetdrv" \
		CFLAGS="-DHOST_XBOX -DDISABLE_FFI -DDISABLE_OPENGL -I$NXDK_DIR/lib -I$NXDK_DIR/lib/net/lwip/src/include -I$NXDK_DIR/lib/net/nforceif/include -I$NXDK_DIR/lib/net/nvnetdrv" \
        CXXFLAGS= \
        LDFLAGS= \
        rtlib fbrt gfxlib2 sfxlib \
        -j"$JOBS"

    [ -d "lib/freebasic/$XBOX_TARGET_KEY" ] || die "Xbox runtime directory was not created"
}

ensure_nxdk_tools() {
    msg "building nxdk host tools"
    if [ -f "$NXDK_DIR/tools/cxbe/cxbe" ]; then
        if [ ! -x "$NXDK_DIR/tools/cxbe/cxbe" ]; then
            chmod 755 "$NXDK_DIR/tools/cxbe/cxbe" 2>/dev/null ||
                die "nxdk cxbe exists but is not executable: $NXDK_DIR/tools/cxbe/cxbe"
        fi
        echo "==> using nxdk cxbe: $NXDK_DIR/tools/cxbe/cxbe"
        return 0
    fi

    [ -f "$NXDK_DIR/Makefile" ] || die "nxdk cxbe is missing and nxdk Makefile was not found: $NXDK_DIR"
    run "$MAKE_CMD" -C "$NXDK_DIR" cxbe
    [ -x "$NXDK_DIR/tools/cxbe/cxbe" ] || die "nxdk cxbe was not built: $NXDK_DIR/tools/cxbe/cxbe"
}

ensure_nxdk_runtime_libs() {
    msg "building nxdk runtime support libraries"

    eval "$("$NXDK_DIR/bin/activate" -s)"
    run "$MAKE_CMD" -C "$NXDK_DIR" \
        NXDK_ONLY=1 \
        NXDK_NET=y \
        main.exe \
        -j"$JOBS"

    [ -f "$NXDK_DIR/lib/libpdclib.lib" ] || die "nxdk libpdclib.lib was not built"
    [ -f "$NXDK_DIR/lib/libwinapi.lib" ] || die "nxdk libwinapi.lib was not built"
    [ -f "$NXDK_DIR/lib/libnxdk_net.lib" ] || die "nxdk libnxdk_net.lib was not built"
}

write_wrapper() {
    local wrapper="$1"

    cat > "$wrapper" <<'EOF'
#!/bin/sh
set -eu

prefix="${FBXBOX_PREFIX:-/usr/share/freebasic-xbox}"
nxdk="${NXDK_DIR:-$prefix/nxdk}"

if [ -x "$nxdk/bin/activate" ]; then
    eval "$("$nxdk/bin/activate" -s)"
fi

: "${CLANG:=$nxdk/bin/nxdk-cc}"
: "${GCC:=$nxdk/bin/nxdk-cc}"
: "${AS:=$nxdk/bin/nxdk-cc}"
: "${LD:=$nxdk/bin/nxdk-link}"
: "${AR:=$nxdk/bin/nxdk-lib}"
: "${CXBE:=$nxdk/tools/cxbe/cxbe}"

export CLANG GCC AS LD AR CXBE

exec fbc -target xbox -prefix "$prefix" "$@"
EOF
    chmod 755 "$wrapper"
}

package_xbox() {
    local package
    local control_dir
    local installed_size
    local nxdk_rev

    msg "packaging freebasic-xbox"

    rm -rf "$PKGROOT"
    mkdir -p \
        "$PKGROOT/DEBIAN" \
        "$PKGROOT/usr/bin" \
        "$PKGROOT/usr/share/freebasic-xbox/inc" \
        "$PKGROOT/usr/share/freebasic-xbox/lib/freebasic/$XBOX_TARGET_KEY" \
        "$PKGROOT/usr/share/doc/freebasic-xbox"

    rsync -a --delete inc/ "$PKGROOT/usr/share/freebasic-xbox/inc/"
    rsync -a --delete "lib/freebasic/$XBOX_TARGET_KEY/" "$PKGROOT/usr/share/freebasic-xbox/lib/freebasic/$XBOX_TARGET_KEY/"
    rsync -a --delete \
        --exclude '/.git/' \
        --exclude '/.gitmodules' \
        "$NXDK_DIR/" "$PKGROOT/usr/share/freebasic-xbox/nxdk/"

    write_wrapper "$PKGROOT/usr/bin/fbc-xbox"

    if [ -f debian/copyright ]; then
        install -m 644 debian/copyright "$PKGROOT/usr/share/doc/freebasic-xbox/copyright.FreeBASIC"
    elif [ -f copying.txt ]; then
        install -m 644 copying.txt "$PKGROOT/usr/share/doc/freebasic-xbox/copyright.FreeBASIC"
    else
        cat > "$PKGROOT/usr/share/doc/freebasic-xbox/copyright.FreeBASIC" <<EOF
FreeBASIC is distributed under the GNU General Public License version 2 or
later. On Debian systems, the full text of the GNU General Public License
version 2 can be found in /usr/share/common-licenses/GPL-2.
EOF
    fi

    nxdk_rev="$(git -C "$NXDK_DIR" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    (
        cd "$PKGROOT/usr/share/freebasic-xbox/nxdk"
        find . -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \) -print |
            sort |
            sed 's#^\./#/usr/share/freebasic-xbox/nxdk/#'
    ) > "$PKGROOT/usr/share/doc/freebasic-xbox/nxdk-license-files.txt"
    [ -s "$PKGROOT/usr/share/doc/freebasic-xbox/nxdk-license-files.txt" ] ||
        die "no nxdk license files found in packaged nxdk tree"

    cat > "$PKGROOT/usr/share/doc/freebasic-xbox/copyright" <<EOF
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: FreeBASIC Xbox target package
Source: https://freebasic.net/

Files: *
Copyright: FreeBASIC development team and contributors
License: GPL-2+
 See /usr/share/doc/freebasic-xbox/copyright.FreeBASIC for the FreeBASIC
 license text shipped with this source tree.

Files: usr/share/freebasic-xbox/nxdk/*
Copyright: XboxDev nxdk contributors and bundled third-party component authors
License: nxdk-mixed
 The packaged nxdk SDK tree is a mixed-license development kit. nxdk's README
 identifies bundled components including OpenXDK-derived code, pbkit, lwIP,
 PDCLib, compiler-rt-derived runtime code, Mesa-derived shader tools, NVIDIA
 SDK/Cg components, extract-xiso, SDL, zlib, libpng, libjpeg, and related
 third-party source trees. This package ships the upstream license files found
 in the nxdk checkout; see
 /usr/share/doc/freebasic-xbox/nxdk-license-files.txt for the full installed
 list and /usr/share/freebasic-xbox/nxdk/ for the corresponding texts.

License: nxdk-mixed
 This is an aggregate SDK license marker for the bundled nxdk checkout. Refer
 to the component license files listed in nxdk-license-files.txt.
EOF

    cat > "$PKGROOT/usr/share/doc/freebasic-xbox/README.Debian" <<EOF
freebasic-xbox
=============

This package was produced by build_scripts/debianubuntu-build-freebasic-xbox.sh.
It contains the experimental FreeBASIC Xbox target runtime and the nxdk checkout
used to build it.

nxdk revision: $nxdk_rev

Use:
  fbc-xbox program.bas

The Xbox target is being revived from old OpenXDK-oriented FreeBASIC code, so
source-level compatibility fixes may still be required.
EOF

    installed_size="$(du -sk "$PKGROOT/usr" | awk '{print $1}')"
    control_dir="$PKGROOT/DEBIAN"
    cat > "$control_dir/control" <<EOF
Package: freebasic-xbox
Version: $PACKAGE_VERSION
Section: non-free/devel
Priority: optional
Architecture: $ARCH
Maintainer: SJ_Zero <sj@fbxl.net>
Depends: freebasic, clang, llvm, lld, make
Installed-Size: $installed_size
Homepage: https://freebasic.net/
Description: experimental FreeBASIC compiler support for original Xbox
 This package provides fbc-xbox, the FreeBASIC Xbox target runtime, and the
 nxdk SDK tree used to build original Xbox binaries.
EOF

    find "$PKGROOT" -type d -print0 | xargs -0 chmod 755
    find "$PKGROOT" -type f -print0 | xargs -0 chmod 644
    find "$PKGROOT/usr/share/freebasic-xbox/nxdk/bin" -type f -print0 | xargs -0 chmod 755
    if [ -f "$PKGROOT/usr/share/freebasic-xbox/nxdk/tools/cxbe/cxbe" ]; then
        chmod 755 "$PKGROOT/usr/share/freebasic-xbox/nxdk/tools/cxbe/cxbe"
    fi
    chmod 755 "$PKGROOT/usr/bin/fbc-xbox"

    mkdir -p "$OUTDIR"
    package="$OUTDIR/freebasic-xbox_${PACKAGE_VERSION}_${ARCH}.deb"
    fakeroot dpkg-deb --build "$PKGROOT" "$package"

    echo
    echo "==> build completed"
    echo "==> artifact: $package"
}

mkdir -p "$WORKDIR" "$OUTDIR"

install_deps
ensure_nxdk

if [ "$NO_BUILD" -eq 0 ]; then
    ensure_nxdk_runtime_libs
    build_xbox_target
fi

if [ "$NO_PACKAGE" -eq 1 ]; then
    msg "Xbox target build ready"
    exit 0
fi

ensure_nxdk_tools
package_xbox
