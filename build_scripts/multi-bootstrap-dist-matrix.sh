#!/usr/bin/env bash
#
# multi-bootstrap-dist-matrix.sh
#
set -euo pipefail

trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Validate invocation location
##############################################################################

if [ ! -d "build_scripts" ]; then
        echo ""
        echo "ERROR: run this script from the project root."
        exit 1
fi

if [ "$(basename "$PWD")" = "build_scripts" ]; then
        echo ""
        echo "ERROR: do not run this script from build_scripts/"
        exit 1
fi

##############################################################################
# Select make implementation
##############################################################################

if command -v gmake >/dev/null 2>&1; then
        MAKE_CMD=gmake
else
        MAKE_CMD=make
fi

echo "==> using make command: $MAKE_CMD"

##############################################################################
# Version detection
##############################################################################

VERSION=$(sed -n "s/^FBVERSION[[:space:]]*:=[[:space:]]*//p" mk/version.mk | head -n1)
[ -z "$VERSION" ] && { echo "ERROR: could not determine FBVERSION"; exit 1; }

##############################################################################
# CPU detection
##############################################################################

if command -v nproc >/dev/null 2>&1; then
        JOBS=${JOBS:-$(nproc)}
elif command -v sysctl >/dev/null 2>&1; then
        JOBS=${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 1)}
else
        JOBS=${JOBS:-1}
fi

die(){ echo "ERROR: $*" >&2; exit 1; }

##############################################################################
# Cleanup
##############################################################################

echo "==> Removing previous bootstrap archives"
rm -f FreeBASIC-${VERSION}-source-bootstrap-*.tar.xz

##############################################################################
# Ensure host compiler exists
##############################################################################

ensure_host_compiler(){

        if [ ! -x "./bin/fbc" ]; then
                echo "==> Building host compiler..."
                $MAKE_CMD clean || die "clean failed"
                $MAKE_CMD compiler -j"$JOBS" || die "compiler build failed"
        fi

        [ -x "./bin/fbc" ] || die "host compiler not available"
}

##############################################################################
# Build a single bootstrap archive
##############################################################################

build_target(){

        FBC_T="$1"
        DIR="$2"

        PKG="FreeBASIC-${VERSION}-source-bootstrap-${DIR}"

        echo
        echo "============================================================"
        echo "==> Target:        $FBC_T"
        echo "==> Directory key: $DIR"
        echo "============================================================"

        echo "==> Cleaning previous artifacts"

        rm -f "${PKG}.tar.xz"
        rm -rf "${PKG}"
        rm -rf "bootstrap/${DIR}"

        $MAKE_CMD clean-bootstrap-sources >/dev/null 2>&1 || true

        echo "==> Building bootstrap archive"

        $MAKE_CMD \
                FBC_TARGET="$FBC_T" \
                FBTARGET_DIR_OVERRIDE="$DIR" \
                bootstrap-dist-target \
                -j"$JOBS" || die "bootstrap-dist-target failed"

        test -f "${PKG}.tar.xz" || die "missing output: ${PKG}.tar.xz"

        echo "==> OK: ${PKG}.tar.xz"
}

##############################################################################
# Build host compiler
##############################################################################

ensure_host_compiler

##############################################################################
# Linux targets
##############################################################################

build_target linux-x86_64        linux-amd64
build_target linux-x86           linux-i386
build_target linux-aarch64       linux-arm64
build_target linux-arm           linux-armel
build_target linux-arm           linux-armhf
build_target linux-powerpc       linux-powerpc
build_target linux-powerpc64     linux-powerpc64
build_target linux-powerpc64le   linux-ppc64el
build_target linux-riscv64       linux-riscv64
build_target linux-s390x         linux-s390x
build_target linux-loongarch64   linux-loongarch64

##############################################################################
# FreeBSD
##############################################################################

build_target freebsd-x86_64      freebsd-amd64

##############################################################################
# Haiku (optional)
##############################################################################

# build_target haiku-x86           haiku-x86
# build_target haiku-x86_64        haiku-x86_64

##############################################################################
# MinGW
##############################################################################

build_target win32               mingw-x86
build_target win64               mingw-x86_64

##############################################################################
# Cygwin
##############################################################################

build_target cygwin-x86          cygwin-x86
build_target cygwin-x86_64       cygwin-x86_64

##############################################################################
# Summary
##############################################################################

echo
echo "==> All bootstrap tarballs built:"
ls -1 FreeBASIC-${VERSION}-source-bootstrap-*.tar.xz

