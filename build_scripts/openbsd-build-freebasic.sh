#!/usr/bin/env sh

set -eu

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

##############################################################################
# Locate project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/mk" ] && [ -f "$SEARCH_DIR/GNUmakefile" ]; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || die "could not locate FreeBASIC root"

cd "$ROOT"

##############################################################################
# Ensure OpenBSD
##############################################################################

[ "$(uname -s)" = "OpenBSD" ] || die "must run on OpenBSD"

##############################################################################
# Config
##############################################################################

BUILDROOT="${ROOT}/.build-openbsd"
STAGE="${BUILDROOT}/stage"
PKGROOT="${BUILDROOT}/pkgroot"
PKGMETA="${BUILDROOT}/pkgmeta"

OS_NAME="$(uname -s)"
OS_VERSION="$(uname -r)"

OUT="${ROOT}/out/${OS_NAME}-${OS_VERSION}"
PREFIX="/usr/local"

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}.${REV}"
PKGFILE="${OUT}/${PKGNAME}-${PKGVERSION}.tgz"

##############################################################################
# Clean packaging artefacts
##############################################################################

echo "==> cleaning packaging artefacts"

rm -rf "$BUILDROOT"
rm -rf "${ROOT}/out/OpenBSD-"*
find . -name '*.o' -delete || true

mkdir -p "$STAGE" "$PKGROOT" "$PKGMETA" "$OUT"

##############################################################################
# Dependencies (install build tools)
##############################################################################

echo "==> installing dependencies"
run pkg_add -I gmake gcc libffi bash git || true

##############################################################################
# Bootstrap / build
##############################################################################

echo "==> bootstrap-minimal"
run gmake bootstrap-minimal
[ -f bootstrap/fbc ] || die "bootstrap failed"

echo "==> full build"
run gmake all FBC=bootstrap/fbc

##############################################################################
# Stage install
##############################################################################

echo "==> installing"
run gmake install DESTDIR="$STAGE" prefix="$PREFIX"

[ -d "$STAGE/usr/local" ] || die "STAGE/usr/local missing"

##############################################################################
# Copy staged files
##############################################################################

echo "==> copying staged files"

case "$PKGROOT" in ""|"/") die "invalid PKGROOT" ;; esac

rm -rf "$PKGROOT"
mkdir -p "$PKGROOT"

( cd "$STAGE" && tar cf - . ) | ( cd "$PKGROOT" && tar xpf - )

##############################################################################
# Generate +DESC
##############################################################################

printf "FreeBASIC compiler for %s %s\n" "$OS_NAME" "$OS_VERSION" > "$PKGMETA/+DESC"

##############################################################################
# Generate +CONTENTS (FIXED DEPENDENCIES)
##############################################################################

echo "==> generating +CONTENTS"

(
    cd "$PKGROOT/usr/local" || exit 1

    {
        printf "@cwd /usr/local\n"

        printf "@comment built on %s-%s\n" "$OS_NAME" "$OS_VERSION"

        find . \( -type f -o -type l \) | sed 's|^\./||' | sort -u
    } > "$PKGMETA/+CONTENTS"
)

head -5 "$PKGMETA/+CONTENTS"

##############################################################################
# Package (NO -P)
##############################################################################

echo "==> creating package"

run pkg_create \
    -B "$PKGROOT" \
    -f "$PKGMETA/+CONTENTS" \
    -d "$PKGMETA/+DESC" \
    -D COMMENT="FreeBASIC compiler" \
    -D MAINTAINER="sj@fbxl.net" \
    -p /usr/local \
    "$PKGFILE"

[ -f "$PKGFILE" ] || die "package creation failed"

echo "==> package created: $PKGFILE"

##############################################################################
# Install package
##############################################################################

echo "==> installing package"
run pkg_add -D unsigned "$PKGFILE"

##############################################################################
# Test compiler
##############################################################################

echo "==> testing compiler"

FBC_BIN="/usr/local/bin/fbc"
[ -x "$FBC_BIN" ] || die "fbc not installed"

cat > /tmp/fb_test.bas <<'FBEOF'
print "FreeBASIC test OK"
FBEOF

run "$FBC_BIN" /tmp/fb_test.bas

[ -x /tmp/fb_test ] || die "compile failed"

OUTPUT="$(/tmp/fb_test)"
echo "==> output: $OUTPUT"

[ "$OUTPUT" = "FreeBASIC test OK" ] || die "bad output"

echo
echo "==> SUCCESS"
