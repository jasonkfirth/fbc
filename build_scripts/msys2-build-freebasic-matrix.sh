#!/usr/bin/env bash
#
#   Project: FreeBASIC MSYS2 package matrix
#   ---------------------------------------
#
#   File: msys2-build-freebasic-matrix.sh
#
#   Purpose:
#
#       Build the MSYS2-produced FreeBASIC package set by dispatching to the
#       individual package scripts.
#
#   Responsibilities:
#
#       * build the standard Windows win32/win64 package
#       * build the separate Windows ARM64 package
#       * build the Android, JavaScript, Xbox, and DOS packages
#       * build the Wii package when its external toolchain is available
#       * leave each artifact in the output directory owned by its script
#
#   This file intentionally does NOT contain:
#
#       * target-specific build logic
#       * per-package dependency policy
#       * installer generation details
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"
BUILD_WII=1

cd "$ROOT"

if [ ! -d "$ROOT/build_scripts" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo ""
	echo "ERROR: could not locate the FreeBASIC project root."
	exit 1
fi

usage() {
	cat <<EOF
Usage: $0 [--no-wii]

Builds the complete FreeBASIC MSYS2 package matrix.

Options:
  --no-wii  Skip the Wii package when the official Windows devkitPro
            toolchain could not be installed.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--no-wii)
			BUILD_WII=0
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo ""
			echo "ERROR: unknown option: $1"
			usage >&2
			exit 1
			;;
	esac
done

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo ""
		echo "ERROR: this script must be run inside an MSYS2 environment."
		exit 1
		;;
esac

# GitHub supplies its workspace with a native drive-letter path.  Child build
# scripts pass the host compiler through make recipes, where backslashes would
# be interpreted as shell escapes.  Export one MSYS path for the whole matrix.
if [ -n "${HOST_FBC_ROOT:-}" ]; then
	if ! command -v cygpath >/dev/null 2>&1; then
		echo "ERROR: cygpath is required to normalize HOST_FBC_ROOT." >&2
		exit 1
	fi

	HOST_FBC_ROOT="$(cygpath -u "$HOST_FBC_ROOT")"
	export HOST_FBC_ROOT
fi

msg() {
	echo ""
	echo "==> $1"
}

run_script() {
	local script="$1"

	msg "Running $script"
	bash "$script"
}

msg "Building FreeBASIC MSYS2 package matrix"

run_script "$ROOT/build_scripts/msys2-build-freebasic.sh"
run_script "$ROOT/build_scripts/msys2-build-freebasic-android.sh"
run_script "$ROOT/build_scripts/msys2-build-freebasic-js.sh"
if [ "$BUILD_WII" -eq 1 ]; then
	run_script "$ROOT/build_scripts/msys2-build-freebasic-wii.sh"
else
	msg "Skipping the Wii package because its external toolchain is unavailable"
fi
run_script "$ROOT/build_scripts/msys2-build-freebasic-xbox.sh"
run_script "$ROOT/build_scripts/msdos-build-freebasic.sh"

msg "MSYS2 package matrix complete"
echo "Windows packages : $ROOT/out/mingw32"
echo "Android package  : $ROOT/out/mingw32-android"
echo "JavaScript package: $ROOT/out/mingw32-js"
if [ "$BUILD_WII" -eq 1 ]; then
	echo "Wii package      : $ROOT/out/mingw32-wii"
fi
echo "Xbox package     : $ROOT/out/mingw32-xbox"
echo "DOS package      : $ROOT/out/msdos"

# end of msys2-build-freebasic-matrix.sh
