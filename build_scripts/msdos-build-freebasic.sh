#!/usr/bin/env bash

set -euo pipefail

trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msdos-build-freebasic.sh
#
# Build a DOS FreeBASIC distribution from the current source tree.
# Produces a staged DOS package tree and, by default, a .zip archive under
# out/msdos.  The script also owns the DOSBox smoke-test step used to check the
# packaged compiler in a DOS environment.
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }
msg() { echo ""; echo "==> $1"; }

copy_tree() {
	local source="$1"
	local dest="$2"
	local source_win
	local dest_win
	local copy_status

	mkdir -p "$dest"
	if have robocopy.exe && have cygpath; then
		# Native copying avoids MSYS2 chmod calls on hosted NTFS volumes.
		source_win="$(cygpath -aw "$source")"
		dest_win="$(cygpath -aw "$dest")"
		if run env MSYS2_ARG_CONV_EXCL='*' robocopy.exe \
			"$source_win" "$dest_win" /E /COPY:DT /DCOPY:DT \
			/R:3 /W:1 /NFL /NDL /NJH /NJS /NP; then
			copy_status=0
		else
			copy_status=$?
		fi

		# Robocopy uses exit codes 0 through 7 for successful copy variants.
		if [ "$copy_status" -ge 8 ]; then
			die "robocopy failed with exit code $copy_status"
		fi
	elif have rsync; then
		run rsync -a --no-perms --no-owner --no-group \
			"$source/" "$dest/"
	else
		run cp -a "$source"/. "$dest/"
	fi
}

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
  HOST_FBC               Host compiler executable used to seed bootstrap
  HOST_FBC_ROOT          Directory containing host fbc/fbc64 (MSYS2 default: /c/freebasic when present)
  HOST_TRIPLET           Build-host triplet passed to cross make steps
  CURL_BIN               curl executable for host downloads (MSYS2 default: /usr/bin/curl)
  TARGET_TRIPLET         DJGPP target triplet (default: i586-pc-msdosdjgpp)
  DJGPP_CROSS_VERSION    MSYS2 prebuilt toolchain release tag (default: v3.4)
  DJGPP_CROSS_ASSET      MSYS2 prebuilt toolchain asset name
  DJGPP_CROSS_URL        MSYS2 prebuilt toolchain archive URL
  DJGPP_BUILD_REPO_URL   Linux build-djgpp repository URL
  DJGPP_BUILD_REPO_REF   Linux build-djgpp branch or tag (default: master)
  DJGPP_BUILD_GCC_VER    Linux build-djgpp GCC version (default: 12.2.0)
  DJGPP_BASE_URL         DOS-side DJGPP archive base URL
  DOSBOX_BIN             DOSBox-X executable used by the smoke test
  DOSBOX_X_RELEASE_TAG   DOSBox-X release tag for the portable Windows build
  DOSBOX_X_ASSET         DOSBox-X portable asset name
  DOSBOX_X_URL           DOSBox-X portable asset URL
  DOSBOX_X_ROOT          DOSBox-X download/cache directory
  DOSBOX_TIMEOUT         DOSBox timeout in seconds (default: 60)
  DOSBOX_IMAGE_SIZE      DOSBox smoke disk size in MiB (default: 512)
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
DOSBOX_X_RELEASE_TAG="${DOSBOX_X_RELEASE_TAG:-dosbox-x-v2026.05.02-osfree}"
DOSBOX_X_ASSET="${DOSBOX_X_ASSET:-dosbox-x-mingw64-${DOSBOX_X_RELEASE_TAG}-portable.zip}"
DOSBOX_X_URL="${DOSBOX_X_URL:-https://github.com/joncampbell123/dosbox-x/releases/download/${DOSBOX_X_RELEASE_TAG}/${DOSBOX_X_ASSET}}"
DOSBOX_X_ROOT="${DOSBOX_X_ROOT:-$ROOT/.build-msdos/dosbox-x}"

DOSBOX_TIMEOUT="${DOSBOX_TIMEOUT:-60}"
DOSBOX_IMAGE_SIZE="${DOSBOX_IMAGE_SIZE:-512}"
KEEP_BUILDROOT="${KEEP_BUILDROOT:-0}"
SKIP_DOSBOX="${SKIP_DOSBOX:-0}"

if [ -z "${CURL_BIN+x}" ]; then
	if [ "$HOST_KIND" = "msys2" ] && [ -x /usr/bin/curl ]; then
		CURL_BIN="/usr/bin/curl"
	else
		CURL_BIN="curl"
	fi
fi

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

case "$DOSBOX_IMAGE_SIZE" in
	*[!0-9]*|'') die "DOSBOX_IMAGE_SIZE must be a positive integer" ;;
	0) die "DOSBOX_IMAGE_SIZE must be greater than zero" ;;
esac

HOST_MAKE_TOOL_ARGS=()
if [ "$HOST_KIND" = "msys2" ]; then
	HOST_MAKE_TOOL_ARGS=(
		CC=gcc
		CXX=g++
		AR=ar
		RANLIB=ranlib
		AS=as
		LD=ld
		BOOTSTRAP_TERM_LIB=
	)
fi

CROSS_MAKE_TOOL_ARGS=()
MAKE_HOST_TRIPLET="${HOST_TRIPLET:-}"

##############################################################################
# Version metadata
##############################################################################

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"

[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

PKGNAME="FreeBASIC-${FBVERSION}-dos"
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
		"$base/bin/fbc64" \
		"$base/bin/fbc64.exe" \
		"$base/fbc64" \
		"$base/fbc64.exe" \
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

maybe_find_tree_fbc() {
	local base="${1:-}"

	[ -n "$base" ] || return 1

	if [ ! -d "$base" ] && have cygpath; then
		base="$(cygpath -u "$base" 2>/dev/null || printf '%s' "$base")"
	fi

	find_tree_fbc "$base"
}

detect_fbc() {
	local candidate
	local test_candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue

		test_candidate="$candidate"
		if [ ! -f "$test_candidate" ] && have cygpath; then
			test_candidate="$(cygpath -u "$candidate" 2>/dev/null || printf '%s' "$candidate")"
		fi

		if [ -f "$test_candidate" ] && "$test_candidate" -version >/dev/null 2>&1; then
			echo "$test_candidate"
			return 0
		fi
	done

	if have fbc && fbc -version >/dev/null 2>&1; then
		command -v fbc
		return 0
	fi

	return 1
}

detect_host_fbc() {
	detect_fbc \
		"${HOST_FBC:-}" \
		"$(maybe_find_tree_fbc "${HOST_FBC_ROOT:-}" 2>/dev/null || true)" \
		"$(maybe_find_tree_fbc /c/freebasic 2>/dev/null || true)" \
		"$(maybe_find_tree_fbc "$HOST_WORKTREE" 2>/dev/null || true)" \
		"$(maybe_find_tree_fbc "$ROOT" 2>/dev/null || true)"
}

download_file() {
	local dst="$1"
	local url="$2"
	local tmp="${dst}.tmp"

	rm -f "$tmp"
	run "$CURL_BIN" -L --retry 3 --fail -o "$tmp" "$url"
	mv -f "$tmp" "$dst"
}

normalize_program_path() {
	local path="$1"

	if [ "$HOST_KIND" = "msys2" ] && have cygpath; then
		case "$path" in
			[A-Za-z]:[\\/]*)
				cygpath -u "$path"
				return 0
				;;
		esac
	fi

	printf '%s\n' "$path"
}

find_portable_dosbox_x() {
	local root="$1"
	local candidate

	for candidate in \
		"$root/portable/mingw-build/mingw/dosbox-x.exe" \
		"$root/portable/mingw-build/mingw-sdl2/dosbox-x.exe" \
		"$root/portable/dosbox-x.exe"
	do
		if [ -x "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	return 1
}

prepare_portable_dosbox_x() {
	local archive
	local exe

	[ "$HOST_KIND" = "msys2" ] || return 1

	if exe="$(find_portable_dosbox_x "$DOSBOX_X_ROOT")"; then
		printf '%s\n' "$exe"
		return 0
	fi

	have unzip || die "unzip is required to unpack portable DOSBox-X"
	have "$CURL_BIN" || die "curl is required to download portable DOSBox-X"

	archive="$DOSBOX_X_ROOT/$DOSBOX_X_ASSET"
	mkdir -p "$DOSBOX_X_ROOT"

	if [ ! -f "$archive" ]; then
		msg "downloading portable DOSBox-X" >&2
		download_file "$archive" "$DOSBOX_X_URL" >&2
	fi

	rm -rf "$DOSBOX_X_ROOT/portable"
	mkdir -p "$DOSBOX_X_ROOT/portable"
	msg "unpacking portable DOSBox-X" >&2
	run unzip -q -o "$archive" -d "$DOSBOX_X_ROOT/portable" >&2

	exe="$(find_portable_dosbox_x "$DOSBOX_X_ROOT")" || die "portable DOSBox-X archive did not contain dosbox-x.exe"
	printf '%s\n' "$exe"
}

run_timeout_checked() {
	local status

	set +e
	run timeout "$@"
	status=$?
	set -e

	return "$status"
}

configure_msys2_path() {
	if [[ -d /mingw64/bin ]] && [[ ":$PATH:" != *":/mingw64/bin:"* ]]; then
		export PATH="/mingw64/bin:$PATH"
	fi

	if [[ -d /usr/bin ]] && [[ ":$PATH:" != *":/usr/bin:"* ]]; then
		export PATH="/usr/bin:$PATH"
	fi
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
		cross_bindir="$(cd "$(dirname "$cross_gcc_path")" && pwd -P)"
		export PATH="$cross_bindir:$PATH"
	fi
}

find_cross_tool() {
	command -v "${TARGET_TRIPLET}-$1"
}

host_path_for_tool() {
	local tool_path="$1"

	if [ "$HOST_KIND" = "msys2" ]; then
		cygpath -m "$tool_path"
	else
		printf '%s\n' "$tool_path"
	fi
}

detect_make_host_triplet() {
	local host_cc

	for host_cc in "${HOST_CC:-}" gcc cc; do
		[ -n "$host_cc" ] || continue
		if have "$host_cc"; then
			"$host_cc" -dumpmachine 2>/dev/null && return 0
		fi
	done

	return 1
}

configure_cross_make_tool_args() {
	local cross_cc
	local cross_cxx
	local cross_ar
	local cross_ranlib
	local cross_as
	local cross_ld

	if [ -z "$MAKE_HOST_TRIPLET" ]; then
		MAKE_HOST_TRIPLET="$(detect_make_host_triplet || true)"
	fi
	[ -n "$MAKE_HOST_TRIPLET" ] || die "could not determine host triplet for cross make"

	cross_cc="$(host_path_for_tool "$(find_cross_tool gcc)")"
	cross_cxx="$(host_path_for_tool "$(find_cross_tool g++)")"
	cross_ar="$(host_path_for_tool "$(find_cross_tool ar)")"
	cross_ranlib="$(host_path_for_tool "$(find_cross_tool ranlib)")"
	cross_as="$(host_path_for_tool "$(find_cross_tool as)")"
	cross_ld="$(host_path_for_tool "$(find_cross_tool ld)")"

	CROSS_MAKE_TOOL_ARGS=(
		CROSS_BUILD=yes
		HOST_TRIPLET="$MAKE_HOST_TRIPLET"
		CC="$cross_cc"
		CXX="$cross_cxx"
		AR="$cross_ar"
		RANLIB="$cross_ranlib"
		AS="$cross_as"
		LD="$cross_ld"
	)
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
	local djgpp_root="${2:-$root/djgpp}"
	local compat_libdir="$root/lib/dos"
	local legacy_libdir="$root/lib/freebas/dos"
	local host_libdir="$root/lib/freebasic/dos"
	local djgpp_ldscript="$djgpp_root/lib/ldscripts/i386go32.x"

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

stage_dos_compiler_tools() {
	local root="$1"
	local bindir="$root/bin/dos"
	local libdir="$root/lib/dos"
	local source
	local tool

	mkdir -p "$bindir" "$libdir"

	#
	# A standalone DOS fbc resolves its assembler and linker below bin/dos.
	# Falling back to PATH makes fbc run the DJGPP tool through COMMAND.COM;
	# that extra protected-mode process layer cannot reliably start binutils.
	# This also matches the layout of the upstream DOS distribution.
	#
	for tool in ar as dxe3gen gprof ld; do
		source="$DJGPP_DOS_CACHE/bin/$tool.exe"
		[ -f "$source" ] || die "missing DOS compiler tool: $source"
		cp -f "$source" "$bindir/$tool.exe"
	done

	if [ -f "$DJGPP_DOS_CACHE/bin/gdb.exe" ]; then
		cp -f "$DJGPP_DOS_CACHE/bin/gdb.exe" "$bindir/gdb.exe"
	fi

	#
	# Keep the DJGPP startup objects and default libraries beside the
	# FreeBASIC libraries.  The separate DJGPP tree is still included for
	# mixed C builds, but a normal fbc link must be self-contained.
	#
	for source in \
		"$DJGPP_DOS_CACHE/lib/"*.a \
		"$DJGPP_DOS_CACHE/lib/"*.o \
		"$DJGPP_DOS_CACHE/lib/"*.ld
	do
		[ -f "$source" ] || continue
		cp -f "$source" "$libdir/"
	done

	source="$(find "$DJGPP_DOS_CACHE/lib/gcc" -name libgcc.a -type f -print -quit)"
	[ -n "$source" ] || die "missing DOS libgcc.a"
	cp -f "$source" "$libdir/libgcc.a"
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
	# The dollar expressions below belong to the downloaded build script.
	# shellcheck disable=SC2016
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
	# $(DIRS) is literal GNU make syntax in the patch being inspected.
	# shellcheck disable=SC2016
	if ! grep -Fq 'config $(DIRS) : misc.exe' "$djlsr_patch"; then
		cat >> "$djlsr_patch" <<'EOF'
diff -ur djlsr205-orig/src/makefile djlsr205/src/makefile
--- djlsr205-orig/src/makefile	2017-04-29 14:32:47.000000000 +0800
+++ djlsr205/src/makefile	2026-04-23 00:00:00.000000000 +0000
@@ -25,6 +25,8 @@
 misc.exe : misc.c
 	gcc -O2 -Wall misc.c -o misc.exe
 
+config $(DIRS) : misc.exe
+
 $(DIRS) :
 	./misc.exe mkdir $@

EOF
	fi

	# $(DIRS) is literal GNU make syntax in the patch being generated.
	# shellcheck disable=SC2016
	if ! grep -Fq 'subs: config $(DIRS) makemake.exe' "$djlsr_patch"; then
		# shellcheck disable=SC2016
		{
			printf '%s\n' 'diff -ur djlsr205-orig/src/makefile djlsr205/src/makefile'
			printf '%s\n' '--- djlsr205-orig/src/makefile	2017-04-29 14:32:47.000000000 +0800'
			printf '%s\n' '+++ djlsr205/src/makefile	2026-04-23 00:00:00.000000000 +0000'
			printf '%s\n' '@@ -24 +24 @@'
			printf '%s\n' '-all : misc.exe config $(DIRS) makemake.exe subs ../lib/libg.a ../lib/libpc.a'
			printf '%s\n' '+all : misc.exe config $(DIRS) makemake.exe subs'
			printf '%s\n' '@@ -40 +40 @@'
			printf '%s\n' '-subs:'
			printf '%s\n' '+subs: config $(DIRS) makemake.exe'
		} >> "$djlsr_patch"
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
		dosbox-x \
		fdisk \
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
	local packages

	configure_msys2_path

	msg "updating package database"
	run bash "$ROOT/build_scripts/msys2-pacman-retry.sh" -Sy --noconfirm

	packages=(
		base-devel
		bison
		curl
		dos2unix
		flex
		git
		make
		mingw-w64-x86_64-binutils
		mingw-w64-x86_64-gcc
		mingw-w64-x86_64-libffi
		mingw-w64-x86_64-mtools
		mingw-w64-x86_64-pkgconf
		patch
		rsync
		unzip
		zip
	)

	msg "installing MSYS2 build dependencies"
	run bash "$ROOT/build_scripts/msys2-pacman-retry.sh" -S --needed --noconfirm "${packages[@]}"
}

if [ "$HOST_KIND" = "msys2" ]; then
	configure_msys2_path
fi

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
				download_file "$CROSS_ARCHIVE" "$DJGPP_CROSS_URL"
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

configure_cross_make_tool_args

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
			download_file "$zipfile" "$DJGPP_BASE_URL/$rel"
		fi
		msg "extracting $(basename "$rel")"
		run unzip -q -o "$zipfile" -d "$DJGPP_DOS_CACHE"
	done
else
	if [ ! -d "$DJGPP_DOS_CACHE/bin" ] && [ -d "$DISTROOT/djgpp/bin" ]; then
		msg "seeding DOS-side DJGPP payload cache from distribution tree"
		copy_tree "$DISTROOT/djgpp" "$DJGPP_DOS_CACHE"
	fi
	[ -d "$DJGPP_DOS_CACHE/bin" ] || die "missing DOS-side DJGPP payload cache: $DJGPP_DOS_CACHE"
fi

##############################################################################
# Host bootstrap compiler
##############################################################################

HOST_FBC="${HOST_FBC:-}"

if [ "$DO_HOST_BOOTSTRAP" = "1" ]; then
	cd "$HOST_WORKTREE"

	HOST_FBC="$(detect_host_fbc || true)"

	[ -n "$HOST_FBC" ] || die "no runnable host FreeBASIC compiler found; rerun without --skip-deps or install fbc first"

	msg "emitting host bootstrap sources"
	run make bootstrap-emit \
		"${HOST_MAKE_TOOL_ARGS[@]}" \
		BOOT_FBC="$HOST_FBC" \
		BUILD_FBC="$HOST_FBC"

	msg "cleaning host tree"
	run make clean \
		"${HOST_MAKE_TOOL_ARGS[@]}" \
		|| true

	msg "building host bootstrap compiler"
	run make bootstrap-minimal \
		"${HOST_MAKE_TOOL_ARGS[@]}" \
		BUILD_FBC="$HOST_FBC"

	HOST_FBC="$(find_tree_fbc "$HOST_WORKTREE" || true)"
	[ -n "$HOST_FBC" ] || die "host bootstrap compiler missing in $HOST_WORKTREE/bin"
else
	HOST_FBC="$(detect_host_fbc || true)"
	[ -n "$HOST_FBC" ] || die "missing host bootstrap compiler; rerun without --skip-host-bootstrap"
fi

##############################################################################
# Cross-build compiler + runtime
##############################################################################

if [ "$DO_DOS_BUILD" = "1" ]; then
	cd "$DOS_WORKTREE"

	PATH="$(dirname "$HOST_FBC"):$PATH"
	export PATH

	msg "cleaning DOS cross-build tree"
	run make clean-compiler clean-libs clean-build clean-bootstrap \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		"${CROSS_MAKE_TOOL_ARGS[@]}" || true

	msg "emitting DOS bootstrap sources"
	run make bootstrap-emit \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		"${CROSS_MAKE_TOOL_ARGS[@]}" \
		BOOT_FBC="$HOST_FBC" \
		BUILD_FBC="$HOST_FBC"

	msg "building DOS bootstrap compiler"
	run make bootstrap-minimal \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		"${CROSS_MAKE_TOOL_ARGS[@]}" \
		BUILD_FBC="$HOST_FBC"

	msg "resetting compiler and runtime outputs for standalone packaging"
	run make clean-compiler clean-libs \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		"${CROSS_MAKE_TOOL_ARGS[@]}" \
		ENABLE_STANDALONE=1

	msg "rebuilding standalone DOS compiler and runtime with host compiler"
	run make compiler runtime \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		"${CROSS_MAKE_TOOL_ARGS[@]}" \
		ENABLE_STANDALONE=1 \
		BUILD_FBC="$HOST_FBC"
else
	[ -f "$DOS_WORKTREE/fbc.exe" ] || die "missing standalone DOS compiler: $DOS_WORKTREE/fbc.exe"
fi

##############################################################################
# Install and package
##############################################################################

if [ "$DO_STAGE_INSTALL" = "1" ]; then
	cd "$DOS_WORKTREE"

	msg "assembling DOS distribution tree"
	rm -rf \
		"${DISTROOT:?}/fb" \
		"${DISTROOT:?}/djgpp" \
		"${DISTROOT:?}/inc" \
		"${DISTROOT:?}/lib"
	rm -f "$DISTROOT/fbdos.bat"

	run make install \
		DESTDIR="$DISTROOT" \
		prefix="$PREFIX" \
		TARGET_TRIPLET="$TARGET_TRIPLET" \
		"${CROSS_MAKE_TOOL_ARGS[@]}" \
		ENABLE_STANDALONE=1 \
		BUILD_FBC="$HOST_FBC"

	mkdir -p "$DISTROOT/fb"
	if [ -f "$DISTROOT/fbc.exe" ]; then
		mv -f "$DISTROOT/fbc.exe" "$DISTROOT/fb/fbc.exe"
	fi
	[ -d "$DISTROOT/inc" ] || die "installed DOS include tree is missing"
	[ -d "$DISTROOT/lib" ] || die "installed DOS library tree is missing"
	mv "$DISTROOT/inc" "$DISTROOT/fb/inc"
	mv "$DISTROOT/lib" "$DISTROOT/fb/lib"

	copy_tree "$DJGPP_DOS_CACHE" "$DISTROOT/djgpp"
	stage_dos_compiler_tools "$DISTROOT/fb"
	prepare_dos_runtime_layout "$DISTROOT/fb" "$DISTROOT/djgpp"

	copy_tree "$ROOT/doc" "$DISTROOT/fb/doc"
	run rsync -a --no-perms --no-owner --no-group \
		--delete --delete-excluded --prune-empty-dirs \
		--exclude-from "$ROOT/mk/example-copy-excludes.rsync" \
		"$ROOT/examples/" "$DISTROOT/fb/examples/"
	cp -f "$ROOT/changelog.txt" "$DISTROOT/fb/changelog.txt"
	cp -f "$ROOT/readme.txt" "$DISTROOT/fb/readme.txt"

	cat > "$DISTROOT/fbdos.bat" <<'EOF'
@echo off
set DJGPP=C:\DJGPP\DJGPP.ENV
set PATH=C:\FB;C:\DJGPP\BIN;%PATH%
echo FreeBASIC DOS environment ready.
EOF
else
	# DOS executables do not require, and archive extraction does not retain,
	# a Unix execute bit.  The smoke test runs these files inside DOSBox-X.
	[ -f "$DISTROOT/fb/fbc.exe" ] || die "missing DOS distribution tree: $DISTROOT/fb/fbc.exe"
	[ -f "$DISTROOT/fb/bin/dos/as.exe" ] || die "missing DOS assembler: $DISTROOT/fb/bin/dos/as.exe"
fi

##############################################################################
# DOSBox smoke test
##############################################################################

run_dosbox_test() {
	local dosbox_bin
	local dosbox_kind
	local -a dosbox_run_env
	local test_root
	local mount_root
	local autoexec_bat
	local trace_log
	local result_txt
	local build_log
	local image_file
	local mtools_source_paths
	local partition_start
	local partition_offset
	local source_path
	local dosbox_status
	local used_image

	dosbox_kind=""
	used_image=0
	if [ -n "${DOSBOX_BIN:-}" ]; then
		dosbox_bin="$(normalize_program_path "$DOSBOX_BIN")"
		case "$(basename "$dosbox_bin" | tr '[:upper:]' '[:lower:]')" in
			dosbox-x*)
				dosbox_kind="dosbox-x"
				;;
			*)
				die "DOSBOX_BIN must point to dosbox-x, not $(basename "$dosbox_bin")"
				;;
		esac
	elif dosbox_bin="$(prepare_portable_dosbox_x)"; then
		dosbox_kind="dosbox-x"
	else
		dosbox_bin="$(command -v dosbox-x || true)"
		if [ -n "$dosbox_bin" ]; then
			dosbox_kind="dosbox-x"
		else
			dosbox_bin="$(command -v dosbox-x.exe || true)"
			if [ -n "$dosbox_bin" ]; then
				dosbox_kind="dosbox-x"
			fi
		fi
	fi

	if [ -z "$dosbox_bin" ]; then
		msg "DOSBox-X not found; skipping DOSBox smoke test"
		return 0
	fi

	dosbox_run_env=()
	if [ "$HOST_KIND" = "linux" ] &&
		[ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
		# DOSBox-X still initializes SDL when -nogui is used.  A release host
		# without X11 or Wayland therefore needs SDL's non-rendering drivers
		# even though every DOS command is supplied on the command line.
		dosbox_run_env=(
			env
			"SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-dummy}"
			"SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-dummy}"
		)
		msg "using SDL dummy video and audio drivers for headless DOSBox-X"
	fi

	msg "running DOSBox-X smoke test"

	test_root="$DOSBOX_ROOT/root"
	mount_root="$test_root"
	if have cygpath; then
		mount_root="$(cygpath -m "$test_root")"
	fi

	rm -rf "$test_root"
	copy_tree "$DISTROOT" "$test_root"
	prepare_dos_runtime_layout "$test_root"

	mkdir -p "$test_root/fb"
	if [ -f "$test_root/fbc.exe" ]; then
		mv -f "$test_root/fbc.exe" "$test_root/fb/fbc.exe"
	fi
	prepare_dos_runtime_layout "$test_root/fb" "$test_root/djgpp"
	[ -f "$test_root/fb/bin/dos/as.exe" ] || die "DOS smoke-test assembler is missing"

	if [ "$dosbox_kind" = "dosbox-x" ] && have mcopy && have sfdisk; then
		local dosbox_image_file
		local mtools_image_file

		cat > "$test_root/hello.bas" <<'EOF'
open "D:\RESULT.TXT" for output as #1
print #1, "FreeBASIC DOS OK"
close #1
EOF

		# Exercise the compiler's 8.3-safe temporary names and normal cleanup
		# while the source and final executable remain on the host mount.
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
C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD.LOG C:\FB\FBC.EXE -v D:\HELLO.BAS -x D:\HELLO.EXE
echo fbc-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
dir D:\HELLO.* >>D:\TRACE.LOG
if exist D:\HELLO.EXE goto runhello
echo hello-exe-missing>>D:\TRACE.LOG
goto afterhello
:runhello
echo hello-exe-present>>D:\TRACE.LOG
C:\DJGPP\BIN\REDIR.EXE -eo -o D:\RUN.LOG D:\HELLO.EXE
echo hello-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
:afterhello
dir D:\RESULT.TXT >>D:\TRACE.LOG
EOF

		image_file="$DOSBOX_ROOT/smoke.img"
		dosbox_image_file="$image_file"
		mtools_image_file="$image_file"
		if have cygpath; then
			dosbox_image_file="$(cygpath -m "$image_file")"
			mtools_image_file="$dosbox_image_file"
		fi
		rm -f "$image_file"
		run_timeout_checked "$DOSBOX_TIMEOUT" "${dosbox_run_env[@]}" "$dosbox_bin" \
			-fastlaunch \
			-nogui \
			-nomenu \
			-exit \
			-set "cpu cputype=ppro_slow" \
			-c "imgmake \"$dosbox_image_file\" -t hd -size $DOSBOX_IMAGE_SIZE -fat 16" \
			-c "exit" \
			|| die "DOSBox image creation failed"

		partition_start="$(sfdisk -d "$image_file" | sed -n 's/.*start= *\([0-9][0-9]*\).*/\1/p' | head -n1)"
		[ -n "$partition_start" ] || die "could not determine DOSBox image partition start"
		partition_offset="$((partition_start * 512))"

		mtools_source_paths=("$test_root"/*)
		if have cygpath; then
			mtools_source_paths=()
			for source_path in "$test_root"/*; do
				mtools_source_paths+=("$(cygpath -m "$source_path")")
			done
		fi
		run env MTOOLS_SKIP_CHECK=1 mcopy -i "${mtools_image_file}@@${partition_offset}" -s "${mtools_source_paths[@]}" ::

		dosbox_status=0
		run_timeout_checked "$DOSBOX_TIMEOUT" "${dosbox_run_env[@]}" "$dosbox_bin" \
			-fastlaunch \
			-nogui \
			-nomenu \
			-exit \
			-set "cpu cputype=ppro_slow" \
			-c "mount d \"$mount_root\"" \
			-c "imgmount c \"$dosbox_image_file\"" \
			-c "d:" \
			-c "FBTEST.BAT" \
			-c "exit" \
			|| dosbox_status=$?
		used_image=1
	elif [ "$HOST_KIND" = "linux" ]; then
		msg "DOSBox-X image path needs mcopy and sfdisk; falling back to mounted-directory smoke test"
	fi

	if [ "$used_image" != "1" ]; then
		cat > "$test_root/hello.bas" <<'EOF'
open "result.txt" for output as #1
print #1, "FreeBASIC DOS OK"
close #1
EOF

		# Compile normally so the smoke test exercises the compiler's 8.3-safe
		# intermediate names and its successful-link cleanup path.
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
if exist hello.exe goto runhello
echo hello-exe-missing>>trace.log
goto afterhello
:runhello
echo hello-exe-present>>trace.log
hello.exe >>trace.log
echo hello-errorlevel=%ERRORLEVEL%>>trace.log
:afterhello
dir >>trace.log
EOF

		dosbox_status=0
		case "$dosbox_kind" in
			dosbox-x)
				run_timeout_checked "$DOSBOX_TIMEOUT" "${dosbox_run_env[@]}" "$dosbox_bin" \
					-fastlaunch \
					-nogui \
					-nomenu \
					-exit \
					-set "cpu cputype=ppro_slow" \
					-c "mount c \"$mount_root\"" \
					-c "c:" \
					-c "FBTEST.BAT" \
					-c "exit" \
					|| dosbox_status=$?
				;;
			*)
				run_timeout_checked "$DOSBOX_TIMEOUT" "${dosbox_run_env[@]}" "$dosbox_bin" \
					-set "cpu cputype=pentium_pro" \
					-exit \
					-c "mount c \"$mount_root\"" \
					-c "c:" \
					-c "FBTEST.BAT" \
					-c "exit" \
					|| dosbox_status=$?
				;;
		esac
	fi

	if [ "${dosbox_status:-0}" -ne 0 ]; then
		msg "DOSBox exited with status ${dosbox_status}; inspecting smoke-test outputs"
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

	if find "$test_root" -maxdepth 1 -type f \
		\( -iname '*.asm' -o -iname '*.o' -o -iname '*.tmp' \) \
		-print -quit | grep -q .
	then
		die "DOSBox smoke test left compiler temporary files behind"
	fi
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

# end of msdos-build-freebasic.sh
