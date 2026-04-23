#!/usr/bin/env bash

set -euo pipefail

trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }
msg() { echo ""; echo "==> $1"; }

run_root() {
	if [ "$(id -u)" -eq 0 ]; then
		run "$@"
	elif have sudo; then
		run sudo "$@"
	else
		die "this step requires administrator privileges; rerun as root or install sudo"
	fi
}

usage() {
	cat <<'EOF'
Usage: ./build_scripts/msdos-build-freebasic.sh [options]

Options:
  --skip-deps            Skip host dependency installation
  --skip-source-copy     Reuse existing host/DOS worktrees
  --skip-toolchain       Reuse existing host-side DJGPP cross toolchain
  --skip-djgpp-payload   Reuse existing DOS-side DJGPP payload cache
  --skip-host-bootstrap  Reuse existing host-side FreeBASIC compiler
  --skip-dos-build       Reuse existing DOS compiler/runtime build
  --skip-install         Reuse existing distribution tree
  --skip-dosbox          Skip DOSBox smoke test
  --skip-package         Skip final zip creation
  --keep-buildroot       Keep existing buildroot instead of deleting it first
  --dosbox-only          Re-run only the DOSBox smoke test against the existing distribution tree
  --package-only         Re-run only the zip packaging against the existing distribution tree
  --help                 Show this help

Environment:
  BUILDROOT              Temporary build root (default: <repo>/.build-msdos/<host-kind>)
  OUT                    Output directory (default: <repo>/out/msdos)
  PREFIX                 Install prefix inside the DOS package (default: /fb)
  MAKE_JOBS              Parallel make job count (default: auto-detect host cores)
  TARGET_TRIPLET         DJGPP target triplet (default: i586-pc-msdosdjgpp)
  DJGPP_CROSS_VERSION    MSYS2 prebuilt toolchain release tag (default: v3.4)
  DJGPP_CROSS_ASSET      MSYS2 prebuilt toolchain asset name
  DJGPP_CROSS_URL        MSYS2 prebuilt toolchain archive URL
  DJGPP_BUILD_REPO_URL   Linux build-djgpp repository URL
  DJGPP_BUILD_REPO_REF   Linux build-djgpp branch or tag (default: master)
  DJGPP_BUILD_GCC_VER    Linux build-djgpp GCC version (default: 12.2.0)
  DJGPP_BASE_URL         DOS-side DJGPP archive base URL
  DOSBOX_TIMEOUT         DOSBox timeout in seconds (default: 60)
  SKIP_DOSBOX            Set to 1 to skip DOSBox even without --skip-dosbox
EOF
}

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
# Host detection
##############################################################################

HOST_UNAME="$(uname -s)"
HOST_KIND=""

case "$HOST_UNAME" in
	MINGW*|MSYS*)
		HOST_KIND="msys2"
		;;
	Linux)
		HOST_KIND="linux"
		;;
	*)
		die "unsupported host environment: $HOST_UNAME (expected Linux or MSYS2)"
		;;
esac

##############################################################################
# Configuration
##############################################################################

TARGET_TRIPLET="${TARGET_TRIPLET:-i586-pc-msdosdjgpp}"
PREFIX="${PREFIX:-/fb}"

BUILDROOT="${BUILDROOT:-$ROOT/.build-msdos/$HOST_KIND}"
HOST_WORKTREE="${HOST_WORKTREE:-$BUILDROOT/host-worktree}"
DOS_WORKTREE="${DOS_WORKTREE:-$BUILDROOT/dos-worktree}"
DOWNLOADS="${DOWNLOADS:-$BUILDROOT/downloads}"
TOOLROOT="${TOOLROOT:-$BUILDROOT/toolchains}"
DOSBOX_ROOT="${DOSBOX_ROOT:-$BUILDROOT/dosbox}"
OUT="${OUT:-$ROOT/out/msdos}"

DISTROOT=""

DJGPP_CROSS_VERSION="${DJGPP_CROSS_VERSION:-v3.4}"
DJGPP_CROSS_ASSET="${DJGPP_CROSS_ASSET:-djgpp-mingw-gcc1220-standalone.zip}"
DJGPP_CROSS_URL="${DJGPP_CROSS_URL:-https://github.com/andrewwutw/build-djgpp/releases/download/${DJGPP_CROSS_VERSION}/${DJGPP_CROSS_ASSET}}"
DJGPP_BUILD_REPO_URL="${DJGPP_BUILD_REPO_URL:-https://github.com/andrewwutw/build-djgpp.git}"
DJGPP_BUILD_REPO_REF="${DJGPP_BUILD_REPO_REF:-master}"
DJGPP_BUILD_GCC_VER="${DJGPP_BUILD_GCC_VER:-12.2.0}"
DJGPP_BASE_URL="${DJGPP_BASE_URL:-https://www.delorie.com/pub/djgpp/current}"

DOSBOX_TIMEOUT="${DOSBOX_TIMEOUT:-60}"
KEEP_BUILDROOT="${KEEP_BUILDROOT:-0}"
SKIP_DOSBOX="${SKIP_DOSBOX:-0}"

DO_DEPS=1
DO_SOURCE_COPY=1
DO_CROSS_TOOLCHAIN=1
DO_DJGPP_PAYLOAD=1
DO_HOST_BOOTSTRAP=1
DO_DOS_BUILD=1
DO_STAGE_INSTALL=1
DO_DOSBOX_TEST=1
DO_PACKAGE=1

for arg in "$@"; do
	case "$arg" in
		--skip-deps) DO_DEPS=0 ;;
		--skip-source-copy) DO_SOURCE_COPY=0 ;;
		--skip-toolchain) DO_CROSS_TOOLCHAIN=0 ;;
		--skip-djgpp-payload) DO_DJGPP_PAYLOAD=0 ;;
		--skip-host-bootstrap) DO_HOST_BOOTSTRAP=0 ;;
		--skip-dos-build) DO_DOS_BUILD=0 ;;
		--skip-install) DO_STAGE_INSTALL=0 ;;
		--skip-dosbox) DO_DOSBOX_TEST=0 ;;
		--skip-package) DO_PACKAGE=0 ;;
		--keep-buildroot) KEEP_BUILDROOT=1 ;;
		--dosbox-only)
			KEEP_BUILDROOT=1
			DO_DEPS=0
			DO_SOURCE_COPY=0
			DO_CROSS_TOOLCHAIN=0
			DO_DJGPP_PAYLOAD=0
			DO_HOST_BOOTSTRAP=0
			DO_DOS_BUILD=0
			DO_STAGE_INSTALL=0
			DO_DOSBOX_TEST=1
			DO_PACKAGE=0
			;;
		--package-only)
			KEEP_BUILDROOT=1
			DO_DEPS=0
			DO_SOURCE_COPY=0
			DO_CROSS_TOOLCHAIN=0
			DO_DJGPP_PAYLOAD=0
			DO_HOST_BOOTSTRAP=0
			DO_DOS_BUILD=0
			DO_STAGE_INSTALL=0
			DO_DOSBOX_TEST=0
			DO_PACKAGE=1
			;;
		--help)
			usage
			exit 0
			;;
		*)
			die "unknown option: $arg"
			;;
	esac
done

if [ "$SKIP_DOSBOX" = "1" ]; then
	DO_DOSBOX_TEST=0
fi

##############################################################################
# Version metadata
##############################################################################

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

PKGNAME="FreeBASIC-${FBVERSION}.${REV}-dos"
DISTROOT="${OUT}/${PKGNAME}"
PKGFILE="${OUT}/${PKGNAME}.zip"
DJGPP_DOS_CACHE="${DJGPP_DOS_CACHE:-$BUILDROOT/djgpp-dos}"

##############################################################################
# Helpers
##############################################################################

copy_source_tree() {
	local dst="$1"
	run rsync -a --delete --delete-excluded --prune-empty-dirs \
		--exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
		./ "$dst"/
}

prepare_worktree() {
	local label="$1"
	local dst="$2"

	if [ "$DO_SOURCE_COPY" = "1" ]; then
		rm -rf "$dst"
		mkdir -p "$dst"
		msg "copying source tree ($label)"
		copy_source_tree "$dst"
	else
		[ -d "$dst" ] || die "missing existing worktree: $dst"
		msg "reusing existing worktree ($label): $dst"
	fi
}

find_tree_fbc() {
	local base="$1"
	local candidate

	for candidate in \
		"$base/bin/fbc" \
		"$base/bin/fbc.exe" \
		"$base/fbc" \
		"$base/fbc.exe"
	do
		if [ -x "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

detect_fbc() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -f "$candidate" ] && "$candidate" -version >/dev/null 2>&1; then
			echo "$candidate"
			return 0
		fi
	done

	if have fbc && fbc -version >/dev/null 2>&1; then
		command -v fbc
		return 0
	fi

	return 1
}

find_cross_bindir() {
	[ -d "$CROSS_ROOT" ] || return 0
	find "$CROSS_ROOT" -type f \( -name "${TARGET_TRIPLET}-gcc" -o -name "${TARGET_TRIPLET}-gcc.exe" \) | head -n1
}

configure_cross_toolchain_env() {
	local cross_gcc_path
	local cross_bindir

	cross_gcc_path="$(find_cross_bindir)"
	if [ -n "$cross_gcc_path" ]; then
		cross_bindir="$(dirname "$cross_gcc_path")"
		export PATH="$cross_bindir:$PATH"
	fi
}

detect_make_jobs() {
	local jobs="${MAKE_JOBS:-}"

	if [ -n "$jobs" ]; then
		case "$jobs" in
			*[!0-9]*|'') die "MAKE_JOBS must be a positive integer" ;;
			0) die "MAKE_JOBS must be greater than zero" ;;
		esac
		echo "$jobs"
		return 0
	fi

	if have nproc; then
		jobs="$(nproc 2>/dev/null || true)"
	elif have getconf; then
		jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)"
	fi

	case "$jobs" in
		''|*[!0-9]*|0) jobs=1 ;;
	esac

	echo "$jobs"
}

configure_make_parallelism() {
	local jobs

	jobs="$(detect_make_jobs)"

	if [[ "${MAKEFLAGS:-}" == *"-j"* ]]; then
		msg "reusing existing make parallelism from MAKEFLAGS: ${MAKEFLAGS}"
		return 0
	fi

	export MAKE_JOBS="$jobs"
	export MAKEFLAGS="${MAKEFLAGS:-} -j${MAKE_JOBS}"
	msg "using make parallelism: -j${MAKE_JOBS}"
}

prepare_dos_runtime_layout() {
	local root="$1"
	local compat_libdir="$root/lib/dos"
	local legacy_libdir="$root/lib/freebas/dos"
	local host_libdir="$root/lib/freebasic/dos"
	local djgpp_ldscript="$root/djgpp/lib/ldscripts/i386go32.x"

	[ -d "$compat_libdir" ] || return 0

	mkdir -p "$(dirname "$legacy_libdir")"
	mkdir -p "$(dirname "$host_libdir")"
	rm -rf "$legacy_libdir"
	rm -rf "$host_libdir"
	cp -a "$compat_libdir" "$legacy_libdir"
	cp -a "$compat_libdir" "$host_libdir"

	if [ -f "$djgpp_ldscript" ]; then
		cp -f "$djgpp_ldscript" "$legacy_libdir/i386go32.x"
		cp -f "$djgpp_ldscript" "$compat_libdir/i386go32.x"
		cp -f "$djgpp_ldscript" "$host_libdir/i386go32.x"
	fi
}

cleanup_successful_buildroot() {
	local buildroot_parent

	[ "$KEEP_BUILDROOT" = "1" ] && return 0

	msg "cleaning successful build artifacts"
	rm -rf "$BUILDROOT"

	buildroot_parent="$(dirname "$BUILDROOT")"
	if [ "$buildroot_parent" = "$ROOT/.build-msdos" ]; then
		rmdir "$buildroot_parent" 2>/dev/null || true
	fi
}

require_cross_toolchain() {
	have "${TARGET_TRIPLET}-gcc" || die "${TARGET_TRIPLET}-gcc not found"
	have "${TARGET_TRIPLET}-g++" || die "${TARGET_TRIPLET}-g++ not found"
	have "${TARGET_TRIPLET}-ar" || die "${TARGET_TRIPLET}-ar not found"
	have "${TARGET_TRIPLET}-as" || die "${TARGET_TRIPLET}-as not found"
}

patch_linux_build_djgpp() {
	local version_script="$BUILD_DJGPP_ROOT/script/$DJGPP_BUILD_GCC_VER"
	local djlsr_patch="$BUILD_DJGPP_ROOT/patch/patch-djlsr205.txt"

	[ -f "$version_script" ] || die "missing build-djgpp version script: $version_script"
	[ -f "$djlsr_patch" ] || die "missing build-djgpp djlsr patch: $djlsr_patch"

	msg "patching Linux build-djgpp bootstrap for modern GCC"
	run sed -i \
		's@env -u CFLAGS ./configure --enable-fat --prefix=$BUILDDIR/tmpinst --enable-static --disable-shared || exit 1@env -u CFLAGS CC="${CC} -std=gnu17" ./configure --enable-fat --prefix=$BUILDDIR/tmpinst --enable-static --disable-shared || exit 1@' \
		"$version_script"

	msg "patching Linux build-djgpp djlsr sources for modern bison/gcc"
	if ! grep -Fq 'void sortsyms(int (*sortf)(void const *,void const *));' "$djlsr_patch"; then
		cat >> "$djlsr_patch" <<'EOF'
diff -ur djlsr205-orig/src/djasm/djasm.y djlsr205/src/djasm/djasm.y
--- djlsr205-orig/src/djasm/djasm.y	2017-04-29 14:32:47.000000000 +0800
+++ djlsr205/src/djasm/djasm.y	2026-04-23 00:00:00.000000000 +0000
@@ -179,7 +179,7 @@
 void modrm(int mod, int reg, int rm);
 void reg(int reg);
 void addr32(int sib);
-void sortsyms();
+void sortsyms(int (*sortf)(void const *,void const *));
 
 int istemp(char *symname, char which);
 int islocal(char *symname);
EOF
	fi

	msg "patching Linux build-djgpp djlsr makefiles for parallel make"
	if ! grep -Fq 'djlsr205/src/makefile' "$djlsr_patch"; then
		cat >> "$djlsr_patch" <<'EOF'
diff -ur djlsr205-orig/src/makefile djlsr205/src/makefile
--- djlsr205-orig/src/makefile	2017-04-29 14:32:47.000000000 +0800
+++ djlsr205/src/makefile	2026-04-23 00:00:00.000000000 +0000
@@ -25,6 +25,8 @@
 misc.exe : misc.c
 	gcc -O2 -Wall misc.c -o misc.exe
 
+$(DIRS) : misc.exe
+
 $(DIRS) :
 	./misc.exe mkdir $@
 
EOF
	fi
}

##############################################################################
# Dependency installation
##############################################################################

install_linux_dependencies() {
	msg "updating APT package database"
	run_root apt-get update

	msg "installing Linux build dependencies"
	run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
		bison \
		curl \
		dos2unix \
		dosbox \
		dosbox-x \
		flex \
		freebasic \
		g++ \
		g++-multilib \
		gcc \
		gcc-multilib \
		git \
		libc6-dev-i386 \
		libc6-dev-x32 \
		make \
		mtools \
		patch \
		rsync \
		texinfo \
		unzip \
		xz-utils \
		zip \
		zlib1g-dev
}

install_msys2_dependencies() {
	if [[ -d /mingw64/bin ]] && [[ ":$PATH:" != *":/mingw64/bin:"* ]]; then
		export PATH="/mingw64/bin:$PATH"
	fi

	if [[ -d /usr/bin ]] && [[ ":$PATH:" != *":/usr/bin:"* ]]; then
		export PATH="/usr/bin:$PATH"
	fi

	msg "updating package database"
	run pacman -Sy --noconfirm

	msg "installing MSYS2 build dependencies"
	run pacman -S --needed --noconfirm \
		base-devel \
		curl \
		dos2unix \
		git \
		mingw-w64-x86_64-binutils \
		mingw-w64-x86_64-dosbox-staging \
		mingw-w64-x86_64-gcc \
		mingw-w64-x86_64-libffi \
		rsync \
		unzip \
		zip
}

if [ "$DO_DEPS" = "1" ]; then
	case "$HOST_KIND" in
		linux) install_linux_dependencies ;;
		msys2) install_msys2_dependencies ;;
	esac
fi

##############################################################################
# Fresh work area
##############################################################################

if [ "$KEEP_BUILDROOT" != "1" ]; then
	rm -rf "$BUILDROOT"
fi

mkdir -p "$BUILDROOT" "$DOWNLOADS" "$TOOLROOT" "$OUT"

if [ "$DO_STAGE_INSTALL" = "1" ]; then
	rm -rf "$DISTROOT"
	mkdir -p "$DISTROOT"
fi

if [ "$DO_DOSBOX_TEST" = "1" ]; then
	rm -rf "$DOSBOX_ROOT"
	mkdir -p "$DOSBOX_ROOT"
fi

if [ "$DO_HOST_BOOTSTRAP" = "1" ]; then
	prepare_worktree "host bootstrap" "$HOST_WORKTREE"
fi

if [ "$DO_DOS_BUILD" = "1" ]; then
	prepare_worktree "DOS cross build" "$DOS_WORKTREE"
fi

##############################################################################
# Acquire host-side DJGPP cross toolchain
##############################################################################

CROSS_ROOT="$TOOLROOT/djgpp-cross"
CROSS_ARCHIVE="$DOWNLOADS/$DJGPP_CROSS_ASSET"
BUILD_DJGPP_ROOT="$BUILDROOT/build-djgpp"

configure_make_parallelism
configure_cross_toolchain_env

if ! have "${TARGET_TRIPLET}-gcc"; then
	[ "$DO_CROSS_TOOLCHAIN" = "1" ] || die "${TARGET_TRIPLET}-gcc not found (rerun without --skip-toolchain)"

	case "$HOST_KIND" in
		msys2)
			if [ ! -f "$CROSS_ARCHIVE" ]; then
				msg "downloading host DJGPP cross toolchain"
				run curl -L --retry 3 --fail -o "$CROSS_ARCHIVE" "$DJGPP_CROSS_URL"
			fi

			msg "extracting host DJGPP cross toolchain"
			rm -rf "$CROSS_ROOT"
			mkdir -p "$CROSS_ROOT"
			run unzip -q -o "$CROSS_ARCHIVE" -d "$CROSS_ROOT"
			;;
		linux)
			msg "cloning build-djgpp"
			rm -rf "$BUILD_DJGPP_ROOT" "$CROSS_ROOT"
			run git clone --depth 1 --branch "$DJGPP_BUILD_REPO_REF" "$DJGPP_BUILD_REPO_URL" "$BUILD_DJGPP_ROOT"
			patch_linux_build_djgpp

			msg "building Linux DJGPP cross toolchain"
			(
				cd "$BUILD_DJGPP_ROOT"
				run env DJGPP_PREFIX="$CROSS_ROOT" ./build-djgpp.sh "$DJGPP_BUILD_GCC_VER"
			)
			;;
	esac

	configure_cross_toolchain_env
fi

require_cross_toolchain

##############################################################################
# Acquire DOS-side DJGPP payload cache
##############################################################################

if [ "$DO_DJGPP_PAYLOAD" = "1" ]; then
	rm -rf "$DJGPP_DOS_CACHE"
	mkdir -p "$DJGPP_DOS_CACHE"

	for rel in \
		v2/djdev205.zip \
		v2gnu/bnu2351b.zip \
		v2gnu/gcc930b.zip \
		v2gnu/gpp930b.zip \
		v2gnu/mak44b.zip \
		v2misc/csdpmi7b.zip
	do
		zipfile="$DOWNLOADS/$(basename "$rel")"
		if [ ! -f "$zipfile" ]; then
			msg "downloading $(basename "$rel")"
			run curl -L --retry 3 --fail -o "$zipfile" "$DJGPP_BASE_URL/$rel"
		fi
		msg "extracting $(basename "$rel")"
		run unzip -q -o "$zipfile" -d "$DJGPP_DOS_CACHE"
	done
else
	if [ ! -d "$DJGPP_DOS_CACHE/bin" ] && [ -d "$DISTROOT/djgpp/bin" ]; then
		msg "seeding DOS-side DJGPP payload cache from distribution tree"
		mkdir -p "$DJGPP_DOS_CACHE"
		run rsync -a "$DISTROOT/djgpp"/ "$DJGPP_DOS_CACHE"/
	fi
	[ -d "$DJGPP_DOS_CACHE/bin" ] || die "missing DOS-side DJGPP payload cache: $DJGPP_DOS_CACHE"
fi

##############################################################################
# Host bootstrap compiler
##############################################################################

HOST_FBC=""

if [ "$DO_HOST_BOOTSTRAP" = "1" ]; then
	cd "$HOST_WORKTREE"

	HOST_FBC="$(detect_fbc \
		"$(find_tree_fbc "$HOST_WORKTREE" 2>/dev/null || true)" \
		"$(find_tree_fbc "$ROOT" 2>/dev/null || true)" \
		|| true)"

	[ -n "$HOST_FBC" ] || die "no runnable host FreeBASIC compiler found; rerun without --skip-deps or install fbc first"

	msg "emitting host bootstrap sources"
	run make bootstrap-emit

	msg "cleaning host tree"
	run make clean || true

	msg "building host bootstrap compiler"
	run make bootstrap-minimal

	HOST_FBC="$(find_tree_fbc "$HOST_WORKTREE" || true)"
	[ -n "$HOST_FBC" ] || die "host bootstrap compiler missing in $HOST_WORKTREE/bin"
else
	HOST_FBC="$(detect_fbc \
		"$(find_tree_fbc "$HOST_WORKTREE" 2>/dev/null || true)" \
		"$(find_tree_fbc "$ROOT" 2>/dev/null || true)" \
		|| true)"
	[ -n "$HOST_FBC" ] || die "missing host bootstrap compiler; rerun without --skip-host-bootstrap"
fi

##############################################################################
# Cross-build compiler + runtime
##############################################################################

if [ "$DO_DOS_BUILD" = "1" ]; then
	cd "$DOS_WORKTREE"

	export PATH="$(dirname "$HOST_FBC"):$PATH"

	msg "cleaning DOS cross-build tree"
	run make clean TARGET_TRIPLET="$TARGET_TRIPLET" || true

	msg "emitting DOS bootstrap sources"
	run make bootstrap-emit TARGET_TRIPLET="$TARGET_TRIPLET"

	msg "building DOS bootstrap compiler"
	run make bootstrap-minimal TARGET_TRIPLET="$TARGET_TRIPLET"

	msg "rebuilding DOS compiler and runtime with host compiler"
	run make compiler runtime \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		BUILD_FBC="$HOST_FBC"
else
	[ -x "$DOS_WORKTREE/bin/fbc.exe" ] || die "missing DOS compiler: $DOS_WORKTREE/bin/fbc.exe"
fi

##############################################################################
# Install and package
##############################################################################

if [ "$DO_STAGE_INSTALL" = "1" ]; then
	cd "$DOS_WORKTREE"

	msg "assembling DOS distribution tree"
	rm -rf "$DISTROOT/fb" "$DISTROOT/djgpp" "$DISTROOT/lib/freebas"
	rm -f "$DISTROOT/fbdos.bat"

	run make install \
		DESTDIR="$DISTROOT" \
		prefix="$PREFIX" \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		BUILD_FBC="$HOST_FBC"

	run rsync -a "$DJGPP_DOS_CACHE"/ "$DISTROOT/djgpp"/

	mkdir -p "$DISTROOT/fb"
	if [ -f "$DISTROOT/fbc.exe" ]; then
		mv -f "$DISTROOT/fbc.exe" "$DISTROOT/fb/fbc.exe"
	fi

	prepare_dos_runtime_layout "$DISTROOT"

	cat > "$DISTROOT/fbdos.bat" <<'EOF'
@echo off
set DJGPP=C:\DJGPP\DJGPP.ENV
set PATH=C:\FB;C:\DJGPP\BIN;%PATH%
echo FreeBASIC DOS environment ready.
EOF
else
	[ -x "$DISTROOT/fb/fbc.exe" ] || die "missing DOS distribution tree: $DISTROOT/fb/fbc.exe"
fi

##############################################################################
# DOSBox smoke test
##############################################################################

run_dosbox_test() {
	local dosbox_bin
	local dosbox_kind
	local test_root
	local mount_root
	local autoexec_bat
	local trace_log
	local result_txt
	local build_log
	local image_file
	local partition_start
	local partition_offset

	dosbox_kind=""
	dosbox_bin="$(command -v dosbox-x || true)"
	if [ -n "$dosbox_bin" ]; then
		dosbox_kind="dosbox-x"
	else
		dosbox_bin="$(command -v dosbox || true)"
		if [ -z "$dosbox_bin" ]; then
			dosbox_bin="$(command -v dosbox.exe || true)"
		fi
		if [ -n "$dosbox_bin" ]; then
			dosbox_kind="dosbox"
		fi
	fi
	[ -n "$dosbox_bin" ] || return 0

	msg "running DOSBox smoke test"

	test_root="$DOSBOX_ROOT/root"
	mount_root="$test_root"
	if have cygpath; then
		mount_root="$(cygpath -w "$test_root")"
	fi

	rm -rf "$test_root"
	mkdir -p "$test_root"
	run rsync -a "$DISTROOT"/ "$test_root"/
	prepare_dos_runtime_layout "$test_root"

	mkdir -p "$test_root/fb"
	if [ -f "$test_root/fbc.exe" ]; then
		mv -f "$test_root/fbc.exe" "$test_root/fb/fbc.exe"
	fi

	if [ "$dosbox_kind" = "dosbox-x" ] && [ "$HOST_KIND" = "linux" ] && have mcopy && have sfdisk; then
		cat > "$test_root/hello.bas" <<'EOF'
open "C:\RESULT.TXT" for output as #1
print #1, "FreeBASIC DOS OK"
close #1
EOF

		autoexec_bat="$test_root/fbtest.bat"
		cat > "$autoexec_bat" <<'EOF'
@echo off
echo begin>D:\TRACE.LOG
set DJGPP=C:\DJGPP\DJGPP.ENV
echo djgpp=%DJGPP%>>D:\TRACE.LOG
set PATH=C:\FB;C:\DJGPP\BIN;%PATH%
echo path=%PATH%>>D:\TRACE.LOG
if not exist C:\FB\FBC.EXE echo missing-fbc>>D:\TRACE.LOG
if not exist C:\DJGPP\BIN\GCC.EXE echo missing-gcc>>D:\TRACE.LOG
if not exist C:\DJGPP\DJGPP.ENV echo missing-env>>D:\TRACE.LOG
C:\DJGPP\BIN\CWSDPMI.EXE -p >>D:\TRACE.LOG
echo cwsdpmi-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD.LOG C:\FB\FBC.EXE -v C:\HELLO.BAS -x C:\HELLO.EXE
echo fbc-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
dir C:\HELLO.* >>D:\TRACE.LOG
if exist C:\HELLO.EXE (
	echo hello-exe-present>>D:\TRACE.LOG
	C:\DJGPP\BIN\REDIR.EXE -eo -o D:\RUN.LOG C:\HELLO.EXE
	echo hello-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
) else (
	echo hello-exe-missing>>D:\TRACE.LOG
)
if exist C:\RESULT.TXT copy C:\RESULT.TXT D:\RESULT.TXT >NUL
dir C:\RESULT.TXT >>D:\TRACE.LOG
EOF

		image_file="$DOSBOX_ROOT/smoke.img"
		rm -f "$image_file"
		run timeout "$DOSBOX_TIMEOUT" "$dosbox_bin" \
			-fastlaunch \
			-nogui \
			-nomenu \
			-exit \
			-set "cpu cputype=ppro_slow" \
			-c "imgmake \"$image_file\" -t hd -size 256 -fat 16" \
			-c "exit"

		partition_start="$(sfdisk -d "$image_file" | sed -n 's/.*start= *\([0-9][0-9]*\).*/\1/p' | head -n1)"
		[ -n "$partition_start" ] || die "could not determine DOSBox image partition start"
		partition_offset="$((partition_start * 512))"

		run env MTOOLS_SKIP_CHECK=1 mcopy -i "${image_file}@@${partition_offset}" -s "$test_root"/* ::

		run timeout "$DOSBOX_TIMEOUT" "$dosbox_bin" \
			-fastlaunch \
			-nogui \
			-nomenu \
			-exit \
			-set "cpu cputype=ppro_slow" \
			-c "mount d \"$mount_root\"" \
			-c "imgmount c \"$image_file\"" \
			-c "d:" \
			-c "FBTEST.BAT" \
			-c "exit"
	else
		cat > "$test_root/hello.bas" <<'EOF'
open "result.txt" for output as #1
print #1, "FreeBASIC DOS OK"
close #1
EOF

		autoexec_bat="$test_root/fbtest.bat"
		cat > "$autoexec_bat" <<'EOF'
@echo off
echo begin>trace.log
set DJGPP=C:\DJGPP\DJGPP.ENV
echo djgpp=%DJGPP%>>trace.log
set PATH=C:\FB;C:\DJGPP\BIN;%PATH%
echo path=%PATH%>>trace.log
if not exist C:\FB\FBC.EXE echo missing-fbc>>trace.log
if not exist C:\DJGPP\BIN\GCC.EXE echo missing-gcc>>trace.log
if not exist C:\DJGPP\DJGPP.ENV echo missing-env>>trace.log
C:\DJGPP\BIN\CWSDPMI.EXE -p >>trace.log
echo cwsdpmi-errorlevel=%ERRORLEVEL%>>trace.log
C:\FB\FBC.EXE hello.bas >>trace.log
echo fbc-errorlevel=%ERRORLEVEL%>>trace.log
dir hello.* >>trace.log
if exist hello.exe (
	echo hello-exe-present>>trace.log
	hello.exe >>trace.log
	echo hello-errorlevel=%ERRORLEVEL%>>trace.log
) else (
	echo hello-exe-missing>>trace.log
)
dir >>trace.log
EOF

		case "$dosbox_kind" in
			dosbox-x)
				run timeout "$DOSBOX_TIMEOUT" "$dosbox_bin" \
					-fastlaunch \
					-nogui \
					-nomenu \
					-exit \
					-set "cpu cputype=ppro_slow" \
					-c "mount c \"$mount_root\"" \
					-c "c:" \
					-c "FBTEST.BAT" \
					-c "exit"
				;;
			*)
				run timeout "$DOSBOX_TIMEOUT" "$dosbox_bin" \
					-set "cpu cputype=pentium_pro" \
					-exit \
					-c "mount c \"$mount_root\"" \
					-c "c:" \
					-c "FBTEST.BAT" \
					-c "exit"
				;;
		esac
	fi

	trace_log="$test_root/trace.log"
	if [ ! -f "$trace_log" ] && [ -f "$test_root/TRACE.LOG" ]; then
		trace_log="$test_root/TRACE.LOG"
	fi
	[ -f "$trace_log" ] || die "DOSBox smoke test did not produce trace.log"

	result_txt="$test_root/result.txt"
	if [ ! -f "$result_txt" ] && [ -f "$test_root/RESULT.TXT" ]; then
		result_txt="$test_root/RESULT.TXT"
	fi
	build_log="$test_root/BUILD.LOG"
	if [ ! -f "$build_log" ] && [ -f "$test_root/build.log" ]; then
		build_log="$test_root/build.log"
	fi

	if [ ! -f "$result_txt" ]; then
		if grep -q "requires at least a 686" "$trace_log"; then
			msg "DOSBox smoke test skipped: this DOSBox build cannot execute 686-class DOS binaries"
			return 0
		fi
		if [ -f "$build_log" ]; then
			sed -n '1,40p' "$build_log" >&2
		fi
		die "DOSBox smoke test did not produce result.txt"
	fi

	grep -q "FreeBASIC DOS OK" "$result_txt" || die "DOSBox smoke test produced unexpected output"
}

if [ "$DO_DOSBOX_TEST" = "1" ]; then
	run_dosbox_test
fi

##############################################################################
# Zip packaging
##############################################################################

if [ "$DO_PACKAGE" = "1" ]; then
	msg "creating zip package"
	rm -f "$PKGFILE"
	(
		cd "$OUT"
		zip -r "$(basename "$PKGFILE")" "$PKGNAME"
	)

	[ -f "$PKGFILE" ] || die "package creation failed"
fi

cleanup_successful_buildroot

echo
echo "==> distribution root: $DISTROOT"
if [ "$DO_PACKAGE" = "1" ]; then
	echo "==> package created: $PKGFILE"
fi
