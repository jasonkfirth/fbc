#!/bin/sh

set -eu

##############################################################################
# Helpers
##############################################################################

run() {
    echo "==> $*"
    "$@"
}

die() {
    echo "ERROR: $*" >&2
    exit 1
}

usage() {
    cat <<EOF
usage: $0 [--no-build] [--no-package]

  --no-build    skip build/stage and reuse existing staged files
  --no-package  stop after build/stage
EOF
    exit 0
}

require_staged_tree() {
    [ -x "$STAGE$PREFIX/bin/fbc" ] || die "missing staged compiler"
    [ -d "$STAGE$PREFIX/include/freebasic" ] || die "missing staged includes"
    [ -d "$STAGE$PREFIX/lib/freebasic" ] || die "missing staged runtime"
}

##############################################################################
# Options
##############################################################################

NO_BUILD=0
NO_PACKAGE=0

for arg in "$@"; do
    case "$arg" in
        --no-build) NO_BUILD=1 ;;
        --no-package) NO_PACKAGE=1 ;;
        -h|--help) usage ;;
        *) die "unknown option: $arg" ;;
    esac
done

##############################################################################
# Locate project root
##############################################################################

SEARCH_DIR="$(pwd)"
ROOT=""
GNUMAKEFILE="GNUmakefile"

while :; do
    if [ -d "$SEARCH_DIR/mk" ] && [ -f "$SEARCH_DIR/$GNUMAKEFILE" ]; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || die "could not locate FreeBASIC root"
cd "$ROOT"

##############################################################################
# Validate environment
##############################################################################

[ "$(uname -s)" = "DragonFly" ] || die "must run on DragonFly"
[ "$(id -u)" -eq 0 ] || die "must run as root"

##############################################################################
# Config
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-dragonfly}"
STAGE="${STAGE:-$BUILDROOT/stage}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
PKGMETA="${PKGMETA:-$BUILDROOT/pkgmeta}"
OUT="${OUT:-$ROOT/out}"
PREFIX="${PREFIX:-/usr/local}"

PKG_SHLIB_IGNORE="${PKG_SHLIB_IGNORE:-libncurses.so.6,libtinfo.so.6}"

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}_${REV}"
PKGFILE="${OUT}/${PKGNAME}-${PKGVERSION}.pkg"

echo "==> DragonFly build"
echo "==> package: ${PKGNAME}-${PKGVERSION}"

##############################################################################
# Install dependencies
##############################################################################

if [ "$NO_BUILD" -eq 0 ]; then
    run pkg update -f

    run pkg install -y \
        gmake gcc git \
        ncurses libffi \
        mesa-libs libglvnd \
        xorgproto \
        libX11 libXext libXrandr libXrender libXpm \
        libXcursor libXi libXinerama libXxf86vm \
        libxcb libXau libXdmcp
fi

##############################################################################
# Resolve GCC
##############################################################################

GCC_PKG="$(
    pkg query '%n' \
    | awk '/^gcc/ {print}' \
    | sort -V \
    | tail -n1 \
    || true
)"

[ -n "$GCC_PKG" ] || die "could not determine installed gcc package"

GCC_ENTRY="$(
    pkg query '%n %o %v' \
    | awk -v name="$GCC_PKG" '
        $1 == name {
            printf "\"%s\": { origin: \"%s\", version: \"%s\" }",
                   $1, $2, $3
        }'
)"

##############################################################################
# Build
##############################################################################

if [ "$NO_BUILD" -eq 0 ]; then
    rm -rf "$BUILDROOT"
    mkdir -p "$STAGE" "$OUT"

    run gmake -f "$GNUMAKEFILE" clean || true
    run gmake -f "$GNUMAKEFILE" bootstrap-minimal
    [ -f bootstrap/fbc ] || die "bootstrap failed"

    run gmake -f "$GNUMAKEFILE" all FBC=bootstrap/fbc
    run gmake -f "$GNUMAKEFILE" install DESTDIR="$STAGE" prefix="$PREFIX"

    require_staged_tree
else
    echo "==> skipping build"
    require_staged_tree
fi

[ "$NO_PACKAGE" -eq 1 ] && exit 0

##############################################################################
# Prepare pkgroot
##############################################################################

rm -rf "$PKGROOT" "$PKGMETA"
mkdir -p "$PKGROOT" "$PKGMETA" "$OUT"

cp -a "$STAGE"/. "$PKGROOT"/

##############################################################################
# plist
##############################################################################

(
    cd "$PKGROOT$PREFIX" || exit 1
    find . \( -type f -o -type l \) | sed 's|^\./||' | sort
) > "$PKGMETA/+plist"

##############################################################################
# deps
##############################################################################

DEPS="$GCC_ENTRY"

for dep in ncurses libffi mesa-libs libglvnd libX11 libXext libXrandr libXrender libXpm libXcursor libXi libXinerama libXxf86vm libxcb libXau libXdmcp
do
    entry="$(
        pkg query '%n %o %v' \
        | awk -v dep="$dep" '
            $1 == dep {
                printf "\"%s\": { origin: \"%s\", version: \"%s\" }",
                       $1, $2, $3
            }' \
        || true
    )"

    [ -n "$entry" ] && DEPS="${DEPS},${entry}"
done

##############################################################################
# manifest
##############################################################################

cat > "$PKGMETA/+MANIFEST" <<EOF
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
FreeBASIC compiler built from source for DragonFly BSD.
Includes full runtime support (console + gfxlib).
Requires system GCC toolchain.
EOD
EOF

##############################################################################
# create package (critical fix)
##############################################################################

run pkg \
    -o SHLIB_REQUIRE_IGNORE_GLOB="$PKG_SHLIB_IGNORE" \
    create \
    -f txz \
    -m "$PKGMETA" \
    -r "$PKGROOT" \
    -p "$PKGMETA/+plist" \
    -o "$OUT"

##############################################################################
# install
##############################################################################

run pkg add -f "$PKGFILE"

##############################################################################
# test
##############################################################################

echo 'print "OK"' > /tmp/fb.bas
run ${PREFIX}/bin/fbc /tmp/fb.bas -x /tmp/fb
/tmp/fb

echo "==> SUCCESS"
