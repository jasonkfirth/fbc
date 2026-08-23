#!/usr/bin/env bash
#
# Project: FreeBASIC XL CI
# ------------------------
#
# File: build_scripts/ci-install-windows-devkitpro.sh
#
# Purpose:
#
#     Install the official devkitPro Wii toolchain into the MSYS2 environment
#     used by the hosted Windows package job.
#
# Responsibilities:
#
#     * install and verify devkitPro's pinned signing keyring
#     * add the official library and Windows package repositories
#     * install the wii-dev package group with bounded retries
#     * verify the tools and library layout required by the Wii package script
#
# This file intentionally does NOT contain:
#
#     * Wii compiler or runtime build logic
#     * alternate devkitPro package mirrors
#     * package installation policy for non-Windows hosts

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Configuration
##############################################################################

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
PACMAN_RETRY="$SCRIPT_DIR/msys2-pacman-retry.sh"
PACMAN_CONF="${PACMAN_CONF:-/etc/pacman.conf}"

DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
DEVKITPPC="${DEVKITPPC:-$DEVKITPRO/devkitPPC}"

KEYRING_VERSION="20241017-2"
KEYRING_PACKAGE="devkitpro-keyring-${KEYRING_VERSION}-any.pkg.tar.zst"
KEYRING_URL="https://pkg.devkitpro.org/packages/windows/x86_64/${KEYRING_PACKAGE}"
KEYRING_SHA256="2d7d20f2ea33127cf8282d4a0773dea08850c2d63a30d6f9a2385df2293922d2"
KEYRING_FINGERPRINT="BC26F752D25B92CE272E0F44F7FD5492264BB9D0"

DKP_LIBS_SERVER="https://pkg.devkitpro.org/packages"
DKP_WINDOWS_SERVER="https://pkg.devkitpro.org/packages/windows/$(uname -m)"

##############################################################################
# Validation helpers
##############################################################################

fail()
{
	echo "ERROR: $*" >&2
	exit 1
}

require_command()
{
	command -v "$1" >/dev/null 2>&1 || fail "$1 is required"
}

case "$(uname -s)" in
	MSYS*|MINGW*|CYGWIN*) ;;
	*) fail "this bootstrap must run inside a Windows MSYS2 environment" ;;
esac

for command_name in curl grep mktemp pacman pacman-conf pacman-key sed sha256sum; do
	require_command "$command_name"
done

[ -x "$PACMAN_RETRY" ] || [ -f "$PACMAN_RETRY" ] ||
	fail "pacman retry helper was not found: $PACMAN_RETRY"
[ -f "$PACMAN_CONF" ] || fail "pacman configuration was not found: $PACMAN_CONF"
[ -w "$PACMAN_CONF" ] || fail "pacman configuration is not writable: $PACMAN_CONF"

##############################################################################
# devkitPro signing keyring
##############################################################################

install_keyring()
{
	local keyring_file

	if pacman -Q devkitpro-keyring >/dev/null 2>&1; then
		echo "==> devkitPro signing keyring is already installed"
		return 0
	fi

	keyring_file="$(mktemp "${TMPDIR:-/tmp}/devkitpro-keyring.XXXXXX.pkg.tar.zst")"

	echo "==> Downloading the pinned devkitPro signing keyring"
	curl --fail --location --retry 5 \
		--user-agent 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) FreeBASIC-XL-CI' \
		--output "$keyring_file" \
		"$KEYRING_URL"

	printf '%s  %s\n' "$KEYRING_SHA256" "$keyring_file" |
		sha256sum --check --status || fail "devkitPro keyring checksum verification failed"

	bash "$PACMAN_RETRY" --noconfirm -U "$keyring_file"
	rm -f "$keyring_file"
}

install_keyring
pacman-key --populate devkitpro
pacman-key --list-keys "$KEYRING_FINGERPRINT" >/dev/null 2>&1 ||
	fail "the trusted devkitPro signing key was not imported"

##############################################################################
# Official devkitPro repositories
##############################################################################

configure_repositories()
{
	local has_libs=0
	local has_windows=0
	local libs_server
	local windows_server

	if pacman-conf --repo-list | grep -Fxq 'dkp-libs'; then
		has_libs=1
	fi

	if pacman-conf --repo-list | grep -Fxq 'dkp-windows'; then
		has_windows=1
	fi

	if [ "$has_libs" -eq 0 ] && [ "$has_windows" -eq 0 ]; then
		cat >> "$PACMAN_CONF" <<'EOF'

# Official devkitPro packages used by the FreeBASIC Wii package job.
[dkp-libs]
Server = https://pkg.devkitpro.org/packages

[dkp-windows]
Server = https://pkg.devkitpro.org/packages/windows/$arch/
EOF
	elif [ "$has_libs" -ne 1 ] || [ "$has_windows" -ne 1 ]; then
		fail "pacman has an incomplete devkitPro repository configuration"
	fi

	libs_server="$(pacman-conf --repo dkp-libs Server)"
	windows_server="$(pacman-conf --repo dkp-windows Server)"

	[ "${libs_server%/}" = "${DKP_LIBS_SERVER%/}" ] ||
		fail "unexpected dkp-libs server: $libs_server"
	[ "${windows_server%/}" = "${DKP_WINDOWS_SERVER%/}" ] ||
		fail "unexpected dkp-windows server: $windows_server"
}

configure_repositories

##############################################################################
# Wii toolchain installation and verification
##############################################################################

echo "==> Synchronizing the official devkitPro repositories"
bash "$PACMAN_RETRY" --noconfirm -Sy

echo "==> Installing the official devkitPro wii-dev package group"
bash "$PACMAN_RETRY" --needed --noconfirm -S wii-dev

for tool_path in \
	"$DEVKITPPC/bin/powerpc-eabi-gcc.exe" \
	"$DEVKITPPC/bin/powerpc-eabi-g++.exe" \
	"$DEVKITPPC/bin/powerpc-eabi-as.exe" \
	"$DEVKITPPC/bin/powerpc-eabi-ar.exe" \
	"$DEVKITPPC/bin/powerpc-eabi-ranlib.exe" \
	"$DEVKITPRO/tools/bin/elf2dol.exe"; do
	[ -x "$tool_path" ] || fail "devkitPro tool was not installed: $tool_path"
done

[ -d "$DEVKITPRO/libogc/include" ] ||
	fail "libogc headers were not installed"
[ -d "$DEVKITPRO/libogc/lib/wii" ] ||
	fail "libogc Wii libraries were not installed"

"$DEVKITPPC/bin/powerpc-eabi-gcc.exe" --version | sed -n '1p'
echo "==> devkitPro Wii toolchain is ready in $DEVKITPRO"

# end of build_scripts/ci-install-windows-devkitpro.sh
