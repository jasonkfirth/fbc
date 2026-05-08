#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-build-freebasic-android.sh
#
# Build a self-contained Windows FreeBASIC Android distribution from MSYS2.
# Produces a freebasic-android package tree, a .zip archive, and an NSIS
# installer that installs into C:\freebasic-android.
#
# The package contains the fbc-android driver, the Android/aarch64 FreeBASIC
# runtime, Android SDK command line tools, build tools, platform files, and the
# Android NDK used by the build.
##############################################################################

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

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
SKIP_SDK=0
SKIP_BUILD=0
SKIP_PACKAGE=0
SKIP_INSTALLER=0
SKIP_VALIDATE=0
KEEP_BUILDROOT=0

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-build-freebasic-android.sh [options]

Options:
  --skip-deps         Do not install or update MSYS2 packages
  --skip-source-sync  Reuse the existing isolated worktree
  --skip-sdk          Reuse the existing Android SDK/NDK cache
  --skip-build        Skip the fbc-android build
  --skip-package      Skip distribution tree assembly and zip creation
  --skip-installer    Skip NSIS installer creation
  --skip-validate     Skip packaged fbc-android validation
  --keep-buildroot    Keep the build root on failure or success
  --help              Show this help text

Environment:
  BUILDROOT           Temporary build root (default: /tmp/freebasic-android-build)
  OUT                 Output directory (default: <repo>/out/mingw32-android)
  HOST_FBC_ROOT       Optional existing FreeBASIC install used as host compiler fallback
  UCRT64_ROOT         UCRT64 root used for Clang/Java helpers (default: /ucrt64)
  NSIS_EXE            Explicit makensis path (default: /mingw64/bin/makensis.exe)
  JOBS                Parallel make job count (default: detected CPU core count)
  ANDROID_API         NDK API level used for runtime build (default: 26)
  ANDROID_PLATFORM    SDK platform package (default: platforms;android-35)
  ANDROID_BUILDTOOLS  SDK build-tools package (default: build-tools;35.0.1)
  ANDROID_NDK_PACKAGE SDK NDK package (default: ndk;28.0.13004108)
  ANDROID_CMDLINE_TOOLS_URL
                      Android command line tools zip URL
  JAVA_RUNTIME_URL    Portable Windows JDK zip URL
EOF
}

for arg in "$@"; do
	case "$arg" in
		--skip-deps) SKIP_DEPS=1 ;;
		--skip-source-sync) SKIP_SOURCE_SYNC=1 ;;
		--skip-sdk) SKIP_SDK=1 ;;
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
		''|*[!0-9]*) n=1 ;;
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

	if have fbc && fbc -version >/dev/null 2>&1; then
		command -v fbc
		return 0
	fi

	return 1
}

copy_runtime_dlls() {
	local exe="$1"
	local dst="$2"
	local dep

	[ -x "$exe" ] || return 0
	mkdir -p "$dst"

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

copy_msys_tool() {
	local tool="$1"
	local dst="$2"
	local exe

	exe="$(command -v "$tool" 2>/dev/null || true)"
	if [ -n "$exe" ] && [ ! -f "$exe" ]; then
		exe=""
	fi
	if [ -z "$exe" ] && [ -f "/usr/bin/$tool.exe" ]; then
		exe="/usr/bin/$tool.exe"
	fi
	[ -n "$exe" ] || fail "required MSYS2 tool not found: $tool"
	cp -a "$exe" "$dst/"
	copy_runtime_dlls "$exe" "$dst"
}

##############################################################################
# Configuration
##############################################################################

FBVERSION="$(extract_var FBVERSION)"
[ -n "$FBVERSION" ] || fail "could not determine FBVERSION"

BUILDROOT="${BUILDROOT:-/tmp/freebasic-android-build}"
WORKROOT="$BUILDROOT/work"
STAGEROOT="$BUILDROOT/stage"
DISTROOT_BASE="$BUILDROOT/dist"
OUT="${OUT:-$ROOT/out/mingw32-android}"
INSTALL_DIR_WIN="${INSTALL_DIR_WIN:-C:\\freebasic-android}"
INSTALL_SUBDIR="${INSTALL_SUBDIR:-freebasic-android}"
UCRT64_ROOT="${UCRT64_ROOT:-/ucrt64}"
NSIS_EXE="${NSIS_EXE:-/mingw64/bin/makensis.exe}"
JOBS="${JOBS:-$(max_jobs)}"

ANDROID_API="${ANDROID_API:-26}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-platforms;android-35}"
ANDROID_BUILDTOOLS="${ANDROID_BUILDTOOLS:-build-tools;35.0.1}"
ANDROID_NDK_PACKAGE="${ANDROID_NDK_PACKAGE:-ndk;28.0.13004108}"
ANDROID_CMDLINE_TOOLS_URL="${ANDROID_CMDLINE_TOOLS_URL:-https://dl.google.com/android/repository/commandlinetools-win-13114758_latest.zip}"
JAVA_RUNTIME_URL="${JAVA_RUNTIME_URL:-https://api.adoptium.net/v3/binary/latest/21/ga/windows/x64/jdk/hotspot/normal/eclipse}"
ANDROID_TARGET_TRIPLET="${ANDROID_TARGET_TRIPLET:-aarch64-linux-android}"
ANDROID_TARGET_KEY="${ANDROID_TARGET_KEY:-android-aarch64}"

HOST_TRIPLET="$("$UCRT64_ROOT/bin/gcc" -dumpmachine 2>/dev/null || true)"
if [ -z "$HOST_TRIPLET" ]; then
	HOST_TRIPLET="x86_64-w64-mingw32"
fi

SDKROOT="$BUILDROOT/android-sdk"
CMDLINE_ROOT="$SDKROOT/cmdline-tools/latest"
SDKMANAGER="$CMDLINE_ROOT/bin/sdkmanager.bat"
JAVA_ROOT="$BUILDROOT/java"
WORKTREE="$WORKROOT/android"
STAGEDIR="$STAGEROOT/fbc-android"
DISTNAME="FreeBASIC-${FBVERSION}-fbc-android"
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
	msg "Installing MSYS2 packages needed for fbc-android"

	run pacman -Syu --needed --noconfirm
	run pacman -S --needed --noconfirm \
		base-devel \
		rsync \
		unzip \
		zip \
		p7zip \
		wget \
		mingw-w64-ucrt-x86_64-gcc \
		mingw-w64-x86_64-nsis
}

##############################################################################
# Android SDK and NDK
##############################################################################

download_file() {
	local url="$1"
	local dst="$2"

	if have curl; then
		run curl -L --fail -o "$dst" "$url"
	elif have wget; then
		run wget -O "$dst" "$url"
	else
		fail "curl or wget is required to download Android tools"
	fi
}

ensure_java_runtime() {
	local zipfile="$BUILDROOT/java-runtime.zip"
	local extract_root="$BUILDROOT/java-extract"
	local found

	if [ -x "$JAVA_ROOT/bin/java.exe" ] && [ -x "$JAVA_ROOT/bin/jar.exe" ]; then
		return 0
	fi

	msg "Installing portable Java runtime"
	rm -rf "$JAVA_ROOT" "$extract_root"
	mkdir -p "$extract_root"
	download_file "$JAVA_RUNTIME_URL" "$zipfile"
	run unzip -q "$zipfile" -d "$extract_root"

	found="$(find "$extract_root" -mindepth 2 -maxdepth 3 -type f -name java.exe -print | head -n1 || true)"
	[ -n "$found" ] || fail "downloaded Java runtime did not contain java.exe"

	mv "$(dirname "$(dirname "$found")")" "$JAVA_ROOT"
	[ -x "$JAVA_ROOT/bin/java.exe" ] || fail "Java runtime was not staged correctly"
	[ -x "$JAVA_ROOT/bin/jar.exe" ] || fail "Java JDK jar tool was not staged correctly"
}

ensure_commandline_tools() {
	local zipfile="$BUILDROOT/commandlinetools-win.zip"

	[ "$SKIP_SDK" -eq 0 ] || return 0
	if [ -f "$CMDLINE_ROOT/bin/sdkmanager.bat" ]; then
		return 0
	fi

	msg "Installing Android command line tools"
	rm -rf "$SDKROOT/cmdline-tools"
	mkdir -p "$SDKROOT/cmdline-tools"
	download_file "$ANDROID_CMDLINE_TOOLS_URL" "$zipfile"
	run unzip -q "$zipfile" -d "$SDKROOT/cmdline-tools"

	if [ -d "$SDKROOT/cmdline-tools/cmdline-tools" ]; then
		mv "$SDKROOT/cmdline-tools/cmdline-tools" "$CMDLINE_ROOT"
	fi

	[ -f "$CMDLINE_ROOT/bin/sdkmanager.bat" ] || fail "sdkmanager.bat was not installed"
}

ensure_android_sdk() {
	ensure_java_runtime
	ensure_commandline_tools

	[ -f "$SDKMANAGER" ] || fail "sdkmanager.bat not found: $SDKMANAGER"
	[ -x "$JAVA_ROOT/bin/java.exe" ] || fail "Java not found at $JAVA_ROOT/bin/java.exe"
	[ -x "$JAVA_ROOT/bin/jar.exe" ] || fail "jar not found at $JAVA_ROOT/bin/jar.exe"

	if [ "$SKIP_SDK" -eq 1 ]; then
		return 0
	fi

	msg "Installing Android SDK/NDK packages"
	export JAVA_HOME="$JAVA_ROOT"
	export ANDROID_HOME="$SDKROOT"
	export ANDROID_SDK_ROOT="$SDKROOT"
	printf 'y\n%.0s' {1..1000} | "$SDKMANAGER" --sdk_root="$SDKROOT" --licenses >/dev/null || true
	run "$SDKMANAGER" --sdk_root="$SDKROOT" \
		"cmdline-tools;latest" \
		"platform-tools" \
		"$ANDROID_PLATFORM" \
		"$ANDROID_BUILDTOOLS" \
		"$ANDROID_NDK_PACKAGE"
}

find_ndk_root() {
	local candidate

	shopt -s nullglob
	for candidate in "$SDKROOT"/ndk/* "$SDKROOT"/ndk-bundle; do
		[ -d "$candidate/toolchains/llvm/prebuilt" ] || continue
		echo "$candidate"
		shopt -u nullglob
		return 0
	done
	shopt -u nullglob
	return 1
}

find_ndk_prebuilt() {
	local ndk="$1"
	local candidate

	shopt -s nullglob
	for candidate in "$ndk"/toolchains/llvm/prebuilt/windows-x86_64 "$ndk"/toolchains/llvm/prebuilt/*; do
		[ -d "$candidate/bin" ] || continue
		echo "$candidate"
		shopt -u nullglob
		return 0
	done
	shopt -u nullglob
	return 1
}

##############################################################################
# Build
##############################################################################

prepare_worktree() {
	if [ "$SKIP_SOURCE_SYNC" -eq 0 ]; then
		msg "Creating isolated fbc-android worktree"
		rm -rf "$WORKTREE"
		sync_source_tree "$WORKTREE"
		(
			cd "$WORKTREE"
			rm -rf bin bootstrap
			sanitize_source_tree "$HOST_TRIPLET"
		)
	fi
}

build_android_target() {
	local bootstrap_sources_dir="$WORKTREE/bootstrap/win64"
	local seed_fbc=""
	local build_fbc
	local ndk
	local prebuilt
	local host_cc="$UCRT64_ROOT/bin/gcc.exe"
	local host_cxx="$UCRT64_ROOT/bin/g++.exe"
	local host_ar="$UCRT64_ROOT/bin/ar.exe"
	local host_as="$UCRT64_ROOT/bin/as.exe"
	local host_ld="$UCRT64_ROOT/bin/ld.exe"
	local host_ranlib="$UCRT64_ROOT/bin/ranlib.exe"
	local host_strip="$UCRT64_ROOT/bin/strip.exe"
	local host_dlltool="$UCRT64_ROOT/bin/dlltool.exe"
	local cc
	local cxx
	local ar
	local ranlib

	if [ "$SKIP_BUILD" -eq 1 ]; then
		return 0
	fi

	msg "Building fbc-android and Android runtime"
	cd "$WORKTREE"

	seed_fbc="$(detect_fbc \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc64.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc}" \
		"$WORKTREE/bin/fbc.exe" \
		"$WORKTREE/bootstrap/fbc.exe" \
		"$ROOT/bin/fbc.exe" \
		"$ROOT/bootstrap/fbc.exe" \
		|| true)"

	if [ -n "$seed_fbc" ]; then
		msg "Emitting fresh win64 bootstrap sources"
		rm -rf "$bootstrap_sources_dir"
		run make -j"$JOBS" \
			bootstrap-emit \
			FBC_EXE="$seed_fbc" \
			BUILD_FBC="$seed_fbc" \
			TARGET_TRIPLET="$HOST_TRIPLET" \
			CC="$host_cc" CXX="$host_cxx" AR="$host_ar" AS="$host_as" LD="$host_ld" RANLIB="$host_ranlib" STRIP="$host_strip" DLLTOOL="$host_dlltool"
	elif [ -d "$bootstrap_sources_dir" ] && find "$bootstrap_sources_dir" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
		msg "Bootstrap sources already present for win64"
	else
		msg "No direct bootstrap compiler available; seeding from peer bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-seed-peer \
			TARGET_TRIPLET="$HOST_TRIPLET" \
			CC="$host_cc" CXX="$host_cxx" AR="$host_ar" AS="$host_as" LD="$host_ld" RANLIB="$host_ranlib" STRIP="$host_strip" DLLTOOL="$host_dlltool"
	fi

	run make clean TARGET_TRIPLET="$HOST_TRIPLET" || true

	msg "Building host bootstrap compiler"
	run make -j"$JOBS" \
		bootstrap-minimal \
		TARGET_TRIPLET="$HOST_TRIPLET" \
		CC="$host_cc" CXX="$host_cxx" AR="$host_ar" AS="$host_as" LD="$host_ld" RANLIB="$host_ranlib" STRIP="$host_strip" DLLTOOL="$host_dlltool"

	[ -f "$WORKTREE/bootstrap/fbc.exe" ] || fail "bootstrap-minimal did not produce bootstrap/fbc.exe"
	build_fbc="$WORKTREE/bin/fbc.exe"
	[ -f "$build_fbc" ] || fail "bootstrap-minimal did not install bin/fbc.exe"

	ndk="$(find_ndk_root)" || fail "Android NDK not found under $SDKROOT"
	prebuilt="$(find_ndk_prebuilt "$ndk")" || fail "Android NDK LLVM prebuilt toolchain not found"
	cc="$prebuilt/bin/${ANDROID_TARGET_TRIPLET}${ANDROID_API}-clang"
	cxx="$prebuilt/bin/${ANDROID_TARGET_TRIPLET}${ANDROID_API}-clang++"
	ar="$prebuilt/bin/llvm-ar"
	ranlib="$prebuilt/bin/llvm-ranlib"

	[ -x "$cc" ] || cc="$cc.exe"
	[ -x "$cc" ] || cc="${cc%.exe}.cmd"
	[ -x "$cxx" ] || cxx="$cxx.exe"
	[ -x "$cxx" ] || cxx="${cxx%.exe}.cmd"
	[ -x "$ar" ] || ar="$ar.exe"
	[ -x "$ranlib" ] || ranlib="$ranlib.exe"

	[ -x "$cc" ] || fail "Android clang not found: $cc"
	[ -x "$ar" ] || fail "Android llvm-ar not found: $ar"
	[ -x "$ranlib" ] || fail "Android llvm-ranlib not found: $ranlib"

	run make TARGET_TRIPLET="$HOST_TRIPLET" TARGET="$HOST_TRIPLET" \
		compiler-android \
		BUILD_FBC="$build_fbc" \
		BUILD_FBC_TARGET=win64 \
		BUILD_FBCFLAGS= \
		CC="$host_cc" CXX="$host_cxx" AR="$host_ar" AS="$host_as" LD="$host_ld" RANLIB="$host_ranlib" STRIP="$host_strip" DLLTOOL="$host_dlltool" \
		CPPFLAGS= \
		CFLAGS= \
		CXXFLAGS= \
		LDFLAGS= \
		-j"$JOBS"

	run make TARGET_TRIPLET="$ANDROID_TARGET_TRIPLET" TARGET="$ANDROID_TARGET_TRIPLET" \
		MULTILIB= \
		FBTARGET_DIR_OVERRIDE="$ANDROID_TARGET_KEY" \
		BUILD_PREFIX= \
		CC="$cc" \
		CXX="$cxx" \
		CLANG="$cc" \
		AS="$cc" \
		LD="$cc" \
		AR="$ar" \
		RANLIB="$ranlib" \
		BUILD_FBC="$build_fbc" \
		BUILD_FBC_TARGET="$ANDROID_TARGET_KEY" \
		BUILD_FBC_BUILDPREFIX= \
		CPPFLAGS= \
		CFLAGS= \
		CXXFLAGS= \
		LDFLAGS= \
		rtlib fbrt gfxlib2 sfxlib \
		-j"$JOBS"

	rm -rf "$STAGEDIR"
	run make TARGET_TRIPLET="$HOST_TRIPLET" TARGET="$HOST_TRIPLET" \
		DESTDIR="$STAGEDIR" \
		prefix="/$INSTALL_SUBDIR" \
		BUILD_FBC="$build_fbc" \
		BUILD_FBC_TARGET=win64 \
		BUILD_FBCFLAGS= \
		ANDROID_BUILD_LIBDIR="$WORKTREE/lib/freebasic/$ANDROID_TARGET_KEY" \
		install-android

	[ -f "$STAGEDIR/fbc-android.exe" ] || fail "staged fbc-android wrapper is missing"
	[ -x "$STAGEDIR/lib/freebasic-android/bin/fbc-android-compiler.exe" ] ||
		fail "staged fbc-android compiler is missing"
}

##############################################################################
# Distribution
##############################################################################

copy_toolchain() {
	msg "Bundling Android SDK/NDK"
	mkdir -p "$DISTROOT/toolchain"
	if have rsync; then
		run rsync -a --delete \
			--exclude '/.android/' \
			--exclude '/cache/' \
			"$SDKROOT/" "$DISTROOT/toolchain/android-sdk/"
	else
		copy_tree "$SDKROOT" "$DISTROOT/toolchain/android-sdk"
	fi

	msg "Bundling Java runtime"
	copy_tree "$JAVA_ROOT" "$DISTROOT/toolchain/java"
}

copy_msys_runtime() {
	local dst="$DISTROOT/toolchain/msys2/usr/bin"

	msg "Bundling minimal MSYS2 shell runtime"
	mkdir -p "$dst"
	copy_msys_tool bash "$dst"
	copy_msys_tool sed "$dst"
	copy_msys_tool tr "$dst"
	copy_msys_tool mkdir "$dst"
	copy_msys_tool dirname "$dst"
	copy_msys_tool pwd "$dst"
	copy_msys_tool cp "$dst"
	copy_msys_tool rm "$dst"
	copy_msys_tool find "$dst"
	copy_msys_tool uname "$dst"
	copy_msys_tool env "$dst"
	copy_msys_tool sh "$dst"
	mkdir -p "$DISTROOT/toolchain/msys2/tmp"
}

write_launchers() {
	msg "Writing fbc-android launcher scripts"

	cat > "$DISTROOT/fbc-android-package.sh" <<'EOF'
#!/usr/bin/env bash

set -euo pipefail

root="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
PATH="$root/toolchain/msys2/usr/bin:$root/toolchain/java/bin:$root/toolchain/android-sdk/platform-tools:$PATH"
JAVA_HOME="$root/toolchain/java"
ANDROID_HOME="$root/toolchain/android-sdk"
ANDROID_SDK_ROOT="$ANDROID_HOME"
FBANDROID_PREFIX="$root"
FBANDROID_LIBROOT="$root/lib/freebasic-android"
FBANDROID_COMPILER="$root/lib/freebasic-android/bin/fbc-android-compiler.exe"
FBANDROID_INCDIR="$root/include/freebasic-android"
FBANDROID_SHARE="$root/share/freebasic-android"
export PATH JAVA_HOME ANDROID_HOME ANDROID_SDK_ROOT
export FBANDROID_PREFIX FBANDROID_LIBROOT FBANDROID_COMPILER FBANDROID_INCDIR FBANDROID_SHARE

exec "$root/fbc-android.exe" "$@"
EOF
	chmod 755 "$DISTROOT/fbc-android-package.sh"

	cat > "$DISTROOT/fbc-android.cmd" <<'EOF'
@echo off
setlocal
set "FBANDROID_ROOT=%~dp0"
set "PATH=%FBANDROID_ROOT%toolchain\msys2\usr\bin;%PATH%"
"%FBANDROID_ROOT%toolchain\msys2\usr\bin\bash.exe" "%FBANDROID_ROOT%fbc-android-package.sh" %*
exit /b %ERRORLEVEL%
EOF

	cat > "$DISTROOT/freebasic-android-env.cmd" <<'EOF'
@echo off
set "FBANDROID_ROOT=%~dp0"
set "PATH=%FBANDROID_ROOT%toolchain\msys2\usr\bin;%FBANDROID_ROOT%toolchain\java\bin;%FBANDROID_ROOT%toolchain\android-sdk\platform-tools;%PATH%"
set "JAVA_HOME=%FBANDROID_ROOT%toolchain\java"
set "ANDROID_HOME=%FBANDROID_ROOT%toolchain\android-sdk"
set "ANDROID_SDK_ROOT=%FBANDROID_ROOT%toolchain\android-sdk"
set "FBANDROID_PREFIX=%FBANDROID_ROOT%"
set "FBANDROID_LIBROOT=%FBANDROID_ROOT%lib\freebasic-android"
set "FBANDROID_COMPILER=%FBANDROID_ROOT%lib\freebasic-android\bin\fbc-android-compiler.exe"
set "FBANDROID_INCDIR=%FBANDROID_ROOT%include\freebasic-android"
set "FBANDROID_SHARE=%FBANDROID_ROOT%share\freebasic-android"
echo FreeBASIC Android environment ready.
echo fbc-android: %FBANDROID_ROOT%fbc-android.cmd
cmd /k
EOF

	cat > "$DISTROOT/freebasic-android-env.sh" <<'EOF'
#!/usr/bin/env sh

_fbandroid_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PATH="${_fbandroid_root}/toolchain/msys2/usr/bin:${_fbandroid_root}/toolchain/java/bin:${_fbandroid_root}/toolchain/android-sdk/platform-tools:${PATH}"
JAVA_HOME="${_fbandroid_root}/toolchain/java"
ANDROID_HOME="${_fbandroid_root}/toolchain/android-sdk"
ANDROID_SDK_ROOT="${ANDROID_HOME}"
FBANDROID_PREFIX="${_fbandroid_root}"
FBANDROID_LIBROOT="${_fbandroid_root}/lib/freebasic-android"
FBANDROID_COMPILER="${_fbandroid_root}/lib/freebasic-android/bin/fbc-android-compiler.exe"
FBANDROID_INCDIR="${_fbandroid_root}/include/freebasic-android"
FBANDROID_SHARE="${_fbandroid_root}/share/freebasic-android"
export PATH JAVA_HOME ANDROID_HOME ANDROID_SDK_ROOT
export FBANDROID_PREFIX FBANDROID_LIBROOT FBANDROID_COMPILER FBANDROID_INCDIR FBANDROID_SHARE
unset _fbandroid_root
EOF
	chmod 755 "$DISTROOT/freebasic-android-env.sh"
}

write_distribution_notes() {
	msg "Writing fbc-android package notes"

	cat > "$DISTROOT/readme-fbc-android.txt" <<EOF
FreeBASIC Android ${FBVERSION}

This package is intended to run without a separate Android SDK, Android NDK, or
MSYS2 installation.

The installer adds this directory to the Windows system PATH:

    ${INSTALL_DIR_WIN}

Use fbc-android.cmd from cmd.exe or PowerShell:

    fbc-android.cmd --target-api 35 --package org.example.hello hello.bas

The toolchain directory contains the Android SDK and NDK packages installed by
this build script.  The package also includes a small MSYS2 shell runtime
because the tracked fbc-android driver is a shell script.
EOF
}

assemble_distribution() {
	local stage_prefix="$STAGEDIR"

	if [ "$SKIP_PACKAGE" -eq 1 ]; then
		return 0
	fi

	rm -rf "$DISTROOT"
	mkdir -p "$DISTROOT"

	msg "Copying fbc-android staged files"
	copy_tree "$stage_prefix" "$DISTROOT"
	cp -a "$ROOT/src/tools/android/fbc-android" "$DISTROOT/fbc-android.exe"
	chmod 755 "$DISTROOT/fbc-android.exe"
	mkdir -p "$DISTROOT/bin"
	copy_runtime_dlls "$DISTROOT/lib/freebasic-android/bin/fbc-android-compiler.exe" "$DISTROOT/lib/freebasic-android/bin"

	msg "Copying top-level documentation and examples"
	copy_tree "$ROOT/doc" "$DISTROOT/doc"
	copy_examples_tree "$DISTROOT/examples"
	cp -a "$ROOT/changelog.txt" "$DISTROOT/"
	cp -a "$ROOT/readme.txt" "$DISTROOT/"

	copy_toolchain
	copy_msys_runtime
	write_launchers
	write_distribution_notes
}

##############################################################################
# Packaging
##############################################################################

create_zip() {
	local zipfile="$OUT/${DISTNAME}.zip"

	[ "$SKIP_PACKAGE" -eq 0 ] || return 0
	msg "Creating fbc-android distribution zip"
	rm -f "$zipfile"
	(
		cd "$DISTROOT_BASE"
		run zip -qr "$zipfile" "$DISTNAME"
	)
}

create_installer() {
	local installer_nsi="$BUILDROOT/${DISTNAME}.nsi"
	local installer_exe="$OUT/${DISTNAME}-setup.exe"
	local nsis_src="${NSIS_SRCROOT:-/tmp/fba}"
	local dist_win
	local out_win

	[ "$SKIP_INSTALLER" -eq 0 ] || return 0
	[ "$SKIP_PACKAGE" -eq 0 ] || return 0
	[ -x "$NSIS_EXE" ] || fail "makensis not found at $NSIS_EXE; install the nsis package or set NSIS_EXE"
	have cygpath || fail "cygpath not found"

	msg "Preparing short NSIS source path"
	rm -rf "$nsis_src"
	copy_tree "$DISTROOT" "$nsis_src"

	dist_win="$(cygpath -aw "$nsis_src")"
	out_win="$(cygpath -aw "$installer_exe")"

	msg "Generating NSIS installer script"
	cat > "$installer_nsi" <<EOF
Unicode true
SetCompressor zlib
RequestExecutionLevel admin

Name "FreeBASIC Android ${FBVERSION}"
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
\${Using:StrFunc} StrRep
\${Using:StrFunc} UnStrRep

Function RefreshEnvironment
	System::Call 'User32::SendMessageTimeoutA(i 0xffff, i \${WM_SETTINGCHANGE}, i 0, t "Environment", i 0, i 5000, *i .r0)'
FunctionEnd

Function un.RefreshEnvironment
	System::Call 'User32::SendMessageTimeoutA(i 0xffff, i \${WM_SETTINGCHANGE}, i 0, t "Environment", i 0, i 5000, *i .r0)'
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

Function AddInstallDirToPath
	Push "\$INSTDIR"
	Call AddOnePath
	Call RefreshEnvironment
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

Function un.RemoveInstallDirFromPath
	Push "\$INSTDIR"
	Call un.RemoveOnePath
	Call un.RefreshEnvironment
FunctionEnd

Section "Install"
	SetOutPath "\$INSTDIR"
	File /r "$dist_win\\*"
	WriteUninstaller "\$INSTDIR\\uninstall.exe"
	Call AddInstallDirToPath
SectionEnd

Section "Uninstall"
	Call un.RemoveInstallDirFromPath
	Delete "\$INSTDIR\\uninstall.exe"
	RMDir /r "\$INSTDIR"
SectionEnd
EOF

	msg "Creating NSIS installer"
	run "$NSIS_EXE" "$installer_nsi"
}

##############################################################################
# Validation
##############################################################################

validate_distribution() {
	local validate_dir="$BUILDROOT/validate"
	local dist_win
	local validate_win
	local validate_cmd

	[ "$SKIP_VALIDATE" -eq 0 ] || return 0
	[ "$SKIP_PACKAGE" -eq 0 ] || return 0

	msg "Validating packaged fbc-android"
	rm -rf "$validate_dir"
	mkdir -p "$validate_dir"

	cat > "$validate_dir/hello.bas" <<'EOF'
print "freebasic-android package test OK"
EOF

	dist_win="$(cygpath -aw "$DISTROOT")"
	validate_win="$(cygpath -aw "$validate_dir")"
	validate_cmd="$validate_dir/validate.cmd"

	cat > "$validate_cmd" <<EOF
@echo off
call "$dist_win\\fbc-android.cmd" --target-api 35 --package org.freebasic.validate "$validate_win\\hello.bas" -x "$validate_win\\hello.apk"
exit /b %ERRORLEVEL%
EOF

	run cmd.exe //C "$(cygpath -aw "$validate_cmd")"
	[ -f "$validate_dir/hello.apk" ] || fail "packaged fbc-android did not produce hello.apk"
}

##############################################################################
# Main
##############################################################################

if [ "$SKIP_DEPS" -eq 0 ]; then
	install_dependencies
fi

ensure_android_sdk
prepare_worktree
build_android_target
assemble_distribution
create_zip
create_installer
validate_distribution

echo ""
echo "FreeBASIC Android build complete."
echo "Distribution root: $DISTROOT"
echo "Artifacts: $OUT"

# end of msys2-build-freebasic-android.sh
