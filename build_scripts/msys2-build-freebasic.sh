#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-build-freebasic.sh
#
# Build a self-contained Windows FreeBASIC distribution from MSYS2.
# Produces a combined win32/win64 package tree, a .zip archive, and
# an NSIS installer that installs into C:\freebasic.
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
		echo "ERROR: this script must be run inside an MSYS2 MinGW environment."
		exit 1
		;;
esac

##############################################################################
# Options
##############################################################################

SKIP_DEPS=0
SKIP_SOURCE_SYNC=0
SKIP_BUILD32=0
SKIP_BUILD64=0
SKIP_PACKAGE=0
SKIP_INSTALLER=0
SKIP_VALIDATE=0
KEEP_BUILDROOT=0

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-build-freebasic.sh [options]

Options:
  --skip-deps         Do not install or update MSYS2 packages
  --skip-source-sync  Reuse the existing per-target worktrees
  --skip-build32      Skip the win32 build
  --skip-build64      Skip the win64 build
  --skip-package      Skip distribution tree assembly and zip creation
  --skip-installer    Skip NSIS installer creation
  --skip-validate     Skip packaged compiler validation
  --keep-buildroot    Keep the build root on failure or success
  --help              Show this help text

Environment:
  BUILDROOT           Temporary build root (default: <repo>/.build-msys2)
  OUT                 Output directory (default: <repo>/out/mingw32)
  HOST_FBC_ROOT       Optional existing FreeBASIC install used as host compiler fallback
  NSIS_EXE            Explicit makensis path (default: /mingw64/bin/makensis.exe)
  JOBS                Parallel make job count (default: detected CPU core count)
EOF
}

for arg in "$@"; do
	case "$arg" in
		--skip-deps) SKIP_DEPS=1 ;;
		--skip-source-sync) SKIP_SOURCE_SYNC=1 ;;
		--skip-build32) SKIP_BUILD32=1 ;;
		--skip-build64) SKIP_BUILD64=1 ;;
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

copy_library_alias() {
	local libdir="$1"
	local source="$2"
	local alias="$3"
	local source_file
	local ext

	for ext in dll.a a; do
		source_file="$libdir/lib${source}.${ext}"
		if [ -f "$source_file" ] && [ ! -f "$libdir/lib${alias}.${ext}" ]; then
			cp -a "$source_file" "$libdir/lib${alias}.${ext}"
		fi
	done
}

create_arch_library_aliases() {
	local libdir="$1"
	local component

	[ -d "$libdir" ] || return 0

	# Current MSYS2 package names do not always match the older Windows
	# library names used by the FreeBASIC bindings.  Keep these aliases in
	# the packaged lib directory so example builds can use the shipped
	# bindings without requiring users to rename archives by hand.
	copy_library_alias "$libdir" freeimage FreeImage
	copy_library_alias "$libdir" gd bgd
	copy_library_alias "$libdir" gd bgd-static
	copy_library_alias "$libdir" mysqlclient mySQL
	copy_library_alias "$libdir" openal OpenAL32
	copy_library_alias "$libdir" freeglut glut
	copy_library_alias "$libdir" freeglut glut32
	copy_library_alias "$libdir" freeglut GLUT
	copy_library_alias "$libdir" glew32 GLEW
	copy_library_alias "$libdir" glew32 glew
	copy_library_alias "$libdir" glfw3 glfw3dll
	copy_library_alias "$libdir" lua5.1 lua

	for component in \
		allegro \
		allegro_acodec \
		allegro_audio \
		allegro_color \
		allegro_dialog \
		allegro_font \
		allegro_image \
		allegro_memfile \
		allegro_physfs \
		allegro_primitives \
		allegro_ttf
	do
		copy_library_alias "$libdir" "$component" "$component-5.0.10-md"
	done
}

sync_source_tree() {
	local dst="$1"
	mkdir -p "$dst"
	if have rsync; then
		run rsync -a --delete --delete-excluded --prune-empty-dirs \
			--exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
			"$ROOT/" "$dst/"
	else
		fail "rsync is required to create isolated worktrees"
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
# Build configuration
##############################################################################

FBVERSION="$(extract_var FBVERSION)"
REV="$(extract_var REV)"
[ -n "$FBVERSION" ] || fail "missing FBVERSION in mk/version.mk"
[ -n "$REV" ] || fail "missing REV in mk/version.mk"

JOBS="${JOBS:-$(max_jobs)}"
BUILDROOT="${BUILDROOT:-$ROOT/.build-msys2}"
WORKROOT="$BUILDROOT/work"
STAGEROOT="$BUILDROOT/stage"
DISTROOT_BASE="$BUILDROOT/dist"
TMPROOT="$BUILDROOT/tmp"
OUT="${OUT:-$ROOT/out/mingw32}"
DISTNAME_BASE="FreeBASIC-${FBVERSION}-winlibs"
INSTALL_SUBDIR="package"
INSTALL_DIR_WIN='C:\FreeBASIC'
HOST_FBC_ROOT="${HOST_FBC_ROOT:-}"

MINGW32_ROOT="/mingw32"
MINGW64_ROOT="/mingw64"
TRIPLET32="i686-w64-mingw32"
TRIPLET64="x86_64-w64-mingw32"
NSIS_EXE="${NSIS_EXE:-$MINGW64_ROOT/bin/makensis.exe}"

mkdir -p "$WORKROOT" "$STAGEROOT" "$DISTROOT_BASE" "$TMPROOT" "$OUT"
TMPDIR="$TMPROOT"
TMP="$TMPROOT"
TEMP="$TMPROOT"
export TMPDIR TMP TEMP

cleanup() {
	if [ "$KEEP_BUILDROOT" -ne 0 ]; then
		return 0
	fi

	rm -rf "$WORKROOT" "$STAGEROOT" "$BUILDROOT/validate"
	find "$BUILDROOT" -maxdepth 1 -type f -name '*.nsi' -delete 2>/dev/null || true
}
trap cleanup EXIT

##############################################################################
# Dependency installation
##############################################################################

install_dependencies() {
	local msys_packages=(
		base-devel
		coreutils
		make
		tar
		xz
		unzip
		zip
		rsync
		dos2unix
		mingw-w64-x86_64-nsis
	)
	local mingw_suffixes=(
		binutils
		libffi
		SDL
		SDL_gfx
		SDL_image
		SDL_mixer
		SDL_net
		SDL_ttf
		SDL2
		SDL2_gfx
		SDL2_image
		SDL2_mixer
		SDL2_net
		SDL2_ttf
		allegro
		aspell
		cairo
		cunit
		curl
		devil
		expat
		fltk
		flac
		freealut
		freeglut
		freetype
		glfw
		glew
		gmp
		gsl
		gtk2
		gtk3
		gtkglext
		goocanvas
		libcaca
		libffi
		libgd
		libglade
		libharu
		libjpeg-turbo
		libmariadbclient
		libmodplug
		libogg
		libpng
		libsndfile
		libtre
		libtiff
		libvorbis
		libxml2
		libxmp
		libxslt
		libzip
		lua51
		mxml
		mpg123
		openal
		ode
		opus
		opusfile
		pcre
		pcre2
		pdcurses
		portaudio
		postgresql
		raylib
		sqlite3
		zeromq
		zlib
	)
	local pkg

	msg "Updating MSYS2 package database"
	run pacman -Sy --noconfirm

	msg "Installing MSYS2 packaging dependencies"
	run pacman -S --needed --noconfirm "${msys_packages[@]}"

	msg "Installing MinGW toolchain groups"
	run pacman -S --needed --noconfirm \
		mingw-w64-i686-toolchain \
		mingw-w64-x86_64-toolchain

	msg "Installing MinGW dependency sets"
	for pkg in "${mingw_suffixes[@]}"; do
		for arch in i686 x86_64; do
			local fullpkg="mingw-w64-${arch}-${pkg}"

			# Some optional example libraries are only published for one
			# MSYS2 MinGW architecture.  Install every package that exists,
			# but do not make the whole Windows package build fail because
			# an optional binding library was dropped from one repository.
			if pacman -Si "$fullpkg" >/dev/null 2>&1; then
				run pacman -S --needed --noconfirm "$fullpkg"
			else
				echo "WARNING: optional MSYS2 package not found: $fullpkg" >&2
			fi
		done
	done
}

##############################################################################
# Per-target build
##############################################################################

build_target() {
	local arch="$1"
	local mingw_root="$2"
	local target="$3"
	local target_triplet="$4"
	local worktree="$WORKROOT/$target"
	local stagedir="$STAGEROOT/$target"
	local bootstrap_sources_dir="$worktree/bootstrap/$target"
	local saved_path="$PATH"
	local host_fbc=""
	local build_fbc=""
	local cc="$mingw_root/bin/gcc.exe"
	local cxx="$mingw_root/bin/g++.exe"
	local ar="$mingw_root/bin/ar.exe"
	local as="$mingw_root/bin/as.exe"
	local ld="$mingw_root/bin/ld.exe"
	local ranlib="$mingw_root/bin/ranlib.exe"
	local strip="$mingw_root/bin/strip.exe"
	local dlltool="$mingw_root/bin/dlltool.exe"

	msg "Preparing $target worktree"
	sanitize_source_tree "$target_triplet"
	if [ "$SKIP_SOURCE_SYNC" -eq 0 ] || [ ! -d "$worktree" ]; then
		rm -rf "$worktree"
		sync_source_tree "$worktree"
	fi

	rm -rf "$stagedir"
	mkdir -p "$stagedir"

	cd "$worktree"
	PATH="$worktree/bin:$ROOT/bin:$mingw_root/bin:/usr/bin:$saved_path"
	export PATH

	host_fbc="$(detect_fbc \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc64.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc32.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc}" \
		"$WORKROOT/win64/bin/fbc.exe" \
		"$WORKROOT/win64/bootstrap/fbc.exe" \
		"$WORKROOT/win32/bin/fbc.exe" \
		"$WORKROOT/win32/bootstrap/fbc.exe" \
		"$worktree/bin/fbc.exe" \
		"$worktree/bootstrap/fbc.exe" \
		"$ROOT/bin/fbc.exe" \
		"$ROOT/bootstrap/fbc.exe" \
		|| true)"

	if [ -d "$bootstrap_sources_dir" ] && find "$bootstrap_sources_dir" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
		msg "Bootstrap sources already present for $target"
	elif [ -n "$host_fbc" ]; then
		msg "Emitting $target bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-emit \
			BUILD_FBC="$host_fbc" \
			TARGET_TRIPLET="$target_triplet" \
			CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"
	else
		msg "No direct bootstrap compiler available for $target; seeding from peer bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-seed-peer \
			TARGET_TRIPLET="$target_triplet" \
			CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"
	fi

	msg "Cleaning $target worktree"
	run make clean TARGET_TRIPLET="$target_triplet" || true

	msg "Building $target bootstrap compiler ($JOBS threads)"
	run make -j"$JOBS" \
		bootstrap-minimal \
		TARGET_TRIPLET="$target_triplet" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"

	[ -f "$worktree/bootstrap/fbc.exe" ] || fail "bootstrap-minimal did not produce bootstrap/fbc.exe for $target"
	build_fbc="$worktree/bin/fbc.exe"
	[ -f "$build_fbc" ] || fail "bootstrap-minimal did not install bin/fbc.exe for $target"

	msg "Resetting compiler/runtime outputs for standalone packaging"
	run make clean-compiler clean-libs TARGET_TRIPLET="$target_triplet" ENABLE_STANDALONE=1
	rm -f "$worktree/fbc.exe" "$worktree/fbc-new.exe"

	msg "Building $target compiler and runtime ($JOBS threads)"
	run make -j"$JOBS" \
		all \
		ENABLE_STANDALONE=1 \
		BUILD_FBC="$build_fbc" \
		TARGET_TRIPLET="$target_triplet" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"

	msg "Installing $target into staging"
	run make install \
		DESTDIR="$stagedir" \
		prefix="/$INSTALL_SUBDIR" \
		ENABLE_STANDALONE=1 \
		BUILD_FBC="$build_fbc" \
		TARGET_TRIPLET="$target_triplet" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool"

	[ -f "$stagedir/fbc.exe" ] || fail "staged compiler missing for $target"

	cd "$ROOT"
	PATH="$saved_path"
	export PATH
}

##############################################################################
# Distribution assembly
##############################################################################

copy_tool_bins() {
	local srcbin="$1"
	local dstbin="$2"
	local tool

	mkdir -p "$dstbin"

	for tool in \
		ar as c++ cpp dlltool g++ gcc gcc-ar gcc-nm gcc-ranlib gprof \
		ld ld.bfd nm objcopy objdump ranlib readelf strip windres
	do
		if [ -f "$srcbin/$tool.exe" ]; then
			cp -a "$srcbin/$tool.exe" "$dstbin/"
		fi
	done

	find "$srcbin" -maxdepth 1 -type f \( -iname '*.dll' -o -iname 'zlib1.dll' \) -exec cp -a {} "$dstbin/" \;
}

copy_arch_toolchain() {
	local arch="$1"
	local mingw_root="$2"
	local triplet="$3"
	local gcc_version
	local gcc_libdir
	local gcc_support_dir
	local dll
	local lib

	gcc_version="$($mingw_root/bin/gcc -dumpfullversion -dumpversion)"
	[ -n "$gcc_version" ] || fail "could not determine GCC version for $arch"
	gcc_libdir="$mingw_root/lib/gcc/$triplet/$gcc_version"
	gcc_support_dir="$DISTROOT/bin/lib/gcc/$triplet/$gcc_version"

	msg "Bundling $arch MinGW toolchain"
	copy_tool_bins "$mingw_root/bin" "$DISTROOT/bin/$arch"

	if [ -d "$gcc_libdir" ]; then
		copy_tree "$gcc_libdir" "$gcc_support_dir"
		for dll in \
			libgcc_s*.dll \
			libgmp-*.dll \
			libisl-*.dll \
			libmpc-*.dll \
			libmpfr-*.dll \
			libwinpthread-*.dll \
			libzstd.dll \
			zlib1.dll
		do
			for f in "$mingw_root/bin"/$dll; do
				[ -f "$f" ] || continue
				cp -a "$f" "$gcc_support_dir/"
			done
		done
		for lib in libgcc.a libgcc_eh.a; do
			if [ -f "$gcc_libdir/$lib" ]; then
				cp -a "$gcc_libdir/$lib" "$DISTROOT/lib/$arch/"
			fi
		done
	fi

	if [ -d "$mingw_root/$triplet/lib" ]; then
		copy_dir_files "$mingw_root/$triplet/lib" "$DISTROOT/lib/$arch"
	fi

	copy_dir_files "$mingw_root/lib" "$DISTROOT/lib/$arch"
	create_arch_library_aliases "$DISTROOT/lib/$arch"
}

assemble_distribution() {
	local win32_stage="$STAGEROOT/win32"
	local win64_stage="$STAGEROOT/win64"

	DISTROOT="$DISTROOT_BASE/$DISTNAME"
	rm -rf "$DISTROOT"
	mkdir -p "$DISTROOT/bin" "$DISTROOT/lib/win32" "$DISTROOT/lib/win64"

	sanitize_source_tree "$TRIPLET64"

	msg "Copying top-level FreeBASIC content"
	copy_tree "$ROOT/doc" "$DISTROOT/doc"
	copy_tree "$ROOT/examples" "$DISTROOT/examples"
	copy_tree "$ROOT/inc" "$DISTROOT/inc"
	cp -a "$ROOT/changelog.txt" "$DISTROOT/"
	cp -a "$ROOT/readme.txt" "$DISTROOT/"

	if [ -f "$win32_stage/fbc.exe" ]; then
		cp -a "$win32_stage/fbc.exe" "$DISTROOT/fbc32.exe"
	fi
	if [ -f "$win64_stage/fbc.exe" ]; then
		cp -a "$win64_stage/fbc.exe" "$DISTROOT/fbc64.exe"
	fi

	[ -f "$DISTROOT/fbc32.exe" ] || fail "missing staged fbc32.exe"
	[ -f "$DISTROOT/fbc64.exe" ] || fail "missing staged fbc64.exe"

	copy_arch_toolchain win32 "$MINGW32_ROOT" "$TRIPLET32"
	copy_arch_toolchain win64 "$MINGW64_ROOT" "$TRIPLET64"

	msg "Merging staged FreeBASIC runtime libraries"
	copy_dir_files "$win32_stage/lib/win32" "$DISTROOT/lib/win32"
	copy_dir_files "$win64_stage/lib/win64" "$DISTROOT/lib/win64"
}

##############################################################################
# Packaging
##############################################################################

create_zip() {
	local zipfile="$OUT/${DISTNAME}.zip"
	msg "Creating distribution zip"
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
SetCompressor /SOLID lzma
RequestExecutionLevel admin

Name "FreeBASIC ${FBVERSION}"
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
	;
	; Explorer caches the environment block that new console windows inherit.
	; After changing the registry PATH, broadcast WM_SETTINGCHANGE through
	; user32 so newly opened shells see the updated PATH without logoff.
	System::Call 'User32::SendMessageTimeoutA(i 0xffff, i \${WM_SETTINGCHANGE}, i 0, t "Environment", i 0, i 5000, *i .r0)'
FunctionEnd

Function un.RefreshEnvironment
	System::Call 'User32::SendMessageTimeoutA(i 0xffff, i \${WM_SETTINGCHANGE}, i 0, t "Environment", i 0, i 5000, *i .r0)'
FunctionEnd

Function AddInstallDirToPath
	ReadRegStr \$0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path"
	StrCpy \$1 ";\$0;"
	\${StrStr} \$2 \$1 ";\$INSTDIR;"
	StrCmp \$2 "" 0 done
	StrCmp \$0 "" 0 +2
		StrCpy \$0 "\$INSTDIR"
	StrCmp \$0 "\$INSTDIR" done 0
	StrCpy \$0 "\$0;\$INSTDIR"
	WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "\$0"
	Call RefreshEnvironment
	done:
FunctionEnd

Function AddInstallDirToMsys2
	;
	; MSYS2 login shells do not read the Windows PATH exactly as normal
	; console programs do.  Add a small profile.d fragment so pacman,
	; make, and test shells can find the installed FreeBASIC compiler
	; without users editing /etc/profile by hand.
	Call WriteMsys2ProfileFile64
	Call WriteMsys2ProfileFile32
FunctionEnd

Function WriteMsys2ProfileFile64
	IfFileExists "C:\\msys64\\etc\\profile.d\\*.*" 0 done
	FileOpen \$0 "C:\\msys64\\etc\\profile.d\\freebasic.sh" w
	IfErrors done
	Call WriteMsys2ProfileFileContents
	FileClose \$0
	done:
FunctionEnd

Function WriteMsys2ProfileFile32
	IfFileExists "C:\\msys32\\etc\\profile.d\\*.*" 0 done
	FileOpen \$0 "C:\\msys32\\etc\\profile.d\\freebasic.sh" w
	IfErrors done
	Call WriteMsys2ProfileFileContents
	FileClose \$0
	done:
FunctionEnd

Function WriteMsys2ProfileFileContents
	FileWrite \$0 "# FreeBASIC installer PATH setup$\r$\n"
	FileWrite \$0 "if command -v cygpath >/dev/null 2>&1; then$\r$\n"
	FileWrite \$0 "  _freebasic_prefix=\`cygpath -u '$INSTDIR'\`$\r$\n"
	FileWrite \$0 "else$\r$\n"
	FileWrite \$0 "  _freebasic_prefix=/c/FreeBASIC$\r$\n"
	FileWrite \$0 "fi$\r$\n"
	FileWrite \$0 "case :\$$PATH: in$\r$\n"
	FileWrite \$0 "  *:\${_freebasic_prefix}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\${_freebasic_prefix}:\$$PATH ;;$\r$\n"
	FileWrite \$0 "esac$\r$\n"
	FileWrite \$0 "unset _freebasic_prefix$\r$\n"
FunctionEnd

Function un.RemoveInstallDirFromPath
	ReadRegStr \$0 HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path"
	StrCmp \$0 "" done
	StrCpy \$1 ";\$0;"
	\${UnStrRep} \$1 \$1 ";\$INSTDIR;" ";"
	\${UnStrRep} \$1 \$1 ";;" ";"
	StrCpy \$0 \$1
	StrCpy \$2 \$0 1
	StrCmp \$2 ";" 0 +2
		StrCpy \$0 \$0 "" 1
	StrLen \$2 \$0
	IntCmp \$2 0 done done done
	IntOp \$2 \$2 - 1
	StrCpy \$3 \$0 1 \$2
	StrCmp \$3 ";" 0 +2
		StrCpy \$0 \$0 \$2
	WriteRegExpandStr HKLM "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" "Path" "\$0"
	Call un.RefreshEnvironment
	done:
FunctionEnd

Function un.RemoveInstallDirFromMsys2
	Delete "C:\\msys64\\etc\\profile.d\\freebasic.sh"
	Delete "C:\\msys32\\etc\\profile.d\\freebasic.sh"
FunctionEnd

Section "Install"
	SetOutPath "\$INSTDIR"
	File /r "$dist_win\\*"
	WriteUninstaller "\$INSTDIR\\uninstall.exe"
	Call AddInstallDirToPath
	Call AddInstallDirToMsys2
SectionEnd

Section "Uninstall"
	Call un.RemoveInstallDirFromPath
	Call un.RemoveInstallDirFromMsys2
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
	local saved_path="$PATH"

	msg "Validating packaged compilers"
	rm -rf "$validate_dir"
	mkdir -p "$validate_dir"

	cat > "$validate_dir/hello.bas" <<'EOF'
print "FreeBASIC package test OK"
EOF

	PATH="/usr/bin:/c/Windows/System32:/c/Windows"
	export PATH

	run "$DISTROOT/fbc64.exe" "$validate_dir/hello.bas" -x "$validate_dir/hello64.exe"
	[ "$("$validate_dir/hello64.exe")" = "FreeBASIC package test OK" ] || fail "packaged fbc64.exe produced bad output"

	run "$DISTROOT/fbc32.exe" "$validate_dir/hello.bas" -x "$validate_dir/hello32.exe"
	[ "$("$validate_dir/hello32.exe")" = "FreeBASIC package test OK" ] || fail "packaged fbc32.exe produced bad output"

	PATH="$saved_path"
	export PATH
}

##############################################################################
# Main
##############################################################################

if [ "$SKIP_DEPS" -eq 0 ]; then
	install_dependencies
fi

GCC_VERSION="$($MINGW64_ROOT/bin/gcc -dumpfullversion -dumpversion)"
[ -n "$GCC_VERSION" ] || fail "could not determine GCC version from $MINGW64_ROOT/bin/gcc"
DISTNAME="${DISTNAME_BASE}-gcc-${GCC_VERSION}"
DISTROOT="$DISTROOT_BASE/$DISTNAME"

if [ "$SKIP_BUILD64" -eq 0 ]; then
	build_target win64 "$MINGW64_ROOT" win64 "$TRIPLET64"
fi

if [ "$SKIP_BUILD32" -eq 0 ]; then
	build_target win32 "$MINGW32_ROOT" win32 "$TRIPLET32"
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
