#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-build-freebasic-js.sh
#
# Build a self-contained Windows fbc-js distribution from MSYS2.
# Produces a package tree, a .zip archive, and an NSIS installer that installs
# into C:\freebasic-js with the Emscripten/Node toolchain needed by fbc-js.
##############################################################################

SELF_DIR="$(CDPATH='' cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH='' cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/build_scripts" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo ""
	echo "ERROR: could not locate the FreeBASIC project root."
	exit 1
fi

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo ""
		echo "ERROR: this script must be run inside an MSYS2 environment."
		exit 1
		;;
esac

##############################################################################
# Options
##############################################################################

SKIP_DEPS=0
SKIP_SOURCE_SYNC=0
SKIP_BUILD=0
SKIP_PACKAGE=0
SKIP_INSTALLER=0
SKIP_VALIDATE=0
KEEP_BUILDROOT=0

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-build-freebasic-js.sh [options]

Options:
  --skip-deps         Do not install or update MSYS2 packages
  --skip-source-sync  Reuse the existing build worktree
  --skip-build        Skip the fbc-js build
  --skip-package      Skip distribution tree assembly and zip creation
  --skip-installer    Skip NSIS installer creation
  --skip-validate     Skip packaged fbc-js validation
  --keep-buildroot    Keep the build root on failure or success
  --help              Show this help text

Environment:
  BUILDROOT           Temporary build root (default: /tmp/freebasic-js-build)
  OUT                 Output directory (default: <repo>/out/mingw32-js)
  HOST_FBC_ROOT       Optional existing FreeBASIC install used as host compiler fallback
  UCRT64_ROOT         UCRT64 root used for Emscripten/Node (default: /ucrt64)
  NSIS_EXE            Explicit makensis path (default: /mingw64/bin/makensis.exe)
  BINARYEN_OVERLAY    Overlay official Binaryen Windows tools (default: 1)
  BINARYEN_RELEASE    Official Binaryen release to overlay (default: version_131)
  BINARYEN_SHA256     Expected SHA256 for the Binaryen archive
  NODE_OVERLAY        Overlay official Node.js Windows executable (default: 1)
  NODE_RELEASE        Official Node.js release to overlay (default: v24.14.1)
  NODE_SHA256         Expected SHA256 for the Node.js archive
  JOBS                Parallel make job count (default: detected CPU core count)
EOF
}

for arg in "$@"; do
	case "$arg" in
		--skip-deps) SKIP_DEPS=1 ;;
		--skip-source-sync) SKIP_SOURCE_SYNC=1 ;;
		--skip-build) SKIP_BUILD=1 ;;
		--skip-package) SKIP_PACKAGE=1 ;;
		--skip-installer) SKIP_INSTALLER=1 ;;
		--skip-validate) SKIP_VALIDATE=1 ;;
		--keep-buildroot) KEEP_BUILDROOT=1 ;;
		--help)
			usage
			exit 0
			;;
		*)
			echo "ERROR: unknown option: $arg" >&2
			usage >&2
			exit 1
			;;
	esac
done

##############################################################################
# Helpers
##############################################################################

msg() {
	echo ""
	echo "==> $1"
}

fail() {
	echo ""
	echo "ERROR: $1" >&2
	exit 1
}

run() {
	echo "==> $*"
	"$@"
}

have() {
	command -v "$1" >/dev/null 2>&1
}

download_file() {
	local url="$1"
	local dst="$2"
	local tmp="$dst.part"
	local tmp_win="$tmp"
	local ps_code

	rm -f "$tmp"
	if have cygpath; then
		tmp_win="$(cygpath -aw "$tmp")"
	fi

	if [ -x /c/Windows/System32/curl.exe ]; then
		if run /c/Windows/System32/curl.exe -L --fail --show-error --connect-timeout 30 --max-time 900 -o "$tmp_win" "$url"; then
			[ -s "$tmp" ] || fail "download produced an empty file: $url"
			mv -f "$tmp" "$dst"
			return 0
		fi
		rm -f "$tmp"
	fi

	if have curl; then
		if run curl -L --fail --show-error --connect-timeout 30 --max-time 900 -o "$tmp" "$url"; then
			[ -s "$tmp" ] || fail "download produced an empty file: $url"
			mv -f "$tmp" "$dst"
			return 0
		fi
		rm -f "$tmp"
	fi

	if have powershell.exe && have cygpath; then
		ps_code="\$ProgressPreference = 'SilentlyContinue'; [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing -TimeoutSec 900 -Uri '$url' -OutFile '$tmp_win'"
		if run powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ps_code"; then
			[ -s "$tmp" ] || fail "download produced an empty file: $url"
			mv -f "$tmp" "$dst"
			return 0
		fi
		rm -f "$tmp"
	fi

	fail "could not download: $url"
}

cache_download() {
	local url="$1"
	local archive="$2"
	local sha256="$3"

	if [ -f "$archive" ]; then
		if printf '%s  %s\n' "$sha256" "$archive" | sha256sum -c - >/dev/null 2>&1; then
			echo "$archive: OK"
			return 0
		fi

		echo "cached download failed checksum, redownloading: $archive" >&2
		rm -f "$archive"
	fi

	download_file "$url" "$archive"
	printf '%s  %s\n' "$sha256" "$archive" | sha256sum -c -
}

copy_tree() {
	local src="$1"
	local dst="$2"
	mkdir -p "$dst"
	if have rsync; then
		run rsync -a "$src/" "$dst/"
	else
		run cp -a "$src"/. "$dst/"
	fi
}

copy_examples_tree() {
	local dst="$1"
	mkdir -p "$dst"
	if have rsync; then
		run rsync -a --delete --delete-excluded --prune-empty-dirs \
			--exclude-from "$ROOT/mk/example-copy-excludes.rsync" \
			"$ROOT/examples/" "$dst/"
	else
		run cp -a "$ROOT/examples"/. "$dst/"
	fi
}

copy_dir_files() {
	local src="$1"
	local dst="$2"
	mkdir -p "$dst"
	[ -d "$src" ] || return 0
	find "$src" -maxdepth 1 -type f -exec cp -a {} "$dst/" \;
}

sync_source_tree() {
	local dst="$1"
	mkdir -p "$dst"
	if have rsync; then
		run rsync -a --delete --delete-excluded --prune-empty-dirs \
			--exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
			"$ROOT/" "$dst/"
	else
		fail "rsync is required to create an isolated worktree"
	fi
}

sanitize_source_tree() {
	local triplet="${1:-}"
	msg "Removing generated example artifacts from the source tree"
	if [ -n "$triplet" ]; then
		run make TARGET_TRIPLET="$triplet" clean-example-artifacts
	else
		run make clean-example-artifacts
	fi
}

max_jobs() {
	local n=1
	if have nproc; then
		n="$(nproc)"
	elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
		n="$(getconf _NPROCESSORS_ONLN)"
	fi
	case "$n" in
		''|*[!0-9]*)
			n=1
			;;
	esac
	if [ "$n" -lt 1 ]; then
		n=1
	fi
	echo "$n"
}

extract_var() {
	local name="$1"
	awk -F':=' -v key="$name" '
		$1 ~ "^[[:space:]]*" key "[[:space:]]*$" {
			gsub(/[[:space:]]/, "", $2)
			print $2
			exit
		}
	' "$ROOT/mk/version.mk"
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

	if command -v fbc >/dev/null 2>&1 && fbc -version >/dev/null 2>&1; then
		command -v fbc
		return 0
	fi

	return 1
}

##############################################################################
# Configuration
##############################################################################

FBVERSION="$(extract_var FBVERSION)"
[ -n "$FBVERSION" ] || fail "could not determine FBVERSION"

BUILDROOT="${BUILDROOT:-/tmp/freebasic-js-build}"
WORKROOT="$BUILDROOT/work"
STAGEROOT="$BUILDROOT/stage"
DISTROOT_BASE="$BUILDROOT/dist"
OUT="${OUT:-$ROOT/out/mingw32-js}"
INSTALL_DIR_WIN="${INSTALL_DIR_WIN:-C:\\freebasic-js}"
INSTALL_SUBDIR="${INSTALL_SUBDIR:-freebasic-js}"
UCRT64_ROOT="${UCRT64_ROOT:-/ucrt64}"
NSIS_EXE="${NSIS_EXE:-/mingw64/bin/makensis.exe}"
BINARYEN_OVERLAY="${BINARYEN_OVERLAY:-1}"
BINARYEN_RELEASE="${BINARYEN_RELEASE:-version_131}"
BINARYEN_ARCHIVE="${BINARYEN_ARCHIVE:-binaryen-${BINARYEN_RELEASE}-x86_64-windows.tar.gz}"
BINARYEN_URL="${BINARYEN_URL:-https://github.com/WebAssembly/binaryen/releases/download/${BINARYEN_RELEASE}/${BINARYEN_ARCHIVE}}"
BINARYEN_SHA256="${BINARYEN_SHA256:-2f4edac1703a2f695254d6ff52ede03481e67db1f094915763d863158c17d9bc}"
NODE_OVERLAY="${NODE_OVERLAY:-1}"
NODE_RELEASE="${NODE_RELEASE:-v24.14.1}"
NODE_ARCHIVE="${NODE_ARCHIVE:-node-${NODE_RELEASE}-win-x64.zip}"
NODE_URL="${NODE_URL:-https://nodejs.org/dist/${NODE_RELEASE}/${NODE_ARCHIVE}}"
NODE_SHA256="${NODE_SHA256:-6e50ce5498c0cebc20fd39ab3ff5df836ed2f8a31aa093cecad8497cff126d70}"
JOBS="${JOBS:-$(max_jobs)}"

HOST_TRIPLET="$("$UCRT64_ROOT/bin/gcc" -dumpmachine 2>/dev/null || true)"
if [ -z "$HOST_TRIPLET" ]; then
	HOST_TRIPLET="x86_64-w64-mingw32"
fi

DISTNAME_BASE="FreeBASIC-${FBVERSION}-fbc-js"
DISTNAME="$DISTNAME_BASE"
DISTROOT="$DISTROOT_BASE/$DISTNAME"

mkdir -p "$BUILDROOT" "$WORKROOT" "$STAGEROOT" "$DISTROOT_BASE" "$OUT"

cleanup() {
	if [ "$KEEP_BUILDROOT" -eq 0 ]; then
		:
	fi
}
trap cleanup EXIT

##############################################################################
# Dependency installation
##############################################################################

install_dependencies() {
	msg "Installing MSYS2 packages needed for fbc-js"

	run pacman -Syu --needed --noconfirm
	run pacman -S --needed --noconfirm \
		base-devel \
		curl \
		rsync \
		unzip \
		zip \
		mingw-w64-ucrt-x86_64-binutils \
		mingw-w64-ucrt-x86_64-emscripten \
		mingw-w64-ucrt-x86_64-gcc \
		mingw-w64-ucrt-x86_64-nodejs \
		mingw-w64-ucrt-x86_64-python \
		mingw-w64-x86_64-nsis
}

ensure_emscripten_toolchain() {
	local runtime_dir
	local tool

	for tool in emcc em++ emar emranlib node python; do
		have "$tool" || fail "$tool not found after loading the UCRT64 Emscripten environment"
	done

	for runtime_dir in "${EMCC_TEMP_DIR:-}" "${EM_CACHE:-}"; do
		if [ -n "$runtime_dir" ]; then
			mkdir -p "$runtime_dir" ||
				fail "could not create Emscripten runtime directory: $runtime_dir"
		fi
	done

	emcc -v >/dev/null || fail "emcc is present but could not start"
}

##############################################################################
# Build
##############################################################################

build_freebasic_js() {
	local worktree="$WORKROOT/fbc-js"
	local stagedir="$STAGEROOT/fbc-js"
	local bootstrap_sources_dir="$worktree/bootstrap/win64"
	local saved_path="$PATH"
	local host_fbc=""
	local clean_fbc=""
	local build_fbc=""
	local cc="$UCRT64_ROOT/bin/gcc.exe"
	local cxx="$UCRT64_ROOT/bin/g++.exe"
	local ar="$UCRT64_ROOT/bin/ar.exe"
	local as="$UCRT64_ROOT/bin/as.exe"
	local ld="$UCRT64_ROOT/bin/ld.exe"
	local ranlib="$UCRT64_ROOT/bin/ranlib.exe"
	local strip="$UCRT64_ROOT/bin/strip.exe"
	local dlltool="$UCRT64_ROOT/bin/dlltool.exe"
	local gcc_compat="$worktree/tools/gcc-generated-c.exe"
	local gcc_compat_c="$worktree/tools/gcc-generated-c.c"
	local cc_win

	[ -x "$cc" ] || fail "UCRT64 gcc not found at $cc"

	msg "Preparing fbc-js worktree"
	if [ "$SKIP_SOURCE_SYNC" -eq 0 ] || [ ! -d "$worktree" ]; then
		rm -rf "$worktree"
		sync_source_tree "$worktree"
	fi

	rm -rf "$stagedir"
	mkdir -p "$stagedir"

	cd "$worktree"
	PATH="$worktree/bin:$UCRT64_ROOT/bin:/mingw64/bin:/usr/bin:/c/Windows/System32:/c/Windows"
	export PATH

	cc_win="$(cygpath -am "$cc")"
	mkdir -p "$worktree/tools"
	cat > "$gcc_compat_c" <<EOF
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	const char *gcc_path = "$cc_win";
	const int extra_argc = 3;
	char **args;
	int i;
	int status;

	args = (char **)calloc((size_t)argc + (size_t)extra_argc + 1u, sizeof(char *));
	if (args == NULL) {
		fprintf(stderr, "gcc-generated-c: out of memory\\n");
		return 1;
	}

	args[0] = (char *)gcc_path;
	args[1] = (char *)"-fpermissive";
	args[2] = (char *)"-Wno-int-conversion";
	args[3] = (char *)"-Wno-incompatible-pointer-types";

	for (i = 1; i < argc; i++) {
		args[i + extra_argc] = argv[i];
	}

	status = _spawnv(_P_WAIT, gcc_path, (const char * const *)args);
	if (status == -1) {
		perror("gcc-generated-c");
		free(args);
		return 127;
	}

	free(args);
	return status;
}
EOF
	run "$cc" "$gcc_compat_c" -o "$gcc_compat"

	host_fbc="$(detect_fbc \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc64.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc}" \
		"$worktree/bin/fbc.exe" \
		"$worktree/bootstrap/fbc.exe" \
		"$ROOT/bin/fbc.exe" \
		"$ROOT/bootstrap/fbc.exe" \
		"/c/FreeBASIC/fbc.exe" \
		|| true)"

	if [ -n "$host_fbc" ]; then
		msg "Emitting win64 bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-emit \
			FBC_EXE="$host_fbc" \
			TARGET_TRIPLET="$HOST_TRIPLET" \
			CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"
	elif [ -d "$bootstrap_sources_dir" ] && find "$bootstrap_sources_dir" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
		msg "Bootstrap sources already present for win64"
	else
		msg "No direct bootstrap compiler available; seeding from peer bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-seed-peer \
			TARGET_TRIPLET="$HOST_TRIPLET" \
			CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"
	fi

	clean_fbc="$host_fbc"
	if [[ "$clean_fbc" == "$worktree/bin/"* ]]; then
		run cp "$clean_fbc" "$worktree/tools/clean-fbc.exe"
		clean_fbc="$worktree/tools/clean-fbc.exe"
	fi

	msg "Cleaning fbc-js worktree"
	run make clean FBC="$clean_fbc" TARGET_TRIPLET="$HOST_TRIPLET" || true

	msg "Building host bootstrap compiler ($JOBS threads)"
	run make -j"$JOBS" \
		bootstrap-minimal \
		TARGET_TRIPLET="$HOST_TRIPLET" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"

	[ -f "$worktree/bootstrap/fbc.exe" ] || fail "bootstrap-minimal did not produce bootstrap/fbc.exe"
	build_fbc="$worktree/bin/fbc.exe"
	[ -f "$build_fbc" ] || fail "bootstrap-minimal did not install bin/fbc.exe"

	msg "Building native fbc-js driver"
	run make -j"$JOBS" \
		compiler-js \
		FBC="$build_fbc" \
		BUILD_FBC_TARGET="win64" \
		TARGET_TRIPLET="$HOST_TRIPLET" \
		TARGET="$HOST_TRIPLET" \
		CC="$gcc_compat" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"

	msg "Building js-asmjs runtime libraries"
	if [ -f "$UCRT64_ROOT/etc/profile.d/emscripten.sh" ]; then
		# The MSYS2 Emscripten package publishes emcc through this profile
		# fragment instead of only dropping standalone commands on PATH.
		# Source it here so non-login build shells behave like UCRT64 shells.
		# shellcheck disable=SC1090,SC1091
		. "$UCRT64_ROOT/etc/profile.d/emscripten.sh"
	fi
	ensure_emscripten_toolchain

	run make -j"$JOBS" \
		rtlib fbrt gfxlib2 sfxlib \
		FBC="$build_fbc" \
		TARGET_TRIPLET="asmjs-unknown-emscripten" \
		TARGET="asmjs-unknown-emscripten" \
		FBTARGET_DIR_OVERRIDE="js-asmjs" \
		CC=emcc \
		CXX=em++ \
		LD=emcc \
		AR=emar \
		RANLIB=emranlib

	msg "Installing fbc-js into staging"
	run make install-js \
		DESTDIR="$stagedir" \
		prefix="/$INSTALL_SUBDIR" \
		FBC="$build_fbc" \
		BUILD_FBC_TARGET="win64" \
		TARGET_TRIPLET="$HOST_TRIPLET" \
		TARGET="$HOST_TRIPLET" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"

	[ -f "$stagedir/fbc-js.exe" ] || fail "staged fbc-js.exe is missing"
	[ -f "$stagedir/fbc-js-app" ] || fail "staged fbc-js-app is missing"
	[ -d "$stagedir/lib/freebasic-js/js-asmjs" ] || fail "staged js-asmjs runtime is missing"

	cd "$ROOT"
	PATH="$saved_path"
	export PATH
}

##############################################################################
# Distribution assembly
##############################################################################

copy_runtime_dlls() {
	local exe="$1"
	local dst="$2"
	local dep

	mkdir -p "$dst"
	[ -f "$exe" ] || return 0
	have ldd || return 0

	while IFS= read -r dep; do
		[ -n "$dep" ] || continue
		case "$dep" in
			/ucrt64/*|/mingw64/*|/usr/bin/*)
				[ -f "$dep" ] && cp -a "$dep" "$dst/"
				;;
		esac
	done < <(ldd "$exe" | awk '
		/=>/ { print $(NF - 1); next }
		/^\// { print $1; next }
	')
}

copy_ucrt64_toolchain() {
	local dst="$DISTROOT/toolchain/ucrt64"

	[ -d "$UCRT64_ROOT" ] || fail "UCRT64 root not found: $UCRT64_ROOT"

	msg "Bundling UCRT64 Emscripten/Node toolchain"
	mkdir -p "$dst"
	if have rsync; then
		run rsync -a --delete \
			--exclude '/share/doc/' \
			--exclude '/share/info/' \
			--exclude '/share/man/' \
			--exclude '/var/cache/' \
			"$UCRT64_ROOT/" "$dst/"
	else
		copy_tree "$UCRT64_ROOT" "$dst"
	fi
}

repair_emscripten_windows_launchers() {
	local emroot="$DISTROOT/toolchain/ucrt64/lib/emscripten"
	local dir
	local py
	local base
	local exe
	local bat
	local python_rel

	[ -d "$emroot" ] || fail "Emscripten directory was not bundled"

	msg "Repairing bundled Emscripten Windows launchers"

	for dir in "$emroot" "$emroot/tools"; do
		[ -d "$dir" ] || continue

		case "$dir" in
			"$emroot")
				python_rel="..\\..\\bin\\python.exe"
				;;
			"$emroot/tools")
				python_rel="..\\..\\..\\bin\\python.exe"
				;;
			*)
				fail "unexpected Emscripten launcher directory: $dir"
				;;
		esac

		for py in "$dir"/*.py; do
			[ -f "$py" ] || continue

			base="${py##*/}"
			base="${base%.py}"
			exe="$dir/${base}.exe"
			bat="$dir/${base}.bat"

			# The MSYS2 Emscripten package includes small .exe launchers next
			# to the Python frontend scripts.  Those launchers work when MSYS2
			# owns the whole environment, but the standalone FreeBASIC package
			# runs them from an arbitrary install tree.  In that layout they can
			# either hang or fail before reaching python.exe.
			#
			# Emscripten's Windows path resolver falls back from .exe to .bat.
			# Replacing the package-local .exe frontend launchers with batch
			# files keeps internal calls such as shared.EMCC Windows-callable
			# without changing Emscripten's Python code.
			rm -f "$exe"

			cat > "$bat" <<EOF
@echo off
setlocal
set "EMCC="
set "EMLD="
set "EMAS="
set "EMAR="
set "EMRANLIB="
"%~dp0${python_rel}" "%~dp0${base}.py" %*
exit /b %ERRORLEVEL%
EOF
		done
	done
}

write_emscripten_config() {
	local config="$DISTROOT/toolchain/ucrt64/lib/emscripten/.emscripten"

	msg "Writing relocatable Emscripten configuration"

	[ -d "$(dirname "$config")" ] || fail "Emscripten directory was not bundled"

	cat > "$config" <<'EOF'
# .emscripten file for the FreeBASIC fbc-js package.
#
# MSYS2 writes absolute paths into this file.  The standalone package is
# relocatable, so keep the paths relative to this config file instead.

import os

_CONFIG_DIR = os.path.dirname(os.path.abspath(__file__))
_UCRT64_ROOT = os.path.abspath(os.path.join(_CONFIG_DIR, '..', '..'))
_TOOLCHAIN_ROOT = os.path.abspath(os.path.join(_UCRT64_ROOT, '..'))

def _emscripten_path(*parts):
    return os.path.join(_UCRT64_ROOT, *parts).replace(os.sep, '/')

_OFFICIAL_NODE_JS = os.path.join(_TOOLCHAIN_ROOT, 'official-node', 'node.exe')

LLVM_ROOT = _emscripten_path('opt', 'emscripten-llvm', 'bin')
if os.path.exists(_OFFICIAL_NODE_JS):
    NODE_JS = _OFFICIAL_NODE_JS.replace(os.sep, '/')
else:
    NODE_JS = _emscripten_path('bin', 'node.exe')
BINARYEN_ROOT = _emscripten_path()
EOF
}

overlay_official_node() {
	local cache="$BUILDROOT/downloads"
	local archive="$cache/$NODE_ARCHIVE"
	local extract="$BUILDROOT/node-official"
	local nodesrc="$extract/node-${NODE_RELEASE}-win-x64/node.exe"
	local nodedst="$DISTROOT/toolchain/official-node/node.exe"

	[ "$NODE_OVERLAY" -ne 0 ] || return 0
	have sha256sum || fail "sha256sum is required to verify the official Node.js archive"
	have unzip || fail "unzip is required to extract the official Node.js archive"

	msg "Overlaying official Node.js Windows executable"

	mkdir -p "$cache"
	cache_download "$NODE_URL" "$archive" "$NODE_SHA256"

	rm -rf "$extract"
	mkdir -p "$extract"
	run unzip -q "$archive" -d "$extract"

	[ -f "$nodesrc" ] || fail "official Node.js executable was not found"
	mkdir -p "$(dirname "$nodedst")"
	cp -f "$nodesrc" "$nodedst"
}

overlay_official_binaryen() {
	local bindst="$DISTROOT/toolchain/ucrt64/bin"
	local cache="$BUILDROOT/downloads"
	local archive="$cache/$BINARYEN_ARCHIVE"
	local extract="$BUILDROOT/binaryen-official"
	local binsrc

	[ "$BINARYEN_OVERLAY" -ne 0 ] || return 0
	[ -d "$bindst" ] || fail "UCRT64 bin directory was not bundled"
	have sha256sum || fail "sha256sum is required to verify the official Binaryen archive"

	msg "Overlaying official Binaryen Windows tools"

	mkdir -p "$cache"
	cache_download "$BINARYEN_URL" "$archive" "$BINARYEN_SHA256"

	rm -rf "$extract"
	mkdir -p "$extract"
	run tar -xf "$archive" -C "$extract"

	binsrc="$extract/binaryen-${BINARYEN_RELEASE}/bin"
	[ -d "$binsrc" ] || fail "official Binaryen bin directory was not found"

	for tool in \
		wasm-as.exe \
		wasm-ctor-eval.exe \
		wasm-dis.exe \
		wasm-emscripten-finalize.exe \
		wasm-merge.exe \
		wasm-metadce.exe \
		wasm-opt.exe \
		wasm-reduce.exe \
		wasm-shell.exe \
		wasm-split.exe \
		wasm2js.exe
	do
		[ -f "$binsrc/$tool" ] || fail "official Binaryen tool is missing: $tool"
		cp -f "$binsrc/$tool" "$bindst/$tool"
	done
}

write_launchers() {
	msg "Writing fbc-js launcher scripts"

	mkdir -p "$DISTROOT/toolchain/cmd-shims"
	for tool in emcc em++ emar emranlib; do
		cat > "$DISTROOT/toolchain/cmd-shims/${tool}.cmd" <<EOF
@echo off
setlocal
set "FBJS_ROOT=%~dp0..\\.."
set "EMCC="
set "EMLD="
set "EMAS="
set "EMAR="
set "EMRANLIB="
"%FBJS_ROOT%\\toolchain\\ucrt64\\bin\\python.exe" "%FBJS_ROOT%\\toolchain\\ucrt64\\lib\\emscripten\\${tool}.py" %*
exit /b %ERRORLEVEL%
EOF
	done

cat > "$DISTROOT/fbc-js.cmd" <<'EOF'
@echo off
setlocal
set "FBJS_ROOT=%~dp0"
set "PATH=%FBJS_ROOT%toolchain\cmd-shims;%FBJS_ROOT%toolchain\ucrt64\bin;%FBJS_ROOT%toolchain\ucrt64\lib\emscripten;%FBJS_ROOT%;%PATH%"
if not defined EM_CONFIG set "EM_CONFIG=%FBJS_ROOT%toolchain\ucrt64\lib\emscripten\.emscripten"
if not defined EMSDK_PYTHON set "EMSDK_PYTHON=%FBJS_ROOT%toolchain\ucrt64\bin\python.exe"
if not defined EMCC set "EMCC=%FBJS_ROOT%toolchain\cmd-shims\emcc.cmd"
if not defined EMLD set "EMLD=%FBJS_ROOT%toolchain\cmd-shims\emcc.cmd"
if not defined EMAS set "EMAS=%FBJS_ROOT%toolchain\cmd-shims\emcc.cmd"
if not defined EMAR set "EMAR=%FBJS_ROOT%toolchain\cmd-shims\emar.cmd"
if not defined FBJS_CACHE_ROOT (
	if defined LOCALAPPDATA (
		set "FBJS_CACHE_ROOT=%LOCALAPPDATA%\FreeBASIC\fbc-js"
	) else (
		set "FBJS_CACHE_ROOT=%TEMP%\FreeBASIC\fbc-js"
	)
)
if not defined EMCC_TEMP_DIR set "EMCC_TEMP_DIR=%FBJS_CACHE_ROOT%\emcc-temp"
if not defined EM_CACHE set "EM_CACHE=%FBJS_CACHE_ROOT%\em-cache"
if not defined BINARYEN_CORES set "BINARYEN_CORES=1"
if not defined EMCC_BATCH_BUILD set "EMCC_BATCH_BUILD=0"
if not exist "%EMCC_TEMP_DIR%" mkdir "%EMCC_TEMP_DIR%" >nul 2>nul
if not exist "%EM_CACHE%" mkdir "%EM_CACHE%" >nul 2>nul
"%FBJS_ROOT%bin\fbc-js.exe" %*
exit /b %ERRORLEVEL%
EOF

	cat > "$DISTROOT/fbc-js-app.cmd" <<'EOF'
@echo off
setlocal
set "FBJS_ROOT=%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%FBJS_ROOT%fbc-js-app.ps1" %*
exit /b %ERRORLEVEL%
EOF

	cat > "$DISTROOT/fbc-js-app.ps1" <<'EOF'
$ErrorActionPreference = "Stop"
$AppArgs = [string[]] $args

function Show-Usage {
	Write-Host "Usage: fbc-js-app.cmd [options] program.bas [fbc-js options]"
	Write-Host ""
	Write-Host "Options:"
	Write-Host "  -o DIR, --out DIR   Output directory (default: <program>-js)"
	Write-Host "  --assets DIR        Preload DIR at the browser program current directory"
}

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$env:PATH = "$Root\toolchain\cmd-shims;$Root\toolchain\ucrt64\bin;$Root\toolchain\ucrt64\lib\emscripten;$Root;$($env:PATH)"
$env:EMSDK_PYTHON = "$Root\toolchain\ucrt64\bin\python.exe"
$env:EMCC = "$Root\toolchain\cmd-shims\emcc.cmd"
$env:EMLD = "$Root\toolchain\cmd-shims\emcc.cmd"
$env:EMAS = "$Root\toolchain\cmd-shims\emcc.cmd"
$env:EMAR = "$Root\toolchain\cmd-shims\emar.cmd"

if (-not $env:EM_CONFIG) {
	$env:EM_CONFIG = "$Root\toolchain\ucrt64\lib\emscripten\.emscripten"
}

if (-not $env:FBJS_CACHE_ROOT) {
	if ($env:LOCALAPPDATA) {
		$env:FBJS_CACHE_ROOT = "$env:LOCALAPPDATA\FreeBASIC\fbc-js"
	} else {
		$env:FBJS_CACHE_ROOT = "$env:TEMP\FreeBASIC\fbc-js"
	}
}

if (-not $env:EMCC_TEMP_DIR) {
	$env:EMCC_TEMP_DIR = "$env:FBJS_CACHE_ROOT\emcc-temp"
}

if (-not $env:EM_CACHE) {
	$env:EM_CACHE = "$env:FBJS_CACHE_ROOT\em-cache"
}

if (-not $env:BINARYEN_CORES) {
	$env:BINARYEN_CORES = "1"
}

if (-not $env:EMCC_BATCH_BUILD) {
	$env:EMCC_BATCH_BUILD = "0"
}

New-Item -ItemType Directory -Force -Path $env:EMCC_TEMP_DIR, $env:EM_CACHE | Out-Null

$outDir = $null
$assetDir = $env:FBJS_ASSETS
$program = $null
$compilerArgs = New-Object System.Collections.Generic.List[string]

for ($i = 0; $i -lt $AppArgs.Count; $i++) {
	$arg = $AppArgs[$i]

	switch -Regex ($arg) {
		'^(--help|-h|-help)$' {
			Show-Usage
			exit 0
		}
		'^(-o|--out)$' {
			$i++
			if ($i -ge $AppArgs.Count) {
				throw "$arg requires a directory"
			}
			$outDir = $AppArgs[$i]
			continue
		}
		'^--out=' {
			$outDir = $arg.Substring(6)
			continue
		}
		'^--assets$' {
			$i++
			if ($i -ge $AppArgs.Count) {
				throw "--assets requires a directory"
			}
			$assetDir = $AppArgs[$i]
			continue
		}
		'^--assets=' {
			$assetDir = $arg.Substring(9)
			continue
		}
		default {
			if (-not $program -and [System.IO.Path]::GetExtension($arg).Equals(".bas", [System.StringComparison]::OrdinalIgnoreCase)) {
				$program = $arg
			}
			$compilerArgs.Add($arg)
		}
	}
}

if (-not $program) {
	throw "no .bas source file was given"
}

if (-not $outDir) {
	$outDir = [System.IO.Path]::GetFileNameWithoutExtension($program) + "-js"
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$outputFile = Join-Path $outDir "index.html"

$linkArgs = @()
if ($assetDir) {
	if (-not (Test-Path -LiteralPath $assetDir -PathType Container)) {
		throw "assets directory not found: $assetDir"
	}
	$assetFull = (Resolve-Path -LiteralPath $assetDir).ProviderPath
	$linkArgs = @("-Wl", "--preload-file", "-Wl", "$assetFull@/")
}

& "$Root\fbc-js.cmd" "-x" $outputFile @linkArgs @compilerArgs
exit $LASTEXITCODE
EOF

	cat > "$DISTROOT/freebasic-js-env.cmd" <<'EOF'
@echo off
set "FBJS_ROOT=%~dp0"
set "PATH=%FBJS_ROOT%toolchain\cmd-shims;%FBJS_ROOT%toolchain\ucrt64\bin;%FBJS_ROOT%toolchain\ucrt64\lib\emscripten;%FBJS_ROOT%;%PATH%"
if not defined EM_CONFIG set "EM_CONFIG=%FBJS_ROOT%toolchain\ucrt64\lib\emscripten\.emscripten"
if not defined EMSDK_PYTHON set "EMSDK_PYTHON=%FBJS_ROOT%toolchain\ucrt64\bin\python.exe"
if not defined EMCC set "EMCC=%FBJS_ROOT%toolchain\cmd-shims\emcc.cmd"
if not defined EMLD set "EMLD=%FBJS_ROOT%toolchain\cmd-shims\emcc.cmd"
if not defined EMAS set "EMAS=%FBJS_ROOT%toolchain\cmd-shims\emcc.cmd"
if not defined EMAR set "EMAR=%FBJS_ROOT%toolchain\cmd-shims\emar.cmd"
if not defined FBJS_CACHE_ROOT (
	if defined LOCALAPPDATA (
		set "FBJS_CACHE_ROOT=%LOCALAPPDATA%\FreeBASIC\fbc-js"
	) else (
		set "FBJS_CACHE_ROOT=%TEMP%\FreeBASIC\fbc-js"
	)
)
if not defined EMCC_TEMP_DIR set "EMCC_TEMP_DIR=%FBJS_CACHE_ROOT%\emcc-temp"
if not defined EM_CACHE set "EM_CACHE=%FBJS_CACHE_ROOT%\em-cache"
if not defined BINARYEN_CORES set "BINARYEN_CORES=1"
if not defined EMCC_BATCH_BUILD set "EMCC_BATCH_BUILD=0"
if not exist "%EMCC_TEMP_DIR%" mkdir "%EMCC_TEMP_DIR%" >nul 2>nul
if not exist "%EM_CACHE%" mkdir "%EM_CACHE%" >nul 2>nul
echo FreeBASIC JS environment ready.
echo fbc-js: %FBJS_ROOT%bin\fbc-js.exe
echo emcc temp: %EMCC_TEMP_DIR%
cmd /k
EOF

cat > "$DISTROOT/freebasic-js-env.sh" <<'EOF'
#!/usr/bin/env sh

_fbjs_script=${BASH_SOURCE:-$0}
_fbjs_root=$(CDPATH= cd -- "$(dirname "$_fbjs_script")" && pwd)
PATH="${_fbjs_root}/toolchain/ucrt64/bin:${_fbjs_root}/bin:${_fbjs_root}:${PATH}"
PATH="${_fbjs_root}/toolchain/ucrt64/lib/emscripten:${PATH}"
: "${EM_CONFIG:=${_fbjs_root}/toolchain/ucrt64/lib/emscripten/.emscripten}"
: "${FBJS_CACHE_ROOT:=${XDG_CACHE_HOME:-${TMPDIR:-/tmp}/freebasic-js}/fbc-js}"
: "${EMCC_TEMP_DIR:=${FBJS_CACHE_ROOT}/emcc-temp}"
: "${EM_CACHE:=${FBJS_CACHE_ROOT}/em-cache}"
: "${BINARYEN_CORES:=1}"
: "${EMCC_BATCH_BUILD:=0}"
mkdir -p "$EMCC_TEMP_DIR" "$EM_CACHE" 2>/dev/null || true
export PATH EM_CONFIG FBJS_CACHE_ROOT EMCC_TEMP_DIR EM_CACHE BINARYEN_CORES EMCC_BATCH_BUILD
unset _fbjs_script _fbjs_root
EOF

	chmod 755 "$DISTROOT/freebasic-js-env.sh"
}

write_distribution_notes() {
	msg "Writing fbc-js package notes"

	cat > "$DISTROOT/readme-fbc-js.txt" <<EOF
FreeBASIC JS ${FBVERSION}

This package is intended to run without a separate MSYS2 installation.

The installer adds these directories to the Windows system PATH:

    ${INSTALL_DIR_WIN}
    ${INSTALL_DIR_WIN}\\toolchain\\cmd-shims
    ${INSTALL_DIR_WIN}\\toolchain\\ucrt64\\bin
    ${INSTALL_DIR_WIN}\\toolchain\\ucrt64\\lib\\emscripten

The toolchain directory contains the UCRT64 Emscripten environment used by
fbc-js, including emcc, emar, Node.js, Python, Binaryen, Clang/LLVM, runtime
DLLs, headers, libraries, and supporting data files installed by the MSYS2
packages this build script uses.

The build script validates emcc, em++, emar, emranlib, Node.js, and Python
before compiling the JavaScript runtime libraries, and passes those tools
explicitly to make so a plain MSYS2 shell on a new Windows PC follows the same
path as this package build.

The Binaryen command-line tools and Node.js executable are overlaid from the
official upstream Windows releases.  This keeps the standalone package on the
same release levels while avoiding Windows hangs and crashes seen in the
MSYS2-built tool executables.

The package replaces MSYS2's Emscripten Python frontend .exe launchers with
local .bat launchers that call the bundled python.exe directly.  The Windows
launchers also disable Emscripten's batched system-library compile by default
to avoid command-line length failures while filling a new cache.

fbc-js.cmd creates a per-user Emscripten cache/temp area under:

    %LOCALAPPDATA%\\FreeBASIC\\fbc-js

This keeps generated object, cache, and temporary files out of the install
directory, which may not be writable for non-admin users.

For browser games with relative-path assets, use fbc-js-app.cmd.  It writes an
app directory containing index.html and preloads the selected asset folder at
the program's virtual current directory:

    fbc-js-app.cmd --out build\\mygame --assets game-folder game.bas

If MSYS2 is present, the installer also writes:

    C:\\msys64\\etc\\profile.d\\freebasic-js.sh
    C:\\msys32\\etc\\profile.d\\freebasic-js.sh

Those files only make existing MSYS2 login shells see this standalone
installation.  They are not required for normal Windows cmd.exe or PowerShell
use.
EOF
}

assemble_distribution() {
	local stagedir="$STAGEROOT/fbc-js"

	rm -rf "$DISTROOT"
	mkdir -p "$DISTROOT"

	msg "Copying fbc-js staged files"
	copy_tree "$stagedir" "$DISTROOT"
	mkdir -p "$DISTROOT/bin"
	if [ -f "$DISTROOT/fbc-js.exe" ]; then
		mv "$DISTROOT/fbc-js.exe" "$DISTROOT/bin/fbc-js.exe"
	fi
	if [ -f "$DISTROOT/fbc-js-app" ]; then
		mv "$DISTROOT/fbc-js-app" "$DISTROOT/bin/fbc-js-app"
		chmod 755 "$DISTROOT/bin/fbc-js-app"
	fi

	msg "Copying top-level documentation and examples"
	copy_tree "$ROOT/doc" "$DISTROOT/doc"
	copy_examples_tree "$DISTROOT/examples"
	cp -a "$ROOT/changelog.txt" "$DISTROOT/"
	cp -a "$ROOT/readme.txt" "$DISTROOT/"

	copy_runtime_dlls "$DISTROOT/bin/fbc-js.exe" "$DISTROOT/bin"
	copy_ucrt64_toolchain
	repair_emscripten_windows_launchers
	overlay_official_binaryen
	overlay_official_node
	write_emscripten_config
	write_launchers
	write_distribution_notes
}

##############################################################################
# Packaging
##############################################################################

create_zip() {
	local zipfile="$OUT/${DISTNAME}.zip"
	msg "Creating fbc-js distribution zip"
	rm -f "$zipfile"
	(
		cd "$DISTROOT_BASE"
		run zip -qr "$zipfile" "$DISTNAME"
	)
}

create_installer() {
	local installer_nsi="$BUILDROOT/${DISTNAME}.nsi"
	local installer_exe="$OUT/${DISTNAME}-setup.exe"
	local installer_payload_zip="$BUILDROOT/${DISTNAME}-installer-payload.zip"
	local out_win
	local payload_win
	local refresh_environment_win

	[ -x "$NSIS_EXE" ] || fail "makensis not found at $NSIS_EXE; install the nsis package or set NSIS_EXE"
	have cygpath || fail "cygpath not found"
	have zip || fail "zip not found"

	out_win="$(cygpath -aw "$installer_exe")"
	msg "Creating fbc-js NSIS payload zip"
	rm -f "$installer_payload_zip"
	(
		cd "$DISTROOT"
		run zip -qr "$installer_payload_zip" .
	)

	payload_win="$(cygpath -aw "$installer_payload_zip")"
	refresh_environment_win="$(cygpath -aw "$ROOT/build_scripts/windows-refresh-environment.ps1")"

	msg "Generating NSIS installer script"
	cat > "$installer_nsi" <<EOF
Unicode true
SetCompressor zlib
RequestExecutionLevel admin

Name "FreeBASIC JS ${FBVERSION}"
OutFile "$out_win"
InstallDir "$INSTALL_DIR_WIN"
ShowInstDetails show
ShowUninstDetails show

!include "MUI2.nsh"
!include "StrFunc.nsh"
!include "WinMessages.nsh"


!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

\${Using:StrFunc} StrStr
\${Using:StrFunc} UnStrRep

Function RefreshEnvironment
	SetOutPath "\$TEMP"
	File /oname=FreeBASIC-refresh-environment.ps1 "$refresh_environment_win"
	; Launch directly so Windows shell activation cannot hold the installer open.
	Exec '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -File "\$TEMP\\FreeBASIC-refresh-environment.ps1"'
FunctionEnd

Function un.RefreshEnvironment
	SetOutPath "\$TEMP"
	File /oname=FreeBASIC-refresh-environment.ps1 "$refresh_environment_win"
	Exec '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -File "\$TEMP\\FreeBASIC-refresh-environment.ps1"'
FunctionEnd

Function AddOnePath
	Exch \$3
	ReadRegStr \$0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path"
	StrCpy \$1 ";\$0;"
	\${StrStr} \$2 \$1 ";\$3;"
	StrCmp \$2 "" 0 done
	StrCmp \$0 "" 0 +2
		StrCpy \$0 "\$3"
	StrCmp \$0 "\$3" done 0
	StrCpy \$0 "\$0;\$3"
	WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "\$0"
	done:
	Pop \$3
FunctionEnd

Function AddInstallDirsToPath
	Push "\$INSTDIR"
	Call AddOnePath
	Push "\$INSTDIR\\toolchain\\cmd-shims"
	Call AddOnePath
	Push "\$INSTDIR\\toolchain\\ucrt64\\bin"
	Call AddOnePath
	Push "\$INSTDIR\\toolchain\\ucrt64\\lib\\emscripten"
	Call AddOnePath
	Call RefreshEnvironment
FunctionEnd

Function AddInstallDirsToMsys2
	Call WriteMsys2ProfileFile64
	Call WriteMsys2ProfileFile32
FunctionEnd

Function WriteMsys2ProfileFile64
	IfFileExists "C:\\msys64\\etc\\profile.d\\*.*" 0 done
	FileOpen \$0 "C:\\msys64\\etc\\profile.d\\freebasic-js.sh" w
	IfErrors done
	Call WriteMsys2ProfileFileContents
	FileClose \$0
	done:
FunctionEnd

Function WriteMsys2ProfileFile32
	IfFileExists "C:\\msys32\\etc\\profile.d\\*.*" 0 done
	FileOpen \$0 "C:\\msys32\\etc\\profile.d\\freebasic-js.sh" w
	IfErrors done
	Call WriteMsys2ProfileFileContents
	FileClose \$0
	done:
FunctionEnd

Function WriteMsys2ProfileFileContents
	FileWrite \$0 "# FreeBASIC JS installer PATH setup$\r$\n"
	FileWrite \$0 "if command -v cygpath >/dev/null 2>&1; then$\r$\n"
	FileWrite \$0 "  _freebasic_js_prefix=\`cygpath -u '\$INSTDIR'\`$\r$\n"
	FileWrite \$0 "else$\r$\n"
	FileWrite \$0 "  _freebasic_js_prefix=/c/freebasic-js$\r$\n"
	FileWrite \$0 "fi$\r$\n"
	FileWrite \$0 "_freebasic_js_toolchain=\$\${_freebasic_js_prefix}/toolchain/ucrt64/bin$\r$\n"
	FileWrite \$0 "_freebasic_js_emscripten=\$\${_freebasic_js_prefix}/toolchain/ucrt64/lib/emscripten$\r$\n"
	FileWrite \$0 "case :\$\$PATH: in$\r$\n"
	FileWrite \$0 "  *:\$\${_freebasic_js_emscripten}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\$\"\$\${_freebasic_js_emscripten}:\$\$PATH\$\" ;;$\r$\n"
	FileWrite \$0 "esac$\r$\n"
	FileWrite \$0 "case :\$\$PATH: in$\r$\n"
	FileWrite \$0 "  *:\$\${_freebasic_js_toolchain}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\$\"\$\${_freebasic_js_toolchain}:\$\$PATH\$\" ;;$\r$\n"
	FileWrite \$0 "esac$\r$\n"
	FileWrite \$0 "case :\$\$PATH: in$\r$\n"
	FileWrite \$0 "  *:\$\${_freebasic_js_prefix}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\$\"\$\${_freebasic_js_prefix}:\$\$PATH\$\" ;;$\r$\n"
	FileWrite \$0 "esac$\r$\n"
	FileWrite \$0 "unset _freebasic_js_prefix _freebasic_js_toolchain _freebasic_js_emscripten$\r$\n"
FunctionEnd

Function un.RemoveOnePath
	Exch \$3
	ReadRegStr \$0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path"
	StrCmp \$0 "" done
	StrCpy \$1 ";\$0;"
	\${UnStrRep} \$1 \$1 ";\$3;" ";"
	\${UnStrRep} \$1 \$1 ";;" ";"
	StrCpy \$0 \$1
	StrCpy \$2 \$0 1
	StrCmp \$2 ";" 0 +2
		StrCpy \$0 \$0 "" 1
	StrLen \$2 \$0
	IntCmp \$2 0 done done done
	IntOp \$2 \$2 - 1
	StrCpy \$4 \$0 1 \$2
	StrCmp \$4 ";" 0 +2
		StrCpy \$0 \$0 \$2
	WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "\$0"
	done:
	Pop \$3
FunctionEnd

Function un.RemoveInstallDirsFromPath
	Push "\$INSTDIR\\toolchain\\ucrt64\\lib\\emscripten"
	Call un.RemoveOnePath
	Push "\$INSTDIR\\toolchain\\ucrt64\\bin"
	Call un.RemoveOnePath
	Push "\$INSTDIR\\toolchain\\cmd-shims"
	Call un.RemoveOnePath
	Push "\$INSTDIR"
	Call un.RemoveOnePath
	Call un.RefreshEnvironment
FunctionEnd

Function un.RemoveInstallDirsFromMsys2
	Delete "C:\\msys64\\etc\\profile.d\\freebasic-js.sh"
	Delete "C:\\msys32\\etc\\profile.d\\freebasic-js.sh"
FunctionEnd

Section "Install"
	InitPluginsDir
	System::Call 'kernel32::GetCurrentProcessId() i.r1'
	System::Call 'kernel32::GetTickCount() i.r2'
	StrCpy \$0 "\$TEMP\\FreeBASIC-${FBVERSION}-js-payload-\$1-\$2"
	ClearErrors
	CreateDirectory "\$0"
	IfErrors payload_temp_failed
	SetOutPath "\$0"
	SetCompress off
	File /oname=freebasic-js-payload.zip "$payload_win"
	SetCompress auto
	IfFileExists "\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" 0 no_powershell
	SetOutPath "\$INSTDIR"
	;
	; The JS package bundles the compiler, Emscripten, Node.js, Binaryen,
	; Python support, and UCRT runtime files.  Passing that expanded tree to
	; NSIS File /r can hit makensis datablock limits, so store it as a normal
	; zip payload and extract it through Windows PowerShell during install.
	FileOpen \$3 "\$0\\extract-payload.ps1" w
	FileWrite \$3 "param([string] \$\$PayloadZip, [string] \$\$Destination)$\r$\n"
	FileWrite \$3 "\$\$ErrorActionPreference = 'Stop'$\r$\n"
	FileWrite \$3 "Expand-Archive -LiteralPath \$\$PayloadZip -DestinationPath \$\$Destination -Force -ErrorAction Stop$\r$\n"
	FileClose \$3
	ExecWait '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "\$0\\extract-payload.ps1" "\$0\\freebasic-js-payload.zip" "\$INSTDIR"' \$3
	StrCmp \$3 "0" payload_done
		Abort "Failed to extract the FreeBASIC JS payload. PowerShell exit code: \$3"
	payload_done:
	;
	; Deleting a multi-gigabyte archive synchronously can remain blocked in a
	; Windows filesystem filter after Expand-Archive has exited.  Keep the
	; payload outside NSIS's automatically removed plug-in directory, schedule
	; it for deletion at reboot, and let an elevated helper remove it after this
	; installer has exited.  The installer can then finish even if a scanner
	; delays the final archive deletion.
	FileOpen \$3 "\$0\\cleanup-payload.ps1" w
	FileWrite \$3 "param([int] \$\$ParentProcessId, [string] \$\$PayloadDir)$\r$\n"
	FileWrite \$3 "\$\$ErrorActionPreference = 'SilentlyContinue'$\r$\n"
	FileWrite \$3 "\$\$Deadline = [DateTime]::UtcNow.AddMinutes(5)$\r$\n"
	FileWrite \$3 "while ([DateTime]::UtcNow -lt \$\$Deadline -and (Get-Process -Id \$\$ParentProcessId -ErrorAction SilentlyContinue)) {$\r$\n"
	FileWrite \$3 "  Start-Sleep -Milliseconds 250$\r$\n"
	FileWrite \$3 "}$\r$\n"
	FileWrite \$3 "try { [IO.File]::Delete((Join-Path \$\$PayloadDir 'freebasic-js-payload.zip')) } catch {}$\r$\n"
	FileWrite \$3 "try { [IO.File]::Delete((Join-Path \$\$PayloadDir 'extract-payload.ps1')) } catch {}$\r$\n"
	FileWrite \$3 "try { [IO.File]::Delete(\$\$PSCommandPath) } catch {}$\r$\n"
	FileWrite \$3 "try { [IO.Directory]::Delete(\$\$PayloadDir, \$\$false) } catch {}$\r$\n"
	FileClose \$3
	System::Call 'kernel32::MoveFileExW(w "\$0\\freebasic-js-payload.zip", p 0, i 4) i.r4'
	System::Call 'kernel32::MoveFileExW(w "\$0\\extract-payload.ps1", p 0, i 4) i.r4'
	System::Call 'kernel32::MoveFileExW(w "\$0\\cleanup-payload.ps1", p 0, i 4) i.r4'
	System::Call 'kernel32::MoveFileExW(w "\$0", p 0, i 4) i.r4'
	Exec '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "\$0\\cleanup-payload.ps1" "\$1" "\$0"'
	WriteUninstaller "\$INSTDIR\\uninstall.exe"
	Call AddInstallDirsToPath
	Call AddInstallDirsToMsys2
	Goto install_done
	payload_temp_failed:
		Abort "Could not create the temporary FreeBASIC JS payload directory."
	no_powershell:
		Abort "Windows PowerShell is required to extract this installer."
	install_done:
SectionEnd

Section "Uninstall"
	Call un.RemoveInstallDirsFromPath
	Call un.RemoveInstallDirsFromMsys2
	Delete "\$INSTDIR\\uninstall.exe"
	RMDir /r "\$INSTDIR"
SectionEnd
EOF

	msg "Creating NSIS installer"
	rm -f "$installer_exe"
	if ! run "$NSIS_EXE" "$installer_nsi"; then
		rm -f "$installer_payload_zip"
		fail "makensis failed while creating fbc-js installer"
	fi
	rm -f "$installer_payload_zip"
}

validate_installer() {
	local installer_exe="$OUT/${DISTNAME}-setup.exe"

	[ "$SKIP_VALIDATE" -eq 0 ] || return 0
	[ "$SKIP_INSTALLER" -eq 0 ] || return 0
	[ "$SKIP_PACKAGE" -eq 0 ] || return 0
	[ -f "$installer_exe" ] || fail "missing installer for smoke test: $installer_exe"

	run bash "$ROOT/build_scripts/msys2-test-freebasic-installer.sh" \
		--installer "$installer_exe" \
		--kind js
}

##############################################################################
# Validation
##############################################################################

validate_distribution() {
	local validate_dir="$BUILDROOT/validate"
	local dist_win
	local validate_win
	local package_path_win
	local validate_cmd
	local validate_ps1

	msg "Validating packaged fbc-js"
	rm -rf "$validate_dir"
	mkdir -p "$validate_dir"

	cat > "$validate_dir/hello.bas" <<'EOF'
print "freebasic-js package test OK"
EOF
	mkdir -p "$validate_dir/assets"
	printf 'asset smoke\n' > "$validate_dir/assets/readme.txt"

	dist_win="$(cygpath -aw "$DISTROOT")"
	validate_win="$(cygpath -aw "$validate_dir")"
	package_path_win="$dist_win\\toolchain\\cmd-shims;$dist_win\\toolchain\\ucrt64\\bin;$dist_win\\toolchain\\ucrt64\\lib\\emscripten;$dist_win;%PATH%"
	validate_cmd="$validate_dir/validate.cmd"
	validate_ps1="$validate_dir/run-with-timeout.ps1"

	cat > "$validate_cmd" <<EOF
@echo off
set "PATH=$package_path_win"
if not exist "$validate_win\\emcc-temp" mkdir "$validate_win\\emcc-temp"
if not exist "$validate_win\\em-cache" mkdir "$validate_win\\em-cache"
set "EMCC_TEMP_DIR=$validate_win\\emcc-temp"
set "EM_CACHE=$validate_win\\em-cache"
set "BINARYEN_CORES=1"
set "EMCC_BATCH_BUILD=0"
call "$dist_win\\fbc-js.cmd" "$validate_win\\hello.bas" -x "$validate_win\\hello.js" > "$validate_win\\compile.out" 2> "$validate_win\\compile.err"
if errorlevel 1 exit /b %ERRORLEVEL%
node "$validate_win\\hello.js" > "$validate_win\\output.txt" 2> "$validate_win\\output.err"
if errorlevel 1 exit /b %ERRORLEVEL%
call "$dist_win\\fbc-js-app.cmd" --out "$validate_win\\app" --assets "$validate_win\\assets" "$validate_win\\hello.bas" > "$validate_win\\app-compile.out" 2> "$validate_win\\app-compile.err"
exit /b %ERRORLEVEL%
EOF

	cat > "$validate_ps1" <<'EOF'
param(
	[string] $CommandPath,
	[int] $TimeoutSeconds
)

$ErrorActionPreference = "Stop"

function Stop-ProcessTree {
	param([int] $ProcessId)

	$children = Get-CimInstance Win32_Process -Filter "ParentProcessId = $ProcessId"
	foreach ($child in $children) {
		Stop-ProcessTree -ProcessId $child.ProcessId
	}

	Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
}

$process = Start-Process `
	-FilePath "cmd.exe" `
	-ArgumentList @("/C", $CommandPath) `
	-PassThru `
	-WindowStyle Hidden

if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
	Stop-ProcessTree -ProcessId $process.Id
	Write-Error "fbc-js package validation timed out after $TimeoutSeconds seconds"
	exit 124
}

exit $process.ExitCode
EOF

	run powershell.exe -NoProfile -ExecutionPolicy Bypass \
		-File "$(cygpath -aw "$validate_ps1")" \
		-CommandPath "$(cygpath -aw "$validate_cmd")" \
		-TimeoutSeconds 1200
	if grep -Eiq '(^|[[:space:]])(warning|error):' \
		"$validate_dir/compile.err" "$validate_dir/app-compile.err"; then
		cat "$validate_dir/compile.err" "$validate_dir/app-compile.err" >&2
		fail "packaged fbc-js emitted compiler warnings or errors"
	fi
	[ -f "$validate_dir/hello.js" ] || fail "packaged fbc-js did not produce hello.js"
	[ -f "$validate_dir/app/index.html" ] || fail "packaged fbc-js-app did not produce app/index.html"
	grep -q "freebasic-js package test OK" "$validate_dir/output.txt" || fail "generated JavaScript output was wrong"
}

##############################################################################
# Main
##############################################################################

if [ "$SKIP_DEPS" -eq 0 ]; then
	install_dependencies
fi

if [ "$SKIP_BUILD" -eq 0 ]; then
	build_freebasic_js
fi

if [ "$SKIP_PACKAGE" -eq 0 ]; then
	assemble_distribution
	create_zip
fi

if [ "$SKIP_INSTALLER" -eq 0 ]; then
	create_installer
	validate_installer
fi

if [ "$SKIP_VALIDATE" -eq 0 ]; then
	validate_distribution
fi

msg "Done"
echo "Distribution root: $DISTROOT"
echo "Zip archive: $OUT/${DISTNAME}.zip"
echo "Installer: $OUT/${DISTNAME}-setup.exe"

# end of msys2-build-freebasic-js.sh
