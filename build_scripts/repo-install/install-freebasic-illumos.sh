#!/bin/sh

# FreeBASIC package repository
#
# Install the current FreeBASIC IPS package on illumos systems.  The public
# repository is a static pkg(7) repository, so the operating system package
# client can use it directly without a separate package-server process.
#
# This file intentionally does not build packages or change the system's
# OpenIndiana publisher.

set -eu

repo_base=${FREEBASIC_REPO_URL:-https://deb.fbxl.net}
repo_url=${FREEBASIC_ILLUMOS_REPO_URL:-$repo_base/illumos/5.11/amd64/repo}

if ! command -v pkg >/dev/null 2>&1; then
	printf '%s\n' 'ERROR: the illumos pkg command is required' >&2
	exit 1
fi

printf '==> configuring FreeBASIC publisher: %s\n' "$repo_url"
pkg set-publisher --no-refresh -G '*' -M '*' -g "$repo_url" local
pkg refresh local

printf '%s\n' '==> installing FreeBASIC'
pkg install --accept pkg://local/lang/freebasic

printf '%s\n' '==> verifying FreeBASIC'
command -v fbc >/dev/null 2>&1
fbc -version

# end of install-freebasic-illumos.sh
