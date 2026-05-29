#!/usr/bin/env bash

##############################################################################
# FreeBASIC illumos native package builder
##############################################################################
#
# Purpose:
#
#   Build FreeBASIC natively on an illumos system and publish an IPS package
#   repository under out/illumos.
#
# Responsibilities:
#
#   * install the native build/runtime dependency set through pkg(5)
#   * build the bootstrap compiler and full FreeBASIC tree
#   * stage an installation under a temporary build root
#   * publish an IPS package repository and run basic smoke tests
#
# This script intentionally does NOT contain:
#
#   * cross-compilation from another operating system
#   * VM provisioning
#   * cross-ISA packaging from a different illumos ISA
#
##############################################################################

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Environment
##############################################################################

export CC=gcc
export CXX=g++

NATIVE_ISA="$(isainfo -n 2>/dev/null || uname -p)"
OOCE_LIBDIR="/opt/ooce/lib"
FBC_ARCH=""
GNU_TARGET_BIN=""

if [ -d "/opt/ooce/lib/$NATIVE_ISA" ]; then
    OOCE_LIBDIR="/opt/ooce/lib/$NATIVE_ISA"
fi

case "$NATIVE_ISA" in
    amd64|x86_64)
        FBC_ARCH="x86_64"
        GNU_TARGET_BIN="/usr/gnu/x86_64-pc-solaris2.11/bin"
        ;;
    i386|i486|i586|i686)
        FBC_ARCH="x86"
        GNU_TARGET_BIN="/usr/gnu/i386-pc-solaris2.11/bin"
        ;;
    *) echo "ERROR: unsupported illumos ISA: $NATIVE_ISA"; exit 1 ;;
esac

export PATH="/opt/gcc-15/bin:/opt/gcc-14/bin:/opt/gcc-13/bin:/opt/ooce/bin:/usr/gnu/bin:$GNU_TARGET_BIN:/usr/bin:/usr/sbin:/sbin:$PATH"
export PKG_CONFIG_PATH="$OOCE_LIBDIR/pkgconfig:/usr/lib/$NATIVE_ISA/pkgconfig:/usr/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

BUILD_JOBS="${NATIVE_JOBS:-}"

if [ -z "$BUILD_JOBS" ]; then
    BUILD_CPUS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
    BUILD_JOBS="$(( (BUILD_CPUS + 1) / 2 ))"
fi

case "$BUILD_JOBS" in
    ''|*[!0-9]*|0) echo "ERROR: NATIVE_JOBS must be a positive integer"; exit 1 ;;
esac

##############################################################################
# TLS policy
##############################################################################

prepare_tls_policy() {
    #
    # Some illumos cloud images use OpenSSL/libcurl combinations that can
    # stall during TLS 1.3 handshakes through QEMU user networking.  IPS uses
    # libcurl for repository access, so cap the build environment at TLS 1.2.
    #
    cat > /tmp/freebasic-illumos-openssl.cnf <<'EOF'
openssl_conf = openssl_init

[openssl_init]
ssl_conf = ssl_section

[ssl_section]
system_default = system_default_section

[system_default_section]
MaxProtocol = TLSv1.2
EOF

    export OPENSSL_CONF=/tmp/freebasic-illumos-openssl.cnf
}

configure_pkg_proxy() {
    local release

    [ -n "${ILLUMOS_PKG_PROXY:-}" ] || return 0

    release="$(uname -v | sed -n 's/^omnios-\(r[0-9][0-9]*\).*/\1/p')"
    [ -n "$release" ] || release="r151058"

    echo "==> configuring OmniOS package proxy"
    pkg set-publisher --no-refresh -M '*' -O "${ILLUMOS_PKG_PROXY}/${release}/core/" omnios
    pkg set-publisher --no-refresh -M '*' -O "${ILLUMOS_PKG_PROXY}/${release}/extra/" extra.omnios
}

##############################################################################
# Options
##############################################################################

DO_BUILD=1
DO_PACKAGE=1

while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) DO_BUILD=0 ;;
        --no-package) DO_PACKAGE=0 ;;
        -h|--help)
            echo "Usage: $0 [--no-build] [--no-package]"
            exit 0
            ;;
        *) echo "ERROR: unknown option $1"; exit 1 ;;
    esac
    shift
done

##############################################################################
# Locate project root
##############################################################################

SEARCH="$(pwd)"
ROOT=""

while :; do
    if [ -f "$SEARCH/mk/version.mk" ] && [ -f "$SEARCH/GNUmakefile" ]; then
        ROOT="$SEARCH"
        break
    fi
    [ "$SEARCH" = "/" ] && break
    SEARCH="$(dirname "$SEARCH")"
done

[ -n "$ROOT" ] || { echo "ERROR: not in FreeBASIC tree"; exit 1; }
cd "$ROOT"

##############################################################################
# Version extraction
##############################################################################

FBVERSION="$(awk -F':=' '/^FBVERSION/ {gsub(/[ \t]/,"",$2); print $2}' mk/version.mk)"
REV="$(awk -F':=' '/^REV/ {gsub(/[ \t]/,"",$2); print $2}' mk/version.mk)"

[ -n "$FBVERSION" ] || exit 1
[ -n "$REV" ] || exit 1

VERSION_FULL="${FBVERSION}.${REV}"
OSREL="$(uname -r)"
ARCH="$NATIVE_ISA"
TARGET_TRIPLET="${FBC_ARCH}-pc-illumos"
BOOTSTRAP_DIR="$ROOT/bootstrap/illumos-${FBC_ARCH}"

FMRI="pkg://local/lang/freebasic@${FBVERSION},${OSREL}-${REV}"

GCC_PKG_CANDIDATES=(
    developer/gcc15
    developer/gcc14
    developer/gcc13
    developer/gcc10
    developer/gcc
)
if [ -z "${GCC_PKG+x}" ]; then
	GCC_PKG=""
fi

resolve_gcc_package() {
    local pkg

    for pkg in "${GCC_PKG_CANDIDATES[@]}"; do
        if pkg list -H "$pkg" >/dev/null 2>&1; then
            GCC_PKG="$pkg"
            return 0
        fi
        if pkg list -H "${pkg}@*" >/dev/null 2>&1; then
            GCC_PKG="$pkg"
            return 0
        fi
    done
    return 1
}

ensure_gcc_dependency() {
    local c

    if ! resolve_gcc_package; then
        for c in "${GCC_PKG_CANDIDATES[@]}"; do
            if pkg install --accept "$c" >/dev/null 2>&1; then
                resolve_gcc_package && return 0
            fi
        done
    fi
    return 1
}

##############################################################################
# Paths
##############################################################################

BUILDROOT="$ROOT/.build-illumos"
STAGE="$BUILDROOT/stage"
OUT="$ROOT/out"
OUT_ILLUMOS="$OUT/illumos/${OSREL}/${ARCH}"
REPO="$OUT_ILLUMOS/repo"
MANIFEST="$BUILDROOT/manifest.p5m"
PREFIX="/usr/local"

mkdir -p "$BUILDROOT" "$OUT_ILLUMOS"

##############################################################################
# Dependencies
##############################################################################

PKGS_BUILD_REQUIRED=(
    developer/pkg-config
    developer/build/gnu-make
    library/ncurses
    library/libffi
    system/header/header-audio
    ooce/x11/header/x11-protocols
    ooce/x11/header/xcb-protocols
    ooce/x11/library/libx11
    ooce/x11/library/libxau
    ooce/x11/library/libxext
    ooce/x11/library/libxrender
    ooce/x11/library/libxrandr
    ooce/x11/library/libxi
    ooce/x11/library/libxcb
)

PKGS_RUNTIME=(
    library/ncurses
    library/libffi
    ooce/x11/library/libx11
    ooce/x11/library/libxau
    ooce/x11/library/libxext
    ooce/x11/library/libxrender
    ooce/x11/library/libxrandr
    ooce/x11/library/libxi
    ooce/x11/library/libxcb
)

##############################################################################
# Install dependencies (idempotent)
##############################################################################

prepare_tls_policy
configure_pkg_proxy

echo "==> installing dependencies"
pkg refresh || true

resolve_gcc_package || true
ensure_gcc_dependency || true
resolve_gcc_package || true
if [ -n "$GCC_PKG" ]; then
	PKGS_BUILD_REQUIRED=("$GCC_PKG" "${PKGS_BUILD_REQUIRED[@]}")
	PKGS_RUNTIME=("$GCC_PKG" "${PKGS_RUNTIME[@]}")
fi

pkg install --accept "${PKGS_BUILD_REQUIRED[@]}"

if ! command -v gcc >/dev/null 2>&1; then
    for p in "${GCC_PKG_CANDIDATES[@]}"; do
        pkg install --accept "$p" >/dev/null 2>&1 || true
        command -v gcc >/dev/null 2>&1 && break
    done
fi

command -v gcc >/dev/null 2>&1 || { echo "ERROR: gcc not found after dependency install"; exit 1; }
command -v gmake >/dev/null 2>&1 || { echo "ERROR: gmake not found after dependency install"; exit 1; }
if ! command -v python3 >/dev/null 2>&1; then
    for p in runtime/python-39 runtime/python-311 runtime/python-312; do
        pkg install --accept "$p" >/dev/null 2>&1 || true
        command -v python3 >/dev/null 2>&1 && break
    done
fi
command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 not found after dependency install"; exit 1; }

##############################################################################
# Build
##############################################################################

if [ "$DO_BUILD" -eq 1 ]; then

    echo "==> build jobs: $BUILD_JOBS"
    echo "==> target triplet: $TARGET_TRIPLET"
    echo "==> cleaning (preserving bootstrap)"
    gmake -f GNUmakefile -j"$BUILD_JOBS" clean || true

    if [ ! -d "$BOOTSTRAP_DIR" ] ||
        ! find "$BOOTSTRAP_DIR" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print | sed -n '1p' | grep -q .; then
        echo "==> bootstrap-seed-peer"
        gmake -f GNUmakefile \
            -j"$BUILD_JOBS" \
            bootstrap-seed-peer \
            CC=gcc \
            BUILD_FBCFLAGS="-d __FB_ILLUMOS__ -d DISABLE_XPM" \
            TARGET_OS=illumos \
            TARGET_TRIPLET="$TARGET_TRIPLET"
    else
        echo "==> bootstrap-minimal"
        gmake -f GNUmakefile \
            -j"$BUILD_JOBS" \
            bootstrap-minimal \
            CC=gcc \
            BUILD_FBCFLAGS="-d __FB_ILLUMOS__ -d DISABLE_XPM" \
            TARGET_OS=illumos \
            TARGET_TRIPLET="$TARGET_TRIPLET"
    fi

    [ -x "$ROOT/bootstrap/fbc" ] || exit 1

    echo "==> full build"
    gmake -f GNUmakefile \
        -j"$BUILD_JOBS" \
        all \
        FBC="$ROOT/bootstrap/fbc" \
        CC=gcc \
        BUILD_FBCFLAGS="-d __FB_ILLUMOS__ -d DISABLE_XPM" \
        TARGET_OS=illumos \
        TARGET_TRIPLET="$TARGET_TRIPLET"

    echo "==> staging install"
    rm -rf "$STAGE"
    mkdir -p "$STAGE"

    gmake -f GNUmakefile \
        -j"$BUILD_JOBS" \
        install \
        DESTDIR="$STAGE" \
        prefix="$PREFIX" \
        FBC="$ROOT/bootstrap/fbc" \
        BUILD_FBCFLAGS="-d __FB_ILLUMOS__ -d DISABLE_XPM" \
        TARGET_OS=illumos \
        TARGET_TRIPLET="$TARGET_TRIPLET"

    echo "==> staging examples"
    rm -rf "$STAGE$PREFIX/share/freebasic/examples"
    mkdir -p "$STAGE$PREFIX/share/freebasic"
    cp -R "$ROOT/examples" "$STAGE$PREFIX/share/freebasic/examples"

else
    echo "==> --no-build specified"
fi

##############################################################################
# Packaging + install + test
##############################################################################

if [ "$DO_PACKAGE" -eq 1 ]; then

    [ -x "$STAGE$PREFIX/bin/fbc" ] || { echo "ERROR: staged fbc missing"; exit 1; }

    echo "==> generating manifest"
    pkgsend generate "$STAGE" \
        | grep -vE ' path=(usr|usr/local)$' \
        > "$MANIFEST"

    echo "==> injecting metadata + deps"
    {
        echo "set name=pkg.fmri value=${FMRI}"
        echo "set name=pkg.summary value=\"FreeBASIC compiler\""
        echo "set name=pkg.description value=\"FreeBASIC compiler for illumos\""

        for d in "${PKGS_RUNTIME[@]}"; do
            echo "depend type=require fmri=$d"
        done

        cat "$MANIFEST"
    } > "${MANIFEST}.final"

    mv "${MANIFEST}.final" "$MANIFEST"

    echo "==> preparing repo"
    if ! pkgrepo info -s "$REPO" >/dev/null 2>&1; then
        rm -rf "$REPO"
        pkgrepo create "$REPO"
    fi

    pkgrepo -s "$REPO" add-publisher local >/dev/null 2>&1 || true

    echo "==> publishing"
    pkgsend -s "file://$REPO" publish -d "$STAGE" "$MANIFEST"

    echo "==> package dependencies"
    pkg contents -r -g "file://$REPO" -t depend "$FMRI"

    echo "==> installing from repo"
    pkg set-publisher -g "file://$REPO" local >/dev/null 2>&1 || true
    pkg refresh >/dev/null 2>&1 || true
    pkg install "$FMRI" || { echo "ERROR: install failed"; exit 1; }

    ##############################################################################
    # Tests
    ##############################################################################

    echo "==> writing smoke tests"

    rm -rf /tmp/freebasic-illumos-smoke
    mkdir -p /tmp/freebasic-illumos-smoke

    cat > /tmp/freebasic-illumos-smoke/console.bas <<'EOF'
print "Hello world"
EOF

    cat > /tmp/freebasic-illumos-smoke/gfx-truecolor.bas <<'EOF'
#include once "fbgfx.bi"

sub expect_rgb( byval x as integer, byval y as integer, byval expected as uinteger, byref label as string )
    dim as uinteger actual = cuint( point( x, y ) )

    if( actual <> expected ) then
        print "gfx truecolor mismatch: "; label; " actual=&h"; hex( actual, 8 ); " expected=&h"; hex( expected, 8 )
        end 1
    end if
end sub

dim as integer has_extra_page = 1

if( screenres( 64, 64, 32, 2 ) <> 0 ) then
    has_extra_page = 0
    if( screenres( 64, 64, 32 ) <> 0 ) then
        print "gfx truecolor failed: screenres failed"
        end 1
    end if
end if

screenset 0, 0
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
line (24, 8)-(39, 23), rgb( 0, 255, 0 ), bf
line (40, 8)-(55, 23), rgb( 0, 0, 255 ), bf
expect_rgb 8, 8, rgb( 255, 0, 0 ), "red block"
expect_rgb 24, 8, rgb( 0, 255, 0 ), "green block"
expect_rgb 40, 8, rgb( 0, 0, 255 ), "blue block"

if( has_extra_page <> 0 ) then
    screenset 1, 1
else
    screenset 0, 0
end if
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 32)-(55, 55), rgb( 255, 255, 255 ), bf
expect_rgb 8, 32, rgb( 255, 255, 255 ), "screenset page"

screensync
sleep 50, 1
screen 0
EOF

    cat > /tmp/freebasic-illumos-smoke/gfx-screen-modes.bas <<'EOF'
#include once "fbgfx.bi"

sub fail( byref message as string )
    print "gfx legacy failure: "; message
    end 1
end sub

sub expect_index( byval x as integer, byval y as integer, byval expected as ulong, byref label as string )
    dim as ulong actual = culng( point( x, y ) )

    if( actual <> expected ) then
        print "gfx legacy mismatch: "; label; " actual="; actual; " expected="; expected
        end 1
    end if
end sub

sub draw_mode( byval mode as integer )
    dim as integer w, h, depth, bpp, pitch

    screeninfo w, h, depth, bpp, pitch
    if( w < 48 or h < 32 ) then
        fail "mode " & str( mode ) & " reported an unexpectedly small framebuffer"
    end if

    screenset 0, 0
    cls

    if( depth <= 1 ) then
        palette 1, 255, 255, 255
        line (0, 0)-(47, 31), 0, bf
        line (8, 8)-(23, 23), 1, bf
        expect_index 8, 8, 1, "mode " & str( mode ) & " white block"
    else
        palette 1, 255, 0, 0
        palette 2, 0, 255, 0
        palette 3, 0, 0, 255
        line (0, 0)-(63, 31), 0, bf
        line (8, 8)-(23, 23), 1, bf
        line (24, 8)-(39, 23), 2, bf
        line (40, 8)-(55, 23), 3, bf
        expect_index 8, 8, 1, "mode " & str( mode ) & " red block"
        expect_index 24, 8, 2, "mode " & str( mode ) & " green block"
        expect_index 40, 8, 3, "mode " & str( mode ) & " blue block"
    end if

    screensync
    sleep 30, 1
end sub

sub test_mode( byval mode as integer )
    dim as integer stage = 0
    dim as integer unsupported_mode = 0

    if( mode = 0 ) then
        screen 0
        print "SCREEN 0 ok"
        exit sub
    end if

    on local error goto mode_error

    stage = 1
    screen mode
    if( unsupported_mode <> 0 ) then
        screen 0
        print "SCREEN "; mode; " unsupported, err="; err()
        exit sub
    end if

    stage = 2
    if( screenptr() = 0 ) then
        screen 0
        print "SCREEN "; mode; " unsupported"
        exit sub
    end if

    draw_mode mode
    screen 0
    print "SCREEN "; mode; " ok"
    exit sub

mode_error:
    if( stage = 1 ) then
        unsupported_mode = 1
        resume next
    end if

    print "SCREEN "; mode; " failed, err="; err()
    end 1
end sub

for mode as integer = 0 to 13
    test_mode mode
next

end 0
EOF

    cat > /tmp/freebasic-illumos-smoke/sfx.bas <<'EOF'
extern "C"
declare function fb_sfxDeviceCurrent() as long
declare function fb_sfxDeviceInfoName(byval id as long) as const zstring ptr
end extern

print "sfx-start"
dim as long sfx_device = fb_sfxDeviceCurrent()
dim as const zstring ptr sfx_driver = fb_sfxDeviceInfoName(sfx_device)
if sfx_driver <> 0 then
    print "sfx-driver="; *sfx_driver
    if instr(lcase(*sfx_driver), "null") > 0 then
        print "sfx-driver-null"
        end 2
    end if
else
    print "sfx-driver=<none>"
    end 3
end if
sound 440, 0.75
play "L4 CDEFGAB"
sleep 1600, 1
print "sfx-end"
EOF

    echo "==> console smoke"
    "$PREFIX/bin/fbc" /tmp/freebasic-illumos-smoke/console.bas -x /tmp/freebasic-illumos-smoke/console
    [ "$(/tmp/freebasic-illumos-smoke/console)" = "Hello world" ] || exit 1

    echo "==> gfxlib compile"
    "$PREFIX/bin/fbc" /tmp/freebasic-illumos-smoke/gfx-truecolor.bas -x /tmp/freebasic-illumos-smoke/gfx-truecolor
    "$PREFIX/bin/fbc" -lang fblite -exx /tmp/freebasic-illumos-smoke/gfx-screen-modes.bas -x /tmp/freebasic-illumos-smoke/gfx-screen-modes

    echo "==> gfxlib smoke"
    XVFB_PID=""
    if [ -n "${DISPLAY:-}" ]; then
        echo "==> using existing DISPLAY=$DISPLAY"
    else
        command -v Xvfb >/dev/null 2>&1 || { echo "ERROR: DISPLAY is unset and Xvfb is not available for gfxlib smoke"; exit 1; }
        Xvfb :99 -screen 0 800x600x24 >/tmp/freebasic-illumos-xvfb.log 2>&1 &
        XVFB_PID=$!
        trap 'kill "$XVFB_PID" >/dev/null 2>&1 || true' EXIT
        export DISPLAY=:99
        sleep 2
        kill -0 "$XVFB_PID" >/dev/null 2>&1 || { cat /tmp/freebasic-illumos-xvfb.log; exit 1; }
    fi
    timeout 30 /tmp/freebasic-illumos-smoke/gfx-truecolor
    timeout 30 /tmp/freebasic-illumos-smoke/gfx-screen-modes
    if [ -n "$XVFB_PID" ]; then
        kill "$XVFB_PID" >/dev/null 2>&1 || true
        trap - EXIT
    fi

    echo "==> sfxlib compile"
    "$PREFIX/bin/fbc" /tmp/freebasic-illumos-smoke/sfx.bas -x /tmp/freebasic-illumos-smoke/sfx
    [ -f "$PREFIX/share/freebasic/examples/sfxlib/showcase.bas" ] || { echo "ERROR: sfxlib showcase example missing"; exit 1; }
    (
        cd "$PREFIX/share/freebasic/examples/sfxlib"
        "$PREFIX/bin/fbc" showcase.bas -x /tmp/freebasic-illumos-smoke/sfx-showcase
    )
    [ -x /tmp/freebasic-illumos-smoke/sfx-showcase ] || exit 1

    echo "==> sfxlib real audio smoke"
    SFXLIB_DRIVER="Solaris audio" timeout 20 /tmp/freebasic-illumos-smoke/sfx \
        > /tmp/freebasic-illumos-smoke/sfx.out \
        2> /tmp/freebasic-illumos-smoke/sfx.err
    cat /tmp/freebasic-illumos-smoke/sfx.out
    grep -qx 'sfx-start' /tmp/freebasic-illumos-smoke/sfx.out || exit 1
    grep -qx 'sfx-end' /tmp/freebasic-illumos-smoke/sfx.out || exit 1
    grep -q '^sfx-driver=Solaris audio' /tmp/freebasic-illumos-smoke/sfx.out || exit 1
    [ ! -s /tmp/freebasic-illumos-smoke/sfx.err ] || { cat /tmp/freebasic-illumos-smoke/sfx.err; exit 1; }

    echo "==> exampleageddon"
    python3 "$ROOT/build_scripts/exampleageddon-freebasic.py" \
        --root "$ROOT" \
        --outdir "$OUT_ILLUMOS/exampleageddon" \
        --fbc "$PREFIX/bin/fbc" \
        --jobs 1 \
        --fail-on-self-contained

    echo "==> SUCCESS"

else
    echo "==> --no-package specified"
fi
