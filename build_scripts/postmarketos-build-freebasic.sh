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

##############################################################################
# Ensure Alpine / postmarketOS
##############################################################################

if [ ! -f /etc/alpine-release ]; then
    echo "ERROR: this script is for postmarketOS / Alpine"
    exit 1
fi

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

##############################################################################
# Config
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-pmos}"
STAGE="${STAGE:-$BUILDROOT/stage}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
OUT="${OUT:-$ROOT/out}"
APKDIR="${BUILDROOT}/apk"

PREFIX="${PREFIX:-/usr}"

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}-r${REV}"
APKFILE="${OUT}/${PKGNAME}-${PKGVERSION}.apk"

##############################################################################
# Dependencies (build)
##############################################################################

echo "==> installing build dependencies"

run apk update

run apk add \
    build-base \
    bash \
    git \
    mesa-dev \
    libx11-dev libxext-dev libxrandr-dev libxrender-dev \
    libxcursor-dev libxi-dev libxinerama-dev libxxf86vm-dev \
    libxcb-dev

##############################################################################
# Prepare dirs
##############################################################################

rm -rf "$BUILDROOT"
mkdir -p "$STAGE" "$PKGROOT" "$OUT" "$APKDIR"

##############################################################################
# Clean
##############################################################################

echo "==> cleaning"
run gmake clean || true

##############################################################################
# Bootstrap
##############################################################################

echo "==> bootstrap-minimal"
run gmake bootstrap-minimal

[ -f bootstrap/fbc ] || die "bootstrap failed"

##############################################################################
# Full build
##############################################################################

echo "==> full build"
run gmake all FBC=bootstrap/fbc

##############################################################################
# Install (stage)
##############################################################################

echo "==> installing"
run gmake install DESTDIR="$STAGE" prefix="$PREFIX"

[ -d "$STAGE/usr" ] || die "unexpected install layout"

##############################################################################
# Prepare pkg root
##############################################################################

cp -a "$STAGE"/. "$PKGROOT"/

##############################################################################
# Detect runtime dependencies (soname-based)
##############################################################################

echo "==> detecting runtime dependencies"

DEPS_FILE="$BUILDROOT/deps.txt"

ldd "$PKGROOT/usr/bin/fbc" | awk '{print $1}' | grep '\.so' | sort -u > "$DEPS_FILE" || true

##############################################################################
# Generate .PKGINFO
##############################################################################

PKGINFO="$APKDIR/.PKGINFO"

{
    echo "pkgname = $PKGNAME"
    echo "pkgver = $PKGVERSION"
    echo "pkgdesc = FreeBASIC compiler"
    echo "url = https://www.freebasic.net/"
    echo "arch = $(apk --print-arch)"
    echo "license = GPL-2.0"
    
    while read -r so; do
        [ -n "$so" ] && echo "depend = so:$so"
    done < "$DEPS_FILE"

} > "$PKGINFO"

##############################################################################
# Generate .apk tarball
##############################################################################

echo "==> creating apk"

tar -C "$PKGROOT" -czf "$APKFILE" .

[ -f "$APKFILE" ] || die "apk creation failed"

echo "==> package: $APKFILE"

##############################################################################
# Install package
##############################################################################

echo "==> installing package"
run apk add --allow-untrusted "$APKFILE"

##############################################################################
# Test compile + run
##############################################################################

echo "==> testing compiler"

FBC_BIN="/usr/bin/fbc"
[ -x "$FBC_BIN" ] || die "fbc not installed"

TEST_SRC="/tmp/fb_test.bas"
TEST_BIN="/tmp/fb_test"

cat > "$TEST_SRC" <<'FBEOF'
print "FreeBASIC test OK"
FBEOF

run "$FBC_BIN" "$TEST_SRC"

[ -x "$TEST_BIN" ] || die "compile failed"

OUTPUT="$("$TEST_BIN")"
echo "==> output: $OUTPUT"

[ "$OUTPUT" = "FreeBASIC test OK" ] || die "bad output"

echo "==> test passed"

##############################################################################
# Done
##############################################################################

echo
echo "==> SUCCESS"
echo "==> package installed and verified"
