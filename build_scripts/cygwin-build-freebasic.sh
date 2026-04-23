#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# cygwin-build-freebasic.sh
#
# Build and package the native Cygwin FreeBASIC compiler.
#
# The resulting output directory contains:
#   - freebasic-<version>.tar.xz
#   - freebasic-<version>.hint
#   - setup.ini
#
# The package installs into /usr and can be used directly or via a
# small local setup.ini alongside the package and hint file.
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
	CYGWIN*) ;;
	*)
		echo ""
		echo "ERROR: this script must be run inside a Cygwin environment."
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
SKIP_VALIDATE=0
KEEP_BUILDROOT=0

usage() {
	cat <<EOF
Usage: ./build_scripts/cygwin-build-freebasic.sh [options]

Options:
  --skip-deps         Do not install or update required Cygwin packages
  --skip-source-sync  Reuse the existing isolated worktree
  --skip-build        Skip compiler build and staging
  --skip-package      Skip local repository generation
  --skip-validate     Skip staged compiler validation
  --keep-buildroot    Keep temporary build directories
  --help              Show this help text

Environment:
  BUILDROOT           Temporary build root (default: <repo>/.build-cygwin)
  OUT                 Output directory (default: <repo>/out/cygwin)
  CYGWIN_MIRROR       Mirror URL for dependency installation
  SETUP_EXE           Explicit path to setup-x86_64.exe
  TARGET_TRIPLET      Build target triplet (default: x86_64-pc-cygwin)
  HOST_FBC_ROOT       Optional existing FreeBASIC install used as host compiler fallback
  JOBS                Parallel make job count (minimum 12)
EOF
}

for arg in "$@"; do
	case "$arg" in
		--skip-deps) SKIP_DEPS=1 ;;
		--skip-source-sync) SKIP_SOURCE_SYNC=1 ;;
		--skip-build) SKIP_BUILD=1 ;;
		--skip-package) SKIP_PACKAGE=1 ;;
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

resolve_windows_powershell() {
	local candidate
	local system_root="${SYSTEMROOT:-${WINDIR:-}}"

	if [ -n "$system_root" ]; then
		candidate="$(cygpath -u "$system_root/System32/WindowsPowerShell/v1.0/powershell.exe" 2>/dev/null || true)"
		if [ -n "$candidate" ] && [ -x "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	fi

	if have powershell.exe; then
		command -v powershell.exe
		return 0
	fi

	candidate="/cygdrive/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
	if [ -x "$candidate" ]; then
		echo "$candidate"
		return 0
	fi

	return 1
}

remove_inherited_compilers() {
	local worktree="$1"

	#
	# An isolated worktree must prove that it built its own compiler.
	#
	# If a previously built host compiler is copied in from the main
	# source tree, later checks can produce a false positive and treat
	# that inherited binary as the new bootstrap result.
	#
	rm -rf "$worktree/bin"
	rm -f "$worktree/bootstrap/fbc" "$worktree/bootstrap/fbc.exe"
	find "$worktree/bootstrap" -mindepth 2 -maxdepth 2 -type f -name '*.o' -delete 2>/dev/null || true
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
	local n=12
	if have nproc; then
		n="$(nproc)"
	elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
		n="$(getconf _NPROCESSORS_ONLN)"
	fi
	if [ "$n" -lt 12 ]; then
		n=12
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

find_fbc() {
	local base="$1"
	if [ -x "$base.exe" ]; then
		echo "$base.exe"
	elif [ -x "$base" ]; then
		echo "$base"
	else
		return 1
	fi
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

is_cygwin_fbc() {
	local candidate="$1"
	[ -n "$candidate" ] || return 1
	[ -f "$candidate" ] || return 1
	"$candidate" -version 2>&1 | grep -qi 'cygwin'
}

##############################################################################
# Build configuration
##############################################################################

FBVERSION="$(extract_var FBVERSION)"
REV="$(extract_var REV)"
[ -n "$FBVERSION" ] || fail "missing FBVERSION in mk/version.mk"
[ -n "$REV" ] || fail "missing REV in mk/version.mk"

JOBS="${JOBS:-$(max_jobs)}"
BUILDROOT="${BUILDROOT:-$ROOT/.build-cygwin}"
WORKTREE="$BUILDROOT/work"
STAGE="$BUILDROOT/stage"
SETUP_CACHE="$BUILDROOT/setup-cache"
OUT="${OUT:-$ROOT/out/cygwin}"
CYGWIN_MIRROR="${CYGWIN_MIRROR:-https://mirrors.kernel.org/sourceware/cygwin/}"
SETUP_EXE="${SETUP_EXE:-}"
TARGET_TRIPLET="${TARGET_TRIPLET:-x86_64-pc-cygwin}"
HOST_FBC_ROOT="${HOST_FBC_ROOT:-}"
PREFIX="/usr"
PACKAGE_NAME="freebasic"
PACKAGE_VERSION="${FBVERSION}-${REV}"
PACKAGE_FILE="${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.xz"
HINT_FILE="${PACKAGE_NAME}-${PACKAGE_VERSION}.hint"
CYGROOT_WIN="$(cygpath -aw /)"
SETUP_CACHE_WIN="$(cygpath -aw "$SETUP_CACHE")"

mkdir -p "$BUILDROOT" "$OUT" "$SETUP_CACHE"

cleanup() {
	if [ "$KEEP_BUILDROOT" -ne 0 ]; then
		return 0
	fi

	rm -rf "$WORKTREE" "$STAGE"
}
trap cleanup EXIT

##############################################################################
# Dependency installation
##############################################################################

resolve_setup_exe() {
	local candidate
	local powershell_exe

	if [ -n "$SETUP_EXE" ]; then
		[ -f "$SETUP_EXE" ] || fail "SETUP_EXE does not exist: $SETUP_EXE"
		return 0
	fi

	for candidate in \
		/setup-x86_64.exe \
		"$BUILDROOT/setup-x86_64.exe"
	do
		if [ -f "$candidate" ]; then
			SETUP_EXE="$candidate"
			return 0
		fi
	done

	SETUP_EXE="$BUILDROOT/setup-x86_64.exe"
	msg "Downloading setup-x86_64.exe"
	mkdir -p "$(dirname "$SETUP_EXE")"
	if have curl; then
		if curl -L --fail -o "$SETUP_EXE" "https://cygwin.com/setup-x86_64.exe"; then
			return 0
		fi
		echo "==> curl download failed, retrying via PowerShell"
	fi
	local setup_win
	setup_win="$(cygpath -aw "$SETUP_EXE")"
	powershell_exe="$(resolve_windows_powershell)" || fail "unable to locate powershell.exe for setup download fallback"
	run "$powershell_exe" \
		-NoProfile -Command \
		"Invoke-WebRequest -Uri 'https://cygwin.com/setup-x86_64.exe' -OutFile '$setup_win'"
}

install_dependencies() {
	local packages

	resolve_setup_exe
	packages="binutils,diffutils,gawk,gcc-core,gcc-g++,grep,libGL-devel,libX11-devel,libXext-devel,libXpm-devel,libXrandr-devel,libXrender-devel,libffi-devel,libncurses-devel,make,pkg-config,rsync,sed,tar,unzip,xorgproto,xz,zip"

	msg "Installing required Cygwin packages"
	run "$SETUP_EXE" \
		-W \
		-q -n -N -g \
		-s "$CYGWIN_MIRROR" \
		-l "$SETUP_CACHE_WIN" \
		-R "$CYGROOT_WIN" \
		-P "$packages"

	have make || fail "Cygwin package install completed, but make is still unavailable"
	have rsync || fail "Cygwin package install completed, but rsync is still unavailable"
}

##############################################################################
# Build and staging
##############################################################################

build_freebasic() {
	local bootstrap_fbc
	local build_fbc
	local bootstrap_sources_dir="$WORKTREE/bootstrap/cygwin-x86_64"
	local host_fbc=""

	msg "Preparing isolated worktree"
	sanitize_source_tree "$TARGET_TRIPLET"
	if [ "$SKIP_SOURCE_SYNC" -eq 0 ] || [ ! -d "$WORKTREE" ]; then
		rm -rf "$WORKTREE"
		sync_source_tree "$WORKTREE"
	fi
	remove_inherited_compilers "$WORKTREE"

	rm -rf "$STAGE"
	mkdir -p "$STAGE"

	cd "$WORKTREE"
	#
	# Keep the build PATH native to the current Cygwin environment.
	#
	# The source tree may live anywhere on the host filesystem, including
	# inside an MSYS2 checkout path, but that must not give the Cygwin build
	# permission to reuse an unrelated shell, compiler, or toolchain from the
	# parent Windows environment.
	#
	PATH="$WORKTREE/bin:/usr/bin:/bin"
	export PATH

	host_fbc="$(detect_fbc \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/fbc.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc.exe}" \
		"${HOST_FBC_ROOT:+$HOST_FBC_ROOT/bin/fbc}" \
		"$WORKTREE/bin/fbc.exe" \
		"$WORKTREE/bin/fbc" \
		"$WORKTREE/bootstrap/fbc.exe" \
		"$WORKTREE/bootstrap/fbc" \
		|| true)"

	if [ -n "$host_fbc" ] && ! is_cygwin_fbc "$host_fbc"; then
		msg "Ignoring non-Cygwin host compiler for bootstrap emission: $host_fbc"
		host_fbc=""
	fi

	if [ -d "$bootstrap_sources_dir" ] && find "$bootstrap_sources_dir" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
		msg "Bootstrap sources already present for cygwin"
	elif [ -n "$host_fbc" ]; then
		msg "Emitting cygwin bootstrap sources"
		run make -j"$JOBS" \
			bootstrap-emit \
			BUILD_FBC="$host_fbc" \
			TARGET_TRIPLET="$TARGET_TRIPLET"
	else
		msg "No direct bootstrap compiler available for cygwin; seeding from peer bootstrap sources"
		run make -j"$JOBS" bootstrap-seed-peer TARGET_TRIPLET="$TARGET_TRIPLET"
	fi

	msg "Cleaning worktree"
	run make clean TARGET_TRIPLET="$TARGET_TRIPLET"

	msg "Building Cygwin bootstrap compiler ($JOBS threads)"
	run make -j"$JOBS" bootstrap-minimal TARGET_TRIPLET="$TARGET_TRIPLET"
	bootstrap_fbc="$(find_fbc "$WORKTREE/bootstrap/fbc")" || fail "bootstrap compiler not found"
	build_fbc="$(find_fbc "$WORKTREE/bin/fbc")" || fail "bootstrap-minimal did not install bin/fbc"
	is_cygwin_fbc "$build_fbc" || fail "bootstrap-minimal did not produce a native Cygwin compiler"

	msg "Building Cygwin compiler and runtime ($JOBS threads)"
	run make -j"$JOBS" all BUILD_FBC="$build_fbc" TARGET_TRIPLET="$TARGET_TRIPLET"

	msg "Installing into staging"
	run make install DESTDIR="$STAGE" prefix="$PREFIX" BUILD_FBC="$build_fbc" TARGET_TRIPLET="$TARGET_TRIPLET"

	[ -d "$STAGE/usr" ] || fail "staging layout is missing /usr"

	msg "Staging documentation and examples"
	sanitize_source_tree "$TARGET_TRIPLET"
	mkdir -p "$STAGE/usr/share/doc/freebasic"
	mkdir -p "$STAGE/usr/share/freebasic"
	mkdir -p "$STAGE/usr/share/man/man1"
	copy_tree "$ROOT/doc" "$STAGE/usr/share/doc/freebasic/doc"
	copy_tree "$ROOT/examples" "$STAGE/usr/share/freebasic/examples"
	cp -a "$ROOT/readme.txt" "$ROOT/changelog.txt" "$STAGE/usr/share/doc/freebasic/"
	cp -a "$ROOT/doc/fbc.1" "$STAGE/usr/share/man/man1/"

	cd "$ROOT"
}

##############################################################################
# Repository packaging
##############################################################################

generate_metadata() {
	local pkg_path="$OUT/$PACKAGE_FILE"
	local hint_path="$OUT/$HINT_FILE"
	local setup_ini="$OUT/setup.ini"
	local size
	local sha512
	local requires

	requires="bash binutils cygwin gcc-core gcc-g++ libGL-devel libX11-devel libXext-devel libXpm-devel libXrandr-devel libXrender-devel libffi-devel libncurses-devel make pkg-config"
	size="$(wc -c < "$pkg_path" | tr -d '[:space:]')"
	sha512="$(sha512sum "$pkg_path" | awk '{print $1}')"

	cat > "$hint_path" <<EOF
sdesc: "FreeBASIC compiler"
ldesc: "FreeBASIC compiler, runtime libraries, documentation, and examples for native Cygwin builds."
category: Devel
requires: $requires
EOF

	cat > "$setup_ini" <<EOF
release: cygwin
arch: x86_64
setup-timestamp: $(date +%s)

@ freebasic
sdesc: "FreeBASIC compiler"
ldesc: "FreeBASIC compiler, runtime libraries, documentation, and examples for native Cygwin builds."
category: Devel
requires: $requires
version: $PACKAGE_VERSION
install: $PACKAGE_FILE $size $sha512
EOF
}

package_repository() {
	msg "Creating Cygwin package output"
	mkdir -p "$OUT"
	rm -f "$OUT/$PACKAGE_FILE" "$OUT/$HINT_FILE" "$OUT/setup.ini"

	run tar -C "$STAGE" -cJf "$OUT/$PACKAGE_FILE" .
	generate_metadata
}

##############################################################################
# Installer helper
##############################################################################

generate_install_script() {
	local install_script="$OUT/install-freebasic.sh"

	mkdir -p "$OUT"

	cat > "$install_script" <<'EOF'
#!/usr/bin/env bash

set -euo pipefail

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
PACKAGE_FILE="$(find "$SELF_DIR" -maxdepth 1 -type f -name 'freebasic-*.tar.xz' | sort | tail -n 1)"

if [ -z "${PACKAGE_FILE:-}" ]; then
	echo "ERROR: no freebasic-*.tar.xz package found next to this script." >&2
	exit 1
fi

case "$(uname -s)" in
	CYGWIN*) ;;
	*)
		echo "ERROR: this installer must be run inside Cygwin." >&2
		exit 1
		;;
esac

if [ ! -w /usr/bin ]; then
	echo "ERROR: installation requires write access to /usr." >&2
	echo "Re-run this script from an elevated Cygwin shell or as a user with permission to write to /usr." >&2
	exit 1
fi

echo "==> Installing package: $PACKAGE_FILE"
tar -C / -xJf "$PACKAGE_FILE"

echo ""
echo "Installed FreeBASIC into /usr."
echo "Compiler path: /usr/bin/fbc.exe"
EOF

	chmod +x "$install_script"
}

##############################################################################
# Validation
##############################################################################

validate_staged_compiler() {
	local fbc_bin
	local test_dir="$BUILDROOT/validate"
	local test_src="$test_dir/hello.bas"
	local test_bin="$test_dir/hello.exe"
	local output
	local tool
	local saved_path="$PATH"

	msg "Validating staged compiler"
	rm -rf "$test_dir"
	mkdir -p "$test_dir"

	fbc_bin="$(find_fbc "$STAGE/usr/bin/fbc")" || fail "staged compiler not found in /usr/bin"

	#
	# The Cygwin package depends on the host Cygwin toolchain packages instead
	# of bundling GCC/binutils into the FreeBASIC archive.
	#
	# For staged validation we therefore expose the host tools at the staged
	# prefix so the packaged compiler can exercise its normal prefix-based tool
	# lookup without inflating the package contents.
	#
	for tool in gcc g++ as ld ar ranlib; do
		if [ -x "/usr/bin/$tool.exe" ]; then
			rm -f "$STAGE/usr/bin/$tool.exe" "$STAGE/usr/bin/$tool"
			cp -f "/usr/bin/$tool.exe" "$STAGE/usr/bin/$tool.exe"
		elif [ -x "/usr/bin/$tool" ]; then
			rm -f "$STAGE/usr/bin/$tool.exe" "$STAGE/usr/bin/$tool"
			cp -f "/usr/bin/$tool" "$STAGE/usr/bin/$tool"
		fi
	done

	cat > "$test_src" <<'EOF'
print "FreeBASIC Cygwin package test OK"
EOF

	PATH="/usr/bin:/bin"
	export PATH
	run "$fbc_bin" -prefix "$STAGE/usr" "$test_src" -x "$test_bin"
	output="$("$test_bin")"
	PATH="$saved_path"
	export PATH
	output="${output%$'\r'}"
	[ "$output" = "FreeBASIC Cygwin package test OK" ] || fail "staged compiler produced bad output"
}

##############################################################################
# Main
##############################################################################

if [ "$SKIP_DEPS" -eq 0 ]; then
	install_dependencies
fi

if [ "$SKIP_BUILD" -eq 0 ]; then
	build_freebasic
fi

if [ "$SKIP_PACKAGE" -eq 0 ]; then
	package_repository
fi

if [ "$SKIP_VALIDATE" -eq 0 ]; then
	validate_staged_compiler
fi

generate_install_script

msg "Done"
echo "Package: $OUT/$PACKAGE_FILE"
echo "Hint: $OUT/$HINT_FILE"
echo "setup.ini: $OUT/setup.ini"
echo "Install script: $OUT/install-freebasic.sh"
