#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE MIPS emulator workflow
# ---------------------------------------------------
#
# File: build_scripts/wince/prepare-mips-emulator.sh
#
# Purpose:
#
#     Prepare the pinned CERF release and a user-supplied MIPS Windows CE ROM
#     for unattended FreeBASIC qualification on Linux.
#
# Responsibilities:
#
#     - build or reuse the pinned Wine, Xvfb, and archive support image
#     - download and authenticate the pinned public CERF release
#     - validate the selected MIPS III or MIPS IV CERF board id
#     - stage the user's ROM with its recorded SHA-256 digest
#     - create separate MIPS share and log directories
#
# This file intentionally does NOT contain:
#
#     - Windows CE ROM download or redistribution
#     - FreeBASIC compiler, runtime, or test construction
#     - emulator execution or test-result interpretation
#     - ARM Windows CE ROM selection
#
# Licensing boundary:
#
#     The Windows CE ROM must be supplied by the user from a licensed device,
#     SDK, or CERF bundle source. The ROM is staged only below out/wince and is
#     never placed in a FreeBASIC release package.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Pinned release identity and defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CERF_VERSION=6.7.0
CERF_REVISION=90987af
CERF_ARCHIVE="CERF-${CERF_VERSION}-${CERF_REVISION}-Release-Win32.zip"
CERF_URL="https://github.com/gweslab/cerf/releases/download/6.7/$CERF_ARCHIVE"
CERF_SHA256=eeea0beec621f6b12f5b0144a9792dbcaa442ecdef8c5b4189cca8e1ddb384b5

EMULATOR_ROOT="${WINCE_EMULATOR_ROOT:-$ROOT/out/wince/emulator}"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf-mips}"
EMULATOR_IMAGE="${WINCE_EMULATOR_IMAGE:-freebasic-wince-emulator:noble}"
BOARD_ID="${WINCE_MIPS_BOARD_ID:-casio_cassiopeia_em500}"
ROM_SOURCE=""
SKIP_IMAGE=0
PREPARE_STAGE=""

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

cleanup() {
	if [ -n "$PREPARE_STAGE" ] &&
	   [[ "$PREPARE_STAGE" == "$EMULATOR_ROOT"/.prepare-mips.* ]] &&
	   [ -d "$PREPARE_STAGE" ]; then
		rm -rf -- "$PREPARE_STAGE"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/prepare-mips-emulator.sh --rom FILE [options]

Required:
  --rom FILE         Licensed MIPS Windows CE primary ROM image

Options:
  --board-id ID      CERF board id. Default: casio_cassiopeia_em500
  --emulator-dir DIR Persistent emulator root. Default: out/wince/emulator
  --cerf-dir DIR     Prepared CERF tree. Default: out/wince/emulator/cerf-mips
  --image NAME       Emulator support image. Default: $EMULATOR_IMAGE
  --skip-image       Reuse the existing emulator support image
  -h, --help         Show this help

Qualified board ids for the compiler's MIPS III baseline:
  casio_cassiopeia_em500, casio_toricomail, nec_mobilepro_700,
  nec_rockhopper

The ROM is copied to the output tree and its digest is recorded, but it is
never copied to a compiler or application package.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing and guards
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--rom)
			require_value "$1" "${2-}"
			ROM_SOURCE="$2"
			shift 2
			;;
		--board-id)
			require_value "$1" "${2-}"
			BOARD_ID="$2"
			shift 2
			;;
		--emulator-dir)
			require_value "$1" "${2-}"
			EMULATOR_ROOT="$2"
			shift 2
			;;
		--cerf-dir)
			require_value "$1" "${2-}"
			CERF_ROOT="$2"
			shift 2
			;;
		--image)
			require_value "$1" "${2-}"
			EMULATOR_IMAGE="$2"
			shift 2
			;;
		--skip-image)
			SKIP_IMAGE=1
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

[ -n "$ROM_SOURCE" ] || die "--rom is required"
[ -f "$ROM_SOURCE" ] || die "MIPS Windows CE ROM not found: $ROM_SOURCE"

case "$BOARD_ID" in
	casio_cassiopeia_em500|casio_toricomail|nec_mobilepro_700|nec_rockhopper)
		;;
	*)
		die "unsupported board id for the MIPS III compiler baseline: $BOARD_ID"
		;;
esac

for host_tool in awk docker realpath sha256sum; do
	command -v "$host_tool" >/dev/null 2>&1 ||
		die "required host tool not found: $host_tool"
done

EMULATOR_REAL="$(realpath -m "$EMULATOR_ROOT")"
CERF_REAL="$(realpath -m "$CERF_ROOT")"
ROM_REAL="$(realpath -e "$ROM_SOURCE")"
STAGED_ROM="$CERF_REAL/roms-wince-mips.bin"

[ "$EMULATOR_REAL" != / ] || die "refusing to use / as the emulator root"
[ "$CERF_REAL" != / ] || die "refusing to use / as the CERF root"
[[ "$CERF_REAL" == "$EMULATOR_REAL"/* ]] ||
	die "the CERF directory must be below the emulator root"
[ "$ROM_REAL" != "$STAGED_ROM" ] ||
	die "the source ROM is already the staged destination"
[ -s "$ROM_REAL" ] || die "the source ROM is empty"

ROM_SHA256="$(sha256sum "$ROM_REAL" | awk '{ print $1 }')"
mkdir -p "$EMULATOR_REAL/downloads" "$CERF_REAL"

##############################################################################
# Contained CERF preparation
##############################################################################

if [ "$SKIP_IMAGE" -eq 0 ]; then
	msg "building the pinned CERF support image"
	docker build \
		-f "$SCRIPT_DIR/emulator/Dockerfile" \
		-t "$EMULATOR_IMAGE" \
		"$SCRIPT_DIR/emulator"
else
	docker image inspect "$EMULATOR_IMAGE" >/dev/null 2>&1 ||
		die "CERF support image does not exist: $EMULATOR_IMAGE"
fi

PREPARE_STAGE="$(mktemp -d "$EMULATOR_REAL/.prepare-mips.XXXXXX")"
mkdir -p "$PREPARE_STAGE/cerf-release"

msg "downloading and authenticating CERF $CERF_VERSION"
docker run --rm \
	--user "$(id -u):$(id -g)" \
	-v "$EMULATOR_REAL:/emulator" \
	"$EMULATOR_IMAGE" \
	bash -ceu '
		archive="/emulator/downloads/$1"
		partial="$archive.partial"
		if [ ! -f "$archive" ] ||
		   ! printf "%s  %s\n" "$3" "$archive" |
			sha256sum --check --status; then
			rm -f -- "$partial"
			curl --fail --location --silent --show-error \
				--output "$partial" "$2"
			printf "%s  %s\n" "$3" "$partial" |
				sha256sum --check --status
			mv -f -- "$partial" "$archive"
		fi
	' -- "$CERF_ARCHIVE" "$CERF_URL" "$CERF_SHA256"

docker run --rm \
	--user "$(id -u):$(id -g)" \
	-v "$EMULATOR_REAL:/emulator" \
	-v "$PREPARE_STAGE:/stage" \
	"$EMULATOR_IMAGE" \
	bash -ceu '
		unzip -q "/emulator/downloads/$1" -d /stage/cerf-release
	' -- "$CERF_ARCHIVE"

[ -s "$PREPARE_STAGE/cerf-release/cerf.exe" ] ||
	die "CERF archive did not contain cerf.exe"

##############################################################################
# Durable emulator tree
##############################################################################

msg "staging the CERF MIPS tree"
cp -a "$PREPARE_STAGE/cerf-release/." "$CERF_REAL/"
cp "$ROM_REAL" "$STAGED_ROM"
mkdir -p "$CERF_REAL/share" "$CERF_REAL/logs"

printf '%s  %s\n' "$CERF_SHA256" \
	"$EMULATOR_REAL/downloads/$CERF_ARCHIVE" |
	sha256sum --check --status || die "staged CERF archive digest mismatch"
printf '%s  %s\n' "$ROM_SHA256" "$STAGED_ROM" |
	sha256sum --check --status || die "staged MIPS ROM digest mismatch"

{
	printf 'WINCE_MIPS_BOARD_ID=%s\n' "$BOARD_ID"
	printf 'WINCE_MIPS_ROM=%s\n' "$STAGED_ROM"
	printf 'WINCE_MIPS_ROM_SHA256=%s\n' "$ROM_SHA256"
} > "$CERF_REAL/mips-emulator.env"

msg "Windows CE MIPS emulator prepared at $CERF_REAL"
echo "    Board: $BOARD_ID"
echo "    ROM SHA-256: $ROM_SHA256"

# end of build_scripts/wince/prepare-mips-emulator.sh
