#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-build-freebasic-xbox.sh
#
# Build a self-contained Windows FreeBASIC Xbox distribution from MSYS2.
# Produces a freebasic-xbox package tree, a .zip archive, and an NSIS installer
# that installs into C:\freebasic-xbox.
#
# The package contains fbc.exe, a fbc-xbox launcher, the Xbox runtime, headers,
# examples, and the nxdk tree used to build original Xbox binaries.
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
SKIP_NXDK=0
SKIP_BUILD=0
SKIP_PACKAGE=0
SKIP_INSTALLER=0
SKIP_VALIDATE=0
KEEP_BUILDROOT=0
NXDK_DIR_ARG=""

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-build-freebasic-xbox.sh [options]

Options:
  --skip-deps         Do not install or update MSYS2 packages
  --skip-source-sync  Reuse the existing isolated worktree
  --skip-nxdk         Reuse the existing nxdk checkout
  --skip-build        Skip the Xbox target build
  --skip-package      Skip distribution tree assembly and zip creation
  --skip-installer    Skip NSIS installer creation
  --skip-validate     Skip packaged fbc-xbox validation
  --keep-buildroot    Keep the build root on failure or success
  --nxdk-dir DIR      Existing or desired nxdk checkout
  --help              Show this help text

Environment:
  BUILDROOT           Temporary build root (default: /tmp/freebasic-xbox-build)
  OUT                 Output directory (default: <repo>/out/mingw32-xbox)
  HOST_FBC_ROOT       Optional existing FreeBASIC install used as host compiler fallback
  MINGW64_ROOT        MinGW64 root used for nxdk/host helpers (default: /mingw64)
  NSIS_EXE            Explicit makensis path (default: /mingw64/bin/makensis.exe)
  JOBS                Parallel make job count (default: detected CPU core count)
  NXDK_GIT_JOBS       Parallel git submodule jobs (default: JOBS)
  NXDK_REPO           nxdk Git URL (default: https://github.com/XboxDev/nxdk.git)
  NXDK_REF            Optional nxdk branch/tag/commit to checkout
  NXDK_SHALLOW        Use shallow nxdk/submodule clones when NXDK_REF is unset (default: 1)
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--skip-deps) SKIP_DEPS=1; shift ;;
		--skip-source-sync) SKIP_SOURCE_SYNC=1; shift ;;
		--skip-nxdk) SKIP_NXDK=1; shift ;;
		--skip-build) SKIP_BUILD=1; shift ;;
		--skip-package) SKIP_PACKAGE=1; shift ;;
		--skip-installer) SKIP_INSTALLER=1; shift ;;
		--skip-validate) SKIP_VALIDATE=1; shift ;;
		--keep-buildroot) KEEP_BUILDROOT=1; shift ;;
		--nxdk-dir) NXDK_DIR_ARG="$2"; shift 2 ;;
		--help)
			usage
			exit 0
			;;
		*)
			echo "ERROR: unknown option: $1" >&2
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
	local found

	[ -x "$exe" ] || return 0
	mkdir -p "$dst"

	while IFS= read -r dep; do
		[ -n "$dep" ] || continue
		case "$dep" in
			/ucrt64/*|/mingw64/*|/usr/bin/*)
				[ -f "$dep" ] && cp -a "$dep" "$dst/"
				;;
			*.dll)
				for found in \
					"$MINGW64_ROOT/bin/$dep" \
					"/ucrt64/bin/$dep" \
					"/usr/bin/$dep"
				do
					if [ -f "$found" ]; then
						cp -a "$found" "$dst/"
						break
					fi
				done
				;;
		esac
	done < <(PATH="$MINGW64_ROOT/bin:/usr/bin:$PATH" ldd "$exe" | awk '
		/=> not found/ { print $1; next }
		/=>/ { print $(NF - 1); next }
		/^\// { print $1; next }
	')
}

##############################################################################
# Configuration
##############################################################################

FBVERSION="$(extract_var FBVERSION)"
[ -n "$FBVERSION" ] || fail "could not determine FBVERSION"

BUILDROOT="${BUILDROOT:-/tmp/freebasic-xbox-build}"
WORKROOT="$BUILDROOT/work"
DISTROOT_BASE="$BUILDROOT/dist"
OUT="${OUT:-$ROOT/out/mingw32-xbox}"
INSTALL_DIR_WIN="${INSTALL_DIR_WIN:-C:\\freebasic-xbox}"
INSTALL_SUBDIR="${INSTALL_SUBDIR:-freebasic-xbox}"
MINGW64_ROOT="${MINGW64_ROOT:-/mingw64}"
NSIS_EXE="${NSIS_EXE:-/mingw64/bin/makensis.exe}"
JOBS="${JOBS:-$(max_jobs)}"
NXDK_GIT_JOBS="${NXDK_GIT_JOBS:-$JOBS}"

NXDK_REPO="${NXDK_REPO:-https://github.com/XboxDev/nxdk.git}"
NXDK_SHALLOW="${NXDK_SHALLOW:-1}"
NXDK_DIR="${NXDK_DIR_ARG:-${NXDK_DIR:-$BUILDROOT/nxdk}}"
XBOX_TARGET_TRIPLET="${XBOX_TARGET_TRIPLET:-i686-pc-xbox}"
XBOX_TARGET_KEY="${XBOX_TARGET_KEY:-xbox}"

HOST_TRIPLET="$("$MINGW64_ROOT/bin/gcc" -dumpmachine 2>/dev/null || true)"
if [ -z "$HOST_TRIPLET" ]; then
	HOST_TRIPLET="x86_64-w64-mingw32"
fi

WORKTREE="$WORKROOT/xbox"
DISTNAME="FreeBASIC-${FBVERSION}-fbc-xbox"
DISTROOT="$DISTROOT_BASE/$DISTNAME"

mkdir -p "$BUILDROOT" "$WORKROOT" "$DISTROOT_BASE" "$OUT"

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
	msg "Installing MSYS2 packages needed for fbc-xbox"

	run pacman -Syu --needed --noconfirm
	run pacman -S --needed --noconfirm \
		base-devel \
		rsync \
		unzip \
		zip \
		git \
		cmake \
		make \
		clang \
		lld \
		llvm \
		mingw-w64-x86_64-gcc \
		mingw-w64-ucrt-x86_64-gcc \
		mingw-w64-x86_64-nsis
}

##############################################################################
# nxdk
##############################################################################

ensure_nxdk() {
	msg "Preparing nxdk"
	mkdir -p "$(dirname "$NXDK_DIR")"

	if [ -d "$NXDK_DIR/.git" ]; then
		if [ "$SKIP_NXDK" -eq 0 ]; then
			if [ "$NXDK_SHALLOW" = "1" ] && [ -z "${NXDK_REF:-}" ]; then
				run env GIT_TERMINAL_PROMPT=0 \
					git -C "$NXDK_DIR" submodule update --init --recursive --depth 1 --jobs "$NXDK_GIT_JOBS"
			else
				run env GIT_TERMINAL_PROMPT=0 \
					git -C "$NXDK_DIR" submodule update --init --recursive --jobs "$NXDK_GIT_JOBS"
			fi
		fi
	elif [ -f "$NXDK_DIR/bin/activate" ] && [ -f "$NXDK_DIR/bin/nxdk-cc" ]; then
		echo "==> using installed nxdk tree: $NXDK_DIR"
	else
		[ "$SKIP_NXDK" -eq 0 ] || fail "nxdk checkout not found: $NXDK_DIR"
		if [ "$NXDK_SHALLOW" = "1" ] && [ -z "${NXDK_REF:-}" ]; then
			run env GIT_TERMINAL_PROMPT=0 \
				git clone --depth 1 --recursive --shallow-submodules --jobs "$NXDK_GIT_JOBS" "$NXDK_REPO" "$NXDK_DIR"
		else
			run env GIT_TERMINAL_PROMPT=0 \
				git clone --recursive --jobs "$NXDK_GIT_JOBS" "$NXDK_REPO" "$NXDK_DIR"
		fi
	fi

	if [ -n "${NXDK_REF:-}" ]; then
		[ -d "$NXDK_DIR/.git" ] || fail "NXDK_REF requires an nxdk git checkout: $NXDK_DIR"
		run env GIT_TERMINAL_PROMPT=0 git -C "$NXDK_DIR" fetch --tags origin
		run git -C "$NXDK_DIR" checkout "$NXDK_REF"
		run env GIT_TERMINAL_PROMPT=0 \
			git -C "$NXDK_DIR" submodule update --init --recursive --jobs "$NXDK_GIT_JOBS"
	fi

	[ -x "$NXDK_DIR/bin/activate" ] || fail "nxdk activation script not found: $NXDK_DIR/bin/activate"
}

ensure_nxdk_tools() {
	msg "Checking nxdk host tools"

	if [ ! -f "$NXDK_DIR/tools/cxbe/cxbe.exe" ] && [ ! -f "$NXDK_DIR/tools/cxbe/cxbe" ]; then
		[ -f "$NXDK_DIR/Makefile" ] || fail "nxdk cxbe is missing and nxdk Makefile was not found: $NXDK_DIR"
		run env MSYSTEM=MINGW64 PATH="$MINGW64_ROOT/bin:/usr/bin:$PATH" make -C "$NXDK_DIR" cxbe
	fi

	if [ ! -f "$NXDK_DIR/tools/cxbe/cxbe.exe" ] && [ ! -f "$NXDK_DIR/tools/cxbe/cxbe" ]; then
		fail "nxdk cxbe was not built"
	fi

	if [ ! -f "$NXDK_DIR/tools/extract-xiso/build/extract-xiso.exe" ] && \
	   [ ! -f "$NXDK_DIR/tools/extract-xiso/build/extract-xiso" ]; then
		[ -f "$NXDK_DIR/Makefile" ] || fail "nxdk extract-xiso is missing and nxdk Makefile was not found: $NXDK_DIR"
		run env MSYSTEM=MINGW64 PATH="$MINGW64_ROOT/bin:/usr/bin:$PATH" make -C "$NXDK_DIR" extract-xiso
	fi

	if [ ! -f "$NXDK_DIR/tools/extract-xiso/build/extract-xiso.exe" ] && \
	   [ ! -f "$NXDK_DIR/tools/extract-xiso/build/extract-xiso" ]; then
		fail "nxdk extract-xiso was not built"
	fi
}

build_nxdk_runtime_libs() {
	[ "$SKIP_BUILD" -eq 0 ] || return 0

	msg "Building nxdk runtime support libraries"
	eval "$("$NXDK_DIR/bin/activate" -s)"

	run env \
		MSYSTEM=MINGW64 \
		NXDK_DIR="$NXDK_DIR" \
		NXDK_NET=y \
		PATH="/usr/bin:$MINGW64_ROOT/bin:$NXDK_DIR/bin:$PATH" \
		make -C "$NXDK_DIR" \
		NXDK_ONLY=1 \
		main.exe \
		-j"$JOBS"

	[ -f "$NXDK_DIR/lib/libpdclib.lib" ] || fail "nxdk libpdclib.lib was not built"
	[ -f "$NXDK_DIR/lib/libwinapi.lib" ] || fail "nxdk libwinapi.lib was not built"
	[ -f "$NXDK_DIR/lib/libnxdk_net.lib" ] || fail "nxdk libnxdk_net.lib was not built"
}

##############################################################################
# Build
##############################################################################

prepare_worktree() {
	if [ "$SKIP_SOURCE_SYNC" -eq 0 ]; then
		msg "Creating isolated fbc-xbox worktree"
		rm -rf "$WORKTREE"
		sync_source_tree "$WORKTREE"
		(
			cd "$WORKTREE"
			rm -rf bin bootstrap
			sanitize_source_tree "$HOST_TRIPLET"
		)
	fi
}

ensure_host_compiler() {
	local build_fbc=""

	build_fbc="$(detect_fbc \
		"${HOST_FBC_ROOT:-}/fbc64.exe" \
		"${HOST_FBC_ROOT:-}/fbc.exe" \
		"${HOST_FBC_ROOT:-}/bin/fbc64.exe" \
		"${HOST_FBC_ROOT:-}/bin/fbc.exe" \
		"$ROOT/bin/fbc.exe" \
		"$ROOT/bootstrap/fbc.exe" \
		"$ROOT/fbc.exe" || true)"

	[ -n "$build_fbc" ] || fail "no usable host FreeBASIC compiler found; set HOST_FBC_ROOT"
	echo "==> using build FreeBASIC: $build_fbc"

	msg "Building package compiler"
	run make TARGET_TRIPLET="$HOST_TRIPLET" TARGET="$HOST_TRIPLET" BUILD_FBC="$build_fbc" compiler -j"$JOBS"
	[ -x "./bin/fbc.exe" ] || fail "bootstrap compiler was not built"
	HOST_FBC="./bin/fbc.exe"
	PACKAGE_FBC="./bin/fbc.exe"
}

build_xbox_target() {
	local host_fbc
	local cxbe

	[ "$SKIP_BUILD" -eq 0 ] || return 0

	msg "Building FreeBASIC Xbox target with nxdk"
	cd "$WORKTREE"

	ensure_host_compiler
	host_fbc="$HOST_FBC"

	# nxdk exposes nxdk-cc/nxdk-cxx/nxdk-link/nxdk-lib after activation.
	eval "$("$NXDK_DIR/bin/activate" -s)"
	export PATH="$NXDK_DIR/bin:$PATH"

	command -v nxdk-cc >/dev/null 2>&1 || fail "nxdk-cc not found after nxdk activation"
	command -v nxdk-cxx >/dev/null 2>&1 || fail "nxdk-cxx not found after nxdk activation"

	if [ -f "$NXDK_DIR/tools/cxbe/cxbe.exe" ]; then
		cxbe="$NXDK_DIR/tools/cxbe/cxbe.exe"
	else
		cxbe="$NXDK_DIR/tools/cxbe/cxbe"
	fi

	run make \
		TARGET_TRIPLET="$XBOX_TARGET_TRIPLET" \
		TARGET_OS=xbox \
		TARGET_ARCH=x86 \
		FBTARGET_DIR_OVERRIDE="$XBOX_TARGET_KEY" \
		BUILD_PREFIX= \
		CC=nxdk-cc \
		CXX=nxdk-cxx \
		LD=nxdk-link \
		AR=llvm-ar \
		RANLIB=llvm-ranlib \
		CXBE="$cxbe" \
		BUILD_FBC="$host_fbc" \
		BUILD_FBC_TARGET=xbox \
		BUILD_FBC_BUILDPREFIX= \
		CPPFLAGS="-DHOST_XBOX -DDISABLE_FFI -DDISABLE_OPENGL -I$NXDK_DIR/lib -I$NXDK_DIR/lib/net/lwip/src/include -I$NXDK_DIR/lib/net/nforceif/include -I$NXDK_DIR/lib/net/nvnetdrv" \
		CFLAGS="-DHOST_XBOX -DDISABLE_FFI -DDISABLE_OPENGL -I$NXDK_DIR/lib -I$NXDK_DIR/lib/net/lwip/src/include -I$NXDK_DIR/lib/net/nforceif/include -I$NXDK_DIR/lib/net/nvnetdrv" \
		CXXFLAGS= \
		LDFLAGS= \
		rtlib fbrt gfxlib2 sfxlib \
		-j"$JOBS"

	[ -d "$WORKTREE/lib/freebasic/$XBOX_TARGET_KEY" ] || fail "Xbox runtime directory was not created"
}

##############################################################################
# Distribution
##############################################################################

write_launchers() {
	msg "Writing fbc-xbox launcher scripts"

	cat > "$DISTROOT/fbc-xbox-package.sh" <<'EOF'
#!/usr/bin/env bash

set -euo pipefail

script_path="${0//\\//}"
case "$script_path" in
	*/*) script_dir="${script_path%/*}" ;;
	*) script_dir="." ;;
esac

root="$(CDPATH= cd -- "$script_dir" && pwd)"
NXDK_DIR="$root/nxdk"
PATH="$root/toolchain/msys2/usr/bin:$root/bin:$NXDK_DIR/bin:$PATH"
CLANG="$NXDK_DIR/bin/nxdk-cc"
GCC="$NXDK_DIR/bin/nxdk-cc"
AS="$NXDK_DIR/bin/nxdk-cc"
LD="$NXDK_DIR/bin/nxdk-link"
AR="$root/toolchain/msys2/usr/bin/llvm-ar.exe"
RANLIB="$root/toolchain/msys2/usr/bin/llvm-ranlib.exe"
CXBE="$NXDK_DIR/tools/cxbe/cxbe.exe"
if [ ! -f "$CXBE" ]; then
	CXBE="$NXDK_DIR/tools/cxbe/cxbe"
fi
export NXDK_DIR PATH CLANG GCC AS LD AR RANLIB CXBE

exec "$root/bin/fbc.exe" -target xbox -prefix "$root" -i "$root/inc" "$@"
EOF
	chmod 755 "$DISTROOT/fbc-xbox-package.sh"

	cat > "$DISTROOT/fbc-xbox.cmd" <<'EOF'
@echo off
setlocal
set "FBXBOX_ROOT=%~dp0"
if "%FBXBOX_ROOT:~-1%"=="\" set "FBXBOX_ROOT=%FBXBOX_ROOT:~0,-1%"
"%FBXBOX_ROOT%\toolchain\msys2\usr\bin\bash.exe" "%FBXBOX_ROOT%\fbc-xbox-package.sh" %*
exit /b %ERRORLEVEL%
EOF

	cat > "$DISTROOT/freebasic-xbox-env.cmd" <<'EOF'
@echo off
set "FBXBOX_ROOT=%~dp0"
if "%FBXBOX_ROOT:~-1%"=="\" set "FBXBOX_ROOT=%FBXBOX_ROOT:~0,-1%"
set "NXDK_DIR=%FBXBOX_ROOT%\nxdk"
set "PATH=%FBXBOX_ROOT%\toolchain\msys2\usr\bin;%FBXBOX_ROOT%\bin;%NXDK_DIR%\bin;%PATH%"
set "CLANG=%NXDK_DIR%\bin\nxdk-cc"
set "GCC=%NXDK_DIR%\bin\nxdk-cc"
set "AS=%NXDK_DIR%\bin\nxdk-cc"
set "LD=%NXDK_DIR%\bin\nxdk-link"
set "AR=%FBXBOX_ROOT%\toolchain\msys2\usr\bin\llvm-ar.exe"
set "RANLIB=%FBXBOX_ROOT%\toolchain\msys2\usr\bin\llvm-ranlib.exe"
if exist "%NXDK_DIR%\tools\cxbe\cxbe.exe" set "CXBE=%NXDK_DIR%\tools\cxbe\cxbe.exe"
if not defined CXBE set "CXBE=%NXDK_DIR%\tools\cxbe\cxbe"
echo FreeBASIC Xbox environment ready.
echo fbc-xbox: %FBXBOX_ROOT%fbc-xbox.cmd
cmd /k
EOF

	cat > "$DISTROOT/fbc-xbox-xiso.cmd" <<'EOF'
@echo off
setlocal
set "FBXBOX_ROOT=%~dp0"
if "%FBXBOX_ROOT:~-1%"=="\" set "FBXBOX_ROOT=%FBXBOX_ROOT:~0,-1%"

if "%~1"=="" goto usage
if /I "%~1"=="--help" goto usage
if /I "%~1"=="-h" goto usage
if /I "%~1"=="-help" goto usage
if not exist "%~1" (
	echo XBE not found: %~1 1>&2
	exit /b 1
)
for %%I in ("%~1") do set "XBE_PATH=%%~fI"
for %%I in ("%~dpn1.iso") do set "ISO_PATH=%%~fI"
set "ASSET_DIR=%FBXBOX_ASSETS%"
shift

set "EXTRACT_XISO=%FBXBOX_ROOT%\nxdk\tools\extract-xiso\build\extract-xiso.exe"
if not exist "%EXTRACT_XISO%" set "EXTRACT_XISO=%FBXBOX_ROOT%\nxdk\tools\extract-xiso\build\extract-xiso"
if not exist "%EXTRACT_XISO%" (
	echo extract-xiso was not found under %FBXBOX_ROOT%\nxdk\tools\extract-xiso\build 1>&2
	exit /b 1
)

:parse_args
if "%~1"=="" goto parsed_args
if /I "%~1"=="--assets" (
	if "%~2"=="" (
		echo --assets requires a directory 1>&2
		exit /b 2
	)
	for %%I in ("%~2") do set "ASSET_DIR=%%~fI"
	shift
	shift
	goto parse_args
)
if defined ISO_WAS_SET (
	echo Unexpected argument: %~1 1>&2
	exit /b 2
)
for %%I in ("%~1") do set "ISO_PATH=%%~fI"
set "ISO_WAS_SET=1"
shift
goto parse_args

:parsed_args
if defined ASSET_DIR if not exist "%ASSET_DIR%\" (
	echo Assets directory not found: %ASSET_DIR% 1>&2
	exit /b 1
)

set "STAGE=%TEMP%\fbc-xbox-xiso-%RANDOM%%RANDOM%"
mkdir "%STAGE%" >nul 2>nul
if errorlevel 1 (
	echo Could not create temporary XISO staging directory: %STAGE% 1>&2
	exit /b 1
)

if defined ASSET_DIR (
	xcopy /E /I /Y "%ASSET_DIR%\*" "%STAGE%\" >nul
	if errorlevel 4 (
		rmdir /S /Q "%STAGE%" >nul 2>nul
		echo Could not stage assets from %ASSET_DIR% 1>&2
		exit /b 1
	)
)

copy /Y "%XBE_PATH%" "%STAGE%\default.xbe" >nul
if errorlevel 1 (
	rmdir /S /Q "%STAGE%" >nul 2>nul
	echo Could not stage default.xbe 1>&2
	exit /b 1
)

if exist "%ISO_PATH%" del /F /Q "%ISO_PATH%" >nul 2>nul
"%EXTRACT_XISO%" -q -c "%STAGE%" "%ISO_PATH%"
set "STATUS=%ERRORLEVEL%"
rmdir /S /Q "%STAGE%" >nul 2>nul
if not "%STATUS%"=="0" exit /b %STATUS%

echo Wrote %ISO_PATH%
exit /b 0

:usage
echo Usage: fbc-xbox-xiso.cmd program.xbe [program.iso] [--assets dir]
exit /b 2
EOF

	install -m 755 "$ROOT/src/tools/xbox/fbc-xbox-xiso" "$DISTROOT/fbc-xbox-xiso.sh"
	chmod 755 "$DISTROOT/fbc-xbox-xiso.sh"
}

write_distribution_notes() {
	local nxdk_rev

	msg "Writing fbc-xbox package notes"
	nxdk_rev="$(git -C "$NXDK_DIR" rev-parse --short HEAD 2>/dev/null || echo unknown)"

	cat > "$DISTROOT/readme-fbc-xbox.txt" <<EOF
FreeBASIC Xbox ${FBVERSION}

This package is intended to run without a separate nxdk installation.

The installer adds these directories to the Windows system PATH:

    ${INSTALL_DIR_WIN}
    ${INSTALL_DIR_WIN}\\bin
    ${INSTALL_DIR_WIN}\\nxdk\\bin

Use fbc-xbox.cmd from cmd.exe or PowerShell:

    fbc-xbox.cmd program.bas -x program.xbe

The package also includes fbc-xbox-xiso.cmd and fbc-xbox-xiso.sh.  They stage
an XBE as default.xbe and pack the XISO disc-image format expected by
full-system Xbox emulators such as xemu:

    fbc-xbox-xiso.cmd program.xbe program.iso

Programs that need a current-directory asset tree can stage one into the XISO:

    fbc-xbox-xiso.cmd program.xbe program.iso --assets game-folder
    ./fbc-xbox-xiso.sh program.xbe program.iso --assets game-folder

The bundled nxdk tools still include extract-xiso for lower-level packaging
workflows.

This is an experimental nxdk-based revival path for the existing FreeBASIC
Xbox target. The package validation checks that the compiler can invoke the
bundled nxdk clang/link/cxbe toolchain and produce an Xbox XBE file.

The packaged nxdk checkout revision is:

    $nxdk_rev
EOF
}

assemble_distribution() {
	local package_fbc

	[ "$SKIP_PACKAGE" -eq 0 ] || return 0

	rm -rf "$DISTROOT"
	mkdir -p "$DISTROOT/bin" "$DISTROOT/inc" "$DISTROOT/lib/freebasic/$XBOX_TARGET_KEY"

	msg "Copying fbc-xbox staged files"
	package_fbc="${PACKAGE_FBC:-$WORKTREE/bin/fbc.exe}"
	if [ ! -x "$package_fbc" ]; then
		package_fbc="$(detect_fbc \
			"${HOST_FBC_ROOT:-}/fbc64.exe" \
			"${HOST_FBC_ROOT:-}/fbc.exe" \
			"${HOST_FBC_ROOT:-}/bin/fbc64.exe" \
			"${HOST_FBC_ROOT:-}/bin/fbc.exe" \
			"$ROOT/bin/fbc.exe" \
			"$ROOT/bootstrap/fbc.exe" \
			"$ROOT/fbc.exe" || true)"
	fi
	[ -x "$package_fbc" ] || fail "package compiler is missing: $package_fbc"
	cp -a "$package_fbc" "$DISTROOT/bin/fbc.exe"
	copy_runtime_dlls "$DISTROOT/bin/fbc.exe" "$DISTROOT/bin"
	copy_tree "$WORKTREE/inc" "$DISTROOT/inc"
	copy_tree "$WORKTREE/lib/freebasic/$XBOX_TARGET_KEY" "$DISTROOT/lib/freebasic/$XBOX_TARGET_KEY"

	msg "Copying nxdk"
	if have rsync; then
		run rsync -a --delete \
			--exclude '/.git/' \
			--exclude '/**/.git/' \
			"$NXDK_DIR/" "$DISTROOT/nxdk/"
	else
		copy_tree "$NXDK_DIR" "$DISTROOT/nxdk"
		find "$DISTROOT/nxdk" -name .git -type d -prune -exec rm -rf {} + || true
	fi

	copy_nxdk_host_tool_runtimes
	build_nxdk_tool_shims

	msg "Copying top-level documentation and examples"
	copy_tree "$ROOT/doc" "$DISTROOT/doc"
	copy_examples_tree "$DISTROOT/examples"
	cp -a "$ROOT/changelog.txt" "$DISTROOT/"
	cp -a "$ROOT/readme.txt" "$DISTROOT/"

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
	msg "Creating fbc-xbox distribution zip"
	rm -f "$zipfile"
	(
		cd "$DISTROOT_BASE"
		run zip -qr "$zipfile" "$DISTNAME"
	)
}

copy_tool_exe() {
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
	if [ -z "$exe" ] && [ -f "$MINGW64_ROOT/bin/$tool.exe" ]; then
		exe="$MINGW64_ROOT/bin/$tool.exe"
	fi
	[ -n "$exe" ] || fail "required tool not found: $tool"
	cp -a "$exe" "$dst/"
	copy_runtime_dlls "$exe" "$dst"
}

copy_msys_runtime() {
	local bindir="$DISTROOT/toolchain/msys2/usr/bin"
	local tool

	msg "Copying bundled MSYS2 runtime for nxdk launchers"
	mkdir -p "$bindir" "$DISTROOT/toolchain/msys2/tmp"

	for tool in \
		bash \
		sh \
		env \
		dirname \
		pwd \
		sed \
		tr \
		mkdir \
		mktemp \
		make \
		cp \
		rm \
		find \
		uname \
		clang \
		lld \
		llvm-ar \
		llvm-ranlib
	do
		copy_tool_exe "$tool" "$bindir"
	done

	copy_clang_resource_dir
}

copy_nxdk_host_tool_runtimes() {
	local tool
	local dir

	for tool in \
		"$DISTROOT/nxdk/tools/cxbe/cxbe.exe" \
		"$DISTROOT/nxdk/tools/extract-xiso/build/extract-xiso.exe"
	do
		[ -x "$tool" ] || continue
		dir="$(dirname "$tool")"
		copy_runtime_dlls "$tool" "$dir"
	done
}

copy_clang_resource_dir() {
	local src
	local dst

	src="$(clang -print-resource-dir 2>/dev/null || true)"
	if [ -n "$src" ] && have cygpath; then
		src="$(cygpath -u "$src" 2>/dev/null || printf '%s' "$src")"
	fi
	if [ -z "$src" ] || [ ! -d "$src" ]; then
		return 0
	fi

	dst="$DISTROOT/toolchain/msys2/usr/lib/clang/$(basename "$src")"
	msg "Copying bundled clang resource headers"
	rm -rf "$dst"
	mkdir -p "$(dirname "$dst")"
	copy_tree "$src" "$dst"
}

build_nxdk_tool_shims() {
	local shim_src="$BUILDROOT/nxdk-tool-shim.c"
	local shim_exe="$BUILDROOT/nxdk-tool-shim.exe"
	local tool

	msg "Building Windows shims for nxdk script tools"
	[ -x "$MINGW64_ROOT/bin/gcc.exe" ] || fail "gcc not found at $MINGW64_ROOT/bin/gcc.exe"

	cat > "$shim_src" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

static void fail(const char *message)
{
	fprintf(stderr, "nxdk tool shim: %s\n", message);
	exit(1);
}

static void strip_exe_suffix(char *name)
{
	size_t len = strlen(name);

	if (len >= 4 && _stricmp(name + len - 4, ".exe") == 0) {
		name[len - 4] = '\0';
	}
}

int main(int argc, char **argv)
{
	char self[MAX_PATH];
	char dir[MAX_PATH];
	char script[MAX_PATH];
	char bash[MAX_PATH];
	char *base;
	const char **child_argv;
	int i;
	int status;

	if (GetModuleFileNameA(NULL, self, sizeof(self)) == 0) {
		fail("could not determine executable path");
	}

	strncpy(dir, self, sizeof(dir));
	dir[sizeof(dir) - 1] = '\0';
	base = strrchr(dir, '\\');
	if (base == NULL) {
		base = strrchr(dir, '/');
	}
	if (base == NULL) {
		fail("could not determine executable directory");
	}
	*base = '\0';
	base++;
	strip_exe_suffix(base);

	snprintf(script, sizeof(script), "%s\\%s", dir, base);
	snprintf(bash, sizeof(bash), "%s\\..\\..\\toolchain\\msys2\\usr\\bin\\bash.exe", dir);

	child_argv = (const char **)calloc((size_t)argc + 2, sizeof(char *));
	if (child_argv == NULL) {
		fail("out of memory");
	}

	child_argv[0] = bash;
	child_argv[1] = script;
	for (i = 1; i < argc; i++) {
		child_argv[i + 1] = argv[i];
	}
	child_argv[argc + 1] = NULL;

	status = _spawnv(_P_WAIT, bash, child_argv);
	free(child_argv);

	if (status < 0) {
		perror("nxdk tool shim");
		return 1;
	}

	return status;
}
EOF

	run env MSYSTEM=MINGW64 PATH="$MINGW64_ROOT/bin:/usr/bin:$PATH" \
		"$MINGW64_ROOT/bin/gcc.exe" -O2 -static -s "$shim_src" -o "$shim_exe"

	for tool in nxdk-as nxdk-cc nxdk-cmake nxdk-cxx nxdk-lib nxdk-link nxdk-pkg-config; do
		[ -f "$DISTROOT/nxdk/bin/$tool" ] || continue
		cp -a "$shim_exe" "$DISTROOT/nxdk/bin/$tool.exe"
	done
}

create_installer() {
	local installer_nsi="$BUILDROOT/${DISTNAME}.nsi"
	local installer_exe="$OUT/${DISTNAME}-setup.exe"
	local installer_payload_zip="$BUILDROOT/${DISTNAME}-installer-payload.zip"
	local out_win
	local payload_win

	[ "$SKIP_INSTALLER" -eq 0 ] || return 0
	[ "$SKIP_PACKAGE" -eq 0 ] || return 0
	[ -x "$NSIS_EXE" ] || fail "makensis not found at $NSIS_EXE; install the nsis package or set NSIS_EXE"
	have cygpath || fail "cygpath not found"
	have zip || fail "zip not found"

	out_win="$(cygpath -aw "$installer_exe")"
	msg "Creating fbc-xbox NSIS payload zip"
	rm -f "$installer_payload_zip"
	(
		cd "$DISTROOT"
		run zip -qr "$installer_payload_zip" .
	)

	payload_win="$(cygpath -aw "$installer_payload_zip")"

	msg "Generating NSIS installer script"
	cat > "$installer_nsi" <<EOF
Unicode true
SetCompressor zlib
RequestExecutionLevel admin

Name "FreeBASIC Xbox ${FBVERSION}"
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

Function AddInstallDirsToPath
	Push "\$INSTDIR"
	Call AddOnePath
	Push "\$INSTDIR\\bin"
	Call AddOnePath
	Push "\$INSTDIR\\nxdk\\bin"
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

Function un.RemoveInstallDirsFromPath
	Push "\$INSTDIR\\nxdk\\bin"
	Call un.RemoveOnePath
	Push "\$INSTDIR\\bin"
	Call un.RemoveOnePath
	Push "\$INSTDIR"
	Call un.RemoveOnePath
	Call un.RefreshEnvironment
FunctionEnd

Section "Install"
	InitPluginsDir
	SetOutPath "\$PLUGINSDIR"
	SetCompress off
	File /oname=freebasic-xbox-payload.zip "$payload_win"
	SetCompress auto
	IfFileExists "\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" 0 no_powershell
	SetOutPath "\$INSTDIR"
	;
	; The Xbox package carries nxdk plus the FreeBASIC compiler, runtime
	; libraries, and helper launchers.  Keep the installer payload as a zip so
	; makensis does not need to mmap the entire expanded package tree.
	nsExec::ExecToLog '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "\$\$ErrorActionPreference = ''Stop''; Expand-Archive -LiteralPath ''\$PLUGINSDIR\\freebasic-xbox-payload.zip'' -DestinationPath ''\$INSTDIR'' -Force"'
	Pop \$0
	StrCmp \$0 "0" payload_done
		Abort "Failed to extract the FreeBASIC Xbox payload. PowerShell exit code: \$0"
	payload_done:
	WriteUninstaller "\$INSTDIR\\uninstall.exe"
	Call AddInstallDirsToPath
	Goto install_done
	no_powershell:
		Abort "Windows PowerShell is required to extract this installer."
	install_done:
SectionEnd

Section "Uninstall"
	Call un.RemoveInstallDirsFromPath
	Delete "\$INSTDIR\\uninstall.exe"
	RMDir /r "\$INSTDIR"
SectionEnd
EOF

	msg "Creating NSIS installer"
	rm -f "$installer_exe"
	if ! run "$NSIS_EXE" "$installer_nsi"; then
		rm -f "$installer_payload_zip"
		fail "makensis failed while creating fbc-xbox installer"
	fi
	rm -f "$installer_payload_zip"
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

	msg "Validating packaged fbc-xbox"
	rm -rf "$validate_dir"
	mkdir -p "$validate_dir"
	mkdir -p "$validate_dir/assets"

	cat > "$validate_dir/hello.bas" <<'EOF'
print "freebasic-xbox package test OK"
EOF
	printf 'asset smoke\n' > "$validate_dir/assets/readme.txt"

	dist_win="$(cygpath -aw "$DISTROOT")"
	validate_win="$(cygpath -aw "$validate_dir")"
	validate_cmd="$validate_dir/validate.cmd"

	cat > "$validate_cmd" <<EOF
@echo off
set "PATH=%SystemRoot%\\System32;%SystemRoot%;%SystemRoot%\\System32\\Wbem"
pushd "$validate_win"
call "$dist_win\\fbc-xbox.cmd" hello.bas -x hello.xbe
set "FBC_XBOX_STATUS=%ERRORLEVEL%"
if "%FBC_XBOX_STATUS%"=="0" call "$dist_win\\fbc-xbox-xiso.cmd" hello.xbe hello.iso --assets "$validate_win\\assets"
if "%FBC_XBOX_STATUS%"=="0" set "FBC_XBOX_STATUS=%ERRORLEVEL%"
popd
exit /b %FBC_XBOX_STATUS%
EOF

	run cmd.exe //C "$(cygpath -aw "$validate_cmd")"
	[ -f "$validate_dir/hello.xbe" ] || fail "packaged fbc-xbox did not produce hello.xbe"
	[ -f "$validate_dir/hello.iso" ] || fail "packaged fbc-xbox-xiso did not produce hello.iso"
}

##############################################################################
# Main
##############################################################################

if [ "$SKIP_DEPS" -eq 0 ]; then
	install_dependencies
fi

ensure_nxdk
ensure_nxdk_tools
build_nxdk_runtime_libs
prepare_worktree
build_xbox_target
assemble_distribution
create_zip
create_installer
validate_distribution

echo ""
echo "FreeBASIC Xbox build complete."
echo "Distribution root: $DISTROOT"
echo "Artifacts: $OUT"

# end of msys2-build-freebasic-xbox.sh
