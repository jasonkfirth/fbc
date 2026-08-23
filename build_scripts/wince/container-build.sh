#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE release workflow
# ------------------------------------------------
#
# File: wince/container-build.sh
#
# Purpose:
#
#     Build the Linux host compiler and complete Windows CE ARM target
#     libraries inside the pinned CeGCC environment.
#
# Responsibilities:
#
#     - bootstrap a current host FreeBASIC compiler from the isolated source
#     - download, authenticate, patch, and stage ARMv4T Windows CE libffi
#     - build normal and multithreaded rtlib, gfxlib2, and sfxlib archives
#     - compile representative target programs and validate their PE machine
#     - preserve build logs and smoke executables outside the container
#
# This file intentionally does NOT contain:
#
#     - Docker image or isolated source-tree construction
#     - compiler archive construction
#     - emulator, fbctests, Exampleageddon, or OMA orchestration
#     - Windows CE ROM acquisition
#     - Windows CE MIPS toolchain policy
#
# Environment contract:
#
#     /work is the isolated source tree and /wince-output is the persistent
#     Windows CE output directory mounted by the host workflow.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Build identity and defaults
##############################################################################

ROOT=/work
OUTPUT_ROOT=/wince-output
JOBS="${JOBS:-2}"
INCREMENTAL=0

LIBFFI_VERSION=3.5.2
LIBFFI_SHA256=f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc
LIBFFI_URL="https://github.com/libffi/libffi/releases/download/v${LIBFFI_VERSION}/libffi-${LIBFFI_VERSION}.tar.gz"

##############################################################################
# Helpers
##############################################################################

die() {
	echo "ERROR: $*" >&2
	exit 1
}

msg() {
	echo
	echo "==> $*"
}

require_value() {
	local option="$1"
	local value="${2-}"

	[ -n "$value" ] || die "$option requires a value"
}

usage() {
	cat <<EOF
Usage: build_scripts/wince/container-build.sh [options]

Options:
  --jobs N       Parallel build jobs. Default: 2
  --incremental  Preserve compatible compiler and target-library objects.
  -h, --help     Show this help.
EOF
}

verify_download() {
	local archive="$1"

	printf '%s  %s\n' "$LIBFFI_SHA256" "$archive" |
		sha256sum --check --status
}

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--jobs)
			require_value "$1" "${2-}"
			JOBS="$2"
			shift 2
			;;
		--incremental)
			INCREMENTAL=1
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

cd "$ROOT"
mkdir -p "$OUTPUT_ROOT/build-logs" "$OUTPUT_ROOT/deps" \
	"$OUTPUT_ROOT/smoke/arm"

HOST_FEATURE_ARGS=(
	DISABLE_GPM=YesPlease
	DISABLE_X11=YesPlease
	DISABLE_OPENGL=YesPlease
)

##############################################################################
# Host compiler
##############################################################################

msg "building the current Linux host compiler"
if [ "$INCREMENTAL" -eq 0 ] || [ ! -x "$ROOT/bin/fbc" ]; then
	make -j"$JOBS" bootstrap-minimal \
		> "$OUTPUT_ROOT/build-logs/host-bootstrap.log" 2>&1
	make clean-libs
fi

make -j"$JOBS" rtlib \
	BUILD_FBC="$ROOT/bin/fbc" \
	"${HOST_FEATURE_ARGS[@]}" \
	> "$OUTPUT_ROOT/build-logs/host-runtime.log" 2>&1

# The release bootstrap compiler predates the type alias syntax in the
# current headers. Build the current compiler through the compatibility
# definitions before asking it to self-host without them.
make -j"$JOBS" -o libs compiler \
	BUILD_FBC="$ROOT/bin/fbc" \
	BUILD_FBCFLAGS="-d __FB_BOOTSTRAP_COMPAT__" \
	"${HOST_FEATURE_ARGS[@]}" \
	> "$OUTPUT_ROOT/build-logs/host-compiler-bootstrap.log" 2>&1
make clean-compiler
make -j"$JOBS" -o libs compiler \
	BUILD_FBC="$ROOT/bin/fbc" \
	"${HOST_FEATURE_ARGS[@]}" \
	> "$OUTPUT_ROOT/build-logs/host-compiler.log" 2>&1
"$ROOT/bin/fbc" -version

##############################################################################
# ARMv4T Windows CE libffi
##############################################################################

msg "building authenticated ARMv4T Windows CE libffi"
LIBFFI_ARCHIVE="$OUTPUT_ROOT/deps/libffi-${LIBFFI_VERSION}.tar.gz"
LIBFFI_PARTIAL="$LIBFFI_ARCHIVE.partial"
LIBFFI_PATCH="$ROOT/build_scripts/wince/patches/libffi-${LIBFFI_VERSION}-wince-arm.patch"
LIBFFI_RUNTIME="$ROOT/lib/freebasic/wince-arm"

[ -f "$LIBFFI_PATCH" ] || die "libffi compatibility patch is missing"
if [ ! -f "$LIBFFI_ARCHIVE" ] || ! verify_download "$LIBFFI_ARCHIVE"; then
	rm -f -- "$LIBFFI_PARTIAL"
	curl --fail --location --silent --show-error \
		--output "$LIBFFI_PARTIAL" "$LIBFFI_URL"
	verify_download "$LIBFFI_PARTIAL" || die "libffi archive digest mismatch"
	mv -f -- "$LIBFFI_PARTIAL" "$LIBFFI_ARCHIVE"
fi

LIBFFI_BUILD_ROOT="$(mktemp -d "$OUTPUT_ROOT/.libffi-arm-build.XXXXXX")"
cleanup_libffi() {
	if [[ "$LIBFFI_BUILD_ROOT" == "$OUTPUT_ROOT"/.libffi-arm-build.* ]] &&
	   [ -d "$LIBFFI_BUILD_ROOT" ]; then
		rm -rf -- "$LIBFFI_BUILD_ROOT"
	fi
}
trap cleanup_libffi EXIT

mkdir -p "$LIBFFI_BUILD_ROOT/source" "$LIBFFI_BUILD_ROOT/object" \
	"$LIBFFI_BUILD_ROOT/stage"
tar -xzf "$LIBFFI_ARCHIVE" --strip-components=1 \
	-C "$LIBFFI_BUILD_ROOT/source"
patch --directory "$LIBFFI_BUILD_ROOT/source" --strip=1 < "$LIBFFI_PATCH"

(
	cd "$LIBFFI_BUILD_ROOT/object"
	ARM_FLAGS="-O0 -g0 -march=armv4t -mfloat-abi=soft -marm -D_WIN32_WCE=0x0500"
	CFLAGS="$ARM_FLAGS" \
	CCASFLAGS="$ARM_FLAGS" \
	"$LIBFFI_BUILD_ROOT/source/configure" \
		--host=arm-mingw32ce \
		--prefix="$LIBFFI_BUILD_ROOT/stage" \
		--disable-shared \
		--enable-static \
		> "$OUTPUT_ROOT/build-logs/libffi-arm-configure.log" 2>&1
	make -j"$JOBS" > "$OUTPUT_ROOT/build-logs/libffi-arm-build.log" 2>&1
	make install >> "$OUTPUT_ROOT/build-logs/libffi-arm-build.log" 2>&1
)

LIBFFI_BUILT="$LIBFFI_BUILD_ROOT/stage/lib/libffi.a"
[ -s "$LIBFFI_BUILT" ] || die "libffi build did not produce a static archive"
arm-mingw32ce-objdump -f "$LIBFFI_BUILT" \
	> "$OUTPUT_ROOT/build-logs/libffi-arm-objects.txt"
if grep 'file format' "$OUTPUT_ROOT/build-logs/libffi-arm-objects.txt" |
	grep -Fv 'pe-arm-wince-little' >/dev/null; then
	die "libffi contains an object for the wrong platform"
fi
if grep 'architecture:' "$OUTPUT_ROOT/build-logs/libffi-arm-objects.txt" |
	grep -Fv 'architecture: armv4t,' >/dev/null; then
	die "libffi contains an object outside the ARMv4T baseline"
fi

for required_symbol in \
	ffi_call ffi_closure_alloc ffi_prep_cif ffi_prep_closure_loc; do
	if ! arm-mingw32ce-nm --defined-only "$LIBFFI_BUILT" |
		awk '{ print $3 }' | grep -Fx "$required_symbol" >/dev/null; then
		die "libffi is missing required symbol: $required_symbol"
	fi
done

install -d "$LIBFFI_RUNTIME/include" "$LIBFFI_RUNTIME/licenses/libffi"
install -m 0644 "$LIBFFI_BUILT" "$LIBFFI_RUNTIME/libffi.a"
install -m 0644 "$LIBFFI_BUILD_ROOT/stage/include/ffi.h" \
	"$LIBFFI_RUNTIME/include/ffi.h"
install -m 0644 "$LIBFFI_BUILD_ROOT/stage/include/ffitarget.h" \
	"$LIBFFI_RUNTIME/include/ffitarget.h"
install -m 0644 "$LIBFFI_BUILD_ROOT/source/LICENSE" \
	"$LIBFFI_RUNTIME/licenses/libffi/LICENSE"

cleanup_libffi
trap - EXIT

##############################################################################
# Windows CE ARM libraries and smoke programs
##############################################################################

msg "building Windows CE ARM rtlib, gfxlib2, and sfxlib"
if [ "$INCREMENTAL" -eq 0 ]; then
	make clean-libs TARGET_TRIPLET=arm-mingw32ce
fi

make -j"$JOBS" rtlib fbrt gfxlib2 sfxlib \
	TARGET_TRIPLET=arm-mingw32ce \
	BUILD_FBC="$ROOT/bin/fbc" \
	> "$OUTPUT_ROOT/build-logs/arm-libraries.log" 2>&1

for required_library in \
	libfb.a libfbmt.a libfbgfx.a libfbgfxmt.a libsfx.a libsfxmt.a; do
	[ -s "$LIBFFI_RUNTIME/$required_library" ] ||
		die "Windows CE ARM library build is missing $required_library"
done

msg "compiling Windows CE ARM smoke programs"
"$ROOT/src/tools/wince/fbc-wince-arm" -O 0 \
	"$ROOT/tests/wince/basic_file.bas" \
	-x "$OUTPUT_ROOT/smoke/arm/basic-file.exe"
"$ROOT/src/tools/wince/fbc-wince-arm" -O 0 -mt \
	"$ROOT/tests/wince/gfx_link.bas" \
	-x "$OUTPUT_ROOT/smoke/arm/gfx-link.exe"
"$ROOT/src/tools/wince/fbc-wince-arm" -O 0 -mt \
	"$ROOT/tests/wince/sfx_link.bas" \
	-x "$OUTPUT_ROOT/smoke/arm/sfx-link.exe"

for smoke_executable in "$OUTPUT_ROOT"/smoke/arm/*.exe; do
	arm-mingw32ce-objdump -f "$smoke_executable" |
		grep -Eq 'file format pei?-arm-wince-little' ||
		die "smoke executable has the wrong PE machine: $smoke_executable"
done

msg "Windows CE ARM compiler and libraries built successfully"

# end of build_scripts/wince/container-build.sh
