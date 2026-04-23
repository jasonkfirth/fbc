#!/usr/bin/env bash

set -euo pipefail

trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }
usage() {
	cat <<'EOF'
Usage: ./build_scripts/msdos-build-freebasic.sh [options]

Options:
  --skip-deps            Skip pacman dependency installation
  --skip-source-copy     Reuse existing host/DOS worktrees
  --skip-toolchain       Reuse existing host-side DJGPP cross toolchain
  --skip-djgpp-payload   Reuse existing DOS-side DJGPP payload cache
  --skip-host-bootstrap  Reuse existing host bootstrap compiler
  --skip-dos-build       Reuse existing DOS compiler/runtime build
  --skip-install         Reuse existing staged package tree
  --skip-dosbox          Skip DOSBox smoke test
  --skip-package         Skip final zip creation
  --keep-buildroot       Keep existing buildroot instead of deleting it first
  --dosbox-only          Re-run only the DOSBox smoke test against existing stage
  --package-only         Re-run only the zip packaging against existing stage
  --help                 Show this help
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
# Ensure MSYS2 / MinGW
##############################################################################

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*) die "this script must be run inside MSYS2" ;;
esac

##############################################################################
# Configuration
##############################################################################

TARGET_TRIPLET="${TARGET_TRIPLET:-i586-pc-msdosdjgpp}"
PREFIX="${PREFIX:-/fb}"

BUILDROOT="${BUILDROOT:-$ROOT/.build-msdos}"
HOST_WORKTREE="${HOST_WORKTREE:-$BUILDROOT/host-worktree}"
DOS_WORKTREE="${DOS_WORKTREE:-$BUILDROOT/dos-worktree}"
DOWNLOADS="${DOWNLOADS:-$BUILDROOT/downloads}"
TOOLROOT="${TOOLROOT:-$BUILDROOT/toolchains}"
STAGE="${STAGE:-$BUILDROOT/stage}"
DOSBOX_ROOT="${DOSBOX_ROOT:-$BUILDROOT/dosbox}"
OUT="${OUT:-$ROOT/out}"

DJGPP_CROSS_VERSION="${DJGPP_CROSS_VERSION:-v3.4}"
DJGPP_CROSS_ASSET="${DJGPP_CROSS_ASSET:-djgpp-mingw-gcc1220-standalone.zip}"
DJGPP_CROSS_URL="${DJGPP_CROSS_URL:-https://github.com/andrewwutw/build-djgpp/releases/download/${DJGPP_CROSS_VERSION}/${DJGPP_CROSS_ASSET}}"
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
PKGFILE="${OUT}/${PKGNAME}.zip"
HOST_FBC="$BUILDROOT/fbc-host.exe"
DJGPP_DOS_CACHE="${DJGPP_DOS_CACHE:-$BUILDROOT/djgpp-dos}"

##############################################################################
# Dependency install
##############################################################################

if [[ -d /mingw64/bin ]] && [[ ":$PATH:" != *":/mingw64/bin:"* ]]; then
	export PATH="/mingw64/bin:$PATH"
fi

if [[ -d /usr/bin ]] && [[ ":$PATH:" != *":/usr/bin:"* ]]; then
	export PATH="/usr/bin:$PATH"
fi

if [ "$DO_DEPS" = "1" ]; then
	echo "==> updating package database"
	run pacman -Sy --noconfirm

	echo "==> installing build dependencies"
	run pacman -S --needed --noconfirm \
		base-devel \
		curl \
		dos2unix \
		mingw-w64-x86_64-binutils \
		mingw-w64-x86_64-dosbox-staging \
		mingw-w64-x86_64-gcc \
		mingw-w64-x86_64-libffi \
		rsync \
		unzip \
		zip
fi

##############################################################################
# Fresh work area
##############################################################################

if [ "$KEEP_BUILDROOT" != "1" ]; then
	rm -rf "$BUILDROOT"
fi

mkdir -p "$BUILDROOT" "$DOWNLOADS" "$TOOLROOT" "$OUT"

if [ "$DO_STAGE_INSTALL" = "1" ]; then
	rm -rf "$STAGE"
	mkdir -p "$STAGE"
fi

if [ "$DO_DOSBOX_TEST" = "1" ]; then
	rm -rf "$DOSBOX_ROOT"
	mkdir -p "$DOSBOX_ROOT"
fi

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
		echo "==> copying source tree ($label)"
		copy_source_tree "$dst"
	else
		[ -d "$dst" ] || die "missing existing worktree: $dst"
		echo "==> reusing existing worktree ($label): $dst"
	fi
}

if [ "$DO_HOST_BOOTSTRAP" = "1" ]; then
	prepare_worktree "host bootstrap" "$HOST_WORKTREE"
fi

if [ "$DO_DOS_BUILD" = "1" ]; then
	prepare_worktree "DOS cross build" "$DOS_WORKTREE"
fi

##############################################################################
# Acquire host-side DJGPP cross toolchain
##############################################################################

CROSS_ARCHIVE="$DOWNLOADS/$DJGPP_CROSS_ASSET"
CROSS_ROOT="$TOOLROOT/djgpp-cross"
find_cross_bindir() {
	[ -d "$CROSS_ROOT" ] || return 0
	find "$CROSS_ROOT" -type f \( -name "${TARGET_TRIPLET}-gcc" -o -name "${TARGET_TRIPLET}-gcc.exe" \) | head -n1
}

CROSS_GCC_PATH="$(find_cross_bindir)"
if [ -n "$CROSS_GCC_PATH" ]; then
	CROSS_BINDIR="$(dirname "$CROSS_GCC_PATH")"
	export PATH="$CROSS_BINDIR:$PATH"
fi

if ! have "${TARGET_TRIPLET}-gcc"; then
	[ "$DO_CROSS_TOOLCHAIN" = "1" ] || die "${TARGET_TRIPLET}-gcc not found (rerun without --skip-toolchain)"
	if [ ! -f "$CROSS_ARCHIVE" ]; then
		echo "==> downloading host DJGPP cross toolchain"
		run curl -L --retry 3 --fail -o "$CROSS_ARCHIVE" "$DJGPP_CROSS_URL"
	fi

	echo "==> extracting host DJGPP cross toolchain"
	rm -rf "$CROSS_ROOT"
	mkdir -p "$CROSS_ROOT"
	run unzip -q -o "$CROSS_ARCHIVE" -d "$CROSS_ROOT"

	CROSS_GCC_PATH="$(find_cross_bindir)"
	[ -n "$CROSS_GCC_PATH" ] || die "cross compiler not found after extracting $DJGPP_CROSS_ASSET"
	CROSS_BINDIR="$(dirname "$CROSS_GCC_PATH")"
	export PATH="$CROSS_BINDIR:$PATH"
fi

have "${TARGET_TRIPLET}-gcc" || die "${TARGET_TRIPLET}-gcc not found"
have "${TARGET_TRIPLET}-g++" || die "${TARGET_TRIPLET}-g++ not found"
have "${TARGET_TRIPLET}-ar" || die "${TARGET_TRIPLET}-ar not found"
have "${TARGET_TRIPLET}-as" || die "${TARGET_TRIPLET}-as not found"

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
			echo "==> downloading $(basename "$rel")"
			run curl -L --retry 3 --fail -o "$zipfile" "$DJGPP_BASE_URL/$rel"
		fi
		echo "==> extracting $(basename "$rel")"
		run unzip -q -o "$zipfile" -d "$DJGPP_DOS_CACHE"
	done
else
	if [ ! -d "$DJGPP_DOS_CACHE/bin" ] && [ -d "$STAGE/djgpp/bin" ]; then
		echo "==> seeding DOS-side DJGPP payload cache from staged tree"
		mkdir -p "$DJGPP_DOS_CACHE"
		run rsync -a "$STAGE/djgpp"/ "$DJGPP_DOS_CACHE"/
	fi
	[ -d "$DJGPP_DOS_CACHE/bin" ] || die "missing DOS-side DJGPP payload cache: $DJGPP_DOS_CACHE"
fi

##############################################################################
# Host bootstrap
##############################################################################

if [ "$DO_HOST_BOOTSTRAP" = "1" ]; then
	cd "$HOST_WORKTREE"

	echo "==> cleaning"
	run make clean

	echo "==> building host bootstrap compiler"
	run make bootstrap-minimal

	[ -x "$HOST_WORKTREE/bin/fbc.exe" ] || die "host bootstrap compiler missing"
	run cp "$HOST_WORKTREE/bin/fbc.exe" "$HOST_FBC"
else
	[ -x "$HOST_FBC" ] || die "missing host bootstrap compiler: $HOST_FBC"
fi

##############################################################################
# Cross-build compiler + runtime
##############################################################################

if [ "$DO_DOS_BUILD" = "1" ]; then
	cd "$DOS_WORKTREE"

	echo "==> cleaning DOS cross-build tree"
	run make clean

	echo "==> building DOS compiler"
	run make compiler runtime \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		BOOT_FBC="$HOST_FBC"
else
	[ -x "$DOS_WORKTREE/bin/fbc.exe" ] || die "missing DOS compiler: $DOS_WORKTREE/bin/fbc.exe"
fi

##############################################################################
# Install and package
##############################################################################

if [ "$DO_STAGE_INSTALL" = "1" ]; then
	cd "$DOS_WORKTREE"

	echo "==> staging DOS package"
	rm -rf "$STAGE/fb" "$STAGE/djgpp"
	rm -f "$BUILDROOT/stage.install_manifest"
	run make install \
		DESTDIR="$STAGE" \
		prefix="$PREFIX" \
		TARGET_TRIPLET="$TARGET_TRIPLET"

	run rsync -a "$DJGPP_DOS_CACHE"/ "$STAGE/djgpp"/

	cat > "$STAGE/fbdos.bat" <<'EOF'
@echo off
set DJGPP=C:\DJGPP\DJGPP.ENV
set PATH=C:\FB;C:\DJGPP\BIN;%PATH%
echo FreeBASIC DOS environment ready.
EOF
else
	[ -x "$STAGE/fb/fbc.exe" ] || die "missing staged DOS compiler: $STAGE/fb/fbc.exe"
fi

##############################################################################
# DOSBox smoke test
##############################################################################

run_dosbox_test() {
	local dosbox_bin
	local test_root
	local mount_root
	local autoexec_bat

	dosbox_bin="$(command -v dosbox || true)"
	if [ -z "$dosbox_bin" ]; then
		dosbox_bin="$(command -v dosbox.exe || true)"
	fi
	[ -n "$dosbox_bin" ] || return 0

	echo "==> running DOSBox smoke test"

	test_root="$DOSBOX_ROOT/root"
	mount_root="$test_root"
	if have cygpath; then
		mount_root="$(cygpath -w "$test_root")"
	fi
	mkdir -p "$test_root"
	run rsync -a "$STAGE"/ "$test_root"/

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

	timeout "$DOSBOX_TIMEOUT" "$dosbox_bin" \
		-set "cpu cputype=pentium_pro" \
		-exit \
		-c "mount c \"$mount_root\"" \
		-c "c:" \
		-c "call fbtest.bat" \
		-c "exit"

	[ -f "$test_root/trace.log" ] || die "DOSBox smoke test did not produce trace.log"
	[ -f "$test_root/result.txt" ] || die "DOSBox smoke test did not produce result.txt"
	grep -q "FreeBASIC DOS OK" "$test_root/result.txt" || die "DOSBox smoke test produced unexpected output"
}

if [ "$DO_DOSBOX_TEST" = "1" ]; then
	run_dosbox_test
fi

if [ "$DO_PACKAGE" = "1" ]; then
	echo "==> creating zip package"
	rm -f "$PKGFILE"
	( cd "$STAGE" && zip -r "$PKGFILE" . )

	[ -f "$PKGFILE" ] || die "package creation failed"

	echo
	echo "==> package created: $PKGFILE"
fi
