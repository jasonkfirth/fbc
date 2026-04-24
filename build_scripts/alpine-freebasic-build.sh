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
# Ensure Alpine / postmarketOS
##############################################################################

if ! command -v apk >/dev/null 2>&1; then
    echo "ERROR: this script requires an apk-based Alpine/postmarketOS system"
    exit 1
fi

OS_ID=""
OS_VERSION_ID=""
OS_VERSION_CODENAME=""
if [ -f /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    OS_ID="${ID:-}"
    OS_VERSION_ID="${VERSION_ID:-}"
    OS_VERSION_CODENAME="${VERSION_CODENAME:-}"
fi

if [ "$OS_ID" != "alpine" ] && [ "$OS_ID" != "postmarketos" ] && [ ! -f /etc/alpine-release ]; then
    echo "ERROR: this script requires Alpine Linux or postmarketOS"
    exit 1
fi

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

usage() {
    cat <<EOF
Usage: ./build_scripts/alpine-freebasic-build.sh [options]

Options:
  --no-build      Reuse the existing source bootstrap tarball
  --no-package    Stop after ensuring the bootstrap tarball exists
  --skip-deps     Skip apk dependency installation
  --help          Show this help text

Environment:
  BUILDROOT       Temporary build root (default: <repo>/.build-alpine)
  WORKDIR         Workspace for bootstrap/package preparation
  OUTBASE         Output root (default: <repo>/out)
  JOBS            Parallel make job count

Artifacts are written under:
  out/linux/<alpine-or-postmarketos>/<release>/<arch>/
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

BUILDROOT="${BUILDROOT:-$ROOT/.build-alpine}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
OUTBASE="${OUTBASE:-$ROOT/out}"
BUILDDIR="${BUILDDIR:-$WORKDIR/package}"
STAGE="${STAGE:-$BUILDROOT/stage}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
APKBUILDDIR="${APKBUILDDIR:-$BUILDROOT/apkbuild}"
PREFIX="${PREFIX:-/usr}"

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
REV="$(sed -n 's/^REV[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"
[ -n "$REV" ] || REV=1

ARCH="$(apk --print-arch)"
[ -n "$ARCH" ] || die "could not detect Alpine architecture"

case "$ARCH" in
    x86_64)
        BOOTKEY="linux-x86_64"
        FBC_TARGET="linux-x86_64"
        ;;
    x86)
        BOOTKEY="linux-x86"
        FBC_TARGET="linux-x86"
        ;;
    aarch64)
        BOOTKEY="linux-aarch64"
        FBC_TARGET="linux-aarch64"
        ;;
    armv7|armhf)
        BOOTKEY="linux-arm"
        FBC_TARGET="linux-arm"
        ;;
    ppc64le)
        BOOTKEY="linux-powerpc64le"
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
    *)
        die "unsupported Alpine architecture: $ARCH"
        ;;
esac

BOOTSTRAP_TAR="FreeBASIC-${VERSION}-source-bootstrap-${BOOTKEY}.tar.xz"

DISTRO_ID="$OS_ID"
[ -n "$DISTRO_ID" ] || DISTRO_ID="alpine"

if [ -n "$OS_VERSION_CODENAME" ]; then
    CODENAME="$OS_VERSION_CODENAME"
elif [ -n "$OS_VERSION_ID" ]; then
    CODENAME="$OS_VERSION_ID"
elif [ -f /etc/alpine-release ]; then
    CODENAME="$(cat /etc/alpine-release)"
else
    CODENAME="unknown"
fi

if [ -n "${FBC_PACKAGE_DISTRO_ID:-}" ]; then
    DISTRO_ID="$FBC_PACKAGE_DISTRO_ID"
fi

if [ -n "${FBC_PACKAGE_CODENAME:-}" ]; then
    CODENAME="$FBC_PACKAGE_CODENAME"
fi

OUTDIR="${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}"
PKGNAME="freebasic"
PKGVERSION="${VERSION}-r${REV}"
APKFILE="${OUTDIR}/${PKGNAME}-${PKGVERSION}.apk"

mkdir -p "$WORKDIR" "$OUTDIR"

##############################################################################
# Dependencies
##############################################################################

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Alpine build dependencies"

    run apk update
    run apk add --no-cache \
        alpine-sdk \
        bash \
        binutils \
        build-base \
        dos2unix \
        fakeroot \
        git \
        make \
        pkgconf \
        rsync \
        tar \
        xz \
        pax-utils \
        ncurses-dev \
        gpm-dev \
        libffi-dev \
        alsa-lib-dev \
        pulseaudio-dev \
        libx11-dev \
        libxext-dev \
        libxpm-dev \
        libxrandr-dev \
        libxrender-dev \
        mesa-dev \
        glu-dev
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
        bootstrap-dist-target \
        -j"$JOBS"

    [ -f "$BOOTSTRAP_TAR" ] || die "bootstrap tarball was not created"
}

##############################################################################
# Packaging
##############################################################################

stage_source_tree() {
    local bootstrap_srcdir

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
        "$ROOT/" "$BUILDDIR/"

    mkdir -p "$BUILDDIR/bootstrap/$BOOTKEY"
    run rsync -a --no-owner --no-group --delete "$bootstrap_srcdir/" "$BUILDDIR/bootstrap/$BOOTKEY/"
}

ensure_abuild_key() {
    command -v abuild-keygen >/dev/null 2>&1 || die "missing command: abuild-keygen"

    if [ -f /etc/abuild.conf ]; then
        # shellcheck disable=SC1091
        . /etc/abuild.conf
    fi
    if [ -f "${HOME:-/root}/.abuild/abuild.conf" ]; then
        # shellcheck disable=SC1091
        . "${HOME:-/root}/.abuild/abuild.conf"
    fi

    if [ -n "${PACKAGER_PRIVKEY:-}" ] && [ -f "$PACKAGER_PRIVKEY" ]; then
        return 0
    fi

    msg "generating abuild signing key"
    export PACKAGER="FreeBASIC build script <packagers@example.invalid>"
    run abuild-keygen -a -n
}

write_apkbuild() {
    local apkbuild="$APKBUILDDIR/APKBUILD"
    local src_file="$PKGNAME-$VERSION.tar.xz"
    local src_sum

    mkdir -p "$APKBUILDDIR"
    [ -f "$APKBUILDDIR/$src_file" ] || die "missing abuild source tarball: $APKBUILDDIR/$src_file"
    src_sum="$(sha512sum "$APKBUILDDIR/$src_file" | awk '{print $1}')"
    [ -n "$src_sum" ] || die "could not calculate sha512sum for $APKBUILDDIR/$src_file"

    {
        echo "# Maintainer: FreeBASIC packagers <packagers@example.invalid>"
        echo "pkgname=$PKGNAME"
        echo "pkgver=$VERSION"
        echo "pkgrel=$REV"
        echo 'pkgdesc="FreeBASIC compiler"'
        echo 'url="https://www.freebasic.net/"'
        echo 'arch="x86_64 x86 aarch64 armv7 riscv64 ppc64le s390x"'
        echo 'options="!check"'
        echo 'license="GPL-2.0-or-later AND LGPL-2.1-or-later"'
        echo 'depends="gcc binutils musl-dev ncurses-dev gpm-dev libffi-dev alsa-lib-dev pulseaudio-dev libx11-dev libxext-dev libxpm-dev libxrandr-dev libxrender-dev mesa-dev glu-dev"'
        echo 'makedepends="build-base make pkgconf rsync dos2unix pax-utils ncurses-dev gpm-dev libffi-dev alsa-lib-dev pulseaudio-dev libx11-dev libxext-dev libxpm-dev libxrandr-dev libxrender-dev mesa-dev glu-dev"'
        echo 'source="$pkgname-$pkgver.tar.xz"'
        echo 'builddir="$srcdir/$pkgname-$pkgver"'
        echo
        echo 'build() {'
        echo "	make FBC_TARGET=\"$FBC_TARGET\" FBTARGET_DIR_OVERRIDE=\"$BOOTKEY\" bootstrap-minimal -j\"${JOBS:-1}\""
        echo "	make all FBC=bootstrap/fbc BUILD_FBC_TARGET=\"$FBC_TARGET\" FBTARGET_DIR_OVERRIDE=\"$BOOTKEY\" -j\"${JOBS:-1}\""
        echo '	mkdir -p .package-smoke'
        echo '	cat > .package-smoke/console.bas <<-EOF'
        echo '	print "Hello world"'
        echo '	EOF'
        echo '	cat > .package-smoke/gfx.bas <<-EOF'
        echo '	screenres 160, 100, 32'
        echo '	color rgb(255, 255, 255), rgb(0, 0, 0)'
        echo '	cls'
        echo '	draw string (8, 8), "Hello world"'
        echo '	line (8, 28)-(120, 70), rgb(0, 200, 255), bf'
        echo '	print "Hello world"'
        echo '	sleep 50'
        echo '	screen 0'
        echo '	EOF'
        echo '	cat > .package-smoke/sfx.bas <<-EOF'
        echo '	print "sfx-start"'
        echo '	play "ABCDEFG"'
        echo '	print "sfx-end"'
        echo '	EOF'
        echo '	bin/fbc -v -prefix "$builddir" .package-smoke/console.bas -x .package-smoke/console > .package-smoke/console.link.log 2>&1 || { cat .package-smoke/console.link.log; return 1; }'
        echo '	bin/fbc -v -prefix "$builddir" .package-smoke/gfx.bas -x .package-smoke/gfx > .package-smoke/gfx.link.log 2>&1 || { cat .package-smoke/gfx.link.log; return 1; }'
        echo '	bin/fbc -v -prefix "$builddir" .package-smoke/sfx.bas -x .package-smoke/sfx > .package-smoke/sfx.link.log 2>&1 || { cat .package-smoke/sfx.link.log; return 1; }'
        echo '	{'
        echo '		printf "%s\n" crt1.o crti.o crtn.o'
        echo '		grep -hEo -- "([[:space:]]|^)-l[^[:space:]]+" .package-smoke/*.link.log | sed "s/^[[:space:]]*-l/lib/; s/$/.so/"'
        echo '	} | while read -r linker_input; do'
        echo '		path="$(cc -print-file-name="$linker_input")"'
        echo '		[ -n "$path" ] || continue'
        echo '		[ "$path" != "$linker_input" ] || continue'
        echo '		[ -e "$path" ] || continue'
        echo '		apk info --who-owns "$path" 2>/dev/null | sed -n "s/^.* is owned by //p" | sed "s/-[0-9][^-]*-r[0-9][0-9]*$//"'
        echo '	done | awk "NF {print \$1}" | sort -u > .package-smoke/link-depends'
        echo '	cat >> .package-smoke/link-depends <<-EOF'
        echo '	musl-dev'
        echo '	ncurses-dev'
        echo '	gpm-dev'
        echo '	libffi-dev'
        echo '	alsa-lib-dev'
        echo '	pulseaudio-dev'
        echo '	libx11-dev'
        echo '	libxext-dev'
        echo '	libxpm-dev'
        echo '	libxrandr-dev'
        echo '	libxrender-dev'
        echo '	mesa-dev'
        echo '	glu-dev'
        echo '	EOF'
        echo '	sort -u -o .package-smoke/link-depends .package-smoke/link-depends'
        echo '	scanelf --needed --nobanner --format "%n" .package-smoke/console .package-smoke/gfx .package-smoke/sfx | tr "," "\n" | awk "/^lib.*\\.so/ {print \"so:\"\$1}" | sort -u > .package-smoke/so-depends'
        echo '}'
        echo
        echo 'package() {'
        echo '	if [ -f "$builddir/.package-smoke/link-depends" ]; then'
        echo '		while read -r pkg; do'
        echo '			[ -n "$pkg" ] || continue'
        echo '			depends="$depends $pkg"'
        echo '		done < "$builddir/.package-smoke/link-depends"'
        echo '	fi'
        echo '	if [ -f "$builddir/.package-smoke/so-depends" ]; then'
        echo '		while read -r dep; do'
        echo '			[ -n "$dep" ] || continue'
        echo '			depends="$depends $dep"'
        echo '		done < "$builddir/.package-smoke/so-depends"'
        echo '	fi'
        echo "	make install DESTDIR=\"\$pkgdir\" prefix=\"$PREFIX\" FBC=bootstrap/fbc BUILD_FBC_TARGET=\"$FBC_TARGET\" FBTARGET_DIR_OVERRIDE=\"$BOOTKEY\""
        echo '}'
        echo
        echo 'sha512sums="'
        echo "$src_sum  $src_file"
        echo '"'
    } > "$apkbuild"
}

package_current_target() {
    local src_tar built_apk

    msg "preparing Alpine abuild package build"

    rm -rf "$STAGE" "$PKGROOT" "$APKBUILDDIR"
    mkdir -p "$STAGE" "$PKGROOT" "$APKBUILDDIR"
    stage_source_tree

    msg "creating abuild source tarball"
    src_tar="$APKBUILDDIR/$PKGNAME-$VERSION.tar.xz"
    run tar -C "$WORKDIR" -cJf "$src_tar" --transform "s#^package#$PKGNAME-$VERSION#" package
    [ -s "$src_tar" ] || die "source tarball was not created: $src_tar"

    write_apkbuild
    ensure_abuild_key

    rm -f "$APKFILE"

    msg "running abuild"
    cd "$APKBUILDDIR"
    run abuild -F -P "$OUTDIR" clean fetch unpack prepare build rootpkg

    built_apk="$(find "$OUTDIR" -type f -name "$PKGNAME-$PKGVERSION.apk" | head -n1)"
    [ -n "$built_apk" ] || die "abuild did not create $PKGNAME-$PKGVERSION.apk under $OUTDIR"
    if [ "$built_apk" != "$APKFILE" ]; then
        run cp "$built_apk" "$APKFILE"
    fi

    [ -f "$APKFILE" ] || die "apk was not created"

    echo
    echo "==> build completed"
    echo "==> artifact: $APKFILE"
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
