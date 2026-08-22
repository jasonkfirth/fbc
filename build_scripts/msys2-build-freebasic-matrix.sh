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
#       * build the Android, JavaScript, Wii, Xbox, and DOS packages
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

cd "$ROOT"

if [ ! -d "$ROOT/build_scripts" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo ""
	echo "ERROR: could not locate the FreeBASIC project root."
	exit 1
fi

usage() {
	cat <<EOF
Usage: $0

Builds the complete FreeBASIC MSYS2 package matrix.

This script intentionally accepts no build options.  Each package script owns
its own defaults so it can also be run independently.
EOF
}

if [ "$#" -gt 0 ]; then
	case "$1" in
		-h|--help)
			if [ "$#" -eq 1 ]; then
				usage
				exit 0
			fi
			;;
	esac

	echo ""
	echo "ERROR: this script does not accept build options."
	usage >&2
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
run_script "$ROOT/build_scripts/msys2-build-freebasic-wii.sh"
run_script "$ROOT/build_scripts/msys2-build-freebasic-xbox.sh"
run_script "$ROOT/build_scripts/msdos-build-freebasic.sh"

msg "MSYS2 package matrix complete"
echo "Windows packages : $ROOT/out/mingw32"
echo "Android package  : $ROOT/out/mingw32-android"
echo "JavaScript package: $ROOT/out/mingw32-js"
echo "Wii package      : $ROOT/out/mingw32-wii"
echo "Xbox package     : $ROOT/out/mingw32-xbox"
echo "DOS package      : $ROOT/out/msdos"

# end of msys2-build-freebasic-matrix.sh
