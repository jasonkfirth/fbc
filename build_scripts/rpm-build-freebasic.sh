#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: rpm-build-freebasic.sh
#
# Purpose:
#
#     Build one native RPM-family FreeBASIC binary package from inside the
#     distro container that owns the package format.
#
# Responsibilities:
#
#     * install RPM-family build dependencies
#     * stage a clean FreeBASIC source tree with bootstrap sources
#     * write a distro-aware RPM spec for the staged tree
#     * copy the finished RPMs into out/linux/<distro>/<release>/<arch>
#
# This file intentionally does NOT contain:
#
#     * the top-level Linux package matrix
#     * target package validation
#     * Debian/APK/Slackware package policy
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
Usage: ./build_scripts/rpm-build-freebasic.sh [options]

Options:
  --no-build      Reuse the existing source bootstrap tarball
  --no-package    Stop after ensuring the bootstrap tarball exists
  --skip-deps     Skip RPM-family dependency installation
  --help          Show this help text

Environment:
  BUILDROOT       Temporary build root (default: <repo>/.build-rpm)
  WORKDIR         Workspace for source/package preparation
  OUTBASE         Output root (default: <repo>/out)
  JOBS            Parallel make job count

Artifacts are written under:
  out/linux/<fedora-rocky-almalinux-opensuse>/<release>/<arch>/
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

BUILDROOT="${BUILDROOT:-$ROOT/.build-rpm}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
BUILDDIR="${BUILDDIR:-$WORKDIR/package}"
RPMTOP="${RPMTOP:-$BUILDROOT/rpmbuild}"
OUTBASE="${OUTBASE:-$ROOT/out}"
PREFIX="${PREFIX:-/usr}"
SOURCE_COPY_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
REV="$(sed -n 's/^REV[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"
[ -n "$REV" ] || REV=1

RPM_ARCH="$(rpm --eval '%{_arch}' 2>/dev/null || uname -m)"
[ -n "$RPM_ARCH" ] || die "could not detect RPM architecture"

case "$RPM_ARCH" in
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
    armv7hl|armv7hnl|armhfp|armv7l)
        ARCH="armv7hl"
        BOOTKEY="linux-arm"
        FBC_TARGET="linux-arm"
        ;;
    ppc64le|ppc64el)
        ARCH="ppc64le"
        BOOTKEY="linux-powerpc64le"
        FBC_TARGET="linux-powerpc64le"
        ;;
    s390x)
        ARCH="s390x"
        BOOTKEY="linux-s390x"
        FBC_TARGET="linux-s390x"
        ;;
    riscv64)
        ARCH="riscv64"
        BOOTKEY="linux-riscv64"
        FBC_TARGET="linux-riscv64"
        ;;
    *)
        die "unsupported RPM architecture: $RPM_ARCH"
        ;;
esac

TARGET_TRIPLET="$(gcc -dumpmachine 2>/dev/null || true)"
[ -n "$TARGET_TRIPLET" ] || TARGET_TRIPLET="${ARCH}-linux-gnu"

BOOTSTRAP_TAR="$ROOT/FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}.tar.xz"

DISTRO_ID=""
CODENAME=""
if [ -f /etc/os-release ]; then
    DISTRO_ID="$(
        # shellcheck disable=SC1091
        . /etc/os-release
        printf '%s' "${ID:-}"
    )"
    CODENAME="$(
        # shellcheck disable=SC1091
        . /etc/os-release
        printf '%s' "${VERSION_ID:-${VERSION_CODENAME:-unknown}}"
    )"
fi

if [ -n "${FBC_PACKAGE_DISTRO_ID:-}" ]; then
    DISTRO_ID="$FBC_PACKAGE_DISTRO_ID"
fi

if [ -n "${FBC_PACKAGE_CODENAME:-}" ]; then
    CODENAME="$FBC_PACKAGE_CODENAME"
fi

[ -n "$DISTRO_ID" ] || DISTRO_ID="rpm"
[ -n "$CODENAME" ] || CODENAME="unknown"

OUTDIR="${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}"
mkdir -p "$WORKDIR" "$OUTDIR"

##############################################################################
# Dependencies
##############################################################################

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    if command -v dnf >/dev/null 2>&1; then
        msg "installing RPM build dependencies via dnf"
        if [ "$DISTRO_ID" = "fedora" ]; then
            run sed -i -e 's/^enabled=1/enabled=0/' /etc/yum.repos.d/fedora-cisco-openh264.repo
            run sed -i \
                -e 's|^metalink=|#metalink=|' \
                -e 's|^#baseurl=http://download.example/pub/fedora/linux|baseurl=https://dl.fedoraproject.org/pub/fedora/linux|' \
                /etc/yum.repos.d/fedora.repo \
                /etc/yum.repos.d/fedora-updates.repo \
                /etc/yum.repos.d/fedora-updates-testing.repo
        fi
        run dnf clean all
        run dnf --refresh --setopt=install_weak_deps=False install -y \
            rpm-build redhat-rpm-config \
            gcc gcc-c++ make pkgconf-pkg-config rsync tar xz gzip \
            findutils file dos2unix diffutils which \
            ncurses-devel gpm-devel libffi-devel \
            alsa-lib-devel pulseaudio-libs-devel \
            libX11-devel libXext-devel libXpm-devel libXrandr-devel \
            libXrender-devel mesa-libGL-devel mesa-libGLU-devel
        return 0
    fi

    if command -v yum >/dev/null 2>&1; then
        msg "installing RPM build dependencies via yum"
        run yum install -y \
            rpm-build redhat-rpm-config \
            gcc gcc-c++ make pkgconfig rsync tar xz gzip \
            findutils file dos2unix diffutils which \
            ncurses-devel gpm-devel libffi-devel \
            alsa-lib-devel pulseaudio-libs-devel \
            libX11-devel libXext-devel libXpm-devel libXrandr-devel \
            libXrender-devel mesa-libGL-devel mesa-libGLU-devel
        return 0
    fi

    if command -v zypper >/dev/null 2>&1; then
        msg "installing RPM build dependencies via zypper"
        run zypper --non-interactive install -y \
            rpm-build gcc gcc-c++ make pkgconf-pkg-config rsync tar xz gzip \
            findutils file dos2unix diffutils which \
            ncurses-devel gpm-devel libffi-devel \
            alsa-devel libpulse-devel \
            libX11-devel libXext-devel libXpm-devel libXrandr-devel \
            libXrender-devel Mesa-libGL-devel Mesa-libGLU-devel
        return 0
    fi

    die "unsupported RPM-family package manager"
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
# RPM spec generation
##############################################################################

rpm_requires() {
    case "$DISTRO_ID" in
        opensuse*)
            cat <<'EOF'
Requires:       gcc
Requires:       binutils
Requires:       glibc-devel
Requires:       ncurses-devel
Requires:       gpm-devel
Requires:       libffi-devel
Requires:       alsa-devel
Requires:       libpulse-devel
Requires:       libX11-devel
Requires:       libXext-devel
Requires:       libXpm-devel
Requires:       libXrandr-devel
Requires:       libXrender-devel
Requires:       Mesa-libGL-devel
Requires:       Mesa-libGLU-devel
EOF
            ;;
        *)
            cat <<'EOF'
Requires:       gcc
Requires:       binutils
Requires:       glibc-devel
Requires:       ncurses-devel
Requires:       gpm-devel
Requires:       libffi-devel
Requires:       alsa-lib-devel
Requires:       pulseaudio-libs-devel
Requires:       libX11-devel
Requires:       libXext-devel
Requires:       libXpm-devel
Requires:       libXrandr-devel
Requires:       libXrender-devel
Requires:       mesa-libGL-devel
Requires:       mesa-libGLU-devel
EOF
            ;;
    esac
}

write_spec() {
    local spec="$RPMTOP/SPECS/freebasic.spec"

    mkdir -p "$RPMTOP/SPECS"

    {
        cat <<EOF
Name:           freebasic
Version:        $VERSION
Release:        ${REV}%{?dist}
Summary:        FreeBASIC compiler

License:        GPL-2.0-or-later AND LGPL-2.1-or-later
URL:            https://www.freebasic.net/
Source0:        %{name}-%{version}.tar.xz

%define debug_package %{nil}
%define _lto_cflags %{nil}
%define _missing_build_ids_terminate_build 0

BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  pkgconfig
BuildRequires:  rsync
BuildRequires:  dos2unix
BuildRequires:  ncurses-devel
BuildRequires:  gpm-devel
BuildRequires:  libffi-devel
EOF
        if [ "$DISTRO_ID" = "opensuse" ]; then
            cat <<'EOF'
BuildRequires:  alsa-devel
BuildRequires:  libpulse-devel
BuildRequires:  libX11-devel
BuildRequires:  libXext-devel
BuildRequires:  libXpm-devel
BuildRequires:  libXrandr-devel
BuildRequires:  libXrender-devel
BuildRequires:  Mesa-libGL-devel
BuildRequires:  Mesa-libGLU-devel
EOF
        else
            cat <<'EOF'
BuildRequires:  alsa-lib-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  libX11-devel
BuildRequires:  libXext-devel
BuildRequires:  libXpm-devel
BuildRequires:  libXrandr-devel
BuildRequires:  libXrender-devel
BuildRequires:  mesa-libGL-devel
BuildRequires:  mesa-libGLU-devel
EOF
        fi
        rpm_requires
        cat <<EOF

%description
FreeBASIC is a free, open source BASIC compiler for modern platforms. It
includes the compiler, runtime libraries, graphics support, sound support,
headers, examples, and documentation needed to build FreeBASIC programs.

%prep
%autosetup -n %{name}-%{version}

%build
unset CFLAGS CXXFLAGS CPPFLAGS FFLAGS FCFLAGS LDFLAGS RPM_OPT_FLAGS
export CFLAGS=
export CXXFLAGS=
export CPPFLAGS=
export FFLAGS=
export FCFLAGS=
export LDFLAGS=
export RPM_OPT_FLAGS=

$MAKE_CMD TARGET_TRIPLET="$TARGET_TRIPLET" FBC_TARGET="$FBC_TARGET" FBTARGET_DIR_OVERRIDE="$BOOTKEY" CFLAGS= CXXFLAGS= CPPFLAGS= LDFLAGS= bootstrap-minimal -j"$JOBS"
$MAKE_CMD TARGET_TRIPLET="$TARGET_TRIPLET" FBC_TARGET="$FBC_TARGET" FBTARGET_DIR_OVERRIDE="$BOOTKEY" CFLAGS= CXXFLAGS= CPPFLAGS= LDFLAGS= all FBC=bootstrap/fbc BUILD_FBC_TARGET="$FBC_TARGET" -j"$JOBS"

mkdir -p .package-smoke
cat > .package-smoke/console.bas <<'SMOKEEOF'
print "Hello world"
SMOKEEOF
cat > .package-smoke/gfx.bas <<'SMOKEEOF'
screenres 160, 100, 32
screenset 0, 0
color rgb(255, 255, 255), rgb(0, 0, 0)
cls
draw string (8, 8), "Hello world"
line (8, 28)-(120, 70), rgb(0, 200, 255), bf
print "Hello world"
sleep 50
screen 0
SMOKEEOF
cat > .package-smoke/gfx-screen13.bas <<'SMOKEEOF'
screen 13
screenset 0, 0
pset (8, 8), 12
print "gfx screen 13"
screen 0
SMOKEEOF
cat > .package-smoke/sfx.bas <<'SMOKEEOF'
print "sfx-start"
play "ABCDEFG"
print "sfx-end"
SMOKEEOF
bin/fbc -v -prefix "\$PWD" .package-smoke/console.bas -x .package-smoke/console
bin/fbc -v -prefix "\$PWD" .package-smoke/gfx.bas -x .package-smoke/gfx
bin/fbc -v -prefix "\$PWD" .package-smoke/gfx-screen13.bas -x .package-smoke/gfx-screen13
bin/fbc -v -prefix "\$PWD" .package-smoke/sfx.bas -x .package-smoke/sfx
(
    cd examples/sfxlib
    ../../bin/fbc -v -prefix ../.. showcase.bas -x ../../.package-smoke/sfx-showcase
)

%install
$MAKE_CMD TARGET_TRIPLET="$TARGET_TRIPLET" FBC_TARGET="$FBC_TARGET" FBTARGET_DIR_OVERRIDE="$BOOTKEY" CFLAGS= CXXFLAGS= CPPFLAGS= LDFLAGS= install DESTDIR=%{buildroot} prefix=$PREFIX FBC=bootstrap/fbc BUILD_FBC_TARGET="$FBC_TARGET"
mkdir -p %{buildroot}%{_datadir}/freebasic/examples
cp -a examples/. %{buildroot}%{_datadir}/freebasic/examples/

%files
%license doc/gpl.txt doc/lgpl.txt
%doc readme.txt changelog.txt
%{_bindir}/fbc
%{_includedir}/freebasic
/usr/lib/freebasic
%{_datadir}/freebasic

%changelog
* Thu May 14 2026 FreeBASIC packagers <packagers@example.invalid> - $VERSION-$REV
- Automated Linux package factory build.
EOF
    } > "$spec"
}

##############################################################################
# Packaging
##############################################################################

package_current_target() {
    local src_tar

    msg "preparing RPM package build"

    rm -rf "$RPMTOP"
    mkdir -p "$RPMTOP/BUILD" "$RPMTOP/BUILDROOT" "$RPMTOP/RPMS" "$RPMTOP/SOURCES" "$RPMTOP/SPECS" "$RPMTOP/SRPMS"
    stage_source_tree

    msg "creating RPM source tarball"
    src_tar="$RPMTOP/SOURCES/freebasic-$VERSION.tar.xz"
    run tar -C "$WORKDIR" -cJf "$src_tar" --transform "s#^package#freebasic-$VERSION#" package
    [ -s "$src_tar" ] || die "source tarball was not created: $src_tar"

    write_spec

    msg "running rpmbuild"
    run rpmbuild --define "_topdir $RPMTOP" -bb "$RPMTOP/SPECS/freebasic.spec"

    run find "$RPMTOP/RPMS" -type f -name '*.rpm' -exec cp -f {} "$OUTDIR/" \;

    if ! find "$OUTDIR" -maxdepth 1 -type f -name '*.rpm' | grep -q .; then
        die "rpmbuild did not create RPM artifacts under $OUTDIR"
    fi

    echo
    echo "==> build completed"
    echo "==> artifacts:"
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
# end of rpm-build-freebasic.sh
##############################################################################
