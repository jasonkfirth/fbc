#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE installer packaging
# -------------------------------------------------
#
# File: wince-package-debian.sh
#
# Purpose:
#
#     Create an installable amd64 Debian package containing the complete
#     Linux-hosted FreeBASIC Windows CE ARM and MIPS cross-target SDK.
#
# Responsibilities:
#
#     - validate both target runtimes and the Linux x86_64 host compiler
#     - bundle the pinned ARM CeGCC tree and MIPS PE tools
#     - install fbc-wince plus direct architecture-specific commands
#     - compile and inspect representative programs before packaging
#     - create a deterministic package checksum
#
# This file intentionally does NOT contain:
#
#     - compiler, runtime, or target toolchain construction
#     - proprietary Windows CE ROM images
#     - emulator startup or application deployment
#     - Win32 MinGW-w64 package policy
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

WORK_ROOT="${WINCE_WORK_ROOT:-$ROOT/out/wince/work}"
TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$ROOT/out/wince/mips-toolchain}"
TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
PACKAGE_OUTDIR="${FBC_PACKAGE_OUTDIR:-$ROOT/out/linux/ubuntu/resolute/amd64/wince}"
VALIDATION_OUTDIR="${WINCE_PACKAGE_VALIDATION_OUTDIR:-$ROOT/out/wince/packages/validation/debian}"
PACKAGE_REVISION="${WINCE_PACKAGE_REVISION:-1}"
KEEP_STAGE=0
STAGE_ROOT=""
ARM_CONTAINER=""

die() {
	echo "ERROR: $*" >&2
	exit 1
}

msg() {
	echo
	echo "==> $*"
}

require_value() {
	[ -n "${2-}" ] || die "$1 requires a value"
}

require_command() {
	command -v "$1" >/dev/null 2>&1 || die "required tool not found: $1"
}

cleanup() {
	if [ -n "$ARM_CONTAINER" ]; then
		docker rm -f "$ARM_CONTAINER" >/dev/null 2>&1 || true
	fi
	if [ "$KEEP_STAGE" -eq 0 ] && [ -n "$STAGE_ROOT" ] &&
	   [[ "$STAGE_ROOT" == "$PACKAGE_OUTDIR"/.freebasic-wince-deb.* ]]; then
		rm -rf -- "$STAGE_ROOT"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince-package-debian.sh [options]

Options:
  --work-dir DIR       Prepared source tree containing both target runtimes.
  --toolchain-dir DIR  Prepared MIPS PE toolchain directory.
  --image NAME         Pinned ARM CeGCC image.
  --out-dir DIR        Debian package output directory.
  --validation-dir DIR Package-built smoke executable directory.
  --revision N         Debian package revision. Default: 1
  --keep-stage         Preserve the package staging tree.
  -h, --help           Show this help text.

Output:
  freebasic-wince_VERSION-REVISION_amd64.deb

Installed commands:
  fbc-wince --arch arm program.bas
  fbc-wince --arch mips program.bas
  fbc-wince-arm program.bas
  fbc-wince-mips program.bas
EOF
}

trap cleanup EXIT

while [ "$#" -gt 0 ]; do
	case "$1" in
		--work-dir)
			require_value "$1" "${2-}"
			WORK_ROOT="$2"
			shift 2
			;;
		--toolchain-dir)
			require_value "$1" "${2-}"
			TOOLCHAIN_ROOT="$2"
			shift 2
			;;
		--image)
			require_value "$1" "${2-}"
			TOOLCHAIN_IMAGE="$2"
			shift 2
			;;
		--out-dir)
			require_value "$1" "${2-}"
			PACKAGE_OUTDIR="$2"
			shift 2
			;;
		--validation-dir)
			require_value "$1" "${2-}"
			VALIDATION_OUTDIR="$2"
			shift 2
			;;
		--revision)
			require_value "$1" "${2-}"
			PACKAGE_REVISION="$2"
			shift 2
			;;
		--keep-stage)
			KEEP_STAGE=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			die "unknown option: $1"
			;;
	esac
done

case "$PACKAGE_REVISION" in
	''|*[!0-9]*|0) die "--revision must be a positive integer" ;;
esac

for tool in clang docker dpkg dpkg-deb fakeroot file readelf sha256sum; do
	require_command "$tool"
done
[ "$(dpkg --print-architecture)" = amd64 ] ||
	die "the Windows CE installer package must be built on amd64"
docker image inspect "$TOOLCHAIN_IMAGE" >/dev/null 2>&1 ||
	die "ARM CeGCC image not found: $TOOLCHAIN_IMAGE"

##############################################################################
# Prepared build validation
##############################################################################

VERSION_FILE="$WORK_ROOT/mk/version.mk"
COMPILER="$WORK_ROOT/bin/fbc"
ARM_RUNTIME="$WORK_ROOT/lib/freebasic/wince-arm"
MIPS_RUNTIME="$WORK_ROOT/lib/freebasic/wince-mips32el"

[ -f "$VERSION_FILE" ] || die "version file not found: $VERSION_FILE"
[ -x "$COMPILER" ] || die "prepared Linux host compiler not found: $COMPILER"
[ -d "$WORK_ROOT/inc" ] || die "FreeBASIC headers not found in prepared tree"
[ -d "$ARM_RUNTIME" ] || die "Windows CE ARM runtime is missing"
[ -d "$MIPS_RUNTIME" ] || die "Windows CE MIPS runtime is missing"
[ -d "$TOOLCHAIN_ROOT/bin" ] || die "MIPS PE toolchain is missing"

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$VERSION_FILE")"
[ -n "$VERSION" ] || die "could not determine FreeBASIC version"
DEB_VERSION="${VERSION}-${PACKAGE_REVISION}"

COMPILER_DESCRIPTION="$(file -b "$COMPILER")"
[[ "$COMPILER_DESCRIPTION" == *"ELF 64-bit LSB"* ]] ||
	die "prepared compiler is not Linux x86_64: $COMPILER_DESCRIPTION"
readelf -h "$COMPILER" | grep -q 'Machine:.*Advanced Micro Devices X86-64' ||
	die "prepared compiler has the wrong machine type"

COMMON_RUNTIME_FILES=(
	fbrt0.o fbrt1.o fbrt2.o libfb.a libfbmt.a
	libfbgfx.a libfbgfxmt.a libsfx.a libsfxmt.a libffi.a
)
for runtime_file in "${COMMON_RUNTIME_FILES[@]}"; do
	[ -s "$ARM_RUNTIME/$runtime_file" ] ||
		die "ARM runtime is missing $runtime_file"
	[ -s "$MIPS_RUNTIME/$runtime_file" ] ||
		die "MIPS runtime is missing $runtime_file"
done
for toolchain_file in \
	bin/mips-wince-pe-ar \
	bin/mips-wince-pe-ld \
	bin/mips-wince-pe-objdump \
	include/windows.h \
	lib/libcoredll.a; do
	[ -s "$TOOLCHAIN_ROOT/$toolchain_file" ] ||
		die "MIPS toolchain is missing $toolchain_file"
done

##############################################################################
# Debian package staging
##############################################################################

mkdir -p "$PACKAGE_OUTDIR" "$VALIDATION_OUTDIR"
PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
VALIDATION_OUTDIR="$(cd "$VALIDATION_OUTDIR" && pwd)"
[ "$PACKAGE_OUTDIR" != / ] || die "refusing to use / as package output"

STAGE_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.freebasic-wince-deb.XXXXXX")"
PKGROOT="$STAGE_ROOT/package-root"
SDKROOT="$PKGROOT/usr/lib/freebasic-wince"
DOCROOT="$PKGROOT/usr/share/doc/freebasic-wince"
mkdir -p "$PKGROOT/DEBIAN" "$PKGROOT/usr/bin" "$SDKROOT/bin" \
	"$SDKROOT/include/freebasic" "$SDKROOT/lib/freebasic" \
	"$SDKROOT/toolchain" "$DOCROOT"

cp "$WORK_ROOT/src/tools/wince/fbc-wince" "$SDKROOT/bin/fbc-wince"
cp "$WORK_ROOT/src/tools/wince/fbc-wince-arm" "$SDKROOT/bin/fbc-wince-arm"
cp "$WORK_ROOT/src/tools/wince/fbc-wince-mips" "$SDKROOT/bin/fbc-wince-mips"
chmod 755 "$SDKROOT/bin/fbc-wince" "$SDKROOT/bin/fbc-wince-arm" \
	"$SDKROOT/bin/fbc-wince-mips"
cp -a "$WORK_ROOT/inc/." "$SDKROOT/include/freebasic/"
cp -a "$ARM_RUNTIME" "$SDKROOT/lib/freebasic/wince-arm"
cp -a "$MIPS_RUNTIME" "$SDKROOT/lib/freebasic/wince-mips32el"
cp -a "$TOOLCHAIN_ROOT" "$SDKROOT/toolchain/mips-wince-pe"

msg "extracting the pinned ARM CeGCC payload"
ARM_CONTAINER="$(docker create "$TOOLCHAIN_IMAGE" true)"
mkdir -p "$SDKROOT/toolchain/cegcc-arm"
docker cp "$ARM_CONTAINER:/opt/cegcc-arm/." "$SDKROOT/toolchain/cegcc-arm/"
docker rm "$ARM_CONTAINER" >/dev/null
ARM_CONTAINER=""
[ -x "$SDKROOT/toolchain/cegcc-arm/bin/arm-mingw32ce-gcc" ] ||
	die "packaged ARM CeGCC compiler is missing"

ln -s ../lib/freebasic-wince/bin/fbc-wince "$PKGROOT/usr/bin/fbc-wince"
ln -s ../lib/freebasic-wince/bin/fbc-wince-arm "$PKGROOT/usr/bin/fbc-wince-arm"
ln -s ../lib/freebasic-wince/bin/fbc-wince-mips "$PKGROOT/usr/bin/fbc-wince-mips"

if [ -f "$WORK_ROOT/debian/copyright" ]; then
	cp "$WORK_ROOT/debian/copyright" "$DOCROOT/copyright.FreeBASIC"
fi
if [ -f "$WORK_ROOT/docs/wince.md" ]; then
	cp "$WORK_ROOT/docs/wince.md" "$DOCROOT/README.wince.md"
fi

cat > "$DOCROOT/README.Debian" <<EOF
freebasic-wince
===============

This package provides Linux-hosted Windows CE cross-compilers for ARMv4T and
little-endian MIPS II targets. It includes the target runtimes, gfxlib2,
sfxlib, libffi, the pinned ARM CeGCC toolchain, and the MIPS PE linker tools.

Examples:

  fbc-wince hello.bas -x hello-arm.exe
  fbc-wince --arch mips hello.bas -x hello-mips.exe

The default architecture is ARM. Set FBC_WINCE_ARCH=mips or use --arch mips
for MIPS builds. Windows CE ROMs and emulators are not part of this package.
EOF

##############################################################################
# Staged SDK compilation validation
##############################################################################

msg "compiling representative programs with the staged installer SDK"
cp "$COMPILER" "$SDKROOT/bin/fbc"
chmod 755 "$SDKROOT/bin/fbc"

"$SDKROOT/bin/fbc-wince" --arch arm -O 0 -exx \
	"$WORK_ROOT/tests/wince/basic_file.bas" \
	-x "$VALIDATION_OUTDIR/package-arm-basic-file.exe"
"$SDKROOT/bin/fbc-wince" --arch mips -O 0 -exx \
	"$WORK_ROOT/tests/wince/basic_file.bas" \
	-x "$VALIDATION_OUTDIR/package-mips-basic-file.exe"

"$SDKROOT/toolchain/cegcc-arm/bin/arm-mingw32ce-objdump" -f \
	"$VALIDATION_OUTDIR/package-arm-basic-file.exe" |
	grep -Eq 'file format pei?-arm-wince-little' ||
	die "installer smoke did not produce an ARM Windows CE executable"
"$SDKROOT/toolchain/mips-wince-pe/bin/mips-wince-pe-objdump" -f \
	"$VALIDATION_OUTDIR/package-mips-basic-file.exe" |
	grep -F 'file format pei-mips' >/dev/null ||
	die "installer smoke did not produce a MIPS Windows CE executable"

rm "$SDKROOT/bin/fbc"
ln -s /usr/bin/fbc "$SDKROOT/bin/fbc"

##############################################################################
# Package metadata and artifact
##############################################################################

INSTALLED_SIZE="$(du -sk "$PKGROOT/usr" | awk '{ print $1 }')"
cat > "$PKGROOT/DEBIAN/control" <<EOF
Package: freebasic-wince
Version: $DEB_VERSION
Section: devel
Priority: optional
Architecture: amd64
Maintainer: SJ_Zero <sj@fbxl.net>
Depends: freebasic (>= $VERSION), clang, libc6, libgcc-s1, libstdc++6, zlib1g
Installed-Size: $INSTALLED_SIZE
Homepage: https://freebasic.net/
Description: FreeBASIC cross-compilers for Windows CE ARM and MIPS
 This package provides fbc-wince, architecture-specific compiler drivers,
 complete FreeBASIC target libraries, and the required Windows CE PE tools.
 It produces ARMv4T and little-endian MIPS Windows CE executables from an
 Ubuntu or Debian amd64 development host.
EOF

find "$PKGROOT" -type d -exec chmod 755 {} +
DEB_PATH="$PACKAGE_OUTDIR/freebasic-wince_${DEB_VERSION}_amd64.deb"
rm -f "$DEB_PATH" "$DEB_PATH.sha256"
fakeroot dpkg-deb --build "$PKGROOT" "$DEB_PATH"
(
	cd "$PACKAGE_OUTDIR"
	sha256sum "$(basename "$DEB_PATH")" > "$(basename "$DEB_PATH").sha256"
)

msg "Windows CE installer package ready"
echo "    $DEB_PATH"
echo "    $DEB_PATH.sha256"
if [ "$KEEP_STAGE" -eq 1 ]; then
	echo "    staging tree: $STAGE_ROOT"
fi

# end of build_scripts/wince-package-debian.sh
