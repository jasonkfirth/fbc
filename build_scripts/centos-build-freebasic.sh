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
# Ensure CentOS / RHEL-like
##############################################################################

if [ ! -f /etc/redhat-release ]; then
    echo "ERROR: this script is for CentOS / RHEL-like systems"
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

BUILDROOT="${BUILDROOT:-$ROOT/.build-centos}"
STAGE="${STAGE:-$BUILDROOT/stage}"
RPMROOT="${BUILDROOT}/rpmbuild"
OUT="${OUT:-$ROOT/out}"

PREFIX="${PREFIX:-/usr}"

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}"
RPMRELEASE="${REV}"

ARCH="$(uname -m)"

##############################################################################
# Dependencies (build)
##############################################################################

echo "==> installing build dependencies"

if command -v dnf >/dev/null 2>&1; then
    PKG_INSTALL="dnf install -y"
else
    PKG_INSTALL="yum install -y"
fi

run $PKG_INSTALL \
    gcc gcc-c++ make rpm-build \
    mesa-libGL-devel \
    libX11-devel libXext-devel libXrandr-devel libXrender-devel \
    libXcursor-devel libXi-devel libXinerama-devel libXxf86vm-devel \
    libxcb-devel

##############################################################################
# Prepare dirs
##############################################################################

rm -rf "$BUILDROOT"
mkdir -p "$STAGE" "$OUT"

mkdir -p "$RPMROOT"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

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
# Detect runtime dependencies (soname-based)
##############################################################################

echo "==> detecting runtime dependencies"

DEPS_FILE="$BUILDROOT/deps.txt"

ldd "$STAGE/usr/bin/fbc" | awk '{print $1}' | grep '\.so' | sort -u > "$DEPS_FILE" || true

##############################################################################
# Create tarball source
##############################################################################

TARBALL="$RPMROOT/SOURCES/${PKGNAME}-${PKGVERSION}.tar.gz"

tar -C "$STAGE" -czf "$TARBALL" .

##############################################################################
# Generate SPEC file
##############################################################################

SPECFILE="$RPMROOT/SPECS/${PKGNAME}.spec"

echo "==> generating spec file"

{
    echo "Name: $PKGNAME"
    echo "Version: $PKGVERSION"
    echo "Release: $RPMRELEASE"
    echo "Summary: FreeBASIC compiler"
    echo "License: GPLv2"
    echo "BuildArch: $ARCH"
    echo "Source0: $(basename "$TARBALL")"
    echo ""
    echo "%description"
    echo "FreeBASIC compiler built from source with OpenGL/X11 support."
    echo ""
    echo "%prep"
    echo "%setup -q -c -T"
    echo "tar -xzf %{SOURCE0}"
    echo ""
    echo "%build"
    echo "# already built"
    echo ""
    echo "%install"
    echo "mkdir -p %{buildroot}"
    echo "cp -a * %{buildroot}/"
    echo ""
    echo "%files"
    echo "/usr/bin/fbc"
    echo "/usr/lib*/freebasic"
    echo "/usr/include/freebasic"
    echo ""
    echo "%post"
    echo "/sbin/ldconfig || true"
    echo ""
    echo "%postun"
    echo "/sbin/ldconfig || true"
    echo ""
    echo "# Dependencies"

    while read -r so; do
        [ -n "$so" ] && echo "Requires: $so"
    done < "$DEPS_FILE"

} > "$SPECFILE"

##############################################################################
# Build RPM
##############################################################################

echo "==> building RPM"

rpmbuild --define "_topdir $RPMROOT" -bb "$SPECFILE"

##############################################################################
# Copy RPM out
##############################################################################

RPMFILE="$(find "$RPMROOT/RPMS" -name '*.rpm' | head -n1)"
[ -n "$RPMFILE" ] || die "RPM not created"

cp -v "$RPMFILE" "$OUT/"

echo "==> package: $OUT/$(basename "$RPMFILE")"

##############################################################################
# Install package
##############################################################################

echo "==> installing package"

if command -v dnf >/dev/null 2>&1; then
    run dnf install -y "$RPMFILE"
else
    run yum install -y "$RPMFILE"
fi

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
