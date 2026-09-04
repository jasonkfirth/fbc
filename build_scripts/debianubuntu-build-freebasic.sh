#!/usr/bin/env bash

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

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }

cd "$ROOT"
. "$ROOT/build_scripts/build-success-cleanup.sh"

CLEANUP_SUCCESS=0
CLEANUP_DIRS=()

cleanup_build_roots() {
    local path

    [ "$CLEANUP_SUCCESS" -eq 1 ] || return 0

    for path in "${CLEANUP_DIRS[@]}"; do
        [ -n "$path" ] || continue
        fb_remove_build_tree "$ROOT" "$path" || true
    done
}

trap cleanup_build_roots EXIT

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

assert_removable_tree() {
    local path="$1"
    [ -e "$path" ] || return 0
    if ! find "$path" ! -type l \( ! -user "$(id -u)" -o ! -writable \) -print -quit | grep -q .; then
        return 0
    fi
    die "build workspace is not writable: $path
Run: sudo chown -R $(id -un):$(id -gn) '$path'"
}

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

run_root_apt() {
    #
    # Archived distribution mirrors occasionally reset an individual
    # package download while the rest of the transaction succeeds. Let apt
    # retry those transfers before abandoning an otherwise valid build.
    # The timeouts also prevent an unreachable mirror from hanging a matrix
    # row indefinitely.
    #
    run_root apt-get \
        -o Acquire::Retries=5 \
        -o Acquire::http::Timeout=30 \
        -o Acquire::https::Timeout=30 \
        "$@"
}

run_root_apt_install() {
    local attempt=1
    local max_attempts=3
    local retry_delay_seconds=15

    while [ "$attempt" -le "$max_attempts" ]; do
        if run_root_apt install "$@"; then
            return 0
        fi

        if [ "$attempt" -eq "$max_attempts" ]; then
            return 1
        fi

        #
        # A rolling mirror can publish a Packages index before every pool
        # file named by that index reaches the same server.  apt does not
        # retry a 404 even when Acquire::Retries is set.  Give the mirror a
        # short synchronization window, refresh the indexes, then repeat the
        # complete idempotent install transaction.
        #
        echo "==> APT dependency installation failed; refreshing package indexes before retry $((attempt + 1)) of $max_attempts"
        sleep "$retry_delay_seconds"

        if ! run_root_apt update -y; then
            echo "WARNING: APT index refresh failed before dependency retry" >&2
        fi

        attempt=$((attempt + 1))
    done

    return 1
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic.sh [options]

Options:
  --no-build      Reuse the existing source bootstrap tarball
  --no-js         Build packages with DEB_BUILD_PROFILES=pkg.freebasic.nojs
  --no-android    Build packages without DEB_BUILD_PROFILES=pkg.freebasic.android
  --android       Build the Ubuntu freebasic-android package; fail if SDK
                  packages are not available
  --wii           Build the freebasic-wii package; fail if a devkitPro
                  toolchain is not available
  --no-wii        Build packages without DEB_BUILD_PROFILES=pkg.freebasic.wii
  --host-arch A   Build a package for Debian architecture A
  --no-package    Stop after ensuring the bootstrap tarball exists
  --skip-deps     Skip apt dependency installation
  --help          Show this help text

Environment:
  BUILDROOT       Temporary build root (default: <repo>/.build-debianubuntu)
  WORKDIR         Workspace for bootstrap/package preparation
  OUTBASE         Output root (default: <repo>/out)
  FBC_PACKAGE_OUTDIR
                  Full package output directory override
  FBC_PACKAGE_HOST_ARCH
                  Debian architecture to package for when cross-building
  FBC_PACKAGE_CONFIGURE_CROSS_APT
                  Set to 1 to let the script rewrite Ubuntu APT sources for
                  cross-architecture package indexes
  FBC_PACKAGE_ARM_ARCH
                  ARM default arch override for package builds (armv6+fp)
  FBC_PACKAGE_SKIP_BUILD_DEPENDS
                  Set to 1 only when an externally supplied toolchain satisfies
                  profile-specific build dependencies unavailable through APT
  DEVKITPRO       devkitPro root for an externally supplied Wii toolchain
  DEVKITPPC       devkitPPC root (default: <DEVKITPRO>/devkitPPC)
  JOBS            Parallel make job count for bootstrap generation

Artifacts are written under:
  out/linux/<distro>/<codename>/<arch>/
EOF
}

##############################################################################
# Options
##############################################################################

NO_BUILD=0
NO_JS=0
ANDROID=1
ANDROID_EXPLICIT=0
WII=0
WII_EXPLICIT=0
NO_PACKAGE=0
SKIP_DEPS=0
HOST_ARCH_OPT="${FBC_PACKAGE_HOST_ARCH:-}"

while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) NO_BUILD=1; shift ;;
        --no-js) NO_JS=1; shift ;;
        --no-android) ANDROID=0; ANDROID_EXPLICIT=1; shift ;;
        --android) ANDROID=1; ANDROID_EXPLICIT=1; shift ;;
        --wii) WII=1; WII_EXPLICIT=1; shift ;;
        --no-wii) WII=0; WII_EXPLICIT=1; shift ;;
        --host-arch)
            [ $# -ge 2 ] || die "--host-arch requires an architecture"
            HOST_ARCH_OPT="$2"
            shift 2
            ;;
        --no-package) NO_PACKAGE=1; shift ;;
        --skip-deps) SKIP_DEPS=1; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

##############################################################################
# Ensure Debian / Ubuntu style environment
##############################################################################

if ! command -v apt-get >/dev/null 2>&1; then
    echo "ERROR: this script requires an APT-based distribution (Debian/Ubuntu)"
    exit 1
fi

##############################################################################
# Tooling
##############################################################################

if command -v gmake >/dev/null 2>&1; then
    MAKE_CMD="gmake"
else
    MAKE_CMD="make"
fi

if command -v nproc >/dev/null 2>&1; then
    JOBS="${JOBS:-$(nproc)}"
else
    JOBS="${JOBS:-1}"
fi

##############################################################################
# Config
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-debianubuntu}"
WORKDIR="${WORKDIR:-$BUILDROOT/work}"
OUTBASE="${OUTBASE:-$ROOT/out}"
BUILDDIR="${BUILDDIR:-$WORKDIR/package}"
SOURCE_COPY_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"
CLEANUP_DIRS=("$BUILDROOT")

VERSION="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' mk/version.mk | head -n1)"
[ -n "$VERSION" ] || die "could not determine FBVERSION"
[ -f "$SOURCE_COPY_EXCLUDES" ] || die "missing source copy excludes: $SOURCE_COPY_EXCLUDES"

BUILD_ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
[ -n "$BUILD_ARCH" ] || die "could not detect Debian architecture"

HOST_ARCH="${HOST_ARCH_OPT:-$BUILD_ARCH}"
ARCH="$HOST_ARCH"

map_debian_arch() {
    local arch="$1"

    MAP_TARGET_TRIPLET=""
    MAP_BOOTKEY=""
    MAP_FBC_TARGET=""

    case "$arch" in
    amd64)
        MAP_TARGET_TRIPLET="x86_64-linux-gnu"
        MAP_BOOTKEY="linux-amd64"
        MAP_FBC_TARGET="linux-x86_64"
        ;;
    i386)
        MAP_TARGET_TRIPLET="i686-linux-gnu"
        MAP_BOOTKEY="linux-i386"
        MAP_FBC_TARGET="linux-x86"
        ;;
    arm64)
        MAP_TARGET_TRIPLET="aarch64-linux-gnu"
        MAP_BOOTKEY="linux-arm64"
        MAP_FBC_TARGET="linux-aarch64"
        ;;
    armhf)
        MAP_TARGET_TRIPLET="arm-linux-gnueabihf"
        MAP_BOOTKEY="linux-armhf"
        MAP_FBC_TARGET="linux-arm"
        ;;
    armel)
        MAP_TARGET_TRIPLET="arm-linux-gnueabi"
        MAP_BOOTKEY="linux-armel"
        MAP_FBC_TARGET="linux-arm"
        ;;
    powerpc)
        MAP_TARGET_TRIPLET="powerpc-linux-gnu"
        MAP_BOOTKEY="linux-powerpc"
        MAP_FBC_TARGET="linux-powerpc"
        ;;
    ppc64)
        MAP_TARGET_TRIPLET="powerpc64-linux-gnu"
        MAP_BOOTKEY="linux-powerpc64"
        MAP_FBC_TARGET="linux-powerpc64"
        ;;
    ppc64el)
        MAP_TARGET_TRIPLET="powerpc64le-linux-gnu"
        MAP_BOOTKEY="linux-ppc64el"
        MAP_FBC_TARGET="linux-powerpc64le"
        ;;
    s390x)
        MAP_TARGET_TRIPLET="s390x-linux-gnu"
        MAP_BOOTKEY="linux-s390x"
        MAP_FBC_TARGET="linux-s390x"
        ;;
    riscv64)
        MAP_TARGET_TRIPLET="riscv64-linux-gnu"
        MAP_BOOTKEY="linux-riscv64"
        MAP_FBC_TARGET="linux-riscv64"
        ;;
    loong64)
        MAP_TARGET_TRIPLET="loongarch64-linux-gnu"
        MAP_BOOTKEY="linux-loongarch64"
        MAP_FBC_TARGET="linux-loongarch64"
        ;;
    *)
        die "unsupported Debian architecture: $arch"
        ;;
    esac
}

map_debian_arch "$HOST_ARCH"
TARGET_TRIPLET="$MAP_TARGET_TRIPLET"

map_debian_arch "$BUILD_ARCH"
BUILD_TARGET_TRIPLET="$MAP_TARGET_TRIPLET"
BUILD_BOOTKEY="$MAP_BOOTKEY"
BUILD_FBC_TARGET="$MAP_FBC_TARGET"

CROSS_PACKAGE_BUILD=0
if [ "$BUILD_ARCH" != "$HOST_ARCH" ]; then
    CROSS_PACKAGE_BUILD=1
fi

android_supported_for_arch() {
    # Google's Linux NDK host tools, and Ubuntu's installer for them, are
    # amd64-only.  The package still emits Android ARM target runtimes.
    case "$1" in
        amd64)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

if [ "$ANDROID" -eq 1 ] && ! android_supported_for_arch "$ARCH"; then
    if [ "$ANDROID_EXPLICIT" -eq 1 ]; then
        die "Android SDK/NDK packages are not available for Debian architecture: $ARCH"
    fi
    echo "==> disabling Android package profile for unsupported architecture: $ARCH"
    ANDROID=0
fi

apt_package_available() {
    local pkg="$1"
    local candidate

    candidate="$(apt-cache policy "$pkg" 2>/dev/null | sed -n 's/^[[:space:]]*Candidate:[[:space:]]*//p' | head -n1)"
    [ -n "$candidate" ] && [ "$candidate" != "(none)" ]
}

apt_any_package_available() {
    local pkg

    for pkg in "$@"; do
        apt_package_available "$pkg" && return 0
    done

    return 1
}

android_sdk_packages_available() {
    apt_package_available openjdk-17-jdk-headless || return 1
    apt_package_available android-sdk || return 1
    apt_package_available android-sdk-platform-tools || return 1
    apt_package_available android-sdk-build-tools || return 1
    apt_package_available dalvik-exchange || return 1
    apt_any_package_available android-sdk-platform-23 google-android-platform-26-installer || return 1
    apt_package_available google-android-ndk-r28-installer || return 1
    apt_package_available aapt || return 1
    apt_package_available apksigner || return 1
    apt_package_available gradle || return 1

    return 0
}

wii_sdk_packages_available() {
    apt_package_available devkitppc || return 1
    apt_package_available libogc || return 1
    apt_package_available gamecube-tools || return 1

    return 0
}

wii_external_toolchain_available() {
    local devkitpro="${DEVKITPRO:-/opt/devkitpro}"
    local devkitppc="${DEVKITPPC:-$devkitpro/devkitPPC}"

    [ -x "$devkitppc/bin/powerpc-eabi-gcc" ] || return 1
    [ -x "$devkitppc/bin/powerpc-eabi-ar" ] || return 1
    [ -x "$devkitppc/bin/powerpc-eabi-ranlib" ] || return 1
    [ -x "$devkitpro/tools/bin/elf2dol" ] || return 1
    [ -d "$devkitpro/libogc/include" ] || return 1
    [ -d "$devkitpro/libogc/lib/wii" ] || return 1
}

disable_android_if_sdk_unavailable() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0
    [ "$ANDROID" -eq 1 ] || return 0

    if android_sdk_packages_available; then
        return 0
    fi

    if [ "$ANDROID_EXPLICIT" -eq 1 ]; then
        die "Android SDK/NDK packages are not available from the configured APT repositories"
    fi

    echo "==> disabling Android package profile because APT cannot install the required Android SDK/NDK packages"
    ANDROID=0
}

disable_wii_if_sdk_unavailable() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0
    [ "$WII" -eq 1 ] || return 0

    if wii_external_toolchain_available || wii_sdk_packages_available; then
        return 0
    fi

    if [ "$WII_EXPLICIT" -eq 1 ]; then
        die "Wii devkitPro packages are not available from the configured APT repositories"
    fi

    echo "==> disabling Wii package profile because APT cannot install devkitppc/libogc/gamecube-tools"
    WII=0
}

ubuntu_uses_old_releases() {
    [ "$DISTRO_ID" = "ubuntu" ] || return 1

    case "$CODENAME" in
    oracular)
        return 0
        ;;
    *)
        return 1
        ;;
    esac
}

raspbian_uses_legacy_archive() {
    [ "$DISTRO_ID" = "raspbian" ] || return 1

    case "$CODENAME" in
    buster)
        return 0
        ;;
    *)
        return 1
        ;;
    esac
}

disable_existing_apt_sources() {
    local disabled_dir="$1"
    local f

    run_root mkdir -p "$disabled_dir"

    for f in /etc/apt/sources.list /etc/apt/sources.list.d/*.sources /etc/apt/sources.list.d/*.list; do
        [ -e "$f" ] || continue
        case "$f" in
            */fbc-cross.sources|*/fbc-ubuntu-archive.sources|*/fbc-debian-ports.sources)
                continue
                ;;
        esac
        run_root mv "$f" "$disabled_dir/$(basename "$f")"
    done
}

configure_ubuntu_archived_apt_sources() {
    local tmp_sources
    local disabled_dir

    [ "$CROSS_PACKAGE_BUILD" -eq 0 ] || return 0
    ubuntu_uses_old_releases || return 0

    msg "configuring Ubuntu old-releases apt sources for archived release: $CODENAME"

    disabled_dir="/etc/apt/sources.list.d/fbc-archive-disabled"
    tmp_sources="$(mktemp)"

    cat > "$tmp_sources" <<EOF
Types: deb
URIs: http://old-releases.ubuntu.com/ubuntu
Suites: ${CODENAME} ${CODENAME}-updates ${CODENAME}-backports ${CODENAME}-security
Components: main restricted universe multiverse
Architectures: ${BUILD_ARCH}
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF

    disable_existing_apt_sources "$disabled_dir"
    run_root install -m 644 "$tmp_sources" /etc/apt/sources.list.d/fbc-ubuntu-archive.sources
    rm -f "$tmp_sources"
}

configure_raspbian_legacy_apt_sources() {
    local f

    raspbian_uses_legacy_archive || return 0

    msg "configuring Raspbian legacy apt mirror for archived release: $CODENAME"

    # The Raspberry Pi OS buster image still points at the live Raspbian
    # mirror, but buster has moved to legacy.raspbian.org.  Keep the
    # Raspberry Pi Foundation package source unchanged; only the Raspbian
    # distro archive has moved.
    for f in /etc/apt/sources.list /etc/apt/sources.list.d/*.list; do
        [ -e "$f" ] || continue
        run_root sed -i \
            -e 's|http://raspbian.raspberrypi.org/raspbian/?|http://legacy.raspbian.org/raspbian/|g' \
            -e 's|http://raspbian.raspberrypi.org/raspbian|http://legacy.raspbian.org/raspbian|g' \
            "$f"
    done
}

configure_ubuntu_cross_apt_sources() {
    local tmp_sources
    local disabled_dir

    [ "$CROSS_PACKAGE_BUILD" -eq 1 ] || return 0
    [ "$DISTRO_ID" = "ubuntu" ] || return 0
    [ -n "$CODENAME" ] || return 0
    [ "$CODENAME" != "unknown" ] || return 0

    if [ "${FBC_PACKAGE_CONFIGURE_CROSS_APT:-0}" != "1" ]; then
        echo "==> leaving Ubuntu apt sources unchanged; set FBC_PACKAGE_CONFIGURE_CROSS_APT=1 to enable cross-source rewriting"
        return 0
    fi

    msg "configuring Ubuntu apt sources for cross architecture: $HOST_ARCH"

    disabled_dir="/etc/apt/sources.list.d/fbc-cross-disabled"
    tmp_sources="$(mktemp)"

    if ubuntu_uses_old_releases; then
        cat > "$tmp_sources" <<EOF
Types: deb
URIs: http://old-releases.ubuntu.com/ubuntu
Suites: ${CODENAME} ${CODENAME}-updates ${CODENAME}-backports ${CODENAME}-security
Components: main restricted universe multiverse
Architectures: ${BUILD_ARCH} ${HOST_ARCH}
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF
    else
        cat > "$tmp_sources" <<EOF
Types: deb
URIs: http://archive.ubuntu.com/ubuntu
Suites: ${CODENAME} ${CODENAME}-updates ${CODENAME}-backports
Components: main restricted universe multiverse
Architectures: ${BUILD_ARCH}
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: http://security.ubuntu.com/ubuntu
Suites: ${CODENAME}-security
Components: main restricted universe multiverse
Architectures: ${BUILD_ARCH}
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports
Suites: ${CODENAME} ${CODENAME}-updates ${CODENAME}-backports ${CODENAME}-security
Components: main restricted universe multiverse
Architectures: ${HOST_ARCH}
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF
    fi

    disable_existing_apt_sources "$disabled_dir"
    run_root install -m 644 "$tmp_sources" /etc/apt/sources.list.d/fbc-cross.sources
    rm -f "$tmp_sources"
}

needs_debian_ports_cross_apt() {
    [ "$CROSS_PACKAGE_BUILD" -eq 1 ] || return 1
    [ "$DISTRO_ID" = "debian" ] || return 1
    [ "$CODENAME" = "sid" ] || return 1

    case "$HOST_ARCH" in
    powerpc|ppc64|loong64)
        return 0
        ;;
    *)
        return 1
        ;;
    esac
}

needs_isolated_target_sysroot() {
    [ "$CROSS_PACKAGE_BUILD" -eq 1 ] || return 1
    [ "$DISTRO_ID" = "debian" ] || return 1
    [ "$CODENAME" = "sid" ] || return 1
}

install_debian_ports_cross_keyring() {
    needs_debian_ports_cross_apt || return 0

    if [ "${FBC_PACKAGE_CONFIGURE_CROSS_APT:-0}" != "1" ]; then
        return 0
    fi

    msg "installing Debian Ports archive keyring"

    run_root_apt update -y
    run_root_apt install -y --no-install-recommends \
        ca-certificates \
        debian-ports-archive-keyring
}

configure_debian_ports_cross_apt_sources() {
    local tmp_sources

    needs_debian_ports_cross_apt || return 0

    if [ "${FBC_PACKAGE_CONFIGURE_CROSS_APT:-0}" != "1" ]; then
        echo "==> leaving Debian Ports apt sources unchanged; set FBC_PACKAGE_CONFIGURE_CROSS_APT=1 to enable cross-source setup"
        return 0
    fi

    msg "configuring Debian Ports apt sources for cross architecture: $HOST_ARCH"

    tmp_sources="$(mktemp)"

    cat > "$tmp_sources" <<EOF
Types: deb
URIs: http://ftp.ports.debian.org/debian-ports
Suites: sid unreleased
Components: main
Architectures: ${HOST_ARCH}
Signed-By: /usr/share/keyrings/debian-ports-archive-keyring.gpg
EOF

    run_root install -m 644 "$tmp_sources" /etc/apt/sources.list.d/fbc-debian-ports.sources
    rm -f "$tmp_sources"
}

find_common_multiarch_version() {
    local package="$1"
    local build_versions
    local host_versions
    local installed_build_version
    local version

    build_versions="$(
        apt-cache madison "${package}:${BUILD_ARCH}" |
        awk -F '|' '{ gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2 }'
    )"
    host_versions="$(
        apt-cache madison "${package}:${HOST_ARCH}" |
        awk -F '|' '{ gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2 }'
    )"

    #
    # Rolling images can contain a version that has already disappeared from
    # the build architecture's package index but is still current for the
    # target architecture.  Include that installed version when looking for
    # the exact match required by Multi-Arch: same packages.
    #
    installed_build_version="$(
        dpkg-query -W -f='${Version}' "${package}:${BUILD_ARCH}" 2>/dev/null || true
    )"
    if [ -n "$installed_build_version" ]; then
        build_versions="${installed_build_version}"$'\n'"${build_versions}"
    fi

    [ -n "$build_versions" ] || return 1
    [ -n "$host_versions" ] || return 1

    while IFS= read -r version; do
        [ -n "$version" ] || continue
        if printf '%s\n' "$host_versions" | grep -Fqx "$version"; then
            printf '%s\n' "$version"
            return 0
        fi
    done <<< "$build_versions"

    return 1
}

install_debian_target_sysroot() {
    local packages=("$@")
    local sysroot_base="$BUILDROOT/debian-target-sysroot"
    local apt_state="$sysroot_base/apt-state"
    local apt_archives="$sysroot_base/apt-cache/archives"
    local sysroot="$sysroot_base/root"
    local controls="$sysroot_base/controls"
    local shlibs_file="$sysroot_base/shlibs.local"
    local downloaded_packages=()
    local package
    local package_arch
    local package_control
    local package_index=0

    needs_isolated_target_sysroot || return 0
    [ "${#packages[@]}" -gt 0 ] || die "Debian target sysroot package list is empty"

    msg "creating isolated Debian target sysroot for $HOST_ARCH"

    assert_removable_tree "$sysroot_base"
    fb_remove_build_tree "$ROOT" "$sysroot_base" ||
        die "could not remove the previous Debian target sysroot"

    mkdir -p \
        "$apt_state" \
        "$apt_archives/partial" \
        "$sysroot" \
        "$controls"
    : > "$apt_state/status"
    : > "$apt_state/extended_states"

    #
    # Sid architectures and cross toolchains are published asynchronously.
    # During a C library transition, the target development packages can be
    # newer than the libc headers required by the cross compiler itself.
    # Debian Ports can also carry a different version of a Multi-Arch: same
    # library than the amd64 build container. Installing either combination
    # into one dpkg database is impossible. Resolve the target dependency
    # closure against an empty target-only package state instead, then
    # extract that closure into a compiler sysroot.
    #
    run_root_apt \
        -o "APT::Architecture=${HOST_ARCH}" \
        -o "APT::Architectures=${HOST_ARCH}" \
        -o "Dir::State::status=${apt_state}/status" \
        -o "Dir::State::extended_states=${apt_state}/extended_states" \
        -o "Dir::State::lists=/var/lib/apt/lists" \
        -o "Dir::Cache::archives=${apt_archives}" \
        -o Debug::NoLocking=true \
        install -y --download-only --no-install-recommends \
        "${packages[@]}"

    shopt -s nullglob
    downloaded_packages=("$apt_archives"/*.deb)
    shopt -u nullglob
    [ "${#downloaded_packages[@]}" -gt 0 ] ||
        die "Debian target sysroot resolver downloaded no packages"

    : > "$shlibs_file"
    for package in "${downloaded_packages[@]}"; do
        package_arch="$(dpkg-deb --field "$package" Architecture)"
        case "$package_arch" in
        all|"$HOST_ARCH")
            ;;
        *)
            die "Debian target sysroot contains unexpected ${package_arch} package: $(basename "$package")"
            ;;
        esac

        dpkg-deb --extract "$package" "$sysroot"

        package_index=$((package_index + 1))
        package_control="$controls/$package_index"
        mkdir -p "$package_control"
        dpkg-deb --control "$package" "$package_control"
        if [ -f "$package_control/shlibs" ]; then
            cat "$package_control/shlibs" >> "$shlibs_file"
        fi
    done

    sort -u "$shlibs_file" -o "$shlibs_file"
    [ -s "$shlibs_file" ] ||
        die "Debian target sysroot packages supplied no shared-library metadata"

    export FBC_PACKAGE_SYSROOT="$sysroot"
    export FBC_PACKAGE_TARGET_SHLIBS="$shlibs_file"

    echo "==> Debian target sysroot packages: ${#downloaded_packages[@]}"
    echo "==> Debian target sysroot: $FBC_PACKAGE_SYSROOT"
}

assert_orig_tarball_clean() {
    local archive="$1"
    local bad_entry

    bad_entry="$(
        tar -tJf "$archive" |
        while IFS= read -r entry; do
            rel="${entry#*/}"
            case "$rel" in
                */obj/*|\
                .build-alpine/*|\
                .build-debianubuntu*/*|\
                .git/*|\
                bin/*|\
                dist/*|\
                doc/manual/FB-manual-*|\
                lib/freebasic/*|\
                lib/freebasic-wii/*/*.a|\
                lib/freebasic-wii/*/*.o|\
                out|\
                out/*|\
                package-root*/*|\
                packages/*|\
                pkgroot*/*|\
                stage/*|\
                */__pycache__/*|\
                tests/*.asm|\
                tests/*.dol|\
                tests/*.exe|\
                tests/log-tests-*.inc|\
                tests/log-tests-*.lst|\
                tests/*.o|\
                tests/*.obj|\
                tests/unit-tests.inc|\
                tests/unit-tests-obj.lst|\
                tests/*.xml|\
                *.log|\
                *.pid|\
                *.status|\
                *.tmp|\
                tmp/*)
                    printf '%s\n' "$entry"
                    break
                    ;;
            esac
        done
    )"

    if [ -n "$bad_entry" ]; then
        die "orig tarball contains generated build content: $bad_entry"
    fi
}

BOOTSTRAP_TAR="$ROOT/FreeBASIC-${VERSION}-source-bootstrap-${BUILD_BOOTKEY}.tar.xz"

ARM_MAKE_ARGS=()
case "${FBC_PACKAGE_ARM_ARCH:-}" in
    "")
        ;;
    armv6+fp)
        ARM_MAKE_ARGS=(
            ARM_VER=v6
            ARM_FLOAT_ABI=hf
            DEFAULT_CPUTYPE_ARM=FB_CPUTYPE_ARMV6_FP
        )
        ;;
    *)
        die "unsupported FBC_PACKAGE_ARM_ARCH: $FBC_PACKAGE_ARM_ARCH"
        ;;
esac

DISTRO_ID=""
CODENAME=""
if [ -f /etc/os-release ]; then
    DISTRO_ID="$(
        # shellcheck disable=SC1091
        . /etc/os-release
        printf '%s' "${ID:-}"
    )"
    CODENAME="$(
        # shellcheck disable=SC1091
        . /etc/os-release
        printf '%s' "${VERSION_CODENAME:-}"
    )"
fi

if [ -z "$DISTRO_ID" ] && command -v lsb_release >/dev/null 2>&1; then
    DISTRO_ID="$(lsb_release -is 2>/dev/null | tr '[:upper:]' '[:lower:]' || true)"
fi

if [ -z "$CODENAME" ] && command -v lsb_release >/dev/null 2>&1; then
    CODENAME="$(lsb_release -sc 2>/dev/null || true)"
fi

[ -n "$DISTRO_ID" ] || DISTRO_ID="unknown"
[ -n "$CODENAME" ] || CODENAME="unknown"

if [ -n "${FBC_PACKAGE_DISTRO_ID:-}" ]; then
    DISTRO_ID="$FBC_PACKAGE_DISTRO_ID"
fi

if [ -n "${FBC_PACKAGE_CODENAME:-}" ]; then
    CODENAME="$FBC_PACKAGE_CODENAME"
fi

android_supported_for_distro() {
    [ "$DISTRO_ID" = "ubuntu" ]
}

if [ "$ANDROID" -eq 1 ] && ! android_supported_for_distro; then
    if [ "$ANDROID_EXPLICIT" -eq 1 ]; then
        die "freebasic-android is only packaged from Ubuntu targets; $DISTRO_ID/$CODENAME cannot provide the Android SDK/NDK dependency set"
    fi

    echo "==> disabling Android package profile for non-Ubuntu target: $DISTRO_ID/$CODENAME"
    ANDROID=0
fi

OUTDIR="${FBC_PACKAGE_OUTDIR:-${OUTBASE}/linux/${DISTRO_ID}/${CODENAME}/${ARCH}}"

mkdir -p "$WORKDIR" "$OUTDIR"

##############################################################################
# Dependency installation
##############################################################################

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Debian/Ubuntu build dependencies"

    export DEBIAN_FRONTEND=noninteractive
    export TERM=dumb
    export NCURSES_NO_UTF8_ACS=1
    export DEB_BUILD_MAINT_OPTIONS="hardening=+all"

    if [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
        install_debian_ports_cross_keyring
        run_root dpkg --add-architecture "$HOST_ARCH"
        configure_ubuntu_cross_apt_sources
        configure_debian_ports_cross_apt_sources
        configure_raspbian_legacy_apt_sources
    else
        configure_ubuntu_archived_apt_sources
        configure_raspbian_legacy_apt_sources
    fi

    run_root_apt update -y
    disable_android_if_sdk_unavailable
    disable_wii_if_sdk_unavailable

    local js_deps=()
    if [ "$NO_JS" -eq 0 ]; then
        js_deps=(emscripten nodejs)
    fi
    local android_deps=()
    if [ "$ANDROID" -eq 1 ]; then
        android_deps=(
            openjdk-17-jdk-headless
            android-sdk
            android-sdk-platform-tools
            android-sdk-build-tools
            dalvik-exchange
            android-sdk-platform-23
            google-android-ndk-r28-installer
            gradle
            aapt
            apksigner
            zip
            unzip
        )
    fi
    local wii_deps=()
    if [ "$WII" -eq 1 ] && ! wii_external_toolchain_available; then
        wii_deps=(
            devkitppc
            libogc
            gamecube-tools
        )
    fi
    local cross_deps=()
    local native_library_deps=()
    local target_deps=()
    local target_package_names=()
    local multiarch_version_locks=()
    if [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
        native_library_deps=(
            libncurses-dev
            libtinfo-dev
        )
    else
        native_library_deps=(
            libncurses-dev
            libtinfo-dev
            libgpm-dev
            libffi-dev
            libasound2-dev
            libpulse-dev
            libx11-dev
            libxpm-dev
            libxext-dev
            libxrandr-dev
            libxrender-dev
            libxcb1-dev
            libxau-dev
            libxdmcp-dev
            libxi-dev
            libxinerama-dev
            libxxf86vm-dev
            libgl1-mesa-dev
            libglu1-mesa-dev
        )
    fi
    if [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
        cross_deps=()
        case "$HOST_ARCH" in
        ppc64|loong64)
            ;;
        *)
            cross_deps+=("crossbuild-essential-${HOST_ARCH}")
            ;;
        esac
        cross_deps+=(
            "binutils-${TARGET_TRIPLET}"
            "gcc-${TARGET_TRIPLET}"
            "g++-${TARGET_TRIPLET}"
        )

        target_package_names=(
            libc6-dev
            libncurses-dev
            libtinfo-dev
            libgpm-dev
            libffi-dev
            libasound2-dev
            libpulse-dev
            libx11-dev
            libxpm-dev
            libxext-dev
            libxrandr-dev
            libxrender-dev
            libxcb1-dev
            libxau-dev
            libxdmcp-dev
            libxi-dev
            libxinerama-dev
            libxxf86vm-dev
            libgl1-mesa-dev
            libglu1-mesa-dev
        )

        if needs_isolated_target_sysroot; then
            # The target libraries are installed into an isolated sysroot
            # after the native build tools have been installed.
            target_deps=()
        else
            local target_package
            for target_package in "${target_package_names[@]}"; do
                target_deps+=("${target_package}:${HOST_ARCH}")
            done

            local linux_libc_architecture
            local linux_libc_version
            linux_libc_architecture="$(
                apt-cache show linux-libc-dev |
                awk '/^Architecture:/ { print $2; exit }'
            )"
            [ -n "$linux_libc_architecture" ] ||
                die "could not determine the linux-libc-dev package architecture"

            if [ "$linux_libc_architecture" = "all" ]; then
                # Debian 13 and newer provide one Multi-Arch: foreign package.
                # An architecture qualifier is invalid for this package layout.
                linux_libc_version="$(
                    apt-cache madison linux-libc-dev |
                    awk -F '|' '
                        {
                            gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2)
                            if ($2 != "") {
                                print $2
                                exit
                            }
                        }
                    '
                )"
                [ -n "$linux_libc_version" ] ||
                    die "linux-libc-dev has no installable version"
                echo "==> locking architecture-independent linux-libc-dev to version: $linux_libc_version"
                multiarch_version_locks=(
                    "linux-libc-dev=${linux_libc_version}"
                )
            else
                linux_libc_version="$(find_common_multiarch_version linux-libc-dev)" ||
                    die "linux-libc-dev has no version available for both ${BUILD_ARCH} and ${HOST_ARCH}"
                echo "==> locking linux-libc-dev for ${BUILD_ARCH} and ${HOST_ARCH} to common version: $linux_libc_version"
                multiarch_version_locks=(
                    "linux-libc-dev:${BUILD_ARCH}=${linux_libc_version}"
                    "linux-libc-dev:${HOST_ARCH}=${linux_libc_version}"
                )
            fi

            local libzstd_build_candidate
            local libzstd_target_candidate
            local libzstd_version
            libzstd_build_candidate="$(
                apt-cache policy "libzstd1:${BUILD_ARCH}" |
                awk '/Candidate:/ { print $2; exit }'
            )"
            libzstd_target_candidate="$(
                apt-cache policy "libzstd1:${HOST_ARCH}" |
                awk '/Candidate:/ { print $2; exit }'
            )"

            if [ -n "$libzstd_build_candidate" ] &&
               [ -n "$libzstd_target_candidate" ] &&
               [ "$libzstd_build_candidate" != "$libzstd_target_candidate" ]; then
                if libzstd_version="$(find_common_multiarch_version libzstd1)"; then
                    #
                    # Mesa and LLVM pull libzstd1 into graphics cross-builds.
                    # Debian architectures are published independently, so
                    # apt must sometimes retain the image's installed native
                    # version while installing the matching target package.
                    #
                    echo "==> locking libzstd1 for ${BUILD_ARCH} and ${HOST_ARCH} to common version: $libzstd_version"
                    multiarch_version_locks+=(
                        "libzstd1:${BUILD_ARCH}=${libzstd_version}"
                        "libzstd1:${HOST_ARCH}=${libzstd_version}"
                    )
                fi
            fi
        fi
    fi

    run_root_apt_install -y --allow-downgrades --no-install-recommends \
        ca-certificates \
        build-essential gcc g++ binutils make \
        pkgconf rsync \
        debhelper dpkg-dev devscripts fakeroot lintian \
        quilt dos2unix \
        tar xz-utils \
        "${native_library_deps[@]}" \
        "${cross_deps[@]}" \
        "${target_deps[@]}" \
        "${multiarch_version_locks[@]}" \
        "${js_deps[@]}" \
        "${android_deps[@]}" \
        "${wii_deps[@]}" \
        perl python3 git

    if needs_isolated_target_sysroot; then
        install_debian_target_sysroot "${target_package_names[@]}"
    fi
}

##############################################################################
# Bootstrap generation
##############################################################################

ensure_host_compiler() {
    if [ ! -x "./bin/fbc" ]; then
        msg "building host compiler"
        run "$MAKE_CMD" clean
        run "$MAKE_CMD" bootstrap-minimal -j"$JOBS"
        run "$MAKE_CMD" compiler -j"$JOBS" BUILD_FBCFLAGS="-d __FB_BOOTSTRAP_COMPAT__"
        run "$MAKE_CMD" clean-compiler
        run "$MAKE_CMD" compiler -j"$JOBS"
    fi

    [ -x "./bin/fbc" ] || die "host compiler not available"
}

build_bootstrap_tarball() {
    msg "building bootstrap tarball: $BOOTSTRAP_TAR"

    ensure_host_compiler

    rm -f "$BOOTSTRAP_TAR"
    fb_remove_build_tree "$ROOT" "$ROOT/bootstrap/${BUILD_BOOTKEY}" || die "could not remove bootstrap/${BUILD_BOOTKEY}"
    "$MAKE_CMD" clean-bootstrap-sources >/dev/null 2>&1 || true

    run "$MAKE_CMD" \
        TARGET_TRIPLET="$BUILD_TARGET_TRIPLET" \
        FBC_TARGET="$BUILD_FBC_TARGET" \
        FBTARGET_DIR_OVERRIDE="$BUILD_BOOTKEY" \
        BOOTSTRAP_DIST_WORKTREE=1 \
        "${ARM_MAKE_ARGS[@]}" \
        bootstrap-dist-target \
        -j"$JOBS"

    [ -f "$BOOTSTRAP_TAR" ] || die "bootstrap tarball was not created"
}

##############################################################################
# Debian packaging
##############################################################################

package_current_target() {
    local srcdir
    local pkgname
    local fullver
    local project_version
    local upver
    local origtar
    local rc
    local bootstrap_srcdir
    local deb_build_options
    local lintian_log
    local lintian_help
    local lintian_fail_args=()
    local override_file
    local target_standards_version=""
    local unexpected_lintian
    local build_source_package=0

    msg "preparing Debian package build"

    assert_removable_tree "$BUILDDIR"
    assert_removable_tree "$WORKDIR/bootstrap-from-tar"

    rm -rf "$BUILDDIR"
    run mkdir -p "$BUILDDIR"

    pkgname="$(dpkg-parsechangelog --file "$ROOT/debian/changelog" --show-field Source 2>/dev/null || true)"
    fullver="$(dpkg-parsechangelog --file "$ROOT/debian/changelog" --show-field Version 2>/dev/null || true)"

    [ -n "$pkgname" ] || die "could not parse package name"
    [ -n "$fullver" ] || die "could not parse package version"

    project_version="$(
        sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' \
            "$ROOT/mk/version.mk" | head -n1
    )"
    [ -n "$project_version" ] || die "could not determine FreeBASIC version"

    upver="${fullver%%-*}"
    [ "$upver" = "$project_version" ] || \
        die "Debian package version $fullver does not match FreeBASIC $project_version"

    if [ "$BUILD_ARCH" = "amd64" ] && [ "$CROSS_PACKAGE_BUILD" -eq 0 ]; then
        build_source_package=1
    fi

    srcdir="${pkgname}-${upver}"
    bootstrap_srcdir="$ROOT/bootstrap/$BUILD_BOOTKEY"

    msg "staging Debian source tree"

    if [ ! -d "$bootstrap_srcdir" ]; then
        [ -f "$BOOTSTRAP_TAR" ] || die "missing bootstrap sources: $bootstrap_srcdir and $BOOTSTRAP_TAR"
        assert_removable_tree "$WORKDIR/bootstrap-from-tar"
        run mkdir -p "$WORKDIR/bootstrap-from-tar"
        run tar -xJf "$BOOTSTRAP_TAR" -C "$WORKDIR/bootstrap-from-tar" \
            "FreeBASIC-${upver}-source-bootstrap-${BUILD_BOOTKEY}/bootstrap/${BUILD_BOOTKEY}"
        bootstrap_srcdir="$WORKDIR/bootstrap-from-tar/FreeBASIC-${upver}-source-bootstrap-${BUILD_BOOTKEY}/bootstrap/${BUILD_BOOTKEY}"
    fi

    run mkdir -p "$BUILDDIR/$srcdir"

    run rsync -a --no-owner --no-group \
        --delete --delete-excluded --prune-empty-dirs \
        --exclude-from "$SOURCE_COPY_EXCLUDES" \
        --exclude '/bootstrap/*/' \
        "$ROOT/" "$BUILDDIR/$srcdir/"

    run mkdir -p "$BUILDDIR/$srcdir/bootstrap/$BUILD_BOOTKEY"
    run rsync -a --no-owner --no-group --delete "$bootstrap_srcdir/" "$BUILDDIR/$srcdir/bootstrap/$BUILD_BOOTKEY/"

    cd "$BUILDDIR/$srcdir"

    [ -f debian/control ] || die "missing debian/control"
    [ -f debian/changelog ] || die "missing debian/changelog"
    [ -f GNUmakefile ] || [ -f makefile ] || [ -f Makefile ] || die "missing GNUmakefile/makefile/Makefile"

    if [ "$CODENAME" != "unknown" ]; then
        awk -v distribution="$CODENAME" '
            NR == 1 {
                if (!sub(/\) [^;]+;/, ") " distribution ";")) {
                    exit 1
                }
            }
            { print }
        ' debian/changelog > debian/changelog.tmp
        mv debian/changelog.tmp debian/changelog
    fi

    case "$DISTRO_ID/$CODENAME" in
    ubuntu/jammy)
        target_standards_version="4.6.0"
        ;;
    raspbian/buster)
        target_standards_version="4.3.0"
        ;;
    esac

    if [ -n "$target_standards_version" ]; then
        # Archived validators only know the Policy version shipped with their
        # release. The package does not rely on later policy features, so use
        # the newest version understood by the target's Lintian.
        awk -v target_standards_version="$target_standards_version" '
            BEGIN {
                replacement = "Standards-Version: " target_standards_version
            }
            /^Standards-Version:/ {
                print replacement
                next
            }
            { print }
        ' debian/control > debian/control.tmp
        mv debian/control.tmp debian/control
    fi

    if { [ "$DISTRO_ID" = "ubuntu" ] && [ "$CODENAME" = "jammy" ]; } ||
        { [ "$DISTRO_ID" = "raspbian" ] && [ "$CODENAME" = "buster" ]; }
    then
        #
        # Older Lintian releases treat file names as tag context. Newer
        # releases treat them as item pointers and require square brackets in
        # overrides. Keep the modern form in the source tree and adapt staged
        # packages for the old parser used by Jammy and Buster.
        #
        for override_file in \
            debian/source/lintian-overrides \
            debian/freebasic-nuttx.lintian-overrides
        do
            awk '
                /^(freebasic source: source-is-missing|freebasic-nuttx: national-encoding) \[[^][]+\]$/ {
                    sub(/ \[/, " ")
                    sub(/\]$/, "")
                }
                { print }
            ' "$override_file" > "$override_file.tmp"
            mv "$override_file.tmp" "$override_file"
        done
    fi

    if [ "$ANDROID" -eq 0 ]; then
        # Keep Debian and other non-Android package sets from advertising a
        # sidecar package they did not build.  Ubuntu Android packages are
        # still emitted when the android build profile is enabled.
        awk '$0 == " freebasic-android (= ${binary:Version})," { next } { print }' debian/control > debian/control.tmp
        mv debian/control.tmp debian/control
    fi

    if [ "$NO_JS" -eq 0 ]; then
        [ -f src/tools/js/fbc-js-app ] || die "missing JS app helper: src/tools/js/fbc-js-app"
    fi
    if [ "$ANDROID" -eq 1 ]; then
        [ -f src/tools/android/fbc-android ] || die "missing Android package helper: src/tools/android/fbc-android"
    fi
    if [ "$WII" -eq 1 ]; then
        [ -f src/tools/wii/fbc-wii ] || die "missing Wii package helper: src/tools/wii/fbc-wii"
    fi

    echo "==> package name: $pkgname"
    echo "==> upstream version: $upver"
    echo "==> build arch: $BUILD_ARCH"
    echo "==> host arch: $HOST_ARCH"
    echo "==> output dir: $OUTDIR"
    echo "==> build jobs: $JOBS"
    [ -z "${FBC_PACKAGE_ARM_ARCH:-}" ] || echo "==> ARM default arch: $FBC_PACKAGE_ARM_ARCH"
    [ "$NO_JS" -eq 0 ] || echo "==> build profile: pkg.freebasic.nojs"
    [ "$ANDROID" -eq 0 ] || echo "==> build profile: pkg.freebasic.android"
    [ "$WII" -eq 0 ] || echo "==> build profile: pkg.freebasic.wii"

    cd "$BUILDDIR"

    # Debian source archives must contain the editable sources for every
    # shipped object. Windows compatibility libraries, generated manual
    # tools, and host runtime objects are not used by a Debian build.
    rm -rf "$srcdir/contrib/winlibs-legacy"
    rm -f "$srcdir/contrib/swig/swig.exe"
    rm -f "$srcdir"/doc/manual/*.chm
    rm -f "$srcdir"/lib/linux-*/libgcc_s.so.1
    find \
        "$srcdir/doc/fbchkdoc" \
        "$srcdir/doc/fbdoc" \
        "$srcdir/doc/makefbhelp" \
        -maxdepth 1 -type f -perm /111 -delete
    find "$srcdir/doc/libfbdoc" -maxdepth 1 -type f \
        \( -name '*.o' -o -name '*.a' \) -delete
    if [ -d "$srcdir/lib/freebasic-wii" ]; then
        find "$srcdir/lib/freebasic-wii" -type f \
            \( -name '*.o' -o -name '*.a' \) -delete
    fi

    origtar="${pkgname}_${upver}.orig.tar.xz"
    rm -f "$origtar"
    if [ "$build_source_package" -eq 1 ]; then
        run tar -cJf "$origtar" \
            --exclude="$srcdir/debian" \
            "$srcdir"

        assert_orig_tarball_clean "$origtar"
    else
        echo "==> source package: skipped for binary architecture $BUILD_ARCH"
    fi

    cd "$srcdir"

    if [ -f debian/rules ]; then
        chmod +x debian/rules || true
    fi

    msg "running dpkg-buildpackage"

    local build_profiles=()
    if [ "$NO_JS" -eq 1 ]; then
        build_profiles+=(pkg.freebasic.nojs)
    fi
    if [ "$ANDROID" -eq 1 ]; then
        build_profiles+=(pkg.freebasic.android)
    fi
    if [ "$WII" -eq 1 ]; then
        build_profiles+=(pkg.freebasic.wii)
    fi

    deb_build_options="${DEB_BUILD_OPTIONS:-}"
    case " $deb_build_options " in
        *" parallel="*)
            ;;
        *)
            deb_build_options="${deb_build_options:+$deb_build_options }parallel=$JOBS"
            ;;
    esac

    set +e
    local dpkg_args=(-us -uc)
    if [ "$build_source_package" -eq 0 ]; then
        dpkg_args=(-b "${dpkg_args[@]}")
    fi
    if [ "$CROSS_PACKAGE_BUILD" -eq 1 ]; then
        dpkg_args=(-a "$HOST_ARCH" -d "${dpkg_args[@]}")
    elif [ "${FBC_PACKAGE_SKIP_BUILD_DEPENDS:-0}" -eq 1 ]; then
        dpkg_args=(-d "${dpkg_args[@]}")
    fi

    if [ "${#build_profiles[@]}" -gt 0 ]; then
        DEB_BUILD_OPTIONS="$deb_build_options" DEB_BUILD_PROFILES="${build_profiles[*]}" dpkg-buildpackage "${dpkg_args[@]}" 2>&1 | tee "$OUTDIR/build.log"
    else
        DEB_BUILD_OPTIONS="$deb_build_options" dpkg-buildpackage "${dpkg_args[@]}" 2>&1 | tee "$OUTDIR/build.log"
    fi
    rc=${PIPESTATUS[0]}
    set -e

    if [ "$rc" -ne 0 ]; then
        echo "ERROR: dpkg-buildpackage failed (exit=$rc)"
        tail -200 "$OUTDIR/build.log" || true
        exit "$rc"
    fi

    msg "collecting package artifacts"

    rm -f "$OUTDIR"/freebasic*.deb \
          "$OUTDIR"/freebasic*.ddeb \
          "$OUTDIR"/freebasic*.dsc \
          "$OUTDIR"/freebasic*.tar.* \
          "$OUTDIR"/freebasic*.buildinfo \
          "$OUTDIR"/freebasic*.changes \
          "$OUTDIR"/lintian-freebasic*.log

    shopt -s nullglob
    for f in ../*.deb ../*.ddeb ../*.dsc ../*.tar.* ../*.buildinfo ../*.changes; do
        [ -e "$f" ] || continue
        cp -av "$f" "$OUTDIR/" || true
    done
    shopt -u nullglob

    local changes_files=()
    local changes_file=""
    local lintian_rc=0

    shopt -s nullglob
    changes_files=("$OUTDIR"/*.changes)
    shopt -u nullglob

    if [ "${#changes_files[@]}" -eq 0 ]; then
        echo "ERROR: no .changes file was produced for Lintian"
        exit 1
    fi

    lintian_help="$(lintian --help 2>&1)" || die "could not query Lintian command-line options"

    if grep -Eq -- '(^|[[:space:]])--fail-on([=[:space:]]|$)' <<< "$lintian_help"; then
        lintian_fail_args=(--fail-on "error,warning")
    elif grep -q -- '--fail-on-warnings' <<< "$lintian_help"; then
        #
        # Raspbian Buster provides Lintian 2.15. It predates --fail-on, but
        # its deprecated --fail-on-warnings option returns failure for either
        # warnings or errors. This keeps archived package rows as strict as
        # rows validated by current Lintian releases.
        #
        lintian_fail_args=(--fail-on-warnings)
    else
        die "installed Lintian cannot be configured to fail on warnings"
    fi

    for changes_file in "${changes_files[@]}"; do
        lintian_log="$OUTDIR/lintian-$(basename "$changes_file").log"

        set +e
        lintian -IE --pedantic "${lintian_fail_args[@]}" "$changes_file" 2>&1 |
            tee "$lintian_log"
        lintian_rc=${PIPESTATUS[0]}
        set -e

        unexpected_lintian="$(
            grep -E '^[EW]:' "$lintian_log" |
                grep -Ev '^W: freebasic-dbgsym: elf-error In program headers: Unable to find program interpreter name \[usr/lib/debug/\.build-id/[[:xdigit:]]{2}/[[:xdigit:]]+\.debug\]$' ||
                true
        )"

        if [ -n "$unexpected_lintian" ]; then
            echo "ERROR: Lintian reported errors or warnings for $(basename "$changes_file")"
            printf '%s\n' "$unexpected_lintian"

            if [ "$lintian_rc" -eq 0 ]; then
                lintian_rc=1
            fi

            exit "$lintian_rc"
        fi

        if [ "$lintian_rc" -ne 0 ]; then
            #
            # Binutils debug files retain the executable's program headers
            # while objcopy removes the interpreter bytes. Jammy's Lintian
            # reports that normal dbgsym layout as an elf-error. Accept only
            # that exact warning; every other error or warning still fails.
            #
            if grep -Eq '^W: freebasic-dbgsym: elf-error In program headers: Unable to find program interpreter name \[usr/lib/debug/\.build-id/[[:xdigit:]]{2}/[[:xdigit:]]+\.debug\]$' "$lintian_log"
            then
                echo "==> accepted Jammy Lintian's known dbgsym interpreter warning"
            else
                echo "ERROR: Lintian failed for $(basename "$changes_file") (exit=$lintian_rc)"
                exit "$lintian_rc"
            fi
        fi
    done

    echo
    echo "==> build completed"
    echo "==> artifacts in $OUTDIR"
    ls -lh "$OUTDIR"
}

##############################################################################
# Main
##############################################################################

install_deps

if [ "$NO_BUILD" -eq 0 ]; then
    build_bootstrap_tarball
else
    [ -f "$BOOTSTRAP_TAR" ] || die "missing bootstrap tarball: $BOOTSTRAP_TAR"
fi

if [ "$NO_PACKAGE" -eq 1 ]; then
    msg "bootstrap tarball ready"
    CLEANUP_SUCCESS=1
    echo "==> $BOOTSTRAP_TAR"
    exit 0
fi

package_current_target
CLEANUP_SUCCESS=1
