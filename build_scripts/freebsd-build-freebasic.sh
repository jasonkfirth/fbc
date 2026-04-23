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
    if [ -d "$SEARCH_DIR/mk" ] && { [ -f "$SEARCH_DIR/makefile" ] || [ -f "$SEARCH_DIR/Makefile" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }

cd "$ROOT"
[ "$(uname -s)" = "FreeBSD" ] || { echo "ERROR: must run on FreeBSD"; exit 1; }

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

##############################################################################
# Config
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-freebsd}"
STAGE="${STAGE:-$BUILDROOT/stage}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
PKGMETA="${PKGMETA:-$BUILDROOT/pkgmeta}"
OUT="${OUT:-$ROOT/out}"
PREFIX="${PREFIX:-/usr/local}"

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}_${REV}"
PKGFILE="${OUT}/${PKGNAME}-${PKGVERSION}.pkg"

##############################################################################
# Install build dependencies
##############################################################################

echo "==> installing build dependencies"

run pkg update -f

run pkg install -y \
    gmake gcc binutils bash \
    mesa-libs libglvnd \
    xorgproto \
    libX11 libXext libXrandr libXrender libXpm \
    libXcursor libXi libXinerama libXxf86vm \
    libxcb libXau libXdmcp \
    ncurses

##############################################################################
# Resolve installed GCC package (meta preferred, fallback to gcc)
##############################################################################

echo "==> resolving installed GCC package"

GCC_PKG="$(
    pkg query '%n' \
    | grep -E '^gcc-[0-9]+_[0-9]+' \
    | sort -V \
    | tail -n1 \
    || true
)"

if [ -z "$GCC_PKG" ]; then
    GCC_PKG="$(
        pkg query '%n' \
        | grep -E '^gcc$' \
        || true
    )"
fi

if [ -z "$GCC_PKG" ]; then
    echo "ERROR: could not determine installed gcc package"
    pkg query '%n' | grep gcc || true
    exit 1
fi

echo "==> using GCC package: $GCC_PKG"

GCC_ENTRY="$(
    pkg query '%n %o %v' \
    | awk -v name="$GCC_PKG" '$1 == name { printf "\"%s\": { origin: \"%s\", version: \"%s\" }", $1, $2, $3 }'
)"

[ -n "$GCC_ENTRY" ] || die "failed to query gcc metadata"

##############################################################################
# Other dependencies
##############################################################################

DEPS_LIST=(
    ncurses
    mesa-libs
    libglvnd
    libX11
    libXext
    libXrandr
    libXrender
    libXpm
    libXcursor
    libXi
    libXinerama
    libXxf86vm
    libxcb
    libXau
    libXdmcp
)

##############################################################################
# Build
##############################################################################

rm -rf "$BUILDROOT"
mkdir -p "$STAGE" "$PKGROOT" "$PKGMETA" "$OUT"

run gmake clean || true
run gmake bootstrap-minimal
run gmake all FBC=bootstrap/fbc
run gmake install DESTDIR="$STAGE" prefix="$PREFIX"

cp -a "$STAGE"/. "$PKGROOT"/

##############################################################################
# Generate plist
##############################################################################

echo "==> generating plist"
(
    cd "$PKGROOT"
    find usr/local \( -type f -o -type l \) | sort | sed 's|^usr/local/||'
) > "$PKGMETA/+plist"

##############################################################################
# Generate dependency block
##############################################################################

echo "==> generating manifest deps"

DEPS="$GCC_ENTRY"

for dep in "${DEPS_LIST[@]}"; do
    entry="$(
        pkg query '%n %o %v' \
        | awk -v dep="$dep" '$1 == dep { printf "\"%s\": { origin: \"%s\", version: \"%s\" }", $1, $2, $3 }' \
        || true
    )"

    if [ -n "$entry" ]; then
        DEPS="${DEPS},${entry}"
    else
        echo "WARNING: dependency not found in installed pkg db: $dep"
    fi
done

##############################################################################
# Manifest
##############################################################################

cat > "$PKGMETA/+MANIFEST" <<MANIFEST
name: ${PKGNAME}
version: ${PKGVERSION}
origin: lang/freebasic
comment: FreeBASIC compiler
maintainer: root@localhost
www: https://www.freebasic.net/
prefix: ${PREFIX}
licenses: ["GPLv2"]
licenselogic: single
categories: ["lang"]

deps: {
  ${DEPS}
}

desc: <<EOD
FreeBASIC compiler built from source.
Includes full runtime support (console + gfxlib).
Requires system GCC toolchain.
EOD
MANIFEST

##############################################################################
# Create package
##############################################################################

echo "==> creating package"
run pkg create -m "$PKGMETA" -r "$PKGROOT" -p "$PKGMETA/+plist" -o "$OUT"

[ -f "$PKGFILE" ] || die "expected package not found: $PKGFILE"

##############################################################################
# Install package locally (sanity check)
##############################################################################

if pkg info | grep -q "^freebasic"; then
    run pkg delete -y freebasic || true
fi

run pkg add -f "$PKGFILE"

##############################################################################
# Test compiler
##############################################################################

echo "==> testing compiler"

cat > /tmp/fb_test.bas <<'FBEOF'
print "FreeBASIC test OK"
FBEOF

/usr/local/bin/fbc /tmp/fb_test.bas
OUTPUT="$(/tmp/fb_test)"

echo "==> output: $OUTPUT"

[ "$OUTPUT" = "FreeBASIC test OK" ] || die "bad output"

echo "==> SUCCESS"

