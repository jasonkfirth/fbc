#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE emulator workflow
# ------------------------------------------------
#
# File: wince/prepare-arm-emulator.sh
#
# Purpose:
#
#     Prepare the pinned CERF release and a user-supplied Windows Mobile 6 ARM
#     image for unattended FreeBASIC qualification on Linux.
#
# Responsibilities:
#
#     - build the pinned Wine, Xvfb, MSI, and archive support image
#     - download and authenticate the public CERF 6.7 release
#     - authenticate and extract the user-supplied Microsoft emulator MSI
#     - select the qualified QVGA ARM ROM and create CERF share/log folders
#     - preserve all emulator material below out/wince/emulator
#
# This file intentionally does NOT contain:
#
#     - Microsoft ROM download or redistribution
#     - FreeBASIC compiler or runtime construction
#     - emulator execution or test-result interpretation
#     - Windows CE MIPS ROM selection
#
# Licensing boundary:
#
#     The Microsoft emulator image must be obtained by the user from the
#     official Download Center.  This script accepts that local MSI, but never
#     mirrors it or places it in a FreeBASIC release package.

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

ROM_MSI_SHA256=79d9a20de89295e931b6d7cb0dea78e54a0918900e9603471f88b397edd1d155
# msiextract uses the install-time leaf name rather than the MSI table's
# internal CGen identifier used by lower-level extraction tools.
ROM_FILE_NAME=SP_USA_GSM_QVGA_VR.bin
ROM_SHA256=a8caba22e8f9aa4f7ad3972249a04883b39c79e03d1b975d777510f4544e07cb

EMULATOR_ROOT="${WINCE_EMULATOR_ROOT:-$ROOT/out/wince/emulator}"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf}"
EMULATOR_IMAGE="${WINCE_EMULATOR_IMAGE:-freebasic-wince-emulator:noble}"
ROM_MSI=""
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
	   [[ "$PREPARE_STAGE" == "$EMULATOR_ROOT"/.prepare-arm.* ]] &&
	   [ -d "$PREPARE_STAGE" ]; then
		rm -rf -- "$PREPARE_STAGE"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/prepare-arm-emulator.sh --rom-msi FILE [options]

Required:
  --rom-msi FILE      Windows Mobile 6 Standard Images (USA).msi obtained
                      from the official Microsoft Download Center.

Options:
  --emulator-dir DIR  Persistent emulator root. Default: out/wince/emulator
  --cerf-dir DIR      Prepared CERF tree. Default: out/wince/emulator/cerf
  --image NAME        Emulator support image. Default: $EMULATOR_IMAGE
  --skip-image        Reuse an existing support image.
  -h, --help          Show this help.

The expected MSI SHA-256 is:
  $ROM_MSI_SHA256
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing and guards
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--rom-msi)
			require_value "$1" "${2-}"
			ROM_MSI="$2"
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

[ -n "$ROM_MSI" ] || die "--rom-msi is required"
[ -f "$ROM_MSI" ] || die "Microsoft emulator-image MSI not found: $ROM_MSI"

for host_tool in docker realpath sha256sum; do
	command -v "$host_tool" >/dev/null 2>&1 ||
		die "required host tool not found: $host_tool"
done

EMULATOR_REAL="$(realpath -m "$EMULATOR_ROOT")"
CERF_REAL="$(realpath -m "$CERF_ROOT")"
ROM_MSI_REAL="$(realpath -e "$ROM_MSI")"
[ "$EMULATOR_REAL" != / ] || die "refusing to use / as the emulator root"
[ "$CERF_REAL" != / ] || die "refusing to use / as the CERF root"
[[ "$CERF_REAL" == "$EMULATOR_REAL"/* ]] ||
	die "the CERF directory must be below the emulator root"

printf '%s  %s\n' "$ROM_MSI_SHA256" "$ROM_MSI_REAL" |
	sha256sum --check --status ||
	die "the Microsoft MSI does not match the qualified USA image package"

mkdir -p "$EMULATOR_REAL/downloads" "$CERF_REAL"

##############################################################################
# Contained extraction
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

PREPARE_STAGE="$(mktemp -d "$EMULATOR_REAL/.prepare-arm.XXXXXX")"
mkdir -p "$PREPARE_STAGE/cerf-release" "$PREPARE_STAGE/microsoft-images"

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

msg "extracting CERF and the user-supplied ARM ROM"
docker run --rm \
	--user "$(id -u):$(id -g)" \
	-v "$EMULATOR_REAL:/emulator" \
	-v "$PREPARE_STAGE:/stage" \
	--mount "type=bind,src=$ROM_MSI_REAL,dst=/input/rom.msi,readonly" \
	"$EMULATOR_IMAGE" \
	bash -ceu '
		unzip -q "/emulator/downloads/$1" \
			-d /stage/cerf-release
		msiextract -C /stage/microsoft-images /input/rom.msi >/dev/null
	' -- "$CERF_ARCHIVE"

EXTRACTED_ROM="$(find "$PREPARE_STAGE/microsoft-images" -type f \
	-name "$ROM_FILE_NAME" -print -quit)"
[ -n "$EXTRACTED_ROM" ] || die "qualified QVGA ARM ROM was not found in the MSI"
printf '%s  %s\n' "$ROM_SHA256" "$EXTRACTED_ROM" |
	sha256sum --check --status || die "extracted ARM ROM digest mismatch"
[ -s "$PREPARE_STAGE/cerf-release/cerf.exe" ] ||
	die "CERF archive did not contain cerf.exe"

##############################################################################
# Durable emulator tree
##############################################################################

msg "staging the qualified CERF ARM tree"
cp -a "$PREPARE_STAGE/cerf-release/." "$CERF_REAL/"
cp "$EXTRACTED_ROM" "$CERF_REAL/roms-wince-arm.bin"
mkdir -p "$CERF_REAL/share" "$CERF_REAL/logs"

printf '%s  %s\n' "$CERF_SHA256" "$EMULATOR_REAL/downloads/$CERF_ARCHIVE" |
	sha256sum --check --status || die "staged CERF archive digest mismatch"
printf '%s  %s\n' "$ROM_SHA256" "$CERF_REAL/roms-wince-arm.bin" |
	sha256sum --check --status || die "staged ARM ROM digest mismatch"

msg "Windows CE ARM emulator prepared at $CERF_REAL"

# end of build_scripts/wince/prepare-arm-emulator.sh
