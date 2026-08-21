#!/usr/bin/env bash
#
# Project: FreeBASIC Debian/Ubuntu Win32 cross-target package
# ----------------------------------------------------------
#
# File: debianubuntu-build-freebasic-win32.sh
#
# Purpose:
#
#     Build an installable amd64 package that provides fbc-win32 and the
#     complete 32-bit Windows target runtime on Debian or Ubuntu.
#
# Responsibilities:
#
#     - prepare an isolated source tree and Linux host compiler
#     - build pinned Win32 libffi and all FreeBASIC target libraries
#     - stage the relocatable fbc-win32 compiler driver
#     - compile and inspect a representative PE executable
#     - create a Debian package and SHA-256 manifest
#
# This file intentionally does NOT contain:
#
#     - Wine execution or Windows VM policy
#     - Win64 or Windows ARM64 target construction
#     - Windows application installer generation
#     - Windows CE target behavior
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Build identity and defaults
##############################################################################

readonly LIBFFI_VERSION="3.5.2"
readonly LIBFFI_SHA256="f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc"
readonly LIBFFI_URL="https://github.com/libffi/libffi/releases/download/v${LIBFFI_VERSION}/libffi-${LIBFFI_VERSION}.tar.gz"

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

NO_BUILD=0
NO_PACKAGE=0
SKIP_DEPS=0
KEEP_WORK=0
JOBS="${JOBS:-2}"
PACKAGE_REVISION="${FBWIN32_PACKAGE_REVISION:-1}"
BUILDROOT="${BUILDROOT:-$ROOT/.build-debianubuntu-win32}"
WORK_ROOT="$BUILDROOT/work"
OUTBASE="${OUTBASE:-$ROOT/out}"

die() {
	echo "ERROR: $*" >&2
	exit 1
}

msg() {
	echo
	echo "==> $*"
}

run() {
	echo "==> $*"
	"$@"
}

run_root() {
	if [ "$(id -u)" -eq 0 ]; then
		run "$@"
	elif command -v sudo >/dev/null 2>&1; then
		run sudo "$@"
	else
		die "root privileges are required to install build dependencies"
	fi
}

cleanup() {
	[ "$KEEP_WORK" -eq 0 ] || return 0
	[ -d "$BUILDROOT" ] || return 0
	[[ "$BUILDROOT" == "$ROOT"/.build-debianubuntu-win32* ]] || return 0
	rm -rf -- "$BUILDROOT"
}

usage() {
	cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic-win32.sh [options]

Options:
  --no-build       Reuse the prepared isolated compiler and Win32 runtime.
  --no-package     Stop after building the target runtime.
  --skip-deps      Do not install APT build dependencies.
  --jobs N         Parallel build jobs. Default: $JOBS
  --revision N     Debian package revision. Default: $PACKAGE_REVISION
  --keep-work      Preserve the isolated build tree after success.
  -h, --help       Show this help text.

Environment:
  BUILDROOT        Isolated build root.
  OUTBASE          Artifact root. Default: out
  FBC_PACKAGE_OUTDIR
                   Full package output directory override.

Artifacts are written below:
  out/linux/<distro>/<codename>/amd64/win32/

On Ubuntu Resolute amd64 this creates freebasic-win32 and installs the
fbc-win32 command. MinGW-w64 is kept as a maintained package dependency.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--no-build)
			NO_BUILD=1
			shift
			;;
		--no-package)
			NO_PACKAGE=1
			shift
			;;
		--skip-deps)
			SKIP_DEPS=1
			shift
			;;
		--jobs)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			JOBS="$2"
			shift 2
			;;
		--revision)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			PACKAGE_REVISION="$2"
			shift 2
			;;
		--keep-work)
			KEEP_WORK=1
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

case "$JOBS" in
	''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac
case "$PACKAGE_REVISION" in
	''|*[!0-9]*|0) die "--revision must be a positive integer" ;;
esac
command -v apt-get >/dev/null 2>&1 ||
	die "this workflow requires a Debian or Ubuntu host"

if [ "$SKIP_DEPS" -eq 0 ]; then
	msg "installing Win32 cross-target build dependencies"
	export DEBIAN_FRONTEND=noninteractive
	run_root apt-get update -y
	run_root apt-get install -y --no-install-recommends \
		binutils build-essential ca-certificates coreutils curl dpkg-dev fakeroot file \
		gcc-mingw-w64-i686 g++-mingw-w64-i686 binutils-mingw-w64-i686 libffi-dev \
		libgpm-dev libncurses-dev make pkgconf rsync \
		tar xz-utils
fi

for tool in curl dpkg dpkg-deb fakeroot file i686-w64-mingw32-gcc \
	i686-w64-mingw32-objdump make readelf rsync sha256sum; do
	command -v "$tool" >/dev/null 2>&1 || die "required tool not found: $tool"
done
[ "$(dpkg --print-architecture)" = amd64 ] ||
	die "this workflow creates an amd64 installer and requires an amd64 host"

DISTRO_ID=unknown
CODENAME=unknown
if [ -f /etc/os-release ]; then
	DISTRO_ID="$(. /etc/os-release; printf '%s' "${ID:-unknown}")"
	CODENAME="$(. /etc/os-release; printf '%s' "${VERSION_CODENAME:-unknown}")"
fi
DISTRO_ID="${FBC_PACKAGE_DISTRO_ID:-$DISTRO_ID}"
CODENAME="${FBC_PACKAGE_CODENAME:-$CODENAME}"
PACKAGE_OUTDIR="${FBC_PACKAGE_OUTDIR:-$OUTBASE/linux/$DISTRO_ID/$CODENAME/amd64/win32}"

mkdir -p "$BUILDROOT" "$PACKAGE_OUTDIR"

##############################################################################
# Isolated compiler and target runtime build
##############################################################################

if [ "$NO_BUILD" -eq 0 ]; then
	SOURCE_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"
	[ -f "$SOURCE_EXCLUDES" ] || die "source copy exclusions are missing"
	mkdir -p "$WORK_ROOT"
	msg "refreshing the isolated Win32 source tree"
	run rsync -a --delete --delete-excluded \
		--exclude-from="$SOURCE_EXCLUDES" \
		--exclude='/.build-debianubuntu-win32/' \
		"$ROOT/" "$WORK_ROOT/"

	msg "building the Linux x86_64 host compiler"
	(
		cd "$WORK_ROOT"
		make -j"$JOBS" bootstrap-minimal
		make -j"$JOBS" compiler FBC="$WORK_ROOT/bin/fbc"
	)

	msg "building pinned libffi for i686 MinGW-w64"
	LIBFFI_ARCHIVE="$BUILDROOT/libffi-${LIBFFI_VERSION}.tar.gz"
	if [ ! -f "$LIBFFI_ARCHIVE" ] ||
	   ! printf '%s  %s\n' "$LIBFFI_SHA256" "$LIBFFI_ARCHIVE" |
		sha256sum --check --status; then
		rm -f "$LIBFFI_ARCHIVE.partial"
		curl --fail --location --silent --show-error \
			-o "$LIBFFI_ARCHIVE.partial" "$LIBFFI_URL"
		printf '%s  %s\n' "$LIBFFI_SHA256" "$LIBFFI_ARCHIVE.partial" |
			sha256sum --check --status || die "libffi checksum verification failed"
		mv "$LIBFFI_ARCHIVE.partial" "$LIBFFI_ARCHIVE"
	fi

	LIBFFI_ROOT="$BUILDROOT/libffi"
	rm -rf "$LIBFFI_ROOT"
	mkdir -p "$LIBFFI_ROOT/source" "$LIBFFI_ROOT/build" "$LIBFFI_ROOT/stage"
	tar -xzf "$LIBFFI_ARCHIVE" --strip-components=1 -C "$LIBFFI_ROOT/source"
	(
		cd "$LIBFFI_ROOT/build"
		CC=i686-w64-mingw32-gcc \
		CXX=i686-w64-mingw32-g++ \
		AR=i686-w64-mingw32-ar \
		RANLIB=i686-w64-mingw32-ranlib \
		CFLAGS='-O2 -g0' \
			"$LIBFFI_ROOT/source/configure" \
			--host=i686-w64-mingw32 \
			--prefix="$LIBFFI_ROOT/stage" \
			--disable-shared \
			--enable-static
		make -j"$JOBS"
		make install
	)

	TARGET_RUNTIME="$WORK_ROOT/lib/freebasic/win32-x86"
	TARGET_ARGS=(
		TARGET_TRIPLET=i686-w64-mingw32
		BUILD_PREFIX=i686-w64-mingw32-
		CC=i686-w64-mingw32-gcc
		CXX=i686-w64-mingw32-g++
		AS=i686-w64-mingw32-as
		LD=i686-w64-mingw32-ld
		AR=i686-w64-mingw32-ar
		RANLIB=i686-w64-mingw32-ranlib
		FBC="$WORK_ROOT/bin/fbc"
		BUILD_FBC="$WORK_ROOT/bin/fbc"
		BUILD_FBC_TARGET=win32
		BUILD_FBC_BUILDPREFIX=i686-w64-mingw32-
		DISABLE_NCURSES=YesPlease
	)
	(
		cd "$WORK_ROOT"
		make clean-libs "${TARGET_ARGS[@]}"
		mkdir -p "$TARGET_RUNTIME/include" "$TARGET_RUNTIME/licenses/libffi"
		cp "$LIBFFI_ROOT/stage/lib/libffi.a" "$TARGET_RUNTIME/libffi.a"
		cp "$LIBFFI_ROOT/stage/include/ffi.h" "$TARGET_RUNTIME/include/ffi.h"
		cp "$LIBFFI_ROOT/stage/include/ffitarget.h" \
			"$TARGET_RUNTIME/include/ffitarget.h"
		cp "$LIBFFI_ROOT/source/LICENSE" "$TARGET_RUNTIME/licenses/libffi/LICENSE"
		make -j"$JOBS" rtlib fbrt gfxlib2 sfxlib \
			CPPFLAGS="-I$TARGET_RUNTIME/include" \
			CFLAGS="-O2 -I$TARGET_RUNTIME/include" \
			"${TARGET_ARGS[@]}"
	)
fi

if [ "$NO_PACKAGE" -eq 1 ]; then
	msg "Win32 target build ready"
	exit 0
fi

##############################################################################
# Debian installer staging
##############################################################################

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "$WORK_ROOT/mk/version.mk")"
[ -n "$VERSION" ] || die "could not determine FreeBASIC version"
DEB_VERSION="${VERSION}-${PACKAGE_REVISION}"
COMPILER="$WORK_ROOT/bin/fbc"
TARGET_RUNTIME="$WORK_ROOT/lib/freebasic/win32-x86"
WRAPPER="$WORK_ROOT/src/tools/win32/fbc-win32"

[ -x "$COMPILER" ] || die "prepared Linux host compiler is missing"
[ -x "$WRAPPER" ] || die "fbc-win32 wrapper is missing"
[ -d "$WORK_ROOT/inc" ] || die "FreeBASIC headers are missing"
[ -d "$TARGET_RUNTIME" ] || die "Win32 runtime directory is missing"

for runtime_file in fbrt0.o fbrt1.o fbrt2.o libfb.a libfbmt.a \
	libfbgfx.a libfbgfxmt.a libsfx.a libsfxmt.a libffi.a; do
	[ -s "$TARGET_RUNTIME/$runtime_file" ] ||
		die "Win32 runtime is missing $runtime_file"
done

STAGE_ROOT="$BUILDROOT/package-stage"
PKGROOT="$STAGE_ROOT/package-root"
SDKROOT="$PKGROOT/usr/lib/freebasic-win32"
DOCROOT="$PKGROOT/usr/share/doc/freebasic-win32"
rm -rf "$STAGE_ROOT"
mkdir -p "$PKGROOT/DEBIAN" "$PKGROOT/usr/bin" "$SDKROOT/bin" \
	"$SDKROOT/include/freebasic" "$SDKROOT/lib/freebasic" "$DOCROOT"

cp "$WRAPPER" "$SDKROOT/bin/fbc-win32"
chmod 755 "$SDKROOT/bin/fbc-win32"
cp -a "$WORK_ROOT/inc/." "$SDKROOT/include/freebasic/"
cp -a "$TARGET_RUNTIME" "$SDKROOT/lib/freebasic/win32-x86"
ln -s ../lib/freebasic-win32/bin/fbc-win32 "$PKGROOT/usr/bin/fbc-win32"

if [ -f "$WORK_ROOT/debian/copyright" ]; then
	cp "$WORK_ROOT/debian/copyright" "$DOCROOT/copyright"
fi
cat > "$DOCROOT/README.Debian" <<EOF
freebasic-win32
===============

This package provides a Linux-hosted 32-bit Windows compiler driver and the
complete FreeBASIC Win32 runtime, gfxlib2, sfxlib, and libffi libraries.

Example:

  fbc-win32 hello.bas -x hello.exe

The maintained Ubuntu i686 MinGW-w64 packages provide the C compiler, linker,
Windows headers, and import libraries. Wine is optional and is not required
to compile programs.
EOF

msg "compiling a representative program with the staged Win32 SDK"
cp "$COMPILER" "$SDKROOT/bin/fbc"
chmod 755 "$SDKROOT/bin/fbc"
SMOKE_EXE="$BUILDROOT/package-win32-basic-file.exe"
"$SDKROOT/bin/fbc-win32" -O 0 -exx \
	"$WORK_ROOT/tests/wince/basic_file.bas" -x "$SMOKE_EXE"
i686-w64-mingw32-objdump -f "$SMOKE_EXE" |
	grep -F 'file format pei-i386' >/dev/null ||
	die "package smoke did not produce a 32-bit Windows PE executable"
rm "$SDKROOT/bin/fbc"
ln -s /usr/bin/fbc "$SDKROOT/bin/fbc"

INSTALLED_SIZE="$(du -sk "$PKGROOT/usr" | awk '{ print $1 }')"
cat > "$PKGROOT/DEBIAN/control" <<EOF
Package: freebasic-win32
Version: $DEB_VERSION
Section: devel
Priority: optional
Architecture: amd64
Maintainer: SJ_Zero <sj@fbxl.net>
Depends: freebasic (>= $VERSION), gcc-mingw-w64-i686, binutils-mingw-w64-i686
Installed-Size: $INSTALLED_SIZE
Homepage: https://freebasic.net/
Description: FreeBASIC cross-compiler support for 32-bit Windows
 This package provides fbc-win32, the complete 32-bit Windows FreeBASIC
 runtime, target headers, gfxlib2, sfxlib, and libffi. It uses Ubuntu's
 maintained i686 MinGW-w64 toolchain to produce Windows PE executables.
EOF

find "$PKGROOT" -type d -exec chmod 755 {} +
DEB_PATH="$PACKAGE_OUTDIR/freebasic-win32_${DEB_VERSION}_amd64.deb"
rm -f "$DEB_PATH" "$DEB_PATH.sha256"
fakeroot dpkg-deb --build "$PKGROOT" "$DEB_PATH"
(
	cd "$PACKAGE_OUTDIR"
	sha256sum "$(basename "$DEB_PATH")" > "$(basename "$DEB_PATH").sha256"
)

msg "Win32 Debian/Ubuntu package workflow completed"
echo "    $DEB_PATH"
echo "    $DEB_PATH.sha256"

cleanup

# end of build_scripts/debianubuntu-build-freebasic-win32.sh
