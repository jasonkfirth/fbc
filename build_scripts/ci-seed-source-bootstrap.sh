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

# GitHub exposes RUNNER_TEMP as a native drive-letter path on Windows.  GNU
# tar treats the colon in that spelling as a remote-host separator, even when
# the command is running inside MSYS2.  Convert it once so every later archive
# operation sees an ordinary POSIX path.
case "$(uname -s 2>/dev/null || true)" in
	MSYS*|MINGW*|CYGWIN*)
		command -v cygpath >/dev/null 2>&1 || {
			echo "ERROR: cygpath is required for Windows archive paths" >&2
			exit 1
		}
		ARCHIVE="$(cygpath -u "$ARCHIVE")"
		;;
esac

DOWNLOAD_TEMP=""
cleanup_download_temp() {
	if [ -n "$DOWNLOAD_TEMP" ]; then
		rm -f -- "$DOWNLOAD_TEMP"
	fi
}
trap cleanup_download_temp EXIT

# A failed or interrupted download must not leave a partial archive that a
# later invocation mistakes for a reusable bootstrap source package.
ARCHIVE_VALID=0
if [ -f "$ARCHIVE" ] && xz -t "$ARCHIVE" >/dev/null 2>&1; then
	ARCHIVE_VALID=1
fi

if [ "$ARCHIVE_VALID" -eq 0 ]; then
	command -v curl >/dev/null 2>&1 || {
		echo "ERROR: curl is required to download the source bootstrap archive" >&2
		exit 1
	}

	mkdir -p "$(dirname "$ARCHIVE")"
	DOWNLOAD_TEMP="${ARCHIVE}.part.$$"
	curl --fail --location --retry 10 --retry-max-time 300 \
		"https://github.com/freebasic/fbc/releases/download/${BOOTSTRAP_VERSION}/${PACKAGE}.tar.xz" \
		--output "$DOWNLOAD_TEMP"
	xz -t "$DOWNLOAD_TEMP"
	mv -f -- "$DOWNLOAD_TEMP" "$ARCHIVE"
	DOWNLOAD_TEMP=""
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
