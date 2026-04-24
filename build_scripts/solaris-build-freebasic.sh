#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Environment
##############################################################################

export PATH="/usr/gnu/bin:/usr/bin:/usr/sbin:/sbin:$PATH"
export CC=gcc
export CXX=g++

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
ARCH="amd64"

FMRI="pkg://local/lang/freebasic@${FBVERSION},${OSREL}-${REV}"

##############################################################################
# Paths
##############################################################################

BUILDROOT="$ROOT/.build-solaris"
STAGE="$BUILDROOT/stage"
OUT="$ROOT/out"
OUT_SOLARIS="$OUT/Solaris/${OSREL}/${ARCH}"
REPO="$OUT_SOLARIS/repo"
MANIFEST="$BUILDROOT/manifest.p5m"
PREFIX="/usr/local"

mkdir -p "$BUILDROOT" "$OUT_SOLARIS"

##############################################################################
# Dependencies
##############################################################################

PKGS_BUILD=(
    developer/gcc
    developer/build/gnu-make
    library/ncurses
    library/libffi
    x11/library/libx11
    x11/library/libxext
    x11/library/libxrender
    x11/library/libxrandr
    x11/library/libxcursor
    x11/library/libxi
    x11/library/libxinerama
    x11/library/libxxf86vm
    x11/library/libxcb
    x11/library/mesa
)

PKGS_RUNTIME=(
    library/ncurses
    library/libffi
    x11/library/libx11
    x11/library/libxext
    x11/library/libxrender
    x11/library/libxrandr
    x11/library/libxcursor
    x11/library/libxi
    x11/library/libxinerama
    x11/library/libxxf86vm
    x11/library/libxcb
    x11/library/mesa
)

##############################################################################
# Install dependencies (idempotent)
##############################################################################

echo "==> installing dependencies"
pkg refresh || true

for p in "${PKGS_BUILD[@]}"; do
    pkg install --accept "$p" >/dev/null 2>&1 || true
done

##############################################################################
# Build
##############################################################################

if [ "$DO_BUILD" -eq 1 ]; then

    echo "==> cleaning (preserving bootstrap)"
    gmake -f GNUmakefile clean || true

    echo "==> bootstrap-minimal"
    gmake -f GNUmakefile \
        bootstrap-minimal \
        CC=gcc \
        TARGET_TRIPLET="$(gcc -dumpmachine)"

    [ -x "$ROOT/bootstrap/fbc" ] || exit 1

    echo "==> full build"
    gmake -f GNUmakefile \
        all \
        FBC="$ROOT/bootstrap/fbc" \
        CC=gcc

    echo "==> staging install"
    rm -rf "$STAGE"
    mkdir -p "$STAGE"

    gmake -f GNUmakefile \
        install \
        DESTDIR="$STAGE" \
        prefix="$PREFIX" \
        FBC="$ROOT/bootstrap/fbc"

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
        echo "set name=pkg.description value=\"FreeBASIC compiler for Solaris\""

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

    echo "==> installing from repo"
    pkg set-publisher -g "file://$REPO" local >/dev/null 2>&1 || true
    pkg refresh >/dev/null 2>&1 || true
    pkg install "$FMRI" || { echo "ERROR: install failed"; exit 1; }

    ##############################################################################
    # Tests
    ##############################################################################

    echo "==> console test"

    cat > /tmp/fb_test.bas <<'EOF'
print "FreeBASIC OK"
EOF

    "$PREFIX/bin/fbc" /tmp/fb_test.bas
    [ "$(/tmp/fb_test)" = "FreeBASIC OK" ] || exit 1

    echo "==> gfx test"

    cat > /tmp/fb_gfx_test.bas <<'EOF'
screenres 320,200,32
pset (10,10), rgb(255,0,0)
sleep 10
EOF

    "$PREFIX/bin/fbc" /tmp/fb_gfx_test.bas
    /tmp/fb_gfx_test >/dev/null 2>&1 || true

    echo "==> SUCCESS"

else
    echo "==> --no-package specified"
fi
