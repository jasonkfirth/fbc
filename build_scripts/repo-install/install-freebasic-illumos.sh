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
compiler=/usr/local/bin/fbc

if ! command -v pkg >/dev/null 2>&1; then
	printf '%s\n' 'ERROR: the illumos pkg command is required' >&2
	exit 1
fi

printf '==> configuring FreeBASIC publisher: %s\n' "$repo_url"
pkg set-publisher --no-refresh -G '*' -M '*' -g "$repo_url" local
pkg refresh local

printf '%s\n' '==> installing FreeBASIC'
if pkg install --accept pkg://local/lang/freebasic; then
	:
else
	install_status=$?

	# IPS returns 4 when the requested package is already current.  The
	# verification below still confirms that the installed compiler works.
	if [ "$install_status" -ne 4 ]; then
		exit "$install_status"
	fi

	printf '%s\n' '==> FreeBASIC is already current'
fi

printf '%s\n' '==> verifying FreeBASIC'
if command -v fbc >/dev/null 2>&1; then
	compiler=$(command -v fbc)
elif [ ! -x "$compiler" ]; then
	printf 'ERROR: the installed compiler is missing: %s\n' "$compiler" >&2
	exit 1
fi

"$compiler" -version

if ! command -v fbc >/dev/null 2>&1; then
	printf '%s\n' 'NOTE: add /usr/local/bin to PATH to invoke fbc by name.'
fi

# end of install-freebasic-illumos.sh
