#!/usr/bin/env bash
#
# Project: FreeBASIC repository publisher
# ---------------------------------------
#
# File: make-freebasic-repositories.sh
#
# Purpose:
#
#     Prepare the local out/ tree so it can be served as deb.fbxl.net.
#
# Responsibilities:
#
#     * generate apt metadata for .deb package directories
#     * generate rpm metadata for .rpm package directories when createrepo_c is available
#     * generate apk metadata for .apk package directories when apk is available
#     * write SHA256SUMS for every package directory
#     * publish per-distro installer scripts under out/install/
#
# This file intentionally does NOT contain:
#
#     * package building
#     * package signing key management
#     * web server configuration
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
	if [ -d "$SEARCH_DIR/build_scripts" ] && { [ -f "$SEARCH_DIR/GNUmakefile" ] || [ -f "$SEARCH_DIR/makefile" ] || [ -f "$SEARCH_DIR/Makefile" ]; }; then
		ROOT="$SEARCH_DIR"
		break
	fi

	[ "$SEARCH_DIR" = "/" ] && break
	SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root" >&2; exit 1; }
cd "$ROOT"

##############################################################################
# Helpers
##############################################################################

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf 'WARNING: %s\n' "$*" >&2; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

run() {
	printf '==> %s\n' "$*"
	"$@"
}

run_root() {
	if [ "$(id -u)" -eq 0 ]; then
		run "$@"
		return
	fi

	if command -v sudo >/dev/null 2>&1; then
		run sudo "$@"
		return
	fi

	die "root privileges are required for dependency installation"
}

usage() {
	cat <<EOF
Usage: ./build_scripts/make-freebasic-repositories.sh [options]

Options:
  --out-base DIR       Repository root to prepare (default: out)
  --repo-url URL       Public repository URL used in generated examples
                       (default: https://deb.fbxl.net)
  --skip-deps          Do not install Ubuntu-side metadata tools
  --strict             Fail if optional metadata tools are unavailable
  --help               Show this help text

The script is meant to run on Ubuntu after package builds have populated out/.
The resulting out/ tree can be published directly as deb.fbxl.net.
EOF
}

##############################################################################
# Options
##############################################################################

OUT_BASE="$ROOT/out"
REPO_URL="${FREEBASIC_REPO_URL:-https://deb.fbxl.net}"
SKIP_DEPS=0
STRICT=0

while [ "$#" -gt 0 ]; do
	case "$1" in
	--out-base) OUT_BASE="$2"; shift 2 ;;
	--repo-url) REPO_URL="$2"; shift 2 ;;
	--skip-deps) SKIP_DEPS=1; shift ;;
	--strict) STRICT=1; shift ;;
	-h|--help) usage; exit 0 ;;
	*) die "unknown option: $1" ;;
	esac
done

[ -d "$OUT_BASE" ] || die "out base does not exist: $OUT_BASE"
OUT_BASE="$(cd "$OUT_BASE" && pwd -P)"

##############################################################################
# Ubuntu-side metadata tools
##############################################################################

apt_has_package() {
	apt-cache show "$1" >/dev/null 2>&1
}

install_ubuntu_tools() {
	[ "$SKIP_DEPS" -eq 0 ] || return 0

	command -v apt-get >/dev/null 2>&1 || {
		warn "apt-get was not found; skipping dependency installation"
		return 0
	}

	msg "installing repository metadata tools"
	run_root apt-get update -y

	deps=(ca-certificates coreutils dpkg-dev gzip xz-utils)

	if apt_has_package createrepo-c; then
		deps+=(createrepo-c)
	fi

	if apt_has_package apk-tools; then
		deps+=(apk-tools)
	fi

	run_root apt-get install -y --no-install-recommends "${deps[@]}"
}

need_or_warn() {
	local tool="$1"
	local purpose="$2"

	if command -v "$tool" >/dev/null 2>&1; then
		return 0
	fi

	if [ "$STRICT" -ne 0 ]; then
		die "missing $tool for $purpose"
	fi

	warn "missing $tool; skipping $purpose"
	return 1
}

##############################################################################
# Package directory helpers
##############################################################################

has_files() {
	local dir="$1"
	local pattern="$2"

	find "$dir" -maxdepth 1 -type f -name "$pattern" -print -quit | grep -q .
}

write_sha256sums() {
	local dir="$1"

	(
		cd "$dir"
		find . -maxdepth 1 -type f \
			\( -name 'freebasic*.deb' \
			-o -name 'freebasic*.ddeb' \
			-o -name 'freebasic*.rpm' \
			-o -name 'freebasic*.apk' \
			-o -name 'freebasic*.tar.xz' \
			-o -name 'freebasic*.txz' \
			-o -name 'freebasic*.tgz' \
			-o -name 'freebasic*.pkg' \
			-o -name 'freebasic*.hpkg' \) \
			-printf '%P\0' |
			sort -z |
			xargs -0 -r sha256sum > SHA256SUMS
	)
}

write_apt_release() {
	local dir="$1"
	local distro="$2"
	local release="$3"
	local arch="$4"

	(
		cd "$dir"
		{
			printf 'Origin: FreeBASIC\n'
			printf 'Label: FreeBASIC\n'
			printf 'Suite: %s\n' "$release"
			printf 'Codename: %s\n' "$release"
			printf 'Architectures: %s\n' "$arch"
			printf 'Components: .\n'
			printf 'Description: FreeBASIC local package repository for %s/%s/%s\n' "$distro" "$release" "$arch"
			printf 'Date: %s\n' "$(date -Ru)"
			printf 'SHA256:\n'
			for f in Packages Packages.gz SHA256SUMS; do
				[ -f "$f" ] || continue
				printf ' %s %s %s\n' "$(sha256sum "$f" | awk '{print $1}')" "$(wc -c < "$f")" "$f"
			done
		} > Release
	)
}

prepare_deb_dir() {
	local dir="$1"
	local rel distro release arch

	need_or_warn dpkg-scanpackages "apt metadata for $dir" || return 0

	rel="${dir#"$OUT_BASE/linux/"}"
	IFS=/ read -r distro release arch _ <<EOF
$rel
EOF

	msg "generating apt metadata for $distro/$release/$arch"
	(
		cd "$dir"
		dpkg-scanpackages . /dev/null > Packages
		gzip -kf Packages
	)
	write_apt_release "$dir" "$distro" "$release" "$arch"
}

prepare_rpm_dir() {
	local dir="$1"

	need_or_warn createrepo_c "rpm metadata for $dir" || return 0

	msg "generating rpm metadata for ${dir#"$OUT_BASE/"}"
	createrepo_c -q "$dir"
}

prepare_apk_dir() {
	local dir="$1"

	need_or_warn apk "apk metadata for $dir" || return 0

	msg "generating apk metadata for ${dir#"$OUT_BASE/"}"
	(
		cd "$dir"
		apk index -o APKINDEX.tar.gz ./*.apk
	)
}

prepare_package_dir() {
	local dir="$1"

	write_sha256sums "$dir"

	if has_files "$dir" '*.deb'; then
		prepare_deb_dir "$dir"
	fi

	if has_files "$dir" '*.rpm'; then
		prepare_rpm_dir "$dir"
	fi

	if has_files "$dir" '*.apk'; then
		prepare_apk_dir "$dir"
	fi
}

##############################################################################
# Installer publication
##############################################################################

write_index() {
	local install_dir="$1"

	cat > "$install_dir/README.txt" <<EOF
FreeBASIC installer scripts
===========================

These scripts install packages from:

    $REPO_URL

Examples:

    sh install-freebasic-ubuntu.sh
    sh install-freebasic-debian.sh --release trixie --arch amd64
    sh install-freebasic-alpine.sh --release 3.24 --arch x86_64
    sh install-freebasic-opensuse.sh --release tumbleweed --arch x86_64
    sh install-freebasic-macos.sh --arch arm64

Override the repository root while testing:

    FREEBASIC_REPO_URL=http://localhost:8000 sh install-freebasic-ubuntu.sh

EOF
}

publish_installers() {
	local install_dir="$OUT_BASE/install"

	msg "publishing installer scripts to ${install_dir#"$ROOT/"}"
	mkdir -p "$install_dir"
	cp build_scripts/repo-install/*.sh "$install_dir/"
	chmod 0755 "$install_dir"/*.sh
	write_index "$install_dir"
}

##############################################################################
# Main
##############################################################################

main() {
	local dir

	install_ubuntu_tools

	msg "preparing package repository metadata under $OUT_BASE"

	while IFS= read -r dir; do
		prepare_package_dir "$dir"
	done < <(
		find "$OUT_BASE" -type f \
			\( -name 'freebasic*.deb' \
			-o -name 'freebasic*.ddeb' \
			-o -name 'freebasic*.rpm' \
			-o -name 'freebasic*.apk' \
			-o -name 'freebasic*.tar.xz' \
			-o -name 'freebasic*.txz' \
			-o -name 'freebasic*.tgz' \
			-o -name 'freebasic*.pkg' \
			-o -name 'freebasic*.hpkg' \) \
			-printf '%h\n' |
			sort -u
	)

	publish_installers

	msg "repository preparation complete"
	printf 'Repository root: %s\n' "$OUT_BASE"
	printf 'Public URL:      %s\n' "$REPO_URL"
	printf 'Installers:      %s/install/\n' "$REPO_URL"
}

main "$@"

##############################################################################
# end of make-freebasic-repositories.sh
##############################################################################
