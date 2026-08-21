#!/usr/bin/env bash
#
# Project: FreeBASIC XL CI
# ------------------------
#
# File: build_scripts/ci-seed-source-bootstrap.sh
#
# Purpose:
#
#     Seed the ignored bootstrap directory used by clean-checkout CI jobs.
#     The archive contains generated C sources, not a prebuilt compiler, so
#     every caller still builds the bootstrap compiler for its own host.
#
# Responsibilities:
#
#     * download or reuse the pinned upstream source-bootstrap archive
#     * copy one donor target's generated compiler sources into a requested
#       bootstrap target directory
#     * verify that the resulting directory contains compiler source files
#
# This file intentionally does NOT contain:
#
#     * compiler or runtime build commands
#     * target toolchain installation
#     * release packaging or artifact publication

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate the project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
	if [ -f "$SEARCH_DIR/GNUmakefile" ] && [ -d "$SEARCH_DIR/build_scripts" ]; then
		ROOT="$SEARCH_DIR"
		break
	fi

	[ "$SEARCH_DIR" = "/" ] && break
	SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || {
	echo "ERROR: could not locate the FreeBASIC source root" >&2
	exit 1
}

##############################################################################
# Arguments and archive selection
##############################################################################

TARGET_DIR="${1:-}"
DONOR_DIR="${2:-linux-x86_64}"
BOOTSTRAP_VERSION="${BOOTSTRAP_VERSION:-1.10.1}"

[ -n "$TARGET_DIR" ] || {
	echo "Usage: $0 TARGET_BOOTSTRAP_DIR [DONOR_BOOTSTRAP_DIR]" >&2
	exit 2
}

case "$TARGET_DIR" in
	/*|*..*|*/*)
		echo "ERROR: target bootstrap directory must be a single directory name: $TARGET_DIR" >&2
		exit 2
		;;
esac

case "$DONOR_DIR" in
	/*|*..*|*/*)
		echo "ERROR: donor bootstrap directory must be a single directory name: $DONOR_DIR" >&2
		exit 2
		;;
esac

PACKAGE="FreeBASIC-${BOOTSTRAP_VERSION}-source-bootstrap"
ARCHIVE="${BOOTSTRAP_ARCHIVE:-${RUNNER_TEMP:-/tmp}/${PACKAGE}.tar.xz}"

if [ ! -f "$ARCHIVE" ]; then
	command -v curl >/dev/null 2>&1 || {
		echo "ERROR: curl is required to download the source bootstrap archive" >&2
		exit 1
	}

	mkdir -p "$(dirname "$ARCHIVE")"
	curl --fail --location --retry 5 \
		"https://github.com/freebasic/fbc/releases/download/${BOOTSTRAP_VERSION}/${PACKAGE}.tar.xz" \
		--output "$ARCHIVE"
fi

##############################################################################
# Extract and validate the generated source
##############################################################################

cd "$ROOT"
mkdir -p "bootstrap/$TARGET_DIR"

tar -xJf "$ARCHIVE" --strip-components=3 -C "bootstrap/$TARGET_DIR" \
	"$PACKAGE/bootstrap/$DONOR_DIR"

if ! find "bootstrap/$TARGET_DIR" -maxdepth 1 -type f \
	\( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
	echo "ERROR: source bootstrap extraction produced no compiler sources" >&2
	exit 1
fi

echo "==> Seeded bootstrap/$TARGET_DIR from $PACKAGE/bootstrap/$DONOR_DIR"

# end of build_scripts/ci-seed-source-bootstrap.sh
