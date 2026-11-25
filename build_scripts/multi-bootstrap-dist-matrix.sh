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

. "$PWD/build_scripts/build-success-cleanup.sh"

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
        fb_remove_object_trees "$ROOT" || die "could not clear root-owned object trees"
        fb_remove_build_tree "$PWD" "$PWD/lib/freebasic" || die "could not clear host runtime library outputs"

        if ! fb_host_compiler_is_usable "$ROOT"; then
                echo "==> Building host compiler..."
                fb_prepare_host_compiler_rebuild "$ROOT" || die "could not prepare host compiler rebuild"
                $MAKE_CMD clean || die "clean failed"
                $MAKE_CMD compiler -j"$JOBS" || die "compiler build failed"
        fi

        fb_host_compiler_is_usable "$ROOT" || die "host compiler not available"
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
        fb_remove_build_tree "$PWD" "$PWD/bootstrap/${DIR}" || die "could not remove bootstrap/${DIR}"
        fb_remove_build_tree "$PWD" "$PWD/stage/bootstrap-dist" || die "could not remove bootstrap staging directory"

        $MAKE_CMD clean-bootstrap-sources >/dev/null 2>&1 || true

        echo "==> Building bootstrap archive"

        $MAKE_CMD \
                FBC_TARGET="$FBC_T" \
                FBTARGET_DIR_OVERRIDE="$DIR" \
                BOOTSTRAP_DIST_WORKTREE=1 \
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
