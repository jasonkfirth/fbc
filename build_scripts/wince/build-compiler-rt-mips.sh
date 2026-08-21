#!/usr/bin/env bash
#
# FreeBASIC Windows CE MIPS compiler-rt builder
# ------------------------------------------------
#
# File: build_scripts/wince/build-compiler-rt-mips.sh
#
# Purpose:
#
#     Build the Clang software-floating-point and wide-integer helpers needed
#     by the MIPS III Windows CE target.
#
# Responsibilities:
#
#     - download and authenticate the pinned compiler-rt source release
#     - compile the generic builtin set directly as MIPS PE COFF
#     - exclude host and Apple-only source files
#     - validate the target format and required software-float symbols
#     - stage the archive and LLVM license with the target runtime
#
# This file intentionally does NOT contain:
#
#     - sanitizers, unwinding, profiling, or C++ runtime support
#     - Windows CE SDK or linker construction
#     - another CPU ABI
#     - emulator or package orchestration

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Build identity
##############################################################################

COMPILER_RT_VERSION="21.1.8"
COMPILER_RT_SHA256="dd54ae21aee1780fac59445b51ebff601ad016b31ac3a7de3b21126fd3ccb229"
COMPILER_RT_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${COMPILER_RT_VERSION}/compiler-rt-${COMPILER_RT_VERSION}.src.tar.xz"

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_ROOT="${WINCE_OUTPUT_ROOT:-$ROOT/out/wince}"
TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$OUTPUT_ROOT/mips-toolchain}"
RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"

##############################################################################
# Helpers and authenticated source
##############################################################################

die() {
	echo "ERROR: $*" >&2
	exit 1
}

for tool in awk clang curl sha256sum tar; do
	command -v "$tool" >/dev/null 2>&1 || die "required host tool not found: $tool"
done

[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar" ] ||
	die "run build-mips-toolchain.sh first"
[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-nm" ] ||
	die "MIPS symbol inspector is missing"
[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" ] ||
	die "MIPS object inspector is missing"

mkdir -p "$OUTPUT_ROOT/deps" "$OUTPUT_ROOT/logs"
ARCHIVE="$OUTPUT_ROOT/deps/compiler-rt-${COMPILER_RT_VERSION}.src.tar.xz"
PARTIAL_ARCHIVE="$ARCHIVE.partial"

verify_archive() {
	printf '%s  %s\n' "$COMPILER_RT_SHA256" "$1" |
		sha256sum --check --status
}

if [ ! -f "$ARCHIVE" ] || ! verify_archive "$ARCHIVE"; then
	rm -f -- "$PARTIAL_ARCHIVE"
	curl --fail --location --silent --show-error \
		--output "$PARTIAL_ARCHIVE" "$COMPILER_RT_URL"
	verify_archive "$PARTIAL_ARCHIVE" ||
		die "compiler-rt archive digest mismatch"
	mv -f -- "$PARTIAL_ARCHIVE" "$ARCHIVE"
fi

BUILD_ROOT="$(mktemp -d "$OUTPUT_ROOT/.compiler-rt-mips-build.XXXXXX")"
cleanup() {
	if [[ "$BUILD_ROOT" == "$OUTPUT_ROOT"/.compiler-rt-mips-build.* ]]; then
		rm -rf -- "$BUILD_ROOT"
	fi
}
trap cleanup EXIT

SOURCE_ROOT="$BUILD_ROOT/source"
OBJECT_ROOT="$BUILD_ROOT/object"
mkdir -p "$SOURCE_ROOT" "$OBJECT_ROOT"
tar -xJf "$ARCHIVE" --strip-components=1 -C "$SOURCE_ROOT"

##############################################################################
# Generic builtin archive
##############################################################################

mapfile -t GENERIC_SOURCES < <(
	awk '
		$1 == "set(GENERIC_SOURCES" { capture = 1; next }
		capture && $1 == ")" { exit }
		capture && $1 ~ /[.]c$/ { print $1 }
	' "$SOURCE_ROOT/lib/builtins/CMakeLists.txt"
)

[ "${#GENERIC_SOURCES[@]}" -gt 100 ] ||
	die "could not recover compiler-rt generic source list"

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
	-I"$SOURCE_ROOT/lib/builtins"
	-isystem
	"$TOOLCHAIN_ROOT/include"
)

OBJECTS=()
for source in "${GENERIC_SOURCES[@]}"; do
	case "$source" in
		apple_versioning.c|os_version_check.c|trampoline_setup.c)
			continue
			;;
	esac

	object="$OBJECT_ROOT/${source%.c}.o"
	clang "${MIPS_CFLAGS[@]}" -c \
		"$SOURCE_ROOT/lib/builtins/$source" -o "$object"
	OBJECTS+=( "$object" )
done > "$OUTPUT_ROOT/logs/compiler-rt-mips-${COMPILER_RT_VERSION}.log" 2>&1

BUILT_ARCHIVE="$BUILD_ROOT/libclang_rt.builtins-mips.a"
"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar" rcs "$BUILT_ARCHIVE" \
	"${OBJECTS[@]}"
"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ranlib" "$BUILT_ARCHIVE"

if "$TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" -f "$BUILT_ARCHIVE" |
	grep 'file format' | grep -Fv 'file format pe-mips' >/dev/null; then
	die "compiler-rt contains a non-MIPS PE object"
fi

for symbol in __adddf3 __divdi3 __fixsfsi __mulsf3 __truncdfsf2; do
	if ! "$TOOLCHAIN_ROOT/bin/mips-wince-pe-nm" --defined-only \
		"$BUILT_ARCHIVE" | awk '{ print $3 }' | grep -Fx "$symbol" >/dev/null; then
		die "compiler-rt is missing required symbol: $symbol"
	fi
done

install -d "$RUNTIME_ROOT/licenses/compiler-rt"
install -m 0644 "$BUILT_ARCHIVE" \
	"$RUNTIME_ROOT/libclang_rt.builtins-mips.a"
install -m 0644 "$SOURCE_ROOT/LICENSE.TXT" \
	"$RUNTIME_ROOT/licenses/compiler-rt/LICENSE.TXT"

printf 'Staged Windows CE MIPS compiler-rt in %s\n' "$RUNTIME_ROOT"

# end of build_scripts/wince/build-compiler-rt-mips.sh
