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
		rsync \
		unzip \
		zip \
		mingw-w64-ucrt-x86_64-binutils \
		mingw-w64-ucrt-x86_64-binaryen \
		mingw-w64-ucrt-x86_64-emscripten \
		mingw-w64-ucrt-x86_64-gcc \
		mingw-w64-ucrt-x86_64-nodejs \
		mingw-w64-ucrt-x86_64-python \
		mingw-w64-x86_64-nsis
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

	msg "Cleaning fbc-js worktree"
	run make clean TARGET_TRIPLET="$HOST_TRIPLET" || true

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
		# shellcheck disable=SC1090
		. "$UCRT64_ROOT/etc/profile.d/emscripten.sh"
	fi
	have emcc || fail "emcc not found after loading the UCRT64 Emscripten profile"

	run make -j"$JOBS" \
		rtlib fbrt gfxlib2 sfxlib \
		FBC="$build_fbc" \
		TARGET_TRIPLET="asmjs-unknown-emscripten" \
		TARGET="asmjs-unknown-emscripten" \
		FBTARGET_DIR_OVERRIDE="js-asmjs"

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

write_launchers() {
	msg "Writing fbc-js launcher scripts"

	cat > "$DISTROOT/fbc-js.cmd" <<'EOF'
@echo off
setlocal
set "FBJS_ROOT=%~dp0"
set "PATH=%FBJS_ROOT%toolchain\ucrt64\bin;%FBJS_ROOT%;%PATH%"
set "PATH=%FBJS_ROOT%toolchain\ucrt64\lib\emscripten;%PATH%"
"%FBJS_ROOT%bin\fbc-js.exe" %*
exit /b %ERRORLEVEL%
EOF

	cat > "$DISTROOT/freebasic-js-env.cmd" <<'EOF'
@echo off
set "FBJS_ROOT=%~dp0"
set "PATH=%FBJS_ROOT%toolchain\ucrt64\bin;%FBJS_ROOT%;%PATH%"
set "PATH=%FBJS_ROOT%toolchain\ucrt64\lib\emscripten;%PATH%"
echo FreeBASIC JS environment ready.
echo fbc-js: %FBJS_ROOT%bin\fbc-js.exe
cmd /k
EOF

	cat > "$DISTROOT/freebasic-js-env.sh" <<'EOF'
#!/usr/bin/env sh

_fbjs_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PATH="${_fbjs_root}/toolchain/ucrt64/bin:${_fbjs_root}:${PATH}"
PATH="${_fbjs_root}/toolchain/ucrt64/lib/emscripten:${PATH}"
export PATH
unset _fbjs_root
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
    ${INSTALL_DIR_WIN}\\toolchain\\ucrt64\\bin
    ${INSTALL_DIR_WIN}\\toolchain\\ucrt64\\lib\\emscripten

The toolchain directory contains the UCRT64 Emscripten environment used by
fbc-js, including emcc, emar, Node.js, Python, Binaryen, Clang/LLVM, runtime
DLLs, headers, libraries, and supporting data files installed by the MSYS2
packages this build script uses.

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

	msg "Copying top-level documentation and examples"
	copy_tree "$ROOT/doc" "$DISTROOT/doc"
	copy_tree "$ROOT/examples" "$DISTROOT/examples"
	cp -a "$ROOT/changelog.txt" "$DISTROOT/"
	cp -a "$ROOT/readme.txt" "$DISTROOT/"

	copy_runtime_dlls "$DISTROOT/bin/fbc-js.exe" "$DISTROOT/bin"
	copy_ucrt64_toolchain
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
	local dist_win
	local out_win

	[ -x "$NSIS_EXE" ] || fail "makensis not found at $NSIS_EXE; install the nsis package or set NSIS_EXE"
	have cygpath || fail "cygpath not found"

	dist_win="$(cygpath -aw "$DISTROOT")"
	out_win="$(cygpath -aw "$installer_exe")"

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

Function AddInstallDirsToPath
	Push "\$INSTDIR"
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
	FileWrite \$0 "  *) export PATH=\$\${_freebasic_js_emscripten}:\$\$PATH ;;$\r$\n"
	FileWrite \$0 "esac$\r$\n"
	FileWrite \$0 "case :\$\$PATH: in$\r$\n"
	FileWrite \$0 "  *:\$\${_freebasic_js_toolchain}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\$\${_freebasic_js_toolchain}:\$\$PATH ;;$\r$\n"
	FileWrite \$0 "esac$\r$\n"
	FileWrite \$0 "case :\$\$PATH: in$\r$\n"
	FileWrite \$0 "  *:\$\${_freebasic_js_prefix}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\$\${_freebasic_js_prefix}:\$\$PATH ;;$\r$\n"
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
	Push "\$INSTDIR"
	Call un.RemoveOnePath
	Call un.RefreshEnvironment
FunctionEnd

Function un.RemoveInstallDirsFromMsys2
	Delete "C:\\msys64\\etc\\profile.d\\freebasic-js.sh"
	Delete "C:\\msys32\\etc\\profile.d\\freebasic-js.sh"
FunctionEnd

Section "Install"
	SetOutPath "\$INSTDIR"
	File /r "$dist_win\\*"
	WriteUninstaller "\$INSTDIR\\uninstall.exe"
	Call AddInstallDirsToPath
	Call AddInstallDirsToMsys2
SectionEnd

Section "Uninstall"
	Call un.RemoveInstallDirsFromPath
	Call un.RemoveInstallDirsFromMsys2
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
	local package_path_win
	local validate_cmd

	msg "Validating packaged fbc-js"
	rm -rf "$validate_dir"
	mkdir -p "$validate_dir"

	cat > "$validate_dir/hello.bas" <<'EOF'
print "freebasic-js package test OK"
EOF

	dist_win="$(cygpath -aw "$DISTROOT")"
	validate_win="$(cygpath -aw "$validate_dir")"
	package_path_win="$dist_win\\toolchain\\ucrt64\\lib\\emscripten;$dist_win\\toolchain\\ucrt64\\bin;$dist_win;%PATH%"
	validate_cmd="$validate_dir/validate.cmd"

	cat > "$validate_cmd" <<EOF
@echo off
set "PATH=$package_path_win"
if not exist "$validate_win\\emcc-temp" mkdir "$validate_win\\emcc-temp"
set "EMCC_TEMP_DIR=$validate_win\\emcc-temp"
call "$dist_win\\fbc-js.cmd" "$validate_win\\hello.bas" -x "$validate_win\\hello.js"
if errorlevel 1 exit /b %ERRORLEVEL%
node "$validate_win\\hello.js" > "$validate_win\\output.txt" 2> "$validate_win\\output.err"
exit /b %ERRORLEVEL%
EOF

	run cmd.exe //C "$(cygpath -aw "$validate_cmd")"
	[ -f "$validate_dir/hello.js" ] || fail "packaged fbc-js did not produce hello.js"
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
fi

if [ "$SKIP_VALIDATE" -eq 0 ]; then
	validate_distribution
fi

msg "Done"
echo "Distribution root: $DISTROOT"
echo "Zip archive: $OUT/${DISTNAME}.zip"
echo "Installer: $OUT/${DISTNAME}-setup.exe"
