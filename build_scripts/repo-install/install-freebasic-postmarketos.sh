#!/bin/sh
#
# FreeBASIC repository installer: postmarketOS
#

set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd -P)
if [ -r "$script_dir/freebasic-install-common.sh" ]; then
	. "$script_dir/freebasic-install-common.sh"
else
	repo_url="${FREEBASIC_REPO_URL:-https://deb.fbxl.net}"
	tmp="${TMPDIR:-/tmp}/freebasic-install-common.$$"
	if command -v curl >/dev/null 2>&1; then
		curl -fsSL "$repo_url/install/freebasic-install-common.sh" -o "$tmp"
	else
		wget -q -O "$tmp" "$repo_url/install/freebasic-install-common.sh"
	fi
	. "$tmp"
fi

freebasic_install_main --family apk --distro postmarketos "$@"

# end of install-freebasic-postmarketos.sh
