#!/bin/sh
#
# Project: FreeBASIC repository installers
# ----------------------------------------
#
# File: freebasic-install-common.sh
#
# Purpose:
#
#     Shared implementation used by the per-distro install scripts published
#     below deb.fbxl.net/install/.
#
# Responsibilities:
#
#     * detect release and architecture defaults
#     * add a FreeBASIC package source where the OS supports a simple local repo
#     * install the FreeBASIC package set from deb.fbxl.net or a staging URL
#     * fall back to direct package downloads for OSes without simple repo lists
#
# This file intentionally does NOT contain:
#
#     * package repository metadata generation
#     * package building
#     * distro-specific build validation
#

set -eu

fb_msg() { printf '==> %s\n' "$*"; }
fb_die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

fb_run_root() {
	if [ "$(id -u)" -eq 0 ]; then
		"$@"
		return
	fi

	if command -v sudo >/dev/null 2>&1; then
		sudo "$@"
		return
	fi

	fb_die "this step needs root privileges; rerun as root or install sudo"
}

fb_fetch() {
	url="$1"
	output="$2"

	if command -v curl >/dev/null 2>&1; then
		curl -fsSL "$url" -o "$output"
		return
	fi

	if command -v wget >/dev/null 2>&1; then
		wget -q -O "$output" "$url"
		return
	fi

	fb_die "curl or wget is required to download $url"
}

fb_detect_release() {
	if [ -f /etc/os-release ]; then
		# shellcheck disable=SC1091
		. /etc/os-release
		printf '%s\n' "${VERSION_CODENAME:-${VERSION_ID:-unknown}}"
		return
	fi

	printf '%s\n' "unknown"
}

fb_detect_arch() {
	family="$1"

	case "$family" in
	deb)
		dpkg --print-architecture
		;;
	apk)
		if command -v apk >/dev/null 2>&1; then
			apk --print-arch
		else
			uname -m
		fi
		;;
	slackware)
		case "$(uname -m)" in
		i?86) printf '%s\n' "i586" ;;
		*) uname -m ;;
		esac
		;;
	freebsd|dragonfly|openbsd|netbsd|haiku)
		case "$(uname -m)" in
		x86_64|amd64) printf '%s\n' "x86-64" ;;
		*) uname -m ;;
		esac
		;;
	*)
		uname -m
		;;
	esac
}

fb_package_url() {
	case "$family" in
	deb|rpm|apk|slackware)
		printf '%s/linux/%s/%s/%s\n' "$repo_base" "$distro" "$release" "$arch"
		;;
	freebsd|dragonfly|openbsd|netbsd|haiku)
		printf '%s/%s/%s\n' "$repo_base" "$distro" "$arch"
		;;
	*)
		fb_die "unsupported package family: $family"
		;;
	esac
}

fb_make_temp_dir() {
	tmp="${TMPDIR:-/tmp}/freebasic-install.$$"
	rm -rf "$tmp"
	mkdir -p "$tmp"
	printf '%s\n' "$tmp"
}

fb_find_remote_package() {
	repo_url="$1"
	pattern="$2"
	tmpdir="$3"
	index="$tmpdir/SHA256SUMS"

	fb_fetch "$repo_url/SHA256SUMS" "$index"
	awk -v pattern="$pattern" '
		$2 ~ pattern {
			name = $2
			sub(/^\*/, "", name)
			print name
			exit
		}
	' "$index"
}

fb_download_remote_package() {
	repo_url="$1"
	pattern="$2"
	tmpdir="$3"
	name="$(fb_find_remote_package "$repo_url" "$pattern" "$tmpdir")"

	[ -n "$name" ] || fb_die "no package matching $pattern listed at $repo_url"

	fb_fetch "$repo_url/$name" "$tmpdir/$name"

	if command -v sha256sum >/dev/null 2>&1; then
		(
			cd "$tmpdir"
			grep "[[:space:]]\\*\\{0,1\\}$name\$" SHA256SUMS | sha256sum -c -
		)
	fi

	printf '%s\n' "$tmpdir/$name"
}

fb_install_deb() {
	list_file="/etc/apt/sources.list.d/freebasic-${distro}-${release}-${arch}.list"

	if [ "$skip_repo" -eq 0 ]; then
		fb_msg "adding apt source: $repo_url"
		fb_run_root sh -c "printf '%s\n' 'deb [trusted=yes] $repo_url ./' > '$list_file'"
		fb_run_root apt-get update -y
	fi

	if [ "$skip_install" -eq 0 ]; then
		export DEBIAN_FRONTEND=noninteractive
		if apt-cache show freebasic-full >/dev/null 2>&1; then
			fb_run_root apt-get install -y --no-install-recommends freebasic-full
		else
			fb_run_root apt-get install -y --no-install-recommends freebasic
		fi
	fi
}

fb_install_rpm() {
	if [ "$skip_repo" -eq 0 ]; then
		if command -v zypper >/dev/null 2>&1; then
			fb_msg "adding zypper source: $repo_url"
			fb_run_root zypper --non-interactive rr "freebasic-${distro}-${release}-${arch}" >/dev/null 2>&1 || true
			fb_run_root zypper --non-interactive ar -f -G "$repo_url" "freebasic-${distro}-${release}-${arch}"
			fb_run_root zypper --non-interactive refresh "freebasic-${distro}-${release}-${arch}"
		else
			repo_file="/etc/yum.repos.d/freebasic-${distro}-${release}-${arch}.repo"
			fb_msg "adding rpm source: $repo_url"
			fb_run_root mkdir -p /etc/yum.repos.d
			fb_run_root sh -c "cat > '$repo_file' <<EOF
[freebasic-${distro}-${release}-${arch}]
name=FreeBASIC ${distro} ${release} ${arch}
baseurl=$repo_url
enabled=1
gpgcheck=0
EOF"
			if command -v dnf >/dev/null 2>&1; then
				fb_run_root dnf makecache -y || true
			elif command -v yum >/dev/null 2>&1; then
				fb_run_root yum makecache -y || true
			fi
		fi
	fi

	if [ "$skip_install" -ne 0 ]; then
		return
	fi

	if command -v dnf >/dev/null 2>&1; then
		fb_run_root dnf install -y freebasic && return
	elif command -v yum >/dev/null 2>&1; then
		fb_run_root yum install -y freebasic && return
	elif command -v zypper >/dev/null 2>&1; then
		fb_run_root zypper --non-interactive install -y freebasic && return
	fi

	tmpdir="$(fb_make_temp_dir)"
	pkg="$(fb_download_remote_package "$repo_url" 'freebasic.*\.rpm$' "$tmpdir")"
	if command -v rpm >/dev/null 2>&1; then
		fb_run_root rpm -Uvh --replacepkgs "$pkg"
	else
		fb_die "no rpm package manager found"
	fi
}

fb_install_apk() {
	if [ "$skip_repo" -eq 0 ]; then
		fb_msg "adding apk source: $repo_url"
		fb_run_root sh -c "grep -qx '$repo_url' /etc/apk/repositories 2>/dev/null || printf '%s\n' '$repo_url' >> /etc/apk/repositories"
		fb_run_root apk update || true
	fi

	if [ "$skip_install" -eq 0 ]; then
		fb_run_root apk add --allow-untrusted freebasic && return

		tmpdir="$(fb_make_temp_dir)"
		pkg="$(fb_download_remote_package "$repo_url" 'freebasic.*\.apk$' "$tmpdir")"
		fb_run_root apk add --allow-untrusted "$pkg"
	fi
}

fb_install_slackware() {
	tmpdir="$(fb_make_temp_dir)"
	pkg="$(fb_download_remote_package "$repo_url" 'freebasic.*\.txz$' "$tmpdir")"

	[ "$skip_install" -ne 0 ] || fb_run_root installpkg "$pkg"
}

fb_install_pkg() {
	tmpdir="$(fb_make_temp_dir)"
	pkg="$(fb_download_remote_package "$repo_url" "$1" "$tmpdir")"

	[ "$skip_install" -ne 0 ] || fb_run_root pkg add -f "$pkg"
}

fb_install_pkg_add() {
	tmpdir="$(fb_make_temp_dir)"
	pkg="$(fb_download_remote_package "$repo_url" "$1" "$tmpdir")"

	if [ "$skip_install" -ne 0 ]; then
		return
	fi

	case "$family" in
	openbsd)
		fb_run_root pkg_add -D unsigned "$pkg"
		;;
	netbsd)
		fb_run_root pkg_add "$pkg"
		;;
	esac
}

fb_install_haiku() {
	tmpdir="$(fb_make_temp_dir)"
	pkg="$(fb_download_remote_package "$repo_url" 'freebasic.*\.hpkg$' "$tmpdir")"

	[ "$skip_install" -ne 0 ] || pkgman install -y "$pkg"
}

fb_verify() {
	[ "$skip_verify" -eq 0 ] || return

	if command -v fbc >/dev/null 2>&1; then
		fbc -version
	else
		fb_die "fbc was not found after installation"
	fi
}

freebasic_install_main() {
	family=""
	distro=""
	release=""
	arch=""
	repo_base="${FREEBASIC_REPO_URL:-https://deb.fbxl.net}"
	skip_repo=0
	skip_install=0
	skip_verify=0

	while [ "$#" -gt 0 ]; do
		case "$1" in
		--family) family="$2"; shift 2 ;;
		--distro) distro="$2"; shift 2 ;;
		--release) release="$2"; shift 2 ;;
		--arch) arch="$2"; shift 2 ;;
		--base-url|--repo-url) repo_base="$2"; shift 2 ;;
		--skip-repo) skip_repo=1; shift ;;
		--skip-install) skip_install=1; shift ;;
		--skip-verify) skip_verify=1; shift ;;
		-h|--help)
			cat <<EOF
Usage: $0 [--release NAME] [--arch ARCH] [--base-url URL]

Environment:
  FREEBASIC_REPO_URL  Repository root URL, default https://deb.fbxl.net

EOF
			return 0
			;;
		*)
			fb_die "unknown option: $1"
			;;
		esac
	done

	[ -n "$family" ] || fb_die "--family was not supplied"
	[ -n "$distro" ] || fb_die "--distro was not supplied"

	[ -n "$release" ] || release="$(fb_detect_release)"
	[ -n "$arch" ] || arch="$(fb_detect_arch "$family")"

	repo_url="$(fb_package_url)"
	fb_msg "using FreeBASIC package source: $repo_url"

	case "$family" in
	deb) fb_install_deb ;;
	rpm) fb_install_rpm ;;
	apk) fb_install_apk ;;
	slackware) fb_install_slackware ;;
	freebsd) fb_install_pkg 'freebasic.*\.pkg$' ;;
	dragonfly) fb_install_pkg 'freebasic.*\.pkg$' ;;
	openbsd) fb_install_pkg_add 'freebasic.*\.tgz$' ;;
	netbsd) fb_install_pkg_add 'freebasic.*\.tgz$' ;;
	haiku) fb_install_haiku ;;
	*) fb_die "unsupported package family: $family" ;;
	esac

	fb_verify
}

# end of freebasic-install-common.sh
