#!/usr/bin/env bash

##############################################################################
# FreeBASIC illumos native package builder
##############################################################################
#
# Purpose:
#
#   Build FreeBASIC natively on an illumos system and publish an IPS package
#   repository under out/illumos.
#
# Responsibilities:
#
#   * install the native build/runtime dependency set through pkg(5)
#   * build the bootstrap compiler and full FreeBASIC tree
#   * stage an installation under a temporary build root
#   * publish an IPS package repository and run basic smoke tests
#
# This script intentionally does NOT contain:
#
#   * cross-compilation from another operating system
#   * VM provisioning
#   * cross-ISA packaging from a different illumos ISA
#
##############################################################################

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Environment
##############################################################################

CC="gcc"
CXX="g++"
GMAKE=""
FBC_GMAKE_BOOTSTRAP_VERSION="${FBC_GMAKE_BOOTSTRAP_VERSION:-4.4.1}"
FBC_GMAKE_BOOTSTRAP_PREFIX="${FBC_GMAKE_BOOTSTRAP_PREFIX:-/var/tmp/freebasic-gnu-make}"
BUILD_MONITOR_PID=""

NATIVE_ISA="$(isainfo -n 2>/dev/null || uname -p)"
OOCE_LIBDIR="/opt/ooce/lib"
FBC_ARCH=""
GNU_TARGET_BIN=""

if [ -d "/opt/ooce/lib/$NATIVE_ISA" ]; then
    OOCE_LIBDIR="/opt/ooce/lib/$NATIVE_ISA"
fi

case "$NATIVE_ISA" in
    amd64|x86_64)
        FBC_ARCH="x86_64"
        GNU_TARGET_BIN="/usr/gnu/x86_64-pc-solaris2.11/bin"
        ;;
    i386|i486|i586|i686)
        FBC_ARCH="x86"
        GNU_TARGET_BIN="/usr/gnu/i386-pc-solaris2.11/bin"
        ;;
    *) echo "ERROR: unsupported illumos ISA: $NATIVE_ISA"; exit 1 ;;
esac

export PATH="/opt/gcc-15/bin:/opt/gcc-14/bin:/opt/gcc-13/bin:/opt/ooce/bin:/usr/gnu/bin:$GNU_TARGET_BIN:/usr/bin:/usr/sbin:/sbin:$PATH"
for _fbc_gcc_path in \
	/usr/gcc/15/bin \
	/usr/gcc/14/bin \
	/usr/gcc/13/bin \
	/usr/gcc/12/bin \
	/usr/gcc/11/bin \
	/usr/sfw/bin \
	/opt/gcc/bin; do
	[ -d "$_fbc_gcc_path" ] || continue
	case ":$PATH:" in
		*":$_fbc_gcc_path:"*) ;;
		*) PATH="$_fbc_gcc_path:$PATH" ;;
	esac
done
export PATH

export CC
export CXX
export PKG_CONFIG_PATH="$OOCE_LIBDIR/pkgconfig:/usr/lib/$NATIVE_ISA/pkgconfig:/usr/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

BUILD_JOBS="${NATIVE_JOBS:-}"
BOOTSTRAP_CFLAGS="${ILLUMOS_BOOTSTRAP_CFLAGS:--Wfatal-errors -O0}"

if [ -z "$BUILD_JOBS" ]; then
    BUILD_CPUS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
    BUILD_JOBS="$(( (BUILD_CPUS + 1) / 2 ))"
fi

case "$BUILD_JOBS" in
    ''|*[!0-9]*|0) echo "ERROR: NATIVE_JOBS must be a positive integer"; exit 1 ;;
esac

##############################################################################
# TLS policy
##############################################################################

prepare_tls_policy() {
    #
    # Some illumos cloud images use OpenSSL/libcurl combinations that can
    # stall during TLS 1.3 handshakes through QEMU user networking.  IPS uses
    # libcurl for repository access, so cap the build environment at TLS 1.2.
    #
    cat > /tmp/freebasic-illumos-openssl.cnf <<'EOF'
openssl_conf = openssl_init

[openssl_init]
ssl_conf = ssl_section

[ssl_section]
system_default = system_default_section

[system_default_section]
MaxProtocol = TLSv1.2
EOF

    export OPENSSL_CONF=/tmp/freebasic-illumos-openssl.cnf
}

configure_pkg_proxy() {
    [ -n "${ILLUMOS_PKG_PROXY:-}" ] || return 0
    local origin_urls=(
        "${ILLUMOS_PKG_PROXY}/pub/openindiana/hipster/publisher/openindiana.org"
        "${ILLUMOS_PKG_PROXY}/pub/openindiana/hipster/"
        "${ILLUMOS_PKG_PROXY}/pub/openindiana/hipster"
        "${ILLUMOS_PKG_PROXY}/pub/openindiana/hipster/publisher/openindiana.org/"
    )
    local proxy_http
    local proxy_http_fallback
    local proxy_ftp
    local origin_url

    proxy_http="${ILLUMOS_PKG_PROXY}"
    if echo "$proxy_http" | grep -q '^https://'; then
        proxy_http_fallback="http://${proxy_http#https://}"
        origin_urls+=( "${proxy_http_fallback}/pub/openindiana/hipster" )
        origin_urls+=( "${proxy_http_fallback}/pub/openindiana/hipster/publisher/openindiana.org" )
    elif echo "$proxy_http" | grep -q '^ftp://'; then
        proxy_ftp="http://${proxy_http#ftp://}"
        origin_urls+=( "${proxy_ftp}/pub/openindiana/hipster" )
        origin_urls+=( "${proxy_ftp}/pub/openindiana/hipster/publisher/openindiana.org" )
    fi

    local origin_url

    echo "==> configuring OpenIndiana package proxy"
    for origin_url in "${origin_urls[@]}"; do
        if pkg set-publisher --no-refresh -G '*' -M '*' -g "$origin_url" openindiana.org; then
            if pkg refresh openindiana.org >/tmp/pkg-refresh.log 2>&1; then
                return 0
            else
                echo "==> failed refresh for ${origin_url} (publisher command set -g) - first lines of output:"
                sed -n '1,80p' /tmp/pkg-refresh.log
            fi
        fi
        if pkg set-publisher --no-refresh -g "$origin_url" openindiana.org; then
            if pkg refresh openindiana.org >/tmp/pkg-refresh.log 2>&1; then
                return 0
            else
                echo "==> failed refresh for ${origin_url} (publisher command set) - first lines of output:"
                sed -n '1,80p' /tmp/pkg-refresh.log
            fi
        fi
        if pkg set-publisher --no-refresh -O "$origin_url" openindiana.org; then
            if pkg refresh openindiana.org >/tmp/pkg-refresh.log 2>&1; then
                return 0
            else
                echo "==> failed refresh for ${origin_url} (publisher command -O) - first lines of output:"
                sed -n '1,80p' /tmp/pkg-refresh.log
            fi
        fi
    done

    echo "ERROR: failed to configure OpenIndiana package proxy for all known roots"
	return 1
}

grow_root_pool() {
    #
    # The OpenIndiana cloud image is much smaller than the virtual disk used by
    # the host-side VM wrapper.  IPS needs enough room for package payloads and
    # the FreeBASIC build tree, so ask illumos to expand the root pool into the
    # larger QEMU disk before installing dependencies.
    #
    local pool
    local vdev
    local short_vdev
    local disk

    pool="$(zpool list -H -o name 2>/dev/null | sed -n '1p')" || return 0
    [ -n "$pool" ] || return 0

    echo "==> expanding root pool if the VM disk is larger than the image"
    zpool set autoexpand=on "$pool" >/dev/null 2>&1 || true

    zpool status -P "$pool" 2>/dev/null |
        awk '
            $1 ~ /^\/dev\/dsk\// { print $1; next }
            $1 ~ /^c[0-9].*[sp][0-9]+$/ { print $1; next }
        ' |
        while read -r vdev; do
            [ -n "$vdev" ] || continue

            short_vdev="${vdev#/dev/dsk/}"
            disk="$short_vdev"
            case "$short_vdev" in
                *s[0-9]) disk="${short_vdev%s[0-9]}" ;;
                *p[0-9]) disk="${short_vdev%p[0-9]}" ;;
            esac

            if [ -n "$disk" ] && command -v format >/dev/null 2>&1; then
                printf 'partition\nexpand\nlabel\ny\nquit\nquit\n' |
                    format -e "$disk" >/tmp/freebasic-format-grow.log 2>&1 || true
            fi

            zpool online -e "$pool" "$vdev" >/tmp/freebasic-zpool-grow.log 2>&1 ||
                zpool online -e "$pool" "$short_vdev" >>/tmp/freebasic-zpool-grow.log 2>&1 ||
                true
        done

    zpool list "$pool" || true
    df -h / || true
}

ensure_build_swap() {
    #
    # OpenIndiana's cloud image can boot without enough swap for parallel GCC
    # builds.  tmpfs-backed system state also consumes swap accounting, so
    # starving swap can make unrelated services fail while cc1 is compiling.
    #
    local swapfile="${ILLUMOS_BUILD_SWAPFILE:-/var/tmp/freebasic-build.swap}"
    local swapsize="${ILLUMOS_BUILD_SWAP_SIZE:-16G}"
    local pool
    local swapzvol
    local swapdev
    local mb_count

    pool="$(zpool list -H -o name 2>/dev/null | sed -n '1p')" || pool=""
    if [ -n "$pool" ] && command -v zfs >/dev/null 2>&1; then
        swapzvol="${ILLUMOS_BUILD_SWAP_ZVOL:-$pool/freebasic-build-swap}"
        swapdev="/dev/zvol/dsk/$swapzvol"

        if swap -l 2>/dev/null |
            awk -v swapdev="$swapdev" '$1 == swapdev { found = 1 } END { exit found ? 0 : 1 }'; then
            echo "INFO: build swap already enabled: $swapdev"
            return 0
        fi

        echo "==> enabling build swap zvol: $swapzvol ($swapsize)"
        if ! zfs list -H "$swapzvol" >/dev/null 2>&1; then
            zfs create -V "$swapsize" "$swapzvol" || {
                echo "WARN: failed to create build swap zvol"
                swapzvol=""
            }
        fi

        if [ -n "$swapzvol" ]; then
            if [ ! -e "$swapdev" ] && command -v devfsadm >/dev/null 2>&1; then
                devfsadm >/dev/null 2>&1 || true
            fi

            if swap -a "$swapdev"; then
                swap -s || true
                return 0
            fi

            echo "WARN: failed to enable build swap zvol"
        fi
    fi

    if swap -l 2>/dev/null |
        awk -v swapfile="$swapfile" '$1 == swapfile { found = 1 } END { exit found ? 0 : 1 }'; then
        echo "INFO: build swap already enabled: $swapfile"
        return 0
    fi

    echo "==> enabling build swap: $swapfile ($swapsize)"
    rm -f "$swapfile"

    if command -v mkfile >/dev/null 2>&1; then
        mkfile "$swapsize" "$swapfile" || {
            echo "ERROR: failed to create swap file with mkfile"
            return 1
        }
    else
        case "$swapsize" in
            *[gG]) mb_count="$(( ${swapsize%[gG]} * 1024 ))" ;;
            *[mM]) mb_count="${swapsize%[mM]}" ;;
            *) mb_count=8192 ;;
        esac

        dd if=/dev/zero of="$swapfile" bs=1048576 count="$mb_count" || {
            echo "ERROR: failed to create swap file with dd"
            rm -f "$swapfile"
            return 1
        }
    fi

    chmod 1600 "$swapfile" || true
    swap -a "$swapfile" || {
        echo "ERROR: failed to enable build swap"
        rm -f "$swapfile"
        return 1
    }

    swap -s || true
}

raise_build_resource_limits() {
    #
    # Native compiler builds create many short-lived helper processes even when
    # GNU Make is limited to one job.  Some OpenIndiana cloud images leave root
    # in a conservative project; when that project limit is reached, fork()
    # fails with "out of processes" despite there being memory and swap left.
    #
    local project

    ulimit -n 65536 >/dev/null 2>&1 || true
    ulimit -u 16384 >/dev/null 2>&1 || true

    command -v projmod >/dev/null 2>&1 || return 0

    echo "==> raising build process limits"
    for project in user.root default system; do
        projects -l "$project" >/dev/null 2>&1 || continue

        projmod -r -K project.max-processes "$project" >/dev/null 2>&1 || true
        projmod -r -K project.max-lwps "$project" >/dev/null 2>&1 || true
        projmod -r -K task.max-lwps "$project" >/dev/null 2>&1 || true

        projmod -a -K "project.max-processes=(privileged,16384,deny)" "$project" >/dev/null 2>&1 || true
        projmod -a -K "project.max-lwps=(privileged,65536,deny)" "$project" >/dev/null 2>&1 || true
        projmod -a -K "task.max-lwps=(privileged,65536,deny)" "$project" >/dev/null 2>&1 || true
    done

    if command -v prctl >/dev/null 2>&1; then
        for project in user.root default system; do
            projects -l "$project" >/dev/null 2>&1 || continue

            prctl -n project.max-processes -r -v 16384 -t privileged -i project "$project" >/dev/null 2>&1 || true
            prctl -n project.max-lwps -r -v 65536 -t privileged -i project "$project" >/dev/null 2>&1 || true
            prctl -n task.max-lwps -r -v 65536 -t privileged -i project "$project" >/dev/null 2>&1 || true
        done
    fi
}

run_build_command() {
    if command -v newtask >/dev/null 2>&1 &&
        projects -l user.root >/dev/null 2>&1 &&
        newtask -p user.root true >/dev/null 2>&1; then
        newtask -p user.root "$@"
    else
        "$@"
    fi
}

start_build_monitor() {
    [ "${ILLUMOS_BUILD_MONITOR:-1}" != 0 ] || return 0
    [ -z "$BUILD_MONITOR_PID" ] || return 0

    (
        while :; do
            sleep 60 || exit 0
            echo "==> build monitor: $(date)"
            ps -ef |
                /usr/bin/grep -E 'gmake|/make|gcc|g[+][+]|cc1|collect2|fbc|as|ld|pkg-config|find|newtask' |
                /usr/bin/grep -E -v 'grep|freebasic-build-monitor' |
                sed -n '1,24p' || true
        done
    ) &
    BUILD_MONITOR_PID="$!"
}

stop_build_monitor() {
    [ -n "$BUILD_MONITOR_PID" ] || return 0
    kill "$BUILD_MONITOR_PID" >/dev/null 2>&1 || true
    wait "$BUILD_MONITOR_PID" >/dev/null 2>&1 || true
    BUILD_MONITOR_PID=""
}

trap 'stop_build_monitor' EXIT

##############################################################################
# Options
##############################################################################

DO_BUILD=1
DO_PACKAGE=1

while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) DO_BUILD=0 ;;
        --no-package) DO_PACKAGE=0 ;;
        -h|--help)
            echo "Usage: $0 [--no-build] [--no-package]"
            exit 0
            ;;
        *) echo "ERROR: unknown option $1"; exit 1 ;;
    esac
    shift
done

##############################################################################
# Locate project root
##############################################################################

SEARCH="$(pwd)"
ROOT=""

while :; do
    if [ -f "$SEARCH/mk/version.mk" ] && [ -f "$SEARCH/GNUmakefile" ]; then
        ROOT="$SEARCH"
        break
    fi
    [ "$SEARCH" = "/" ] && break
    SEARCH="$(dirname "$SEARCH")"
done

[ -n "$ROOT" ] || { echo "ERROR: not in FreeBASIC tree"; exit 1; }
cd "$ROOT"

##############################################################################
# Version extraction
##############################################################################

FBVERSION="$(awk -F':=' '/^FBVERSION/ {gsub(/[ \t]/,"",$2); print $2}' mk/version.mk)"
REV="$(awk -F':=' '/^REV/ {gsub(/[ \t]/,"",$2); print $2}' mk/version.mk)"

[ -n "$FBVERSION" ] || exit 1
[ -n "$REV" ] || exit 1

VERSION_FULL="${FBVERSION}.${REV}"
OSREL="$(uname -r)"
ARCH="$NATIVE_ISA"
TARGET_TRIPLET="${FBC_ARCH}-pc-illumos"
BOOTSTRAP_DIR="$ROOT/bootstrap/illumos-${FBC_ARCH}"

FMRI="pkg://local/lang/freebasic@${FBVERSION},${OSREL}-${REV}"

GCC_PKG_CANDIDATES=(
	developer/clang-20
	developer/clang-19
	developer/clang-18
	developer/clang-17
	developer/clang-16
	developer/clang-15
	developer/clang-14
	developer/clang-13
	developer/illumos-gcc
	developer/gcc-13
	developer/gcc-12
	developer/gcc-11
	developer/gcc-10
	developer/gcc-9
	developer/gcc-8
	developer/gcc-7
	developer/gcc-6
	developer/gcc-5
	developer/gcc-4
	developer/gcc-3
	developer/gcc-49
	developer/gcc-48
	developer/gcc-47
	developer/clang-21
	developer/gcc-14
	developer/gcc-15
	developer/gcc-16
	ooce/developer/gcc-16
	developer/gcc15
	ooce/developer/gcc15
	system/compiler/gcc-16
	system/compiler/gcc-15
	system/compiler/gcc-14
	system/compiler/gcc
	runtime/gcc
	developer/gcc14
	developer/gcc13
	ooce/developer/gcc13
	developer/gcc12
	developer/gcc11
	developer/gcc
	ooce/developer/gcc
	runtime/developer/gcc
	developer/gnu
	system/gnu
	runtime/clang-21
	runtime/clang-20
	runtime/clang-19
	runtime/clang-18
	runtime/clang-17
	runtime/clang-16
	runtime/clang-15
	runtime/clang-14
	runtime/clang-13
)
GCC_BIN_CANDIDATES=(
	gcc
	gcc-15
	gcc15
	gcc-14
	gcc14
	gcc-13
	gcc13
	gcc-12
	gcc12
	gcc-11
	gcc11
	clang
	clang-21
	clang-20
	clang-19
	clang-18
	clang-17
	clang-16
	clang-15
	clang-14
	clang-13
	clang-12
	cc
	/usr/gcc/15/bin/gcc
	/usr/gcc/14/bin/gcc
	/usr/gcc/13/bin/gcc
	/usr/gcc/12/bin/gcc
	/usr/gcc/11/bin/gcc
	/usr/local/bin/gcc
	/usr/local/gcc/bin/gcc
	/opt/ooce/bin/gcc
)
if [ -z "${GCC_PKG+x}" ]; then
	GCC_PKG=""
fi

resolve_gcc_package() {
	local pkg
	local candidate_full

	for pkg in "${GCC_PKG_CANDIDATES[@]}"; do
		for candidate_full in "$pkg" "pkg://openindiana.org/$pkg"; do
			if pkg list -H "$candidate_full" >/dev/null 2>&1; then
				GCC_PKG="$pkg"
				return 0
			fi
		done
	done
	return 1
}

compiler_binary_present() {
	local candidate

	for candidate in "${GCC_BIN_CANDIDATES[@]}"; do
		[ -z "$candidate" ] && continue
		command -v "$candidate" >/dev/null 2>&1 && return 0
	done

	return 1
}

compiler_can_link() {
	local compiler="$1"
	local test_base="/tmp/freebasic-cc-test.$$"
	local log_file="/tmp/freebasic-cc-test.log"

	cat > "${test_base}.c" <<'EOF'
int main(void)
{
	return 0;
}
EOF

	if "$compiler" "${test_base}.c" -o "$test_base" >"$log_file" 2>&1; then
		rm -f "${test_base}.c" "$test_base" "$log_file"
		return 0
	fi

	echo "INFO: compiler candidate failed link test: $compiler" >&2
	sed -n '1,40p' "$log_file" >&2 || true
	rm -f "${test_base}.c" "$test_base" "$log_file"
	return 1
}

cxx_can_link() {
	local compiler="$1"
	local test_base="/tmp/freebasic-cxx-test.$$"
	local log_file="/tmp/freebasic-cxx-test.log"

	cat > "${test_base}.cc" <<'EOF'
int main()
{
	return 0;
}
EOF

	if "$compiler" "${test_base}.cc" -o "$test_base" >"$log_file" 2>&1; then
		rm -f "${test_base}.cc" "$test_base" "$log_file"
		return 0
	fi

	echo "INFO: C++ compiler candidate failed link test: $compiler" >&2
	sed -n '1,40p' "$log_file" >&2 || true
	rm -f "${test_base}.cc" "$test_base" "$log_file"
	return 1
}

resolve_gcc_binary() {
	local candidate
	local found=""
	for candidate in "${GCC_BIN_CANDIDATES[@]}"; do
		[ -z "$candidate" ] && continue
		if command -v "$candidate" >/dev/null 2>&1 &&
			compiler_can_link "$candidate"; then
			found="$candidate"
			break
		fi
	done
	if [ -z "$found" ]; then
		return 1
	fi

	CC="$found"
	export CC
	return 0
}

resolve_gxx_for_gcc() {
	local gcc_binary="$1"
	local gxx_candidate
	local compiler_base
	local compiler_dir
	local gxx_candidates=()

	compiler_base="${gcc_binary##*/}"
	compiler_dir="${gcc_binary%/*}"
	if [ "$compiler_dir" = "$gcc_binary" ]; then
		compiler_dir=""
	fi

	case "$compiler_base" in
		gcc)
			[ -n "$compiler_dir" ] && gxx_candidates+=( "$compiler_dir/g++" )
			gxx_candidates+=( g++ c++ )
			;;
		gcc-*)
			[ -n "$compiler_dir" ] && gxx_candidates+=( "$compiler_dir/g++-${compiler_base#gcc-}" )
			gxx_candidates+=( "g++-${compiler_base#gcc-}" g++ c++ )
			;;
		gcc[0-9]*)
			[ -n "$compiler_dir" ] && gxx_candidates+=( "$compiler_dir/g++${compiler_base#gcc}" )
			gxx_candidates+=( "g++${compiler_base#gcc}" g++ c++ )
			;;
		clang)
			[ -n "$compiler_dir" ] && gxx_candidates+=( "$compiler_dir/clang++" )
			gxx_candidates+=( clang++ c++ )
			;;
		clang-*)
			[ -n "$compiler_dir" ] && gxx_candidates+=( "$compiler_dir/clang++-${compiler_base#clang-}" )
			gxx_candidates+=( "clang++-${compiler_base#clang-}" clang++ c++ )
			;;
		cc)
			gxx_candidates+=( c++ g++ clang++ )
			;;
	esac

	for gxx_candidate in "${gxx_candidates[@]}"; do
		[ -n "$gxx_candidate" ] || continue
		if command -v "$gxx_candidate" >/dev/null 2>&1 &&
			cxx_can_link "$gxx_candidate"; then
			CXX="$gxx_candidate"
			return 0
		fi
	done

	return 1
}

resolve_gmake_binary() {
	local make_candidate

	# The name alone is not enough here.  FreeBASIC needs GNU Make features,
	# whether the installed command is called gmake or make.
	for make_candidate in \
		"$FBC_GMAKE_BOOTSTRAP_PREFIX/bin/make" \
		"$FBC_GMAKE_BOOTSTRAP_PREFIX/bin/gmake" \
        gmake make /usr/bin/gmake /usr/bin/make; do
        if command -v "$make_candidate" >/dev/null 2>&1 &&
            "$make_candidate" --version 2>/dev/null | grep -qi 'GNU Make'; then
            GMAKE="$make_candidate"
            return 0
        fi
    done

    return 1
}

fetch_gnu_make_source() {
	local destination="$1"
	local url

	shift
	for url in "$@"; do
		[ -n "$url" ] || continue

		echo "INFO: fetching GNU Make source: $url"
		rm -f "$destination"
		if command -v curl >/dev/null 2>&1; then
			curl -fL --retry 3 --connect-timeout 20 --max-time 600 \
				-o "$destination" "$url" || true
		elif command -v wget >/dev/null 2>&1; then
			wget -O "$destination" "$url" || true
		else
			echo "WARN: neither curl nor wget is available for GNU Make bootstrap"
			return 1
		fi

		if [ -s "$destination" ] && tar -tzf "$destination" >/dev/null 2>&1; then
			return 0
		fi
	done

	rm -f "$destination"
	return 1
}

crt_object_available() {
	[ -f /usr/lib/crt1.o ] ||
		[ -f /usr/lib/amd64/crt1.o ] ||
		[ -f "/usr/lib/$NATIVE_ISA/crt1.o" ]
}

run_pkg_owner_search() {
	local query="$1"
	local output_file="/tmp/freebasic-pkg-owner-search.$$"
	local pid
	local waited=0

	rm -f "$output_file"
	pkg search -r -H -o pkg.name "$query" >"$output_file" 2>/dev/null &
	pid="$!"
	while kill -0 "$pid" >/dev/null 2>&1; do
		if [ "$waited" -ge 60 ]; then
			kill "$pid" >/dev/null 2>&1 || true
			wait "$pid" >/dev/null 2>&1 || true
			rm -f "$output_file"
			return 1
		fi
		sleep 1
		waited=$((waited + 1))
	done
	wait "$pid" >/dev/null 2>&1 || {
		rm -f "$output_file"
		return 1
	}
	cat "$output_file"
	rm -f "$output_file"
	return 0
}

normalize_pkg_name() {
	local package_name="$1"

	package_name="${package_name#pkg://openindiana.org/}"
	package_name="${package_name#pkg:/}"
	package_name="${package_name#openindiana.org/}"
	package_name="${package_name%%@*}"
	printf '%s\n' "$package_name"
}

install_crt_objects_from_archive() {
	local archive="/tmp/freebasic-crt-objects.tar.gz"

	[ -n "${FBC_CRT_OBJECTS_URL:-}" ] || return 1
	echo "INFO: installing headers and runtime/compiler support from host archive"
	rm -f "$archive"
	if command -v curl >/dev/null 2>&1; then
		curl -fL --retry 20 --connect-timeout 20 --max-time 600 \
			-o "$archive" "$FBC_CRT_OBJECTS_URL" || return 1
	elif command -v wget >/dev/null 2>&1; then
		wget -O "$archive" "$FBC_CRT_OBJECTS_URL" || return 1
	else
		return 1
	fi
	tar -xzf "$archive" -C / || return 1
	for compiler_tool in \
		/usr/gcc/13/bin/gcc \
		/usr/gcc/13/bin/g++ \
		/usr/gcc/13/bin/cpp \
		/usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/cc1 \
		/usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/cc1plus \
		/usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/collect2 \
		/usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/g++-mapper-server \
		/usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/lto-wrapper \
		/usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/lto1 \
		/usr/gnu/bin/ar \
		/usr/gnu/bin/as \
		/usr/gnu/bin/objcopy \
		/usr/gnu/bin/strip; do
		[ -f "$compiler_tool" ] && chmod 0555 "$compiler_tool"
	done
	rm -f "$archive"
	crt_object_available
}

fetch_static_manifest() {
	local package_name="$1"
	local manifest="$2"

	[ -n "${ILLUMOS_PKG_PROXY:-}" ] || return 1
	command -v python3 >/dev/null 2>&1 || return 1

	python3 - "$ILLUMOS_PKG_PROXY" "$package_name" "$manifest" <<'PY'
import json
import sys
from urllib.parse import quote
from urllib.request import urlopen

base = sys.argv[1].rstrip("/")
package_name = sys.argv[2]
manifest_path = sys.argv[3]

catalog_url = base + "/pub/openindiana/hipster/publisher/openindiana.org/catalog/1/catalog.base.C"
with urlopen(catalog_url, timeout=120) as response:
    catalog = json.loads(response.read().decode("utf-8", "replace"))

entries = catalog.get("openindiana.org", {}).get(package_name, [])
if not entries:
    raise SystemExit(1)

version = entries[-1].get("version", "")
if not version:
    raise SystemExit(1)

def quote_static(text):
    return quote(quote(text, safe=""), safe="")

manifest_url = (
    base +
    "/pub/openindiana/hipster/publisher/openindiana.org/pkg/" +
    quote_static(package_name) +
    "/" +
    quote_static(version)
)
with urlopen(manifest_url, timeout=120) as response:
    manifest = response.read()

with open(manifest_path, "wb") as handle:
    handle.write(manifest)
PY
}

install_crt_objects_from_manifest() {
	local manifest="/tmp/freebasic-c-runtime.manifest"
	local objects="/tmp/freebasic-c-runtime.objects"
	local package_name
	local hash
	local path
	local url
	local destination

	for package_name in system/library/c-runtime system/header SUNWcs; do
		rm -f "$manifest" "$objects"
		if ! pkg contents -r -m "$package_name" >"$manifest" 2>/dev/null &&
			! fetch_static_manifest "$package_name" "$manifest"; then
			continue
		fi

		awk '
			$1 == "file" {
				hash = ""
				path = ""
				for (i = 2; i <= NF; i++) {
					if ($i ~ /^[0-9a-f]{40}$/)
						hash = $i
					if ($i ~ /^path=/)
						path = substr($i, 6)
				}
				if (hash != "" && path ~ /^usr\/lib\/(amd64\/)?crt(1|i|n)\.o$/)
					print hash, path
			}
		' "$manifest" >"$objects"

		[ -s "$objects" ] || continue

		echo "INFO: installing C runtime startup objects from $package_name manifest"
		while read -r hash path; do
			[ -n "$hash" ] && [ -n "$path" ] || continue
			destination="/$path"
			if [ -n "${ILLUMOS_PKG_PROXY:-}" ]; then
				url="${ILLUMOS_PKG_PROXY}/pub/openindiana/hipster/publisher/openindiana.org/file/0/$hash"
			else
				url="http://pkg.openindiana.org/hipster/file/0/$hash"
			fi
			mkdir -p "$(dirname "$destination")"
			if command -v curl >/dev/null 2>&1; then
				curl -fL --retry 20 --connect-timeout 20 --max-time 600 \
					-o "$destination" "$url" || return 1
			elif command -v wget >/dev/null 2>&1; then
				wget -O "$destination" "$url" || return 1
			else
				return 1
			fi
			chmod 0644 "$destination" || true
		done <"$objects"

		crt_object_available && return 0
	done

	return 1
}

relax_incorporation_locks() {
	echo "==> relaxing package incorporation version locks for build dependencies"
	pkg change-facet 'facet.version-lock.*=false' \
		>/tmp/freebasic-pkg-facets.log 2>&1 ||
	pkg change-facet \
		facet.version-lock.system/header=false \
		facet.version-lock.system/library/c-runtime=false \
		facet.version-lock.file/gnu-coreutils=false \
		facet.version-lock.developer/build/gnu-make=false \
		>>/tmp/freebasic-pkg-facets.log 2>&1 || {
		echo "WARN: failed to relax incorporation version locks"
		sed -n '1,80p' /tmp/freebasic-pkg-facets.log || true
	}
	pkg facet -a 2>/dev/null | grep 'facet.version-lock' | sed -n '1,40p' || true
	printf 'y\n' | pkg update --accept \
		consolidation/osnet/osnet-incorporation \
		consolidation/userland/userland-incorporation \
		>/tmp/freebasic-pkg-incorporation-update.log 2>&1 || {
		echo "WARN: failed to update package incorporations"
		sed -n '1,120p' /tmp/freebasic-pkg-incorporation-update.log || true
	}
}

ensure_crt_objects() {
	local pkg_item
	local query
	local owner

	if crt_object_available; then
		return 0
	fi

	echo "==> installing C runtime startup objects"
	for pkg_item in system/library system/library/c-runtime SUNWcs; do
		install_pkg_with_fallback "$pkg_item" || true
		crt_object_available && return 0
	done

	install_crt_objects_from_archive && return 0
	install_crt_objects_from_manifest && return 0

	if [ "${ILLUMOS_ALLOW_IMAGE_UPDATE:-0}" != "1" ]; then
		echo "WARN: skipping full image incorporation update; set ILLUMOS_ALLOW_IMAGE_UPDATE=1 to allow it"
		return 1
	fi

	relax_incorporation_locks
	for pkg_item in system/header system/library/c-runtime; do
		install_pkg_with_fallback "$pkg_item" || true
		crt_object_available && return 0
	done

	echo "==> searching package owner for crt1.o"
	for query in basename:crt1.o path:usr/lib/crt1.o path:usr/lib/amd64/crt1.o; do
		while IFS= read -r owner; do
			owner="$(normalize_pkg_name "$owner")"
			[ -n "$owner" ] || continue
			install_pkg_with_fallback "$owner" || true
			crt_object_available && return 0
		done <<EOF
$(run_pkg_owner_search "$query" || true)
EOF
	done

	if ! crt_object_available; then
		echo "WARN: crt1.o is still missing after startup-object package probes"
		return 1
	fi
}

bootstrap_gnu_make() {
	local version="$FBC_GMAKE_BOOTSTRAP_VERSION"
	local prefix="$FBC_GMAKE_BOOTSTRAP_PREFIX"
	local workdir="/var/tmp/freebasic-gmake-bootstrap.$$"
	local tarball="/var/tmp/make-${version}.tar.gz"
	local bootstrap_ldflags
	local srcdir

	if resolve_gmake_binary; then
		return 0
	fi

	echo "==> bootstrapping GNU Make ${version}"
	if ! fetch_gnu_make_source "$tarball" \
		"${FBC_GMAKE_TARBALL_URL:-}" \
		"http://ftp.gnu.org/gnu/make/make-${version}.tar.gz" \
		"http://ftpmirror.gnu.org/make/make-${version}.tar.gz" \
		"https://ftp.gnu.org/gnu/make/make-${version}.tar.gz"; then
		echo "ERROR: failed to fetch GNU Make source"
		return 1
	fi

	rm -rf "$workdir"
	mkdir -p "$workdir" "$prefix"
	if ! tar -xzf "$tarball" -C "$workdir"; then
		echo "ERROR: failed to extract GNU Make source"
		rm -rf "$workdir"
		return 1
	fi

	srcdir="$workdir/make-$version"
	if [ ! -d "$srcdir" ]; then
		srcdir="$(find "$workdir" -maxdepth 1 -type d -name 'make-*' | sed -n '1p')"
	fi
	if [ -z "$srcdir" ] || [ ! -d "$srcdir" ]; then
		echo "ERROR: extracted GNU Make source directory was not found"
		rm -rf "$workdir"
		return 1
	fi

	bootstrap_ldflags="${LDFLAGS:-} -L/usr/lib/amd64 -L/usr/lib -Wl,-R,/usr/lib/amd64 -Wl,-R,/usr/lib"
	if ! (
		cd "$srcdir" || exit 1
		./configure --prefix="$prefix" CC="$CC" LDFLAGS="$bootstrap_ldflags" || {
			echo "ERROR: GNU Make configure failed; config.log tail follows"
			grep -n -i -E 'conftest|ld.so|fatal|cannot run|error:' config.log | tail -n 120 || true
			tail -n 180 config.log || true
			exit 1
		}
		if [ ! -x ./build.sh ]; then
			echo "ERROR: GNU Make source does not contain executable build.sh"
			exit 1
		fi
		./build.sh || exit 1
		if [ ! -x ./make ] || ! ./make --version 2>/dev/null | grep -qi 'GNU Make'; then
			echo "ERROR: bootstrapped GNU Make binary did not pass version check"
			exit 1
		fi
		./make install || {
			mkdir -p "$prefix/bin"
			cp ./make "$prefix/bin/make"
		}
	); then
		echo "ERROR: failed to bootstrap GNU Make"
		rm -rf "$workdir"
		return 1
	fi

	rm -rf "$workdir"
	PATH="$prefix/bin:$PATH"
	export PATH
	resolve_gmake_binary
}

ensure_gcc_dependency() {
	local c
	local install_output

	if [ -n "$GCC_PKG" ] && pkg list -H "$GCC_PKG" >/dev/null 2>&1; then
		return 0
	fi
	if compiler_binary_present; then
		resolve_gcc_package || true
		echo "INFO: installed compiler binary found; skipping compiler package probes" >&2
		return 0
	fi
	for c in "${GCC_PKG_CANDIDATES[@]}"; do
            if printf 'y\n' | pkg install --accept "$c" >"/tmp/freebasic-gcc-install.out" 2>&1; then
                GCC_PKG="$c"
                return 0
            fi
            if grep -q 'No updates necessary for this image' /tmp/freebasic-gcc-install.out 2>/dev/null; then
                if compiler_binary_present; then
                    GCC_PKG="$c"
                    return 0
                fi
            fi
            install_output="$(sed -n '1,40p' /tmp/freebasic-gcc-install.out 2>/dev/null | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g')"
            echo "INFO: compiler dependency candidate failed: $c" >&2
            echo "  $install_output" >&2
        done
	return 1
}

##############################################################################
# Paths
##############################################################################

BUILDROOT="$ROOT/.build-illumos"
STAGE="$BUILDROOT/stage"
OUT="$ROOT/out"
OUT_ILLUMOS="$OUT/illumos/${OSREL}/${ARCH}"
REPO="$OUT_ILLUMOS/repo"
MANIFEST="$BUILDROOT/manifest.p5m"
PREFIX="/usr/local"

mkdir -p "$BUILDROOT" "$OUT_ILLUMOS"

##############################################################################
# Dependencies
##############################################################################

install_pkg_with_fallback() {
	local entry
	local candidate
	local candidate_try
	local output
	local rc=1
	local namespace="openindiana.org"

	entry="$1"
	IFS='|' read -r -a candidates <<< "$entry"

	for candidate in "${candidates[@]}"; do
		[ -z "$candidate" ] && continue
		if pkg list -H "$candidate" >/dev/null 2>&1 ||
			pkg list -H "pkg://$namespace/$candidate" >/dev/null 2>&1; then
			echo "INFO: dependency package already installed: $candidate" >&2
			return 0
		fi
		for candidate_try in \
			"$candidate" \
			"pkg://$namespace/$candidate"; do
			[ -z "$candidate_try" ] && continue
                if printf 'y\n' | pkg install --accept "$candidate_try" >"/tmp/freebasic-pkg-install.out" 2>&1; then
                    echo "INFO: installed dependency package: $candidate_try" >&2
                    rc=0
                    return 0
                fi
				if grep -q 'No updates necessary for this image' /tmp/freebasic-pkg-install.out 2>/dev/null; then
					echo "INFO: dependency package already satisfied: $candidate_try" >&2
					rc=0
					return 0
				fi
				output="$(sed -n '1,40p' /tmp/freebasic-pkg-install.out 2>/dev/null | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g')"
                echo "INFO: dependency candidate failed: $candidate_try" >&2
                echo "  $output" >&2
		done
	done

	return "$rc"
}

manifest_dependency_fmri() {
	local entry
	local candidate

	entry="$1"
	IFS='|' read -r -a candidates <<< "$entry"

	for candidate in "${candidates[@]}"; do
		[ -z "$candidate" ] && continue
		if pkg list -H "$candidate" >/dev/null 2>&1 ||
			pkg list -H "pkg://openindiana.org/$candidate" >/dev/null 2>&1; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	for candidate in "${candidates[@]}"; do
		[ -z "$candidate" ] && continue
		printf '%s\n' "$candidate"
		return 0
	done

	return 1
}

run_with_timeout() {
	local seconds
	local timeout_cmd

	seconds="$1"
	shift

	for timeout_cmd in timeout gtimeout; do
		if command -v "$timeout_cmd" >/dev/null 2>&1; then
			"$timeout_cmd" "$seconds" "$@"
			return $?
		fi
	done

	python3 - "$seconds" "$@" <<'PY'
import os
import signal
import subprocess
import sys


def kill_process_group(proc, sig):
    try:
        os.killpg(proc.pid, sig)
    except OSError:
        try:
            proc.send_signal(sig)
        except OSError:
            pass


if len(sys.argv) < 3:
    sys.exit(125)

try:
    timeout = float(sys.argv[1])
except ValueError:
    sys.exit(125)

cmd = sys.argv[2:]

try:
    proc = subprocess.Popen(cmd, start_new_session=True)
except OSError as exc:
    print("ERROR: failed to run {}: {}".format(cmd[0], exc), file=sys.stderr)
    sys.exit(127)

try:
    sys.exit(proc.wait(timeout=timeout))
except subprocess.TimeoutExpired:
    kill_process_group(proc, signal.SIGTERM)
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        kill_process_group(proc, signal.SIGKILL)
        proc.wait()
    print("ERROR: command timed out after {} seconds: {}".format(
        sys.argv[1], " ".join(cmd)), file=sys.stderr)
    sys.exit(124)
PY
}

prefer_gnu_userland_tools() {
	#
	# OpenIndiana images can expose the historical illumos utilities before
	# the GNU tools.  Prefer an existing GNU coreutils installation, but do not
	# make it a build dependency.  The FreeBASIC build has portable fallbacks,
	# and older images may be unable to satisfy a current coreutils package
	# under their incorporation locks.
	#
	if [ -x /usr/gnu/bin/sort ] && /usr/gnu/bin/sort -V /dev/null >/dev/null 2>&1; then
		PATH="/usr/gnu/bin:$PATH"
		export PATH
	fi
}

# GNU Make is resolved separately after package installation.  Some OpenIndiana
# images cannot install its Guile dependency under their incorporation locks,
# so bootstrap_gnu_make() provides the reliable source-build fallback.
PKGS_BUILD_REQUIRED=(
    "developer/build/pkg-config|pkgconfig"
    library/ncurses
    library/libffi
    "x11/library/libx11|ooce/x11/library/libx11"
    "x11/library/libxau|ooce/x11/library/libxau"
    "x11/library/libxext|ooce/x11/library/libxext"
    "x11/library/libxrender|ooce/x11/library/libxrender"
    "x11/library/libxrandr|ooce/x11/library/libxrandr"
    "x11/library/libxi|ooce/x11/library/libxi"
    "x11/library/libxcb|ooce/x11/library/libxcb"
)

PKGS_RUNTIME=(
    library/ncurses
    library/libffi
    "x11/library/libx11|ooce/x11/library/libx11"
    "x11/library/libxau|ooce/x11/library/libxau"
    "x11/library/libxext|ooce/x11/library/libxext"
    "x11/library/libxrender|ooce/x11/library/libxrender"
    "x11/library/libxrandr|ooce/x11/library/libxrandr"
    "x11/library/libxi|ooce/x11/library/libxi"
    "x11/library/libxcb|ooce/x11/library/libxcb"
)

##############################################################################
# Install dependencies (idempotent)
##############################################################################

prepare_tls_policy
grow_root_pool
configure_pkg_proxy
ensure_build_swap
raise_build_resource_limits

echo "==> sampling package namespace for required prefixes"
pkg list -Ha pkg://openindiana.org/developer/build/pkg-config 2>/dev/null | sed -n '1,40p' || true
pkg list -Ha pkg://openindiana.org/x11/library/libx11 2>/dev/null | sed -n '1,40p' || true
pkg list -Ha pkg://openindiana.org/library/ncurses 2>/dev/null | sed -n '1,40p' || true

echo "==> installing dependencies"
pkg refresh || true
prefer_gnu_userland_tools || true
install_crt_objects_from_archive || true
echo "==> searching compiler packages"
echo "  compiler diagnostics disabled to avoid pkg tool command compatibility issues"

ensure_gcc_dependency || true
if [ -n "$GCC_PKG" ]; then
	PKGS_BUILD_REQUIRED=("$GCC_PKG" "${PKGS_BUILD_REQUIRED[@]}")
	PKGS_RUNTIME=("$GCC_PKG" "${PKGS_RUNTIME[@]}")
fi

for pkg_item in "${PKGS_BUILD_REQUIRED[@]}"; do
    if ! install_pkg_with_fallback "$pkg_item"; then
        echo "ERROR: failed to install required package candidate: $pkg_item"
        exit 1
    fi
done

PKGS_BUILD_OPTIONAL=()
[ -r /usr/include/stdio.h ] || PKGS_BUILD_OPTIONAL+=(system/header)
command -v ld >/dev/null 2>&1 || PKGS_BUILD_OPTIONAL+=(developer/linker)

for pkg_item in "${PKGS_BUILD_OPTIONAL[@]}"; do
    install_pkg_with_fallback "$pkg_item" ||
        echo "WARN: optional build package not installed: $pkg_item"
done
ensure_crt_objects || true

if ! resolve_gcc_binary; then
	echo "INFO: gcc not on PATH yet, attempting fallback install"
	ensure_gcc_dependency || true
	resolve_gcc_binary || true
fi

resolve_gcc_binary || {
	if command -v cc >/dev/null 2>&1 && compiler_can_link cc; then
		CC="cc"
		export CC
		echo "WARN: gcc not found; using CC=cc fallback"
	else
		echo "ERROR: no C compiler found after dependency install"
		exit 1
	fi
}
resolve_gxx_for_gcc "$CC" || {
	if [ "$CC" = "cc" ] && command -v c++ >/dev/null 2>&1 && cxx_can_link c++; then
		CXX="c++"
		export CXX
	else
		echo "ERROR: matching C++ compiler not found for $CC"
		exit 1
	fi
}
export CXX
bootstrap_gnu_make || { echo "ERROR: GNU make not found after dependency install"; exit 1; }
TOOL_WRAPPER_DIR="/var/tmp/freebasic-tool-wrappers"
rm -rf "$TOOL_WRAPPER_DIR"
mkdir -p "$TOOL_WRAPPER_DIR"
cat > "$TOOL_WRAPPER_DIR/xargs" <<'EOF'
#!/usr/bin/env bash
no_run_if_empty=0
filtered=()

for arg in "$@"; do
	case "$arg" in
		-r|--no-run-if-empty)
			no_run_if_empty=1
			;;
		*)
			filtered+=("$arg")
			;;
	esac
done

tmp="${TMPDIR:-/tmp}/freebasic-xargs.$$"
trap 'rm -f "$tmp"' EXIT
cat > "$tmp"

if [ "$no_run_if_empty" -eq 1 ] && [ ! -s "$tmp" ]; then
	exit 0
fi

exec /usr/bin/xargs "${filtered[@]}" < "$tmp"
EOF
cat > "$TOOL_WRAPPER_DIR/as" <<'EOF'
#!/usr/bin/env bash
compiler="${CC:-clang}"
real_as="${FBC_REAL_AS:-}"
filtered=()

for arg in "$@"; do
	case "$arg" in
		--32|--64|--strip-local-absolute)
			;;
		*)
			filtered+=("$arg")
			;;
	esac
done

if [ -z "$real_as" ]; then
	for candidate in /usr/gnu/bin/as /usr/bin/as; do
		if [ -x "$candidate" ]; then
			real_as="$candidate"
			break
		fi
	done
fi

if [ -n "$real_as" ] && [ -x "$real_as" ]; then
	exec "$real_as" "${filtered[@]}"
fi

case "$(basename "$compiler")" in
	clang|clang-*)
		exec "$compiler" -fintegrated-as -c -x assembler-with-cpp "${filtered[@]}"
		;;
esac

echo "ERROR: no real assembler found for GCC-style assembly" >&2
exit 1
EOF
chmod +x "$TOOL_WRAPPER_DIR/xargs" "$TOOL_WRAPPER_DIR/as"
PATH="$TOOL_WRAPPER_DIR:$PATH"
export PATH

patch_clang_bootstrap_sources() {
	case "$(basename "$CC")" in
		clang|clang-*)
			;;
		*)
			return 0
			;;
	esac

	[ -d "$BOOTSTRAP_DIR" ] || return 0
	echo "==> patching bootstrap C for clang indirect-goto support"
	python3 - "$BOOTSTRAP_DIR" <<'PY'
import pathlib
import sys

bootstrap_dir = pathlib.Path(sys.argv[1])
marker = "_llvmbug18658"
insert = "\t_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;\n"

for path in sorted(bootstrap_dir.glob("*.c")):
    text = path.read_text()
    if "goto *" not in text or marker in text:
        continue

    lines = text.splitlines(True)
    output = []
    previous_nonblank = ""
    changed = False

    for index, line in enumerate(lines):
        output.append(line)
        if line.strip() == "{" and previous_nonblank.rstrip().endswith(")"):
            next_line = lines[index + 1] if index + 1 < len(lines) else ""
            if marker not in next_line:
                output.append(insert)
                changed = True
        if line.strip():
            previous_nonblank = line

    if changed:
        path.write_text("".join(output))
        print("patched", path)
PY
}

if ! command -v python3 >/dev/null 2>&1; then
	for p in runtime/python-39 runtime/python-311 runtime/python-312; do
			printf 'y\n' | pkg install --accept "$p" >/dev/null 2>&1 || true
			command -v python3 >/dev/null 2>&1 && break
	done
fi
command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 not found after dependency install"; exit 1; }
patch_clang_bootstrap_sources
echo "INFO: using compiler: CC=$CC CXX=$CXX"
echo "INFO: using make: $GMAKE"

##############################################################################
# Build
##############################################################################

if [ "$DO_BUILD" -eq 1 ]; then

    echo "==> build jobs: $BUILD_JOBS"
    echo "==> target triplet: $TARGET_TRIPLET"
    echo "==> bootstrap CFLAGS: $BOOTSTRAP_CFLAGS"
    echo "==> source tree was freshly staged; skipping clean"
    run_build_command /bin/sh -c 'printf "INFO: build task active"; id -p 2>/dev/null | sed "s/^/ /"; printf "\n"'
    start_build_monitor

    if [ ! -d "$BOOTSTRAP_DIR" ] ||
        ! find "$BOOTSTRAP_DIR" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print | sed -n '1p' | grep -q .; then
        echo "==> bootstrap-seed-peer"
        run_build_command "$GMAKE" -f GNUmakefile \
            -j"$BUILD_JOBS" \
            bootstrap-seed-peer \
            CC=$CC \
            CFLAGS="$BOOTSTRAP_CFLAGS" \
            HAVE_PREREQS_MK= \
            HOST_OS=illumos \
            HOST_ARCH="$FBC_ARCH" \
            HOST_TRIPLET="$TARGET_TRIPLET" \
            TARGET_OS=illumos \
            TARGET_TRIPLET="$TARGET_TRIPLET"
    else
        echo "==> bootstrap-minimal"
        run_build_command "$GMAKE" -f GNUmakefile \
            -j"$BUILD_JOBS" \
            bootstrap-minimal \
            CC=$CC \
            CFLAGS="$BOOTSTRAP_CFLAGS" \
            HAVE_PREREQS_MK= \
            HOST_OS=illumos \
            HOST_ARCH="$FBC_ARCH" \
            HOST_TRIPLET="$TARGET_TRIPLET" \
            TARGET_OS=illumos \
            TARGET_TRIPLET="$TARGET_TRIPLET"
    fi

    [ -x "$ROOT/bootstrap/fbc" ] || exit 1

    echo "==> full build"
    run_build_command "$GMAKE" -f GNUmakefile \
        -j"$BUILD_JOBS" \
        all \
        FBC="$ROOT/bootstrap/fbc" \
        CC=$CC \
        HAVE_PREREQS_MK= \
        HOST_OS=illumos \
        HOST_ARCH="$FBC_ARCH" \
        HOST_TRIPLET="$TARGET_TRIPLET" \
        TARGET_OS=illumos \
        TARGET_TRIPLET="$TARGET_TRIPLET"

    echo "==> staging install"
    rm -rf "$STAGE"
    mkdir -p "$STAGE"

    run_build_command "$GMAKE" -f GNUmakefile \
        -j"$BUILD_JOBS" \
        install \
        DESTDIR="$STAGE" \
        prefix="$PREFIX" \
        FBC="$ROOT/bootstrap/fbc" \
        HAVE_PREREQS_MK= \
        HOST_OS=illumos \
        HOST_ARCH="$FBC_ARCH" \
        HOST_TRIPLET="$TARGET_TRIPLET" \
        TARGET_OS=illumos \
        TARGET_TRIPLET="$TARGET_TRIPLET"

    if [ -d "$ROOT/examples" ]; then
        echo "==> staging examples"
        rm -rf "$STAGE$PREFIX/share/freebasic/examples"
        mkdir -p "$STAGE$PREFIX/share/freebasic"
        cp -R "$ROOT/examples" "$STAGE$PREFIX/share/freebasic/examples"
    else
        echo "==> examples not present; skipping example staging"
    fi

    stop_build_monitor

else
    echo "==> --no-build specified"
fi

##############################################################################
# Packaging + install + test
##############################################################################

if [ "$DO_PACKAGE" -eq 1 ]; then

    [ -x "$STAGE$PREFIX/bin/fbc" ] || { echo "ERROR: staged fbc missing"; exit 1; }

    echo "==> generating manifest"
    pkgsend generate "$STAGE" \
        | grep -vE ' path=(usr|usr/local)$' \
        > "$MANIFEST"

    echo "==> injecting metadata + deps"
    {
        echo "set name=pkg.fmri value=${FMRI}"
        echo "set name=pkg.summary value=\"FreeBASIC compiler\""
        echo "set name=pkg.description value=\"FreeBASIC compiler for illumos\""

        for d in "${PKGS_RUNTIME[@]}"; do
            dep_fmri="$(manifest_dependency_fmri "$d")" || {
                echo "ERROR: empty package dependency candidate: $d" >&2
                exit 1
            }
            echo "depend type=require fmri=$dep_fmri"
        done

        cat "$MANIFEST"
    } > "${MANIFEST}.final"

    mv "${MANIFEST}.final" "$MANIFEST"

    echo "==> preparing repo"
    if ! pkgrepo info -s "$REPO" >/dev/null 2>&1; then
        rm -rf "$REPO"
        pkgrepo create "$REPO"
    fi

    pkgrepo -s "$REPO" add-publisher local >/dev/null 2>&1 || true

    echo "==> publishing"
    pkgsend -s "file://$REPO" publish -d "$STAGE" "$MANIFEST"

    echo "==> package dependencies"
    pkg contents -r -g "file://$REPO" -t depend "$FMRI"

    echo "==> installing from repo"
    pkg set-publisher -g "file://$REPO" local >/dev/null 2>&1 || true
    pkg refresh >/dev/null 2>&1 || true
    printf 'y\n' | pkg install "$FMRI" || { echo "ERROR: install failed"; exit 1; }

    ##############################################################################
    # Tests
    ##############################################################################

    echo "==> writing smoke tests"

    rm -rf /tmp/freebasic-illumos-smoke
    mkdir -p /tmp/freebasic-illumos-smoke

    cat > /tmp/freebasic-illumos-smoke/console.bas <<'EOF'
print "Hello world"
EOF

    cat > /tmp/freebasic-illumos-smoke/gfx-truecolor.bas <<'EOF'
#include once "fbgfx.bi"

sub expect_rgb( byval x as integer, byval y as integer, byval expected as uinteger, byref label as string )
    dim as uinteger actual = cuint( point( x, y ) )

    if( actual <> expected ) then
        print "gfx truecolor mismatch: "; label; " actual=&h"; hex( actual, 8 ); " expected=&h"; hex( expected, 8 )
        end 1
    end if
end sub

dim as integer has_extra_page = 1

if( screenres( 64, 64, 32, 2 ) <> 0 ) then
    has_extra_page = 0
    if( screenres( 64, 64, 32 ) <> 0 ) then
        print "gfx truecolor failed: screenres failed"
        end 1
    end if
end if

screenset 0, 0
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
line (24, 8)-(39, 23), rgb( 0, 255, 0 ), bf
line (40, 8)-(55, 23), rgb( 0, 0, 255 ), bf
expect_rgb 8, 8, rgb( 255, 0, 0 ), "red block"
expect_rgb 24, 8, rgb( 0, 255, 0 ), "green block"
expect_rgb 40, 8, rgb( 0, 0, 255 ), "blue block"

if( has_extra_page <> 0 ) then
    screenset 1, 1
else
    screenset 0, 0
end if
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 32)-(55, 55), rgb( 255, 255, 255 ), bf
expect_rgb 8, 32, rgb( 255, 255, 255 ), "screenset page"

screensync
sleep 50, 1
screen 0
EOF

    cat > /tmp/freebasic-illumos-smoke/gfx-screen-modes.bas <<'EOF'
#include once "fbgfx.bi"

sub fail( byref message as string )
    print "gfx legacy failure: "; message
    end 1
end sub

sub expect_index( byval x as integer, byval y as integer, byval expected as ulong, byref label as string )
    dim as ulong actual = culng( point( x, y ) )

    if( actual <> expected ) then
        print "gfx legacy mismatch: "; label; " actual="; actual; " expected="; expected
        end 1
    end if
end sub

sub draw_mode( byval mode as integer )
    dim as integer w, h, depth, bpp, pitch

    screeninfo w, h, depth, bpp, pitch
    if( w < 48 or h < 32 ) then
        fail "mode " & str( mode ) & " reported an unexpectedly small framebuffer"
    end if

    screenset 0, 0
    cls

    if( depth <= 1 ) then
        palette 1, 255, 255, 255
        line (0, 0)-(47, 31), 0, bf
        line (8, 8)-(23, 23), 1, bf
        expect_index 8, 8, 1, "mode " & str( mode ) & " white block"
    else
        palette 1, 255, 0, 0
        palette 2, 0, 255, 0
        palette 3, 0, 0, 255
        line (0, 0)-(63, 31), 0, bf
        line (8, 8)-(23, 23), 1, bf
        line (24, 8)-(39, 23), 2, bf
        line (40, 8)-(55, 23), 3, bf
        expect_index 8, 8, 1, "mode " & str( mode ) & " red block"
        expect_index 24, 8, 2, "mode " & str( mode ) & " green block"
        expect_index 40, 8, 3, "mode " & str( mode ) & " blue block"
    end if

    screensync
    sleep 30, 1
end sub

sub test_mode( byval mode as integer )
    dim as integer stage = 0
    dim as integer unsupported_mode = 0

    if( mode = 0 ) then
        screen 0
        print "SCREEN 0 ok"
        exit sub
    end if

    on local error goto mode_error

    stage = 1
    screen mode
    if( unsupported_mode <> 0 ) then
        screen 0
        print "SCREEN "; mode; " unsupported, err="; err()
        exit sub
    end if

    stage = 2
    if( screenptr() = 0 ) then
        screen 0
        print "SCREEN "; mode; " unsupported"
        exit sub
    end if

    draw_mode mode
    screen 0
    print "SCREEN "; mode; " ok"
    exit sub

mode_error:
    if( stage = 1 ) then
        unsupported_mode = 1
        resume next
    end if

    print "SCREEN "; mode; " failed, err="; err()
    end 1
end sub

for mode as integer = 0 to 13
    test_mode mode
next

end 0
EOF

    cat > /tmp/freebasic-illumos-smoke/sfx.bas <<'EOF'
extern "C"
declare function fb_sfxDeviceCurrent() as long
declare function fb_sfxDeviceInfoName(byval id as long) as const zstring ptr
end extern

print "sfx-start"
dim as long sfx_device = fb_sfxDeviceCurrent()
dim as const zstring ptr sfx_driver = fb_sfxDeviceInfoName(sfx_device)
if sfx_driver <> 0 then
    print "sfx-driver="; *sfx_driver
    if instr(lcase(*sfx_driver), "null") > 0 then
        print "sfx-driver-null"
        end 2
    end if
else
    print "sfx-driver=<none>"
    end 3
end if
sound 440, 0.75
play "L4 CDEFGAB"
sleep 1600, 1
print "sfx-end"
EOF

    echo "==> console smoke"
    "$PREFIX/bin/fbc" /tmp/freebasic-illumos-smoke/console.bas -x /tmp/freebasic-illumos-smoke/console
    [ "$(/tmp/freebasic-illumos-smoke/console)" = "Hello world" ] || exit 1

    echo "==> gfxlib compile"
    "$PREFIX/bin/fbc" /tmp/freebasic-illumos-smoke/gfx-truecolor.bas -x /tmp/freebasic-illumos-smoke/gfx-truecolor
    "$PREFIX/bin/fbc" -lang fblite -exx /tmp/freebasic-illumos-smoke/gfx-screen-modes.bas -x /tmp/freebasic-illumos-smoke/gfx-screen-modes

	echo "==> gfxlib smoke"
	XVFB_PID=""
	GFX_SMOKE=1
	if [ -n "${DISPLAY:-}" ]; then
		echo "==> using existing DISPLAY=$DISPLAY"
	else
		if command -v Xvfb >/dev/null 2>&1; then
			Xvfb :99 -screen 0 800x600x24 >/tmp/freebasic-illumos-xvfb.log 2>&1 &
			XVFB_PID=$!
			trap 'kill "$XVFB_PID" >/dev/null 2>&1 || true' EXIT
			export DISPLAY=:99
			sleep 2
			kill -0 "$XVFB_PID" >/dev/null 2>&1 || { cat /tmp/freebasic-illumos-xvfb.log; exit 1; }
		else
			echo "WARN: DISPLAY is unset and Xvfb is not available; skipping gfxlib runtime smoke"
			GFX_SMOKE=0
		fi
	fi
	if [ "$GFX_SMOKE" -eq 1 ]; then
		run_with_timeout 30 /tmp/freebasic-illumos-smoke/gfx-truecolor
		run_with_timeout 30 /tmp/freebasic-illumos-smoke/gfx-screen-modes
		if [ -n "$XVFB_PID" ]; then
			kill "$XVFB_PID" >/dev/null 2>&1 || true
			trap - EXIT
		fi
	else
		echo "==> gfxlib runtime smoke skipped"
	fi

    echo "==> sfxlib compile"
    "$PREFIX/bin/fbc" /tmp/freebasic-illumos-smoke/sfx.bas -x /tmp/freebasic-illumos-smoke/sfx
    if [ -f "$PREFIX/share/freebasic/examples/sfxlib/showcase.bas" ]; then
        (
            cd "$PREFIX/share/freebasic/examples/sfxlib"
            "$PREFIX/bin/fbc" showcase.bas -x /tmp/freebasic-illumos-smoke/sfx-showcase
        )
        [ -x /tmp/freebasic-illumos-smoke/sfx-showcase ] || exit 1
    else
        echo "WARN: examples not installed; skipping sfxlib showcase compile"
    fi

echo "==> sfxlib real audio smoke"
sfx_out="/tmp/freebasic-illumos-smoke/sfx.out"
sfx_err="/tmp/freebasic-illumos-smoke/sfx.err"
sfx_smoke_cmd="/tmp/freebasic-illumos-smoke/sfx"
sfx_driver_selected=0

if ( export SFXLIB_DRIVER="Solaris audio"; run_with_timeout 20 "$sfx_smoke_cmd" ) \
    > "$sfx_out" \
    2> "$sfx_err"; then
    sfx_driver_selected=1
else
    echo "WARN: explicit Solaris audio selection failed, checking fallback contract"
    cat "$sfx_out" || true
    cat "$sfx_err" || true
    echo "INFO: running sfx smoke with default driver selection"
    if ! run_with_timeout 20 "$sfx_smoke_cmd" > "$sfx_out" 2> "$sfx_err"; then
        echo "ERROR: sfx smoke failed with default driver selection"
        cat "$sfx_out" || true
        cat "$sfx_err" || true
        exit 1
    fi
fi

cat "$sfx_out"

driver_line="$(grep -a '^sfx-driver=' "$sfx_out" | head -n 1 || true)"
if [ -z "$driver_line" ]; then
    echo "ERROR: sfx smoke did not print driver line"
    [ -s "$sfx_err" ] && { cat "$sfx_err"; }
    exit 1
fi

grep -qx 'sfx-start' "$sfx_out" || exit 1
grep -qx 'sfx-end' "$sfx_out" || exit 1
echo "$driver_line" | grep -qi '^sfx-driver=.*null' && {
    echo "ERROR: sfx smoke used null audio driver"
    [ -s "$sfx_err" ] && { cat "$sfx_err"; }
    exit 1
}
echo "$driver_line" | grep -qi '^sfx-driver=.*solaris' || {
    echo "ERROR: sfx smoke did not select a Solaris family driver"
    [ -s "$sfx_err" ] && { cat "$sfx_err"; }
    exit 1
}
[ -s "$sfx_err" ] && {
    cat "$sfx_err"
    exit 1
}

if [ "$sfx_driver_selected" -eq 0 ]; then
    echo "WARN: explicit SFXLIB_DRIVER=\"Solaris audio\" failed, default backend selected: ${driver_line#*=}"
fi

    echo "==> fbctests and exampleageddon"
    FBC_GMAKE="$GMAKE" bash "$ROOT/build_scripts/illumos-test-freebasic.sh" \
        --skip-package-install \
        --fbc "$PREFIX/bin/fbc" \
        --fbctests-jobs "$BUILD_JOBS" \
        --exampleageddon-jobs 1 \
        --exampleageddon-compile-timeout "${EXAMPLEAGEDDON_COMPILE_TIMEOUT:-180}" \
        --exampleageddon-run-timeout "${EXAMPLEAGEDDON_RUN_TIMEOUT:-10}"

else
    echo "==> --no-package specified"
fi
