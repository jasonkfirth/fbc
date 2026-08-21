#!/usr/bin/env bash
#
# FreeBASIC Windows CE MIPS library builder
# ------------------------------------------------
#
# File: build_scripts/wince/build-mips-libraries.sh
#
# Purpose:
#
#     Build the complete FreeBASIC Windows CE MIPS target library set and
#     representative media executables from a Debian or Ubuntu host.
#
# Responsibilities:
#
#     - construct the pinned MIPS PE toolchain and target dependencies
#     - rebuild the current Linux host compiler
#     - build rtlib, fbrt, gfxlib2, and sfxlib for MIPS O32
#     - link and inspect basic, graphics, and sound smoke programs
#
# This file intentionally does NOT contain:
#
#     - emulator or Windows CE ROM acquisition
#     - fbctests, Exampleageddon, or OMA orchestration
#     - compiler or application package construction
#     - another CPU ABI

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_ROOT="${WINCE_OUTPUT_ROOT:-$ROOT/out/wince}"
TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$OUTPUT_ROOT/mips-toolchain}"
JOBS="${JOBS:-2}"
INCREMENTAL=0
SKIP_TOOLCHAIN=0

die() {
	echo "ERROR: $*" >&2
	exit 1
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/build-mips-libraries.sh [options]

Options:
  --jobs N          Parallel build jobs. Default: $JOBS
  --incremental     Preserve compatible target object files.
  --skip-toolchain  Reuse already staged MIPS tools and dependencies.
  -h, --help        Show this help.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--jobs)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			JOBS="$2"
			shift 2
			;;
		--incremental)
			INCREMENTAL=1
			shift
			;;
		--skip-toolchain)
			SKIP_TOOLCHAIN=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			die "unknown option: $1"
			;;
	esac
done

case "$JOBS" in
	''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

mkdir -p "$OUTPUT_ROOT/logs" "$OUTPUT_ROOT/smoke/mips"

##############################################################################
# Toolchain and third-party target libraries
##############################################################################

if [ "$SKIP_TOOLCHAIN" -eq 0 ]; then
	JOBS="$JOBS" WINCE_OUTPUT_ROOT="$OUTPUT_ROOT" \
		"$SCRIPT_DIR/build-mips-toolchain.sh" --jobs "$JOBS"
	WINCE_OUTPUT_ROOT="$OUTPUT_ROOT" \
		"$SCRIPT_DIR/build-libffi-mips.sh"
	WINCE_OUTPUT_ROOT="$OUTPUT_ROOT" \
		"$SCRIPT_DIR/build-compiler-rt-mips.sh"
fi

for required_file in \
	"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar" \
	"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ld" \
	"$ROOT/lib/freebasic/wince-mips32el/libffi.a" \
	"$ROOT/lib/freebasic/wince-mips32el/libclang_rt.builtins-mips.a"; do
	[ -s "$required_file" ] || die "required MIPS input is missing: $required_file"
done

##############################################################################
# Host compiler and target libraries
##############################################################################

cd "$ROOT"
make -j"$JOBS" compiler > "$OUTPUT_ROOT/logs/mips-host-compiler.log" 2>&1

TARGET_MAKE_ARGS=(
	TARGET_TRIPLET=mipsel-wince-pe
	BUILD_FBC="$ROOT/bin/fbc"
	FBC="$ROOT/bin/fbc"
	CC="clang --target=mipsel-pc-windows-msvc -D__GNUC__=4 -D__GNUC_MINOR__=2 -isystem $TOOLCHAIN_ROOT/include"
	AR="$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar"
	RANLIB="$TOOLCHAIN_ROOT/bin/mips-wince-pe-ranlib"
)

if [ "$INCREMENTAL" -eq 0 ]; then
	make clean-libs "${TARGET_MAKE_ARGS[@]}"
fi

make -j"$JOBS" rtlib fbrt gfxlib2 sfxlib "${TARGET_MAKE_ARGS[@]}" \
	> "$OUTPUT_ROOT/logs/mips-libraries.log" 2>&1

RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"
for library in libfb.a libfbmt.a libfbgfx.a libfbgfxmt.a libsfx.a libsfxmt.a; do
	[ -s "$RUNTIME_ROOT/$library" ] || die "target build is missing $library"
done

##############################################################################
# Link validation
##############################################################################

"$ROOT/src/tools/wince/fbc-wince-mips" -O 0 -exx \
	"$ROOT/tests/wince/basic_file.bas" \
	-x "$OUTPUT_ROOT/smoke/mips/basic-file.exe" \
	> "$OUTPUT_ROOT/logs/mips-basic-link.log" 2>&1
"$ROOT/src/tools/wince/fbc-wince-mips" -O 0 -exx \
	"$ROOT/tests/wince/gfx_link.bas" \
	-x "$OUTPUT_ROOT/smoke/mips/gfx-link.exe" \
	> "$OUTPUT_ROOT/logs/mips-gfx-link.log" 2>&1
"$ROOT/src/tools/wince/fbc-wince-mips" -O 0 -exx \
	"$ROOT/tests/wince/sfx_link.bas" \
	-x "$OUTPUT_ROOT/smoke/mips/sfx-link.exe" \
	> "$OUTPUT_ROOT/logs/mips-sfx-link.log" 2>&1

for executable in basic-file.exe gfx-link.exe sfx-link.exe; do
	if ! "$TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" -f \
		"$OUTPUT_ROOT/smoke/mips/$executable" |
		grep -F 'file format pei-mips' >/dev/null; then
		die "$executable is not a MIPS PE executable"
	fi

	if ! "$TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" -p \
		"$OUTPUT_ROOT/smoke/mips/$executable" |
		grep -F $'Subsystem\t\t00000009' >/dev/null; then
		die "$executable does not select the Windows CE subsystem"
	fi
done

printf 'Built Windows CE MIPS rtlib, gfxlib2, and sfxlib in %s\n' \
	"$RUNTIME_ROOT"

# end of build_scripts/wince/build-mips-libraries.sh
