#!/usr/bin/env bash
#
# FreeBASIC Windows CE MIPS libffi builder
# ------------------------------------------------
#
# File: build_scripts/wince/build-libffi-mips.sh
#
# Purpose:
#
#     Build and stage a reproducible MIPS O32 Windows CE libffi archive for
#     the FreeBASIC THREADCALL runtime.
#
# Responsibilities:
#
#     - download and authenticate the pinned libffi release
#     - apply the reviewed Windows CE MIPS compatibility patch
#     - compile the O32 implementation directly as MIPS PE COFF
#     - validate the target object format and required public symbols
#     - stage the archive, public headers, and license
#
# This file intentionally does NOT contain:
#
#     - Windows CE SDK or linker construction
#     - FreeBASIC runtime construction
#     - another CPU ABI
#     - emulator or package orchestration

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Build identity
##############################################################################

LIBFFI_VERSION="3.5.2"
LIBFFI_SHA256="f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc"
LIBFFI_URL="https://github.com/libffi/libffi/releases/download/v${LIBFFI_VERSION}/libffi-${LIBFFI_VERSION}.tar.gz"

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_ROOT="${WINCE_OUTPUT_ROOT:-$ROOT/out/wince}"
TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$OUTPUT_ROOT/mips-toolchain}"
RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"
PATCH_FILE="$SCRIPT_DIR/patches/libffi-${LIBFFI_VERSION}-wince-mips.patch"
CONFIG_FILE="$SCRIPT_DIR/mips-toolchain/libffi-fficonfig.h"

##############################################################################
# Helpers and host checks
##############################################################################

die() {
	echo "ERROR: $*" >&2
	exit 1
}

for tool in clang curl patch sed sha256sum tar; do
	command -v "$tool" >/dev/null 2>&1 || die "required host tool not found: $tool"
done

[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar" ] ||
	die "run build-mips-toolchain.sh first"
[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-nm" ] ||
	die "MIPS symbol inspector is missing"
[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" ] ||
	die "MIPS object inspector is missing"
[ -d "$TOOLCHAIN_ROOT/include" ] || die "Windows CE SDK headers are missing"
[ -f "$PATCH_FILE" ] || die "libffi compatibility patch is missing"
[ -f "$CONFIG_FILE" ] || die "libffi target configuration is missing"

mkdir -p "$OUTPUT_ROOT/deps" "$OUTPUT_ROOT/logs"
ARCHIVE="$OUTPUT_ROOT/deps/libffi-${LIBFFI_VERSION}.tar.gz"
PARTIAL_ARCHIVE="$ARCHIVE.partial"

verify_archive() {
	printf '%s  %s\n' "$LIBFFI_SHA256" "$1" | sha256sum --check --status
}

if [ ! -f "$ARCHIVE" ] || ! verify_archive "$ARCHIVE"; then
	rm -f -- "$PARTIAL_ARCHIVE"
	curl --fail --location --silent --show-error \
		--output "$PARTIAL_ARCHIVE" "$LIBFFI_URL"
	verify_archive "$PARTIAL_ARCHIVE" || die "libffi archive digest mismatch"
	mv -f -- "$PARTIAL_ARCHIVE" "$ARCHIVE"
fi

BUILD_ROOT="$(mktemp -d "$OUTPUT_ROOT/.libffi-mips-build.XXXXXX")"
cleanup() {
	if [[ "$BUILD_ROOT" == "$OUTPUT_ROOT"/.libffi-mips-build.* ]]; then
		rm -rf -- "$BUILD_ROOT"
	fi
}
trap cleanup EXIT

SOURCE_ROOT="$BUILD_ROOT/source"
INCLUDE_ROOT="$BUILD_ROOT/include"
OBJECT_ROOT="$BUILD_ROOT/object"
mkdir -p "$SOURCE_ROOT" "$INCLUDE_ROOT" "$OBJECT_ROOT"
tar -xzf "$ARCHIVE" --strip-components=1 -C "$SOURCE_ROOT"
patch --directory "$SOURCE_ROOT" --strip=1 < "$PATCH_FILE"

##############################################################################
# Fixed MIPS O32 configuration
##############################################################################

install -m 0644 "$CONFIG_FILE" "$INCLUDE_ROOT/fficonfig.h"
install -m 0644 "$SOURCE_ROOT/src/mips/ffitarget.h" \
	"$INCLUDE_ROOT/ffitarget.h"
sed \
	-e "s/@VERSION@/$LIBFFI_VERSION/g" \
	-e 's/@TARGET@/MIPS/g' \
	-e 's/@HAVE_LONG_DOUBLE@/0/g' \
	-e "s/@FFI_VERSION_STRING@/$LIBFFI_VERSION/g" \
	-e 's/@FFI_VERSION_NUMBER@/30502/g' \
	-e 's/@FFI_EXEC_TRAMPOLINE_TABLE@/0/g' \
	"$SOURCE_ROOT/include/ffi.h.in" > "$INCLUDE_ROOT/ffi.h"

MIPS_CFLAGS=(
	--target=mipsel-pc-windows-msvc
	-O0
	-g0
	-mcpu=mips2
	-mno-check-zero-division
	-mabi=32
	-msoft-float
	-ffreestanding
	-fno-builtin
	-fno-exceptions
	-fno-unwind-tables
	-fno-asynchronous-unwind-tables
	-D__MINGW32CE__
	-D__MINGW32__
	-D__COREDLL__
	-D__GNUC__=4
	-D__GNUC_MINOR__=2
	-D_WIN32_WCE=0x0500
	-D_M_MRX000=4000
	-DMIPS
	-DFFI_BUILDING
	-DFFI_STATIC_BUILD
	-I"$INCLUDE_ROOT"
	-I"$SOURCE_ROOT/include"
	-I"$SOURCE_ROOT/src"
	-I"$SOURCE_ROOT/src/mips"
	-isystem
	"$TOOLCHAIN_ROOT/include"
)

SOURCES=(
	src/prep_cif.c
	src/types.c
	src/raw_api.c
	src/java_raw_api.c
	src/closures.c
	src/tramp.c
	src/mips/ffi.c
	src/mips/o32.S
)

for source in "${SOURCES[@]}"; do
	object="$OBJECT_ROOT/$(basename "${source%.*}").o"
	clang "${MIPS_CFLAGS[@]}" -c "$SOURCE_ROOT/$source" -o "$object"
done > "$OUTPUT_ROOT/logs/libffi-mips-${LIBFFI_VERSION}.log" 2>&1

BUILT_ARCHIVE="$BUILD_ROOT/libffi.a"
"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar" rcs "$BUILT_ARCHIVE" \
	"$OBJECT_ROOT"/*.o
"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ranlib" "$BUILT_ARCHIVE"

if "$TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" -f "$BUILT_ARCHIVE" |
	grep 'file format' | grep -Fv 'file format pe-mips' >/dev/null; then
	die "libffi contains a non-MIPS PE object"
fi

for symbol in ffi_call ffi_closure_alloc ffi_prep_cif ffi_prep_closure_loc; do
	if ! "$TOOLCHAIN_ROOT/bin/mips-wince-pe-nm" --defined-only \
		"$BUILT_ARCHIVE" | awk '{ print $3 }' | grep -Fx "$symbol" >/dev/null; then
		die "libffi is missing required symbol: $symbol"
	fi
done

install -d "$RUNTIME_ROOT/include" "$RUNTIME_ROOT/licenses/libffi"
install -m 0644 "$BUILT_ARCHIVE" "$RUNTIME_ROOT/libffi.a"
install -m 0644 "$INCLUDE_ROOT/ffi.h" "$RUNTIME_ROOT/include/ffi.h"
install -m 0644 "$INCLUDE_ROOT/ffitarget.h" \
	"$RUNTIME_ROOT/include/ffitarget.h"
install -m 0644 "$SOURCE_ROOT/LICENSE" \
	"$RUNTIME_ROOT/licenses/libffi/LICENSE"

printf 'Staged Windows CE MIPS libffi in %s\n' "$RUNTIME_ROOT"

# end of build_scripts/wince/build-libffi-mips.sh
