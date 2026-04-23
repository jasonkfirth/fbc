#!/bin/sh
set -eu

##############################################################################
# Helpers
##############################################################################
run() {
    echo "==> $*"
    if "$@"; then
        :
    else
        rc=$?
        die "command failed ($rc): $*"
    fi
}

die() {
    echo "ERROR: $*" >&2
    exit 1
}

##############################################################################
# Options
##############################################################################
NO_BUILD=0

for arg in "$@"; do
    case "$arg" in
        --no-build) NO_BUILD=1 ;;
        -h|--help)
            echo "usage: $0 [--no-build]"
            exit 0
            ;;
        *) die "unknown option: $arg" ;;
    esac
done

##############################################################################
# Require root
##############################################################################
[ "$(id -u)" -eq 0 ] || die "must run as root"

##############################################################################
# PATH
##############################################################################
PATH="/usr/pkg/bin:/usr/pkg/sbin:/bin:/usr/bin:/sbin:/usr/sbin:$PATH"
export PATH

##############################################################################
# Locate project root
##############################################################################
START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/mk" ] &&
       { [ -f "$SEARCH_DIR/GNUmakefile" ] ||
         [ -f "$SEARCH_DIR/Makefile" ] ||
         [ -f "$SEARCH_DIR/makefile" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi

    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || die "could not locate FreeBASIC root"
cd "$ROOT"

##############################################################################
# Ensure NetBSD
##############################################################################
[ "$(uname -s)" = "NetBSD" ] || die "must run on NetBSD"

##############################################################################
# Config
##############################################################################
BUILDROOT="${BUILDROOT:-$ROOT/.build-netbsd}"
STAGE="${STAGE:-$BUILDROOT/stage}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
PKGMETA="${PKGMETA:-$BUILDROOT/pkgmeta}"
OUT="${OUT:-$ROOT/out}"

PREFIX="${PREFIX:-/usr/pkg}"

STAGE_PREFIX="${STAGE}${PREFIX}"
PKGROOT_PREFIX="${PKGROOT}${PREFIX}"

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}.${REV}"
PKGFILE="${OUT}/${PKGNAME}-${PKGVERSION}.tgz"

##############################################################################
# Ensure pkgin
##############################################################################
if ! command -v pkgin >/dev/null 2>&1; then
    case "$(uname -m)" in
        amd64|x86_64) ARCH="x86_64" ;;
        i386|i486|i586|i686) ARCH="i386" ;;
        aarch64|earm64) ARCH="aarch64" ;;
        *) ARCH="$(uname -m)" ;;
    esac

    REL="$(uname -r)"
    REL="${REL%%_*}"

    if [ -z "${PKG_PATH:-}" ]; then
        PKG_PATH="http://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/${ARCH}/${REL}/All"
        export PKG_PATH
    fi

    run pkg_add pkgin
fi

##############################################################################
# Dependencies
##############################################################################
BUILD_PKGS="gmake gcc12 ncurses bash git MesaLib libffi libXrender libXcursor"
RUNTIME_PKGS="ncurses MesaLib libffi libXrender libXcursor"
DEPENDS="$(echo "$RUNTIME_PKGS" | tr ' ' '\n' | sort -u | sed 's/$/>=0/' | tr '\n' ' ')"

##############################################################################
# Install build dependencies
##############################################################################
run pkgin -y update
run pkgin -y install $BUILD_PKGS
echo "==> dependency install complete"

##############################################################################
# Build (optional)
##############################################################################
if [ "$NO_BUILD" -eq 0 ]; then
    rm -rf "$BUILDROOT"
    mkdir -p "$STAGE" "$PKGROOT" "$PKGMETA" "$OUT"

    echo "==> cleaning"
    gmake clean || true

    run gmake bootstrap-minimal prefix="$PREFIX"
    [ -f bootstrap/fbc ] || die "bootstrap failed"

    run gmake all FBC=bootstrap/fbc prefix="$PREFIX"
    run gmake install DESTDIR="$STAGE" prefix="$PREFIX"

    [ -x "$STAGE_PREFIX/bin/fbc" ] || die "staged fbc missing after install"
    [ -d "$STAGE_PREFIX/include/freebasic" ] || die "staged include tree missing after install"
    [ -d "$STAGE_PREFIX/lib/freebasic" ] || die "staged runtime tree missing after install"
else
    echo "==> skipping build (--no-build)"

    [ -x "$STAGE_PREFIX/bin/fbc" ] || die "staged fbc missing; run without --no-build first"
    [ -d "$STAGE_PREFIX/include/freebasic" ] || die "staged include tree missing; run without --no-build first"
    [ -d "$STAGE_PREFIX/lib/freebasic" ] || die "staged runtime tree missing; run without --no-build first"

    rm -rf "$PKGROOT" "$PKGMETA"
    mkdir -p "$PKGROOT" "$PKGMETA" "$OUT"
fi

##############################################################################
# Prepare pkgroot
##############################################################################
rm -rf "$PKGROOT"
mkdir -p "$PKGROOT" "$PKGMETA" "$OUT"

cp -R "$STAGE"/. "$PKGROOT"/

[ -x "$PKGROOT_PREFIX/bin/fbc" ] || die "pkgroot fbc missing"
[ -d "$PKGROOT_PREFIX/include/freebasic" ] || die "pkgroot include tree missing"
[ -d "$PKGROOT_PREFIX/lib/freebasic" ] || die "pkgroot runtime tree missing"

##############################################################################
# Generate +CONTENTS
##############################################################################
echo "==> generating +CONTENTS"

CONTENTS="$PKGMETA/+CONTENTS"
FILES="$(mktemp /tmp/freebasic-pkglist.XXXXXX)"
SORTED="${FILES}.sorted"

cd "$PKGROOT_PREFIX" || die "failed to enter staged prefix for plist generation"
/usr/bin/find . -type f -print > "$FILES"
/usr/bin/find . -type l -print >> "$FILES"
/usr/bin/sort "$FILES" | /usr/bin/sed 's|^\./||' > "$SORTED"

{
    echo "@name ${PKGNAME}-${PKGVERSION}"
    echo "@cwd ${PREFIX}"
    cat "$SORTED"
} > "$CONTENTS"

rm -f "$FILES" "$SORTED"
cd "$ROOT" || die "failed to return to project root"

##############################################################################
# Metadata
##############################################################################
echo "==> generating metadata"

echo "FreeBASIC compiler" > "$PKGMETA/+COMMENT"

cat > "$PKGMETA/+DESC" <<EOF
FreeBASIC compiler built from source. Includes gfxlib2 with OpenGL/X11 support.
EOF

case "$(uname -m)" in
    amd64|x86_64) MACHINE_ARCH="x86_64" ;;
    i386) MACHINE_ARCH="i386" ;;
    aarch64) MACHINE_ARCH="aarch64" ;;
    *) MACHINE_ARCH="$(uname -m)" ;;
esac

cat > "$PKGMETA/+BUILD_INFO" <<EOF
ABI=
BUILD_DATE=$(date -u "+%Y-%m-%d %H:%M:%S +0000")
BUILD_HOST=$(uname -s) $(hostname) $(uname -r)
LOCALBASE=${PREFIX}
MACHINE_ARCH=${MACHINE_ARCH}
OBJECT_FMT=ELF
OPSYS=$(uname -s)
OS_VERSION=$(uname -r)
PKGTOOLS_VERSION=20091115
_USE_DESTDIR=user-destdir
EOF

##############################################################################
# Create package
##############################################################################
echo "==> creating package"

pkg_create \
    -B "$PKGMETA/+BUILD_INFO" \
    -c "$PKGMETA/+COMMENT" \
    -d "$PKGMETA/+DESC" \
    -f "$PKGMETA/+CONTENTS" \
    -P "$DEPENDS" \
    -p "$PKGROOT_PREFIX" \
    -I "$PREFIX" \
    "$PKGFILE"

[ -f "$PKGFILE" ] || die "package creation failed"
echo "==> package: $PKGFILE"

##############################################################################
# Install package
##############################################################################
echo "==> installing package"

pkg_delete "${PKGNAME}-${PKGVERSION}" >/dev/null 2>&1 || true
run pkg_add "$PKGFILE"

##############################################################################
# Test
##############################################################################
echo "==> testing compiler"

FBC_BIN="${PREFIX}/bin/fbc"
[ -x "$FBC_BIN" ] || die "fbc not installed"

TEST_SRC="/tmp/fb_test.bas"
TEST_BIN="/tmp/fb_test"

rm -f "$TEST_SRC" "$TEST_BIN"

cat > "$TEST_SRC" <<'EOF'
print "FreeBASIC test OK"
EOF

run "$FBC_BIN" "$TEST_SRC" -x "$TEST_BIN"
[ -x "$TEST_BIN" ] || die "compile failed"

OUTPUT="$("$TEST_BIN")"
echo "==> output: $OUTPUT"

[ "$OUTPUT" = "FreeBASIC test OK" ] || die "bad output"

echo
echo "==> SUCCESS"
echo "==> package installed and verified"
