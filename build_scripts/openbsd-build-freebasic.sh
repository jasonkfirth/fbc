#!/usr/bin/env bash

##############################################################################
# FreeBASIC OpenBSD package builder
##############################################################################
#
# Purpose:
#
#   Build a native OpenBSD FreeBASIC package from a checked out source tree.
#
# Responsibilities:
#
#   * install the OpenBSD build dependencies used by the package
#   * seed and build the native bootstrap compiler
#   * stage the compiler, runtime, headers, and examples
#   * create an unsigned OpenBSD package
#   * install the package locally for a quick compiler sanity check
#
# This script intentionally does NOT contain:
#
#   * VM installation or host-side QEMU orchestration
#   * cross-compilation into OpenBSD packages
#   * full smoke-test or fbctests orchestration
#
##############################################################################

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

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
# Validate environment
##############################################################################

[ "$(uname -s)" = "OpenBSD" ] || die "must run on OpenBSD"
[ "$(id -u)" -eq 0 ] || die "must run as root"

##############################################################################
# Config
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-openbsd}"
STAGE="${STAGE:-$BUILDROOT/stage}"
PKGROOT="${PKGROOT:-$BUILDROOT/pkgroot}"
PKGMETA="${PKGMETA:-$BUILDROOT/pkgmeta}"
OUT="${OUT:-$ROOT/out}"
PREFIX="${PREFIX:-/usr/local}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 1)}"
BOOTSTRAP_DIR="${BOOTSTRAP_DIR:-openbsd-x86_64}"

PATH="/usr/local/bin:/usr/local/sbin:/usr/X11R6/bin:/bin:/sbin:/usr/bin:/usr/sbin:$PATH"
export PATH

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

PKGNAME="freebasic"
PKGVERSION="${FBVERSION}.${REV}"
PKGFILE="${OUT}/${PKGNAME}-${PKGVERSION}.tgz"

case "$JOBS" in
	''|*[!0-9]*|0) die "JOBS must be a positive integer" ;;
esac

##############################################################################
# Dependencies
##############################################################################

echo "==> installing build dependencies"

run pkg_add -I \
	bash \
	g++-11.2.0p19 \
	gcc-11.2.0p19 \
	git \
	gmake \
	libffi \
	rsync-3.4.1

command -v egcc >/dev/null 2>&1 || die "egcc was not installed"
command -v eg++ >/dev/null 2>&1 || die "eg++ was not installed"

##############################################################################
# Build
##############################################################################

echo "==> cleaning build tree"

rm -rf "$BUILDROOT"
mkdir -p "$STAGE" "$PKGROOT" "$PKGMETA" "$OUT"

run gmake clean || true

if ! [ -d "bootstrap/$BOOTSTRAP_DIR" ] ||
   ! find "bootstrap/$BOOTSTRAP_DIR" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print | sed -n '1p' | grep -q .; then
	run gmake -j "$JOBS" bootstrap-seed-peer
fi

run gmake -j "$JOBS" bootstrap-minimal
[ -f bootstrap/fbc ] || die "bootstrap failed"

run gmake -j "$JOBS" all FBC=bootstrap/fbc
run gmake install DESTDIR="$STAGE" prefix="$PREFIX"

mkdir -p "$STAGE$PREFIX/share/freebasic/examples"
rsync -a --delete \
	--exclude-from "$ROOT/mk/example-copy-excludes.rsync" \
	"$ROOT/examples/" "$STAGE$PREFIX/share/freebasic/examples/"

[ -x "$STAGE$PREFIX/bin/fbc" ] || die "staged compiler missing"
[ -d "$STAGE$PREFIX/include/freebasic" ] || die "staged include tree missing"
[ -d "$STAGE$PREFIX/lib/freebasic" ] || die "staged runtime tree missing"

##############################################################################
# Package root
##############################################################################

echo "==> preparing package root"

rm -rf "$PKGROOT"
mkdir -p "$PKGROOT"

( cd "$STAGE" && tar cf - . ) | ( cd "$PKGROOT" && tar xpf - )

##############################################################################
# Metadata
##############################################################################

echo "==> generating package metadata"

cat > "$PKGMETA/+DESC" <<EOF
FreeBASIC compiler built from source for OpenBSD.
Includes the compiler, runtime libraries, headers, and examples.
EOF

(
	cd "$PKGROOT$PREFIX" || exit 1

	{
		printf "@cwd %s\n" "$PREFIX"
		printf "@comment built on OpenBSD-%s\n" "$(uname -r)"
		find . \( -type f -o -type l \) | sed 's|^\./||' | sort -u
	} > "$PKGMETA/+CONTENTS"
)

##############################################################################
# Package
##############################################################################

echo "==> creating package"

run pkg_create \
	-B "$PKGROOT" \
	-f "$PKGMETA/+CONTENTS" \
	-d "$PKGMETA/+DESC" \
	-D COMMENT="FreeBASIC compiler" \
	-D MAINTAINER="root@localhost" \
	-p "$PREFIX" \
	"$PKGFILE"

[ -f "$PKGFILE" ] || die "package creation failed"

##############################################################################
# Local sanity install
##############################################################################

echo "==> installing package"

pkg_delete "$PKGNAME-$PKGVERSION" >/dev/null 2>&1 || true
run pkg_add -D unsigned "$PKGFILE"

echo "==> testing compiler"

FBC_BIN="$PREFIX/bin/fbc"
[ -x "$FBC_BIN" ] || die "fbc not installed"

cat > /tmp/fb_test.bas <<'FBEOF'
print "FreeBASIC test OK"
FBEOF

run "$FBC_BIN" /tmp/fb_test.bas -x /tmp/fb_test

OUTPUT="$(/tmp/fb_test)"
echo "==> output: $OUTPUT"

[ "$OUTPUT" = "FreeBASIC test OK" ] || die "bad output"

echo "==> package created: $PKGFILE"
echo "==> SUCCESS"

##############################################################################
# end of openbsd-build-freebasic.sh
##############################################################################
