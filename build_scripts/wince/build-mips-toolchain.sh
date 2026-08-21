#!/usr/bin/env bash
#
# FreeBASIC Windows CE MIPS toolchain builder
# ------------------------------------------------
#
# File: build_scripts/wince/build-mips-toolchain.sh
#
# Purpose:
#
#     Build and stage the open Windows CE MIPS PE tools and SDK surface used by
#     the FreeBASIC Clang backend.
#
# Responsibilities:
#
#     - fetch a pinned public CeGCC source revision
#     - restore the removed GNU MIPS PE BFD target
#     - build the linker and binary inspection tools without obsolete GCC
#     - extract the pinned CeGCC public headers
#     - generate MIPS import libraries from public definition files
#     - compile the FreeBASIC-owned executable and DLL startup objects
#
# This file intentionally does NOT contain:
#
#     - a proprietary Windows CE ROM
#     - a legacy MIPS GCC build
#     - FreeBASIC runtime, gfxlib2, or sfxlib construction
#     - emulator startup or test orchestration

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Build identity
##############################################################################

CEGCC_REPOSITORY="https://github.com/svn2github/cegcc.git"
CEGCC_REVISION="1dff32ff6009f2a678618a373098ff332916d251"
TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
JOBS="${JOBS:-2}"
SKIP_TOOLCHAIN_IMAGE=0

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_ROOT="${WINCE_OUTPUT_ROOT:-$ROOT/out/wince}"
TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$OUTPUT_ROOT/mips-toolchain}"
PATCH_FILE="$SCRIPT_DIR/patches/binutils-2.20.51-mips-pe.patch"
STARTUP_SOURCE="$SCRIPT_DIR/mips-toolchain/crt0.c"
RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"

##############################################################################
# Helpers
##############################################################################

die() {
	echo "ERROR: $*" >&2
	exit 1
}

require_command() {
	command -v "$1" >/dev/null 2>&1 || die "required host tool not found: $1"
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/build-mips-toolchain.sh [options]

Options:
  --jobs N                 Parallel build jobs. Default: $JOBS
  --skip-toolchain-image   Reuse the pinned CeGCC image for SDK headers.
  --out-dir DIR            Persistent Windows CE output root.
  --toolchain-dir DIR      Installed MIPS toolchain root.
  -h, --help               Show this help.

Host prerequisites:

  sudo apt-get update
  sudo apt-get install -y build-essential git docker.io flex bison texinfo \
    clang llvm rsync xz-utils

No Ubuntu MIPS cross-GCC packages are required.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--jobs)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			JOBS="$2"
			shift 2
			;;
		--skip-toolchain-image)
			SKIP_TOOLCHAIN_IMAGE=1
			shift
			;;
		--out-dir)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			OUTPUT_ROOT="$2"
			shift 2
			;;
		--toolchain-dir)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			TOOLCHAIN_ROOT="$2"
			shift 2
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

for tool in clang docker flex git llvm-dlltool make patch sha256sum strip tar; do
	require_command "$tool"
done

[ -f "$PATCH_FILE" ] || die "missing MIPS PE binutils patch: $PATCH_FILE"
[ -f "$STARTUP_SOURCE" ] || die "missing MIPS startup source: $STARTUP_SOURCE"

mkdir -p "$OUTPUT_ROOT/logs"
BUILD_ROOT="$(mktemp -d "$OUTPUT_ROOT/.mips-toolchain-build.XXXXXX")"
STAGE_ROOT="$(mktemp -d "$OUTPUT_ROOT/.mips-toolchain-stage.XXXXXX")"

cleanup() {
	if [[ "$BUILD_ROOT" == "$OUTPUT_ROOT"/.mips-toolchain-build.* ]]; then
		rm -rf -- "$BUILD_ROOT"
	fi
	if [[ "$STAGE_ROOT" == "$OUTPUT_ROOT"/.mips-toolchain-stage.* ]]; then
		rm -rf -- "$STAGE_ROOT"
	fi
}
trap cleanup EXIT

##############################################################################
# Pinned source and GNU PE tools
##############################################################################

SOURCE_ROOT="$BUILD_ROOT/cegcc"
git clone --filter=blob:none --no-checkout "$CEGCC_REPOSITORY" "$SOURCE_ROOT"
git -C "$SOURCE_ROOT" checkout --detach "$CEGCC_REVISION"
[ "$(git -C "$SOURCE_ROOT" rev-parse HEAD)" = "$CEGCC_REVISION" ] ||
	die "CeGCC revision verification failed"
patch --directory "$SOURCE_ROOT" --strip=1 < "$PATCH_FILE"

BINUTILS_SOURCE="$SOURCE_ROOT/cegcc/src/binutils"
BINUTILS_BUILD="$BUILD_ROOT/binutils"
BINUTILS_PREFIX="$BUILD_ROOT/prefix"
mkdir -p "$BINUTILS_BUILD" "$BINUTILS_PREFIX"

(
	cd "$BINUTILS_BUILD"
	"$BINUTILS_SOURCE/configure" \
		--target=mips-wince-pe \
		--prefix="$BINUTILS_PREFIX" \
		--disable-nls \
		--disable-werror
	make -j"$JOBS" all-binutils all-ld
	make install-binutils install-ld
) > "$OUTPUT_ROOT/logs/mips-toolchain-binutils.log" 2>&1

mkdir -p "$STAGE_ROOT/bin" "$STAGE_ROOT/include" "$STAGE_ROOT/lib" \
	"$STAGE_ROOT/licenses/binutils" "$STAGE_ROOT/licenses/mingw"
for tool in addr2line ar c++filt dlltool ld nm objcopy objdump ranlib readelf \
	size strings strip windmc windres; do
	tool_path="$BINUTILS_PREFIX/bin/mips-wince-pe-$tool"
	[ -x "$tool_path" ] || die "binutils build is missing $tool"
	install -m 0755 "$tool_path" "$STAGE_ROOT/bin/"
done
strip --strip-unneeded "$STAGE_ROOT/bin"/*

if ! "$STAGE_ROOT/bin/mips-wince-pe-objdump" -i | grep -F 'pe-mips' >/dev/null; then
	die "built BFD tools do not advertise the MIPS PE target"
fi

install -m 0644 "$BINUTILS_SOURCE/COPYING3" \
	"$STAGE_ROOT/licenses/binutils/COPYING3"
install -m 0644 "$SOURCE_ROOT/cegcc/src/mingw/DISCLAIMER" \
	"$STAGE_ROOT/licenses/mingw/DISCLAIMER"

##############################################################################
# Public Windows CE SDK headers and imports
##############################################################################

if [ "$SKIP_TOOLCHAIN_IMAGE" -eq 0 ]; then
	docker build --tag "$TOOLCHAIN_IMAGE" "$SCRIPT_DIR" \
		> "$OUTPUT_ROOT/logs/mips-toolchain-sdk-image.log" 2>&1
fi

docker image inspect "$TOOLCHAIN_IMAGE" >/dev/null 2>&1 ||
	die "required CeGCC SDK image is unavailable: $TOOLCHAIN_IMAGE"

docker run --rm "$TOOLCHAIN_IMAGE" \
	tar -C /opt/cegcc-arm/arm-mingw32ce/include -cf - . |
	tar -C "$STAGE_ROOT/include" -xf -

DEFINITION_ROOT="$SOURCE_ROOT/cegcc/src/w32api/libce"
llvm-dlltool -m r4000 -d "$DEFINITION_ROOT/coredll.def" \
	-D COREDLL.dll -l "$STAGE_ROOT/lib/libcoredll.a"
llvm-dlltool -m r4000 -d "$DEFINITION_ROOT/winsock.def" \
	-D WINSOCK.dll -l "$STAGE_ROOT/lib/libwinsock.a"

##############################################################################
# FreeBASIC-owned startup objects
##############################################################################

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
	-D_WIN32_WCE=0x0500
	-D_M_MRX000=4000
	-DMIPS
)

clang "${MIPS_CFLAGS[@]}" -c "$STARTUP_SOURCE" \
	-o "$STAGE_ROOT/lib/crt0.o"
clang "${MIPS_CFLAGS[@]}" -DFB_CRT_BUILD_DLL -c "$STARTUP_SOURCE" \
	-o "$STAGE_ROOT/lib/dllcrt0.o"

for object in crt0.o dllcrt0.o; do
	if ! "$STAGE_ROOT/bin/mips-wince-pe-objdump" -f \
		"$STAGE_ROOT/lib/$object" | grep -F 'file format pe-mips' >/dev/null; then
		die "$object is not a MIPS PE object"
	fi
done

for archive in libcoredll.a libwinsock.a; do
	if ! "$STAGE_ROOT/bin/mips-wince-pe-objdump" -f \
		"$STAGE_ROOT/lib/$archive" | grep -F 'file format pe-mips' >/dev/null; then
		die "$archive does not contain MIPS PE objects"
	fi
done

{
	printf 'CeGCC repository: %s\n' "$CEGCC_REPOSITORY"
	printf 'CeGCC revision: %s\n' "$CEGCC_REVISION"
	printf 'Clang: %s\n' "$(clang --version | head -n 1)"
	printf 'LLVM dlltool: %s\n' "$(llvm-dlltool --version | head -n 1)"
	printf '\nSHA-256:\n'
	(
		cd "$STAGE_ROOT"
		find bin lib -type f -print0 | sort -z | xargs -0 sha256sum
	)
} > "$STAGE_ROOT/MANIFEST.txt"

##############################################################################
# Atomic installation and runtime staging
##############################################################################

BACKUP_ROOT="$(mktemp -d "$OUTPUT_ROOT/.mips-toolchain-previous.XXXXXX")"
if [ -e "$TOOLCHAIN_ROOT" ]; then
	mv -- "$TOOLCHAIN_ROOT" "$BACKUP_ROOT/toolchain"
fi
mv -- "$STAGE_ROOT" "$TOOLCHAIN_ROOT"
STAGE_ROOT="$OUTPUT_ROOT/.mips-toolchain-stage.installed"
if [[ "$BACKUP_ROOT" == "$OUTPUT_ROOT"/.mips-toolchain-previous.* ]]; then
	rm -rf -- "$BACKUP_ROOT"
fi

install -d "$RUNTIME_ROOT"
for runtime_file in crt0.o dllcrt0.o libcoredll.a libwinsock.a; do
	install -m 0644 "$TOOLCHAIN_ROOT/lib/$runtime_file" \
		"$RUNTIME_ROOT/$runtime_file"
done

"$TOOLCHAIN_ROOT/bin/mips-wince-pe-ld" --version | head -n 1
printf 'Staged Windows CE MIPS toolchain in %s\n' "$TOOLCHAIN_ROOT"

# end of build_scripts/wince/build-mips-toolchain.sh
