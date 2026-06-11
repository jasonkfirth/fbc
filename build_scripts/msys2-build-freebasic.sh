#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-build-freebasic.sh
#
# Build a self-contained Windows FreeBASIC distribution from MSYS2.
# Produces a desktop win32/win64 package tree and a separate Windows ARM64
# package tree, each with its own .zip archive and NSIS installer.
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
SKIP_BUILDARM64=0
SKIP_PACKAGE=0
SKIP_INSTALLER=0
SKIP_VALIDATE=0
KEEP_BUILDROOT=0
QEMU_ARM64_VALIDATE=0

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-build-freebasic.sh [options]

Options:
  --skip-deps         Do not install or update MSYS2 packages
  --skip-source-sync  Reuse the existing per-target worktrees
  --skip-build32      Skip the win32 build
  --skip-build64      Skip the win64 build
  --skip-buildarm64   Skip the Windows ARM64 build
  --skip-package      Skip distribution tree assembly and zip creation
  --skip-installer    Skip NSIS installer creation
  --skip-validate     Skip packaged compiler validation
  --keep-buildroot    Keep the build root on failure or success
  --qemu-arm64-validate
                       Run fbcarm64.exe in a Windows ARM64 QEMU guest
  --help              Show this help text

Environment:
  BUILDROOT           Temporary build root (default: <repo>/.build-msys2)
  OUT                 Output directory (default: <repo>/out/mingw32)
  HOST_FBC_ROOT       Optional existing FreeBASIC install used as host compiler fallback
  NSIS_EXE            Explicit makensis path (default: /mingw64/bin/makensis.exe)
  SOURCE_SYNC_EXTRA_EXCLUDES
                       Optional space-separated rsync exclude patterns for local scratch trees
  QEMU_ARM64_DISK     Bootable Windows ARM64 QEMU disk for --qemu-arm64-validate
  QEMU_ARM64_SSH_USER Windows SSH user for --qemu-arm64-validate
  QEMU_ARM64_SSH_PORT Windows SSH forwarded port (default: 2222)
  QEMU_ARM64_SSH_KEY  Optional SSH private key
  QEMU_AARCH64_EFI    Optional AArch64 UEFI firmware path
  JOBS                Parallel make job count (default: detected CPU core count)

Related port package scripts:
  build_scripts/msys2-build-freebasic-js.sh
  build_scripts/msys2-build-freebasic-android.sh
  build_scripts/msys2-build-freebasic-wii.sh
  build_scripts/msys2-build-freebasic-xbox.sh
EOF
}

for arg in "$@"; do
	case "$arg" in
		--skip-deps) SKIP_DEPS=1 ;;
		--skip-source-sync) SKIP_SOURCE_SYNC=1 ;;
		--skip-build32) SKIP_BUILD32=1 ;;
		--skip-build64) SKIP_BUILD64=1 ;;
		--skip-buildarm64) SKIP_BUILDARM64=1 ;;
		--skip-package) SKIP_PACKAGE=1 ;;
		--skip-installer) SKIP_INSTALLER=1 ;;
		--skip-validate) SKIP_VALIDATE=1 ;;
		--keep-buildroot) KEEP_BUILDROOT=1 ;;
		--qemu-arm64-validate) QEMU_ARM64_VALIDATE=1 ;;
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

first_existing_tool() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -x "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

find_clang_resource_dir() {
	local sysroot="$1"
	local resource_dir

	resource_dir="$(
		{
			find "$sysroot/lib/clang" -mindepth 1 -maxdepth 1 -type d 2>/dev/null || true
		} |
			sort -V |
			tail -n 1
	)"

	[ -n "$resource_dir" ] || return 1
	echo "$resource_dir"
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

remove_stale_crt_import_libs() {
	local libdir="$1"

	[ -d "$libdir" ] || return 0

	# The MinGW package must use the CRT import libraries supplied by the
	# active toolchain.  Old libmsvcrt*.dll.a files copied from legacy
	# winlibs can override the current toolchain files and break otherwise
	# valid builds.
	rm -f "$libdir"/libmsvcrt*.dll.a
}

assert_no_stale_crt_import_libs() {
	local libdir="$1"

	[ -d "$libdir" ] || return 0

	if find "$libdir" -maxdepth 1 -type f -name 'libmsvcrt*.dll.a' -print -quit | grep -q .; then
		find "$libdir" -maxdepth 1 -type f -name 'libmsvcrt*.dll.a' -print >&2
		fail "stale CRT import libraries were packaged in $libdir"
	fi
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

copy_legacy_winlibs() {
	local arch="$1"
	local libdir="$2"
	local bindir="$3"
	local legacy_lib_dir="$ROOT/contrib/winlibs-legacy/lib/$arch"
	local legacy_bin_dir="$ROOT/contrib/winlibs-legacy/bin/$arch"

	if [ -d "$legacy_lib_dir" ]; then
		msg "Bundling legacy $arch import libraries"
		copy_dir_files "$legacy_lib_dir" "$libdir"
	fi

	if [ -d "$legacy_bin_dir" ]; then
		msg "Bundling legacy $arch runtime DLLs"
		copy_dir_files "$legacy_bin_dir" "$bindir"
	fi
}

copy_distribution_common_content() {
	local package_root="$1"

	msg "Copying top-level FreeBASIC content to $(basename "$package_root")"
	copy_tree "$ROOT/doc" "$package_root/doc"
	copy_examples_tree "$package_root/examples"
	copy_tree "$ROOT/inc" "$package_root/inc"
	cp -a "$ROOT/changelog.txt" "$package_root/"
	cp -a "$ROOT/readme.txt" "$package_root/"
	write_platform_package_notes "$package_root"
}

sync_source_tree() {
	local dst="$1"
	local extra_exclude
	local rsync_excludes=()
	shift

	for extra_exclude in ${SOURCE_SYNC_EXTRA_EXCLUDES:-}; do
		rsync_excludes+=(--exclude "$extra_exclude")
	done

	mkdir -p "$dst"
	if have rsync; then
		run rsync -a --delete --delete-excluded --prune-empty-dirs \
			--exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
			"${rsync_excludes[@]}" \
			"$@" \
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

detect_external_fbc() {
	local target="$1"
	local candidates=()

	candidates+=(
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc64.exe}"
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc32.exe}"
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc.exe}"
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc}"
	)

	if [ "$target" != "win64" ]; then
		candidates+=(
			"$WORKROOT/win64/bin/fbc.exe"
			"$WORKROOT/win64/bootstrap/fbc.exe"
		)
	fi

	if [ "$target" != "win32" ]; then
		candidates+=(
			"$WORKROOT/win32/bin/fbc.exe"
			"$WORKROOT/win32/bootstrap/fbc.exe"
		)
	fi

	if [ "$target" != "win32-aarch64" ]; then
		candidates+=(
			"$WORKROOT/win32-aarch64/bin/fbc.exe"
			"$WORKROOT/win32-aarch64/bootstrap/fbc.exe"
		)
	fi

	candidates+=(
		"$ROOT/bin/fbc.exe"
		"$ROOT/bootstrap/fbc.exe"
	)

	detect_fbc "${candidates[@]}"
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
ARM64_DISTNAME_BASE="FreeBASIC-${FBVERSION}-winlibs-arm64"
INSTALL_SUBDIR="package"
INSTALL_DIR_WIN='C:\FreeBASIC'
HOST_FBC_ROOT="${HOST_FBC_ROOT:-}"
PACKAGED_DISTROOTS=()

MINGW32_ROOT="/mingw32"
MINGW64_ROOT="/mingw64"
CLANGARM64_ROOT="/clangarm64"
TRIPLET32="i686-w64-mingw32"
TRIPLET64="x86_64-w64-mingw32"
TRIPLETARM64="aarch64-w64-mingw32"
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

	if [ "$QEMU_ARM64_VALIDATE" -ne 0 ]; then
		msys_packages+=(openssh)
	fi

	msg "Updating MSYS2 package database"
	run pacman -Sy --noconfirm

	msg "Installing MSYS2 packaging dependencies"
	run pacman -S --needed --noconfirm "${msys_packages[@]}"

	if [ "$QEMU_ARM64_VALIDATE" -ne 0 ]; then
		msg "Installing QEMU for Windows ARM64 validation"
		run pacman -S --needed --noconfirm mingw-w64-x86_64-qemu
	fi

	msg "Installing MinGW toolchain groups"
	run pacman -S --needed --noconfirm \
		mingw-w64-i686-toolchain \
		mingw-w64-x86_64-toolchain

	if [ "$SKIP_BUILDARM64" -eq 0 ]; then
		msg "Installing Windows ARM64 cross-build toolchains"
		run pacman -S --needed --noconfirm \
			mingw-w64-x86_64-clang \
			mingw-w64-x86_64-lld \
			mingw-w64-x86_64-llvm \
			mingw-w64-x86_64-llvm-tools \
			mingw-w64-x86_64-tools \
			mingw-w64-clang-aarch64-compiler-rt \
			mingw-w64-clang-aarch64-crt \
			mingw-w64-clang-aarch64-headers \
			mingw-w64-clang-aarch64-libunwind \
			mingw-w64-clang-aarch64-libwinpthread \
			mingw-w64-clang-aarch64-winpthreads
	fi

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

		if [ "$SKIP_BUILDARM64" -eq 0 ]; then
			local fullpkg="mingw-w64-clang-aarch64-${pkg}"

			if pacman -Si "$fullpkg" >/dev/null 2>&1; then
				run pacman -S --needed --noconfirm "$fullpkg"
			else
				echo "WARNING: optional MSYS2 package not found: $fullpkg" >&2
			fi
		fi
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
	local cc
	local cxx
	local ar
	local as
	local ld
	local ranlib
	local strip
	local dlltool
	local windres
	local clang
	local tool_root="$mingw_root"
	local tool_path_root="$mingw_root"
	local clang_resource_dir
	local clang_target_flags
	local fbc_clang_target_flags=""
	local bootfbcgen="gcc"
	local build_fbcflags=""

	if [ "$target" = "win32-aarch64" ]; then
		tool_root="$MINGW64_ROOT"
		tool_path_root="$MINGW64_ROOT"
		clang_resource_dir="$(find_clang_resource_dir "$CLANGARM64_ROOT")" || fail "clang resource directory not found under $CLANGARM64_ROOT"
		clang_target_flags="-Qunused-arguments --target=$target_triplet --sysroot=$CLANGARM64_ROOT -resource-dir $clang_resource_dir -fuse-ld=lld --rtlib=compiler-rt --unwindlib=libunwind"
		fbc_clang_target_flags="-gen clang -Wc -Qunused-arguments -Wc --target=$target_triplet -Wc --sysroot=$CLANGARM64_ROOT -Wc -resource-dir -Wc $clang_resource_dir -Wa -Qunused-arguments -Wa --target=$target_triplet -Wa --sysroot=$CLANGARM64_ROOT -Wa -resource-dir -Wa $clang_resource_dir"
		bootfbcgen="clang"
		build_fbcflags="$fbc_clang_target_flags"

		clang="$(first_existing_tool "$tool_root/bin/clang.exe")" || fail "host clang not found under $tool_root"
		cc="$clang $clang_target_flags"
		cxx="$(first_existing_tool "$tool_root/bin/clang++.exe")" || fail "host clang++ not found under $tool_root"
		cxx="$cxx $clang_target_flags -stdlib=libc++"
		ar="$(first_existing_tool "$tool_root/bin/llvm-ar.exe")" || fail "host llvm-ar not found under $tool_root"
		as="$clang"
		ld="$(first_existing_tool "$tool_root/bin/ld.lld.exe")" || fail "host ld.lld not found under $tool_root"
		ranlib="$(first_existing_tool "$tool_root/bin/llvm-ranlib.exe")" || fail "host llvm-ranlib not found under $tool_root"
		strip="$(first_existing_tool "$tool_root/bin/llvm-strip.exe")" || fail "host llvm-strip not found under $tool_root"
		dlltool="$(first_existing_tool "$tool_root/bin/llvm-dlltool.exe")" || fail "host llvm-dlltool not found under $tool_root"
		windres="$(first_existing_tool "$tool_root/bin/llvm-windres.exe")" || fail "host llvm-windres not found under $tool_root"
	else
		cc="$(first_existing_tool "$mingw_root/bin/gcc.exe" "$mingw_root/bin/clang.exe")" || fail "C compiler not found under $mingw_root"
		cxx="$(first_existing_tool "$mingw_root/bin/g++.exe" "$mingw_root/bin/clang++.exe")" || fail "C++ compiler not found under $mingw_root"
		ar="$(first_existing_tool "$mingw_root/bin/ar.exe" "$mingw_root/bin/llvm-ar.exe")" || fail "archiver not found under $mingw_root"
		as="$(first_existing_tool "$mingw_root/bin/as.exe" "$mingw_root/bin/clang.exe")" || fail "assembler not found under $mingw_root"
		ld="$(first_existing_tool "$mingw_root/bin/ld.exe" "$mingw_root/bin/ld.lld.exe")" || fail "linker not found under $mingw_root"
		ranlib="$(first_existing_tool "$mingw_root/bin/ranlib.exe" "$mingw_root/bin/llvm-ranlib.exe")" || fail "ranlib not found under $mingw_root"
		strip="$(first_existing_tool "$mingw_root/bin/strip.exe" "$mingw_root/bin/llvm-strip.exe")" || fail "strip not found under $mingw_root"
		dlltool="$(first_existing_tool "$mingw_root/bin/dlltool.exe" "$mingw_root/bin/llvm-dlltool.exe")" || fail "dlltool not found under $mingw_root"
		windres="$(first_existing_tool "$mingw_root/bin/windres.exe" "$mingw_root/bin/llvm-windres.exe")" || fail "windres not found under $mingw_root"
		clang="$(first_existing_tool "$mingw_root/bin/clang.exe" 2>/dev/null || true)"
	fi

	msg "Preparing $target worktree"
	PATH="$tool_path_root/bin:/usr/bin:$saved_path"
	export PATH
	sanitize_source_tree "$target_triplet"
	PATH="$saved_path"
	export PATH
	host_fbc="$(detect_external_fbc "$target" || true)"
	if [ "$SKIP_SOURCE_SYNC" -eq 0 ] || [ ! -d "$worktree" ]; then
		rm -rf "$worktree"
		if [ -n "$host_fbc" ]; then
			# A runnable compiler can emit the target bootstrap sources inside
			# the worktree, so do not copy stale generated bootstrap trees.
			sync_source_tree "$worktree" --exclude "/bootstrap/"
		else
			sync_source_tree "$worktree"
		fi
	fi

	rm -rf "$stagedir"
	mkdir -p "$stagedir"

	cd "$worktree"
	PATH="$worktree/bin:$ROOT/bin:$tool_path_root/bin:/usr/bin:$saved_path"
	export PATH

	if [ -z "$host_fbc" ]; then
		host_fbc="$(detect_fbc \
			"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc64.exe}" \
			"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc32.exe}" \
			"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc.exe}" \
			"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc}" \
			"$WORKROOT/win64/bin/fbc.exe" \
			"$WORKROOT/win64/bootstrap/fbc.exe" \
			"$WORKROOT/win32/bin/fbc.exe" \
			"$WORKROOT/win32/bootstrap/fbc.exe" \
			"$WORKROOT/win32-aarch64/bin/fbc.exe" \
			"$WORKROOT/win32-aarch64/bootstrap/fbc.exe" \
			"$worktree/bin/fbc.exe" \
			"$worktree/bootstrap/fbc.exe" \
			"$ROOT/bin/fbc.exe" \
			"$ROOT/bootstrap/fbc.exe" \
			|| true)"
	fi

	if [ -n "$host_fbc" ]; then
		msg "Emitting fresh $target bootstrap sources"
		rm -rf "$bootstrap_sources_dir"
		run make -j"$JOBS" \
			bootstrap-emit \
			FBC_EXE="$host_fbc" \
			BUILD_FBC="$host_fbc" \
			BOOTFBCGEN="$bootfbcgen" \
			TARGET_TRIPLET="$target_triplet" \
			CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool" WINDRES="$windres" CLANG="$clang"
	elif [ -d "$bootstrap_sources_dir" ] && find "$bootstrap_sources_dir" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
		msg "Bootstrap sources already present for $target"
	else
		msg "No direct bootstrap compiler available for $target; seeding from peer bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-seed-peer \
			BOOTFBCGEN="$bootfbcgen" \
			TARGET_TRIPLET="$target_triplet" \
			CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool" WINDRES="$windres" CLANG="$clang"
	fi

	msg "Cleaning $target worktree"
	run make clean TARGET_TRIPLET="$target_triplet" || true

	msg "Building $target bootstrap compiler ($JOBS threads)"
	run make -j"$JOBS" \
		bootstrap-minimal \
		TARGET_TRIPLET="$target_triplet" \
		BUILD_FBCFLAGS="$build_fbcflags" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool" WINDRES="$windres" CLANG="$clang"

	[ -f "$worktree/bootstrap/fbc.exe" ] || fail "bootstrap-minimal did not produce bootstrap/fbc.exe for $target"
	build_fbc="$worktree/bin/fbc.exe"
	[ -f "$build_fbc" ] || fail "bootstrap-minimal did not install bin/fbc.exe for $target"
	if "$build_fbc" -version >/dev/null 2>&1; then
		:
	elif [ -n "$host_fbc" ]; then
		msg "$target bootstrap compiler is not runnable on this host; using host compiler for the full target build"
		build_fbc="$host_fbc"
	else
		fail "$target bootstrap compiler is not runnable on this host and no host compiler is available"
	fi

	msg "Resetting compiler/runtime outputs for standalone packaging"
	run make clean-compiler clean-libs TARGET_TRIPLET="$target_triplet" ENABLE_STANDALONE=1
	rm -f "$worktree/fbc.exe" "$worktree/fbc-new.exe"

	msg "Building $target compiler and runtime ($JOBS threads)"
	run make -j"$JOBS" \
		all \
		ENABLE_STANDALONE=1 \
		BUILD_FBC="$build_fbc" \
		BUILD_FBCFLAGS="$build_fbcflags" \
		BUILD_FBC_TARGET="$target" \
		BUILD_FBC_BUILDPREFIX="$target_triplet-" \
		TARGET_TRIPLET="$target_triplet" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool" WINDRES="$windres" CLANG="$clang"

	msg "Installing $target into staging"
	run make install \
		DESTDIR="$stagedir" \
		prefix="/$INSTALL_SUBDIR" \
		ENABLE_STANDALONE=1 \
		BUILD_FBC="$build_fbc" \
		BUILD_FBCFLAGS="$build_fbcflags" \
		BUILD_FBC_TARGET="$target" \
		BUILD_FBC_BUILDPREFIX="$target_triplet-" \
		TARGET_TRIPLET="$target_triplet" \
		CC="$cc" CXX="$cxx" AR="$ar" AS="$as" LD="$ld" RANLIB="$ranlib" STRIP="$strip" DLLTOOL="$dlltool" WINDRES="$windres" CLANG="$clang"

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
		ar as c++ clang clang++ cpp dlltool g++ gcc gcc-ar gcc-nm gcc-ranlib gprof \
		ld ld.bfd ld.lld lld llvm-ar llvm-dlltool llvm-nm llvm-objcopy \
		llvm-objdump llvm-ranlib llvm-readobj llvm-strip llvm-windres nm \
		objcopy objdump ranlib readelf strip windres
	do
		if [ -f "$srcbin/$tool.exe" ]; then
			cp -a "$srcbin/$tool.exe" "$dstbin/"
		fi
	done

	find "$srcbin" -maxdepth 1 -type f \( -iname '*.dll' -o -iname 'zlib1.dll' \) -exec cp -a {} "$dstbin/" \;

	if [ ! -f "$dstbin/ld.exe" ] && [ -f "$dstbin/ld.lld.exe" ]; then
		cp -a "$dstbin/ld.lld.exe" "$dstbin/ld.exe"
	fi
	if [ ! -f "$dstbin/ar.exe" ] && [ -f "$dstbin/llvm-ar.exe" ]; then
		cp -a "$dstbin/llvm-ar.exe" "$dstbin/ar.exe"
	fi
	if [ ! -f "$dstbin/ranlib.exe" ] && [ -f "$dstbin/llvm-ranlib.exe" ]; then
		cp -a "$dstbin/llvm-ranlib.exe" "$dstbin/ranlib.exe"
	fi
	if [ ! -f "$dstbin/dlltool.exe" ] && [ -f "$dstbin/llvm-dlltool.exe" ]; then
		cp -a "$dstbin/llvm-dlltool.exe" "$dstbin/dlltool.exe"
	fi
	if [ ! -f "$dstbin/windres.exe" ] && [ -f "$dstbin/llvm-windres.exe" ]; then
		cp -a "$dstbin/llvm-windres.exe" "$dstbin/windres.exe"
	fi
	if [ ! -f "$dstbin/strip.exe" ] && [ -f "$dstbin/llvm-strip.exe" ]; then
		cp -a "$dstbin/llvm-strip.exe" "$dstbin/strip.exe"
	fi
	if [ ! -f "$dstbin/nm.exe" ] && [ -f "$dstbin/llvm-nm.exe" ]; then
		cp -a "$dstbin/llvm-nm.exe" "$dstbin/nm.exe"
	fi
	if [ ! -f "$dstbin/objcopy.exe" ] && [ -f "$dstbin/llvm-objcopy.exe" ]; then
		cp -a "$dstbin/llvm-objcopy.exe" "$dstbin/objcopy.exe"
	fi
	if [ ! -f "$dstbin/objdump.exe" ] && [ -f "$dstbin/llvm-objdump.exe" ]; then
		cp -a "$dstbin/llvm-objdump.exe" "$dstbin/objdump.exe"
	fi
}

copy_arch_toolchain() {
	local arch="$1"
	local mingw_root="$2"
	local triplet="$3"
	local tool_root="${4:-$mingw_root}"
	local package_root="${5:-$DISTROOT}"
	local gcc_version
	local gcc_libdir
	local gcc_support_dir
	local clang_libdir
	local clang_builtins
	local dll
	local lib

	msg "Bundling $arch MinGW toolchain"
	copy_tool_bins "$tool_root/bin" "$package_root/bin/$arch"

	# The bundled GCC driver is relocated under bin/$arch.  Its built-in
	# search path resolves the MinGW CRT headers through bin/$triplet/include,
	# so copy the headers there instead of depending on a live /mingw32 or
	# /mingw64 tree being present on the user's machine.
	if [ -d "$mingw_root/include" ]; then
		copy_tree "$mingw_root/include" "$package_root/bin/$triplet/include"
	fi
	if [ -d "$mingw_root/$triplet/include" ]; then
		copy_tree "$mingw_root/$triplet/include" "$package_root/bin/$triplet/include"
	fi

	if [ -x "$mingw_root/bin/gcc.exe" ]; then
		gcc_version="$($mingw_root/bin/gcc -dumpfullversion -dumpversion)"
		[ -n "$gcc_version" ] || fail "could not determine GCC version for $arch"
		gcc_libdir="$mingw_root/lib/gcc/$triplet/$gcc_version"
		gcc_support_dir="$package_root/bin/lib/gcc/$triplet/$gcc_version"

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
					cp -a "$gcc_libdir/$lib" "$package_root/lib/$arch/"
				fi
			done
		fi
	fi

	clang_libdir="$mingw_root/lib/clang"
	if [ -d "$clang_libdir" ]; then
		copy_tree "$clang_libdir" "$package_root/bin/lib/clang"
	fi

	if [ -d "$mingw_root/$triplet/lib" ]; then
		copy_dir_files "$mingw_root/$triplet/lib" "$package_root/lib/$arch"
	fi

	copy_dir_files "$mingw_root/lib" "$package_root/lib/$arch"
	if [ "$arch" = "win32-aarch64" ] && [ ! -f "$package_root/lib/$arch/libgcc.a" ]; then
		clang_builtins="$(
			{
				find "$mingw_root/lib/clang" -type f -path '*/lib/windows/libclang_rt.builtins-aarch64.a' 2>/dev/null || true
			} |
				sort -V |
				tail -n 1
		)"
		[ -n "$clang_builtins" ] || fail "could not find ARM64 clang builtins under $mingw_root"
		cp -a "$clang_builtins" "$package_root/lib/$arch/libgcc.a"
	fi
	create_arch_library_aliases "$package_root/lib/$arch"
	copy_legacy_winlibs "$arch" "$package_root/lib/$arch" "$package_root/bin/$arch"
	remove_stale_crt_import_libs "$package_root/lib/$arch"
	assert_no_stale_crt_import_libs "$package_root/lib/$arch"
}

write_platform_package_notes() {
	local package_root="${1:-$DISTROOT}"

	cat > "$package_root/readme-platform-packages.txt" <<'EOF'
FreeBASIC platform package notes
================================

The standard Windows installer contains the Win32 and Win64 compilers.
The Windows ARM64 compiler is packaged as a separate installer because it is
for Windows-on-Arm machines and uses the MSYS2 CLANGARM64 toolchain.

The ARM64 compiler can be smoke-tested under QEMU once a Windows-on-Arm VM has
OpenSSH enabled:

  build_scripts/msys2-qemu-windows-arm64-smoke.sh --disk win11-arm64.qcow2 --ssh-user USER

The non-desktop game targets use dedicated package scripts because they need
extra toolchains and usually need assets staged with the runnable artifact:

  build_scripts/msys2-build-freebasic-js.sh       browser/Node.js via Emscripten
  build_scripts/msys2-build-freebasic-android.sh  Android APKs
  build_scripts/msys2-build-freebasic-wii.sh      Wii DOL/homebrew folders
  build_scripts/msys2-build-freebasic-xbox.sh     Xbox XBE/XISO packages

For games that load files from the current directory, use the port helpers
rather than hand-copying files after every build:

  fbc-js-app.cmd --assets game-folder game.bas
  fbc-android --assets game-folder game.bas
  fbc-wii.cmd --bundle build\mygame --assets game-folder game.bas
  fbc-xbox-xiso.cmd program.xbe program.iso --assets game-folder

Those helpers do not change the FreeBASIC language or runtime APIs.  They only
put the compiled program and its files into the layout expected by the target.
EOF
}

assemble_distribution() {
	local win32_stage="$STAGEROOT/win32"
	local win64_stage="$STAGEROOT/win64"
	local winarm64_stage="$STAGEROOT/win32-aarch64"
	local include_arm64=0

	PACKAGED_DISTROOTS=()
	DISTROOT="$DISTROOT_BASE/$DISTNAME"
	rm -rf "$DISTROOT"
	mkdir -p "$DISTROOT/bin" "$DISTROOT/lib/win32" "$DISTROOT/lib/win64"

	if [ -f "$winarm64_stage/fbc.exe" ]; then
		include_arm64=1
	elif [ "$SKIP_BUILDARM64" -eq 0 ]; then
		fail "missing staged fbcarm64.exe"
	fi

	sanitize_source_tree "$TRIPLET64"

	copy_distribution_common_content "$DISTROOT"

	if [ -f "$win32_stage/fbc.exe" ]; then
		cp -a "$win32_stage/fbc.exe" "$DISTROOT/fbc32.exe"
	fi
	if [ -f "$win64_stage/fbc.exe" ]; then
		cp -a "$win64_stage/fbc.exe" "$DISTROOT/fbc64.exe"
	fi

	[ -f "$DISTROOT/fbc32.exe" ] || fail "missing staged fbc32.exe"
	[ -f "$DISTROOT/fbc64.exe" ] || fail "missing staged fbc64.exe"
	if [ "$include_arm64" -ne 0 ]; then
		[ -n "$ARM64_DISTROOT" ] || fail "missing ARM64 distribution root name"
	fi

	copy_arch_toolchain win32 "$MINGW32_ROOT" "$TRIPLET32" "$MINGW32_ROOT" "$DISTROOT"
	copy_arch_toolchain win64 "$MINGW64_ROOT" "$TRIPLET64" "$MINGW64_ROOT" "$DISTROOT"

	msg "Merging staged FreeBASIC runtime libraries"
	copy_dir_files "$win32_stage/lib/win32" "$DISTROOT/lib/win32"
	copy_dir_files "$win64_stage/lib/win64" "$DISTROOT/lib/win64"
	remove_stale_crt_import_libs "$DISTROOT/lib/win32"
	remove_stale_crt_import_libs "$DISTROOT/lib/win64"
	assert_no_stale_crt_import_libs "$DISTROOT/lib/win32"
	assert_no_stale_crt_import_libs "$DISTROOT/lib/win64"

	PACKAGED_DISTROOTS+=("$DISTROOT")

	if [ "$include_arm64" -ne 0 ]; then
		msg "Assembling Windows ARM64 distribution"
		rm -rf "$ARM64_DISTROOT"
		mkdir -p "$ARM64_DISTROOT/bin" "$ARM64_DISTROOT/lib/win32-aarch64"

		copy_distribution_common_content "$ARM64_DISTROOT"
		cp -a "$winarm64_stage/fbc.exe" "$ARM64_DISTROOT/fbcarm64.exe"
		[ -f "$ARM64_DISTROOT/fbcarm64.exe" ] || fail "missing staged fbcarm64.exe"

		copy_arch_toolchain win32-aarch64 "$CLANGARM64_ROOT" "$TRIPLETARM64" "$CLANGARM64_ROOT" "$ARM64_DISTROOT"

		msg "Merging staged FreeBASIC ARM64 runtime libraries"
		copy_dir_files "$winarm64_stage/lib/win32-aarch64" "$ARM64_DISTROOT/lib/win32-aarch64"
		remove_stale_crt_import_libs "$ARM64_DISTROOT/lib/win32-aarch64"
		assert_no_stale_crt_import_libs "$ARM64_DISTROOT/lib/win32-aarch64"

		PACKAGED_DISTROOTS+=("$ARM64_DISTROOT")
	fi
}

##############################################################################
# Packaging
##############################################################################

create_zip() {
	local package_root="${1:-$DISTROOT}"
	local package_name
	local zipfile

	package_name="$(basename "$package_root")"
	zipfile="$OUT/${package_name}.zip"

	msg "Creating $package_name distribution zip"
	rm -f "$zipfile"
	(
		cd "$DISTROOT_BASE"
		run zip -qr "$zipfile" "$package_name"
	)
}

collect_existing_distribution_roots() {
	PACKAGED_DISTROOTS=()

	[ -d "$DISTROOT" ] || fail "distribution root not found: $DISTROOT"
	PACKAGED_DISTROOTS+=("$DISTROOT")

	if [ -n "${ARM64_DISTROOT:-}" ] && [ -d "$ARM64_DISTROOT" ]; then
		PACKAGED_DISTROOTS+=("$ARM64_DISTROOT")
	fi
}

create_installer() {
	local package_root="${1:-$DISTROOT}"
	local package_name
	local installer_nsi
	local installer_exe
	local installer_payload_zip
	local display_name="FreeBASIC ${FBVERSION}"
	local out_win
	local payload_win

	[ -x "$NSIS_EXE" ] || fail "makensis not found at $NSIS_EXE; install the nsis package or set NSIS_EXE"
	have cygpath || fail "cygpath not found"
	have zip || fail "zip not found"

	package_name="$(basename "$package_root")"
	installer_nsi="$BUILDROOT/${package_name}.nsi"
	installer_exe="$OUT/${package_name}-setup.exe"
	installer_payload_zip="$TMPROOT/${package_name}-installer-payload.zip"
	case "$package_name" in
		*arm64*)
			display_name="FreeBASIC ${FBVERSION} ARM64"
			;;
	esac

	out_win="$(cygpath -aw "$installer_exe")"

	msg "Creating $package_name NSIS payload zip"
	rm -f "$installer_payload_zip"
	(
		cd "$package_root"
		run zip -qr "$installer_payload_zip" .
	)
	payload_win="$(cygpath -aw "$installer_payload_zip")"

	msg "Generating $package_name NSIS installer script"
	cat > "$installer_nsi" <<EOF
Unicode true
SetCompressor /FINAL lzma
RequestExecutionLevel admin

Name "$display_name"
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
	FileWrite \$0 "  _freebasic_prefix=\`cygpath -u '\$INSTDIR'\`$\r$\n"
	FileWrite \$0 "else$\r$\n"
	FileWrite \$0 "  _freebasic_prefix=/c/FreeBASIC$\r$\n"
	FileWrite \$0 "fi$\r$\n"
	FileWrite \$0 "case :\$\$PATH: in$\r$\n"
	FileWrite \$0 "  *:\$\${_freebasic_prefix}:*) ;;$\r$\n"
	FileWrite \$0 "  *) export PATH=\$\"\$\${_freebasic_prefix}:\$\$PATH\$\" ;;$\r$\n"
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
	InitPluginsDir
	SetOutPath "\$PLUGINSDIR"
	SetCompress off
	File /oname=freebasic-payload.zip "$payload_win"
	SetCompress auto
	IfFileExists "\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" 0 no_powershell
	SetOutPath "\$INSTDIR"
	;
	; The package tree is intentionally large because it carries the compiler,
	; toolchain, headers, and import libraries needed for offline use.  Feeding
	; that expanded tree to NSIS with File /r can exceed makensis' practical
	; datablock limits, so the installer stores a normal zip payload and asks
	; Windows PowerShell to extract it into the chosen install directory.
	nsExec::ExecToLog '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "\$\$ErrorActionPreference = ''Stop''; Expand-Archive -LiteralPath ''\$PLUGINSDIR\\freebasic-payload.zip'' -DestinationPath ''\$INSTDIR'' -Force"'
	Pop \$0
	StrCmp \$0 "0" payload_done
		Abort "Failed to extract the FreeBASIC payload. PowerShell exit code: \$0"
	payload_done:
	WriteUninstaller "\$INSTDIR\\uninstall.exe"
	Call AddInstallDirToPath
	Call AddInstallDirToMsys2
	Goto install_done
	no_powershell:
		Abort "Windows PowerShell is required to extract this installer."
	install_done:
SectionEnd

Section "Uninstall"
	Call un.RemoveInstallDirFromPath
	Call un.RemoveInstallDirFromMsys2
	Delete "\$INSTDIR\\uninstall.exe"
	RMDir /r "\$INSTDIR"
SectionEnd
EOF

	msg "Creating $package_name NSIS installer"
	rm -f "$installer_exe"
	if ! run "$NSIS_EXE" "$installer_nsi"; then
		rm -f "$installer_payload_zip"
		fail "makensis failed while creating $package_name installer"
	fi
	rm -f "$installer_payload_zip"
}

##############################################################################
# Validation
##############################################################################

validate_arm64_with_qemu() {
	local qemu_smoke="$ROOT/build_scripts/msys2-qemu-windows-arm64-smoke.sh"
	local arm64_distroot="${1:-$ARM64_DISTROOT}"
	local args=(--dist "$arm64_distroot")

	[ -f "$qemu_smoke" ] || fail "missing QEMU ARM64 smoke script: $qemu_smoke"
	[ -d "$arm64_distroot" ] || fail "missing ARM64 distribution root: $arm64_distroot"

	if [ -n "${QEMU_ARM64_DISK:-}" ]; then
		args+=(--disk "$QEMU_ARM64_DISK")
	fi
	if [ -n "${QEMU_ARM64_SSH_USER:-}" ]; then
		args+=(--ssh-user "$QEMU_ARM64_SSH_USER")
	fi
	if [ -n "${QEMU_ARM64_SSH_PORT:-}" ]; then
		args+=(--ssh-port "$QEMU_ARM64_SSH_PORT")
	fi
	if [ -n "${QEMU_ARM64_SSH_KEY:-}" ]; then
		args+=(--ssh-key "$QEMU_ARM64_SSH_KEY")
	fi
	if [ -n "${QEMU_AARCH64_EFI:-}" ]; then
		args+=(--efi "$QEMU_AARCH64_EFI")
	fi

	msg "Validating packaged fbcarm64.exe under QEMU"
	run bash "$qemu_smoke" "${args[@]}"
}

validate_distribution() {
	local validate_dir="$BUILDROOT/validate"
	local saved_path="$PATH"

	msg "Validating packaged desktop compilers"
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

	if [ -f "$ARM64_DISTROOT/fbcarm64.exe" ]; then
		msg "Validating packaged ARM64 compiler"
		if [ "$QEMU_ARM64_VALIDATE" -ne 0 ]; then
			validate_arm64_with_qemu "$ARM64_DISTROOT"
		else
			case "$(uname -m)" in
				aarch64|arm64)
					run "$ARM64_DISTROOT/fbcarm64.exe" "$validate_dir/hello.bas" -x "$validate_dir/helloarm64.exe"
					[ "$("$validate_dir/helloarm64.exe")" = "FreeBASIC package test OK" ] || fail "packaged fbcarm64.exe produced bad output"
					;;
				*)
					echo "WARNING: skipping fbcarm64.exe runtime validation on non-ARM64 host" >&2
					echo "WARNING: use --qemu-arm64-validate with QEMU_ARM64_DISK and QEMU_ARM64_SSH_USER to run it under QEMU" >&2
					;;
			esac
		fi
	fi

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
CLANG_VERSION="$($MINGW64_ROOT/bin/clang -dumpversion 2>/dev/null || true)"
if [ -n "$CLANG_VERSION" ]; then
	ARM64_DISTNAME="${ARM64_DISTNAME_BASE}-clang-${CLANG_VERSION}"
else
	ARM64_DISTNAME="$ARM64_DISTNAME_BASE"
fi
ARM64_DISTROOT="$DISTROOT_BASE/$ARM64_DISTNAME"

if [ "$SKIP_BUILD64" -eq 0 ]; then
	build_target win64 "$MINGW64_ROOT" win64 "$TRIPLET64"
fi

if [ "$SKIP_BUILDARM64" -eq 0 ]; then
	build_target win32-aarch64 "$CLANGARM64_ROOT" win32-aarch64 "$TRIPLETARM64"
fi

if [ "$SKIP_BUILD32" -eq 0 ]; then
	build_target win32 "$MINGW32_ROOT" win32 "$TRIPLET32"
fi

if [ "$SKIP_PACKAGE" -eq 0 ]; then
	assemble_distribution
else
	collect_existing_distribution_roots
fi

if [ "$SKIP_PACKAGE" -eq 0 ]; then
	for package_root in "${PACKAGED_DISTROOTS[@]}"; do
		create_zip "$package_root"
	done
fi

if [ "$SKIP_INSTALLER" -eq 0 ]; then
	for package_root in "${PACKAGED_DISTROOTS[@]}"; do
		create_installer "$package_root"
	done
fi

if [ "$SKIP_VALIDATE" -eq 0 ]; then
	validate_distribution
fi

msg "Done"
for package_root in "${PACKAGED_DISTROOTS[@]}"; do
	package_name="$(basename "$package_root")"
	echo "Distribution root: $package_root"
	echo "Zip archive: $OUT/${package_name}.zip"
	echo "Installer: $OUT/${package_name}-setup.exe"
done

# end of msys2-build-freebasic.sh
