#!/usr/bin/env bash

# FreeBASIC macOS package builder
# --------------------------------
#
# File: macos-build-freebasic.sh
#
# Purpose:
#
#     Build and package separate x86_64 and arm64 FreeBASIC distributions
#     from either supported macOS host architecture.
#
# Responsibilities:
#
#     - prepare the native and Apple-clang cross toolchains
#     - keep a host-runnable compiler available during cross builds
#     - stage architecture-specific install trees
#     - create tar.xz archives and macOS installer packages
#
# This file intentionally does NOT contain:
#
#     - FreeBASIC compiler or runtime implementation code
#     - universal-binary merging
#     - package installation without an explicit installer invocation

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/build_scripts" ] && [ -f "$SEARCH_DIR/GNUmakefile" ]; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }

cd "$ROOT"

##############################################################################
# Host validation
##############################################################################

[ "$(uname -s)" = "Darwin" ] || { echo "ERROR: this script must run on Darwin/macOS"; exit 1; }

# macOS archive metadata
#
# The package bundles the Command Line Tools SDK. Some SDK headers can deny
# unprivileged reads of extended attributes even though the file contents are
# readable. Apple tar attempts to save that metadata by default and aborts the
# archive with "Failed to get metadata(xattr): Permission denied".
#
# FreeBASIC only needs the file contents, modes, and symlink layout from the
# SDK. Disabling AppleDouble/copyfile metadata and telling tar not to archive
# xattrs keeps package creation from depending on metadata that is not needed by
# the compiler.
export COPYFILE_DISABLE=1

TAR_METADATA_ARGS=(--no-xattrs)

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

SUDO_EXPLANATION_SHOWN=0

print_command_quoted() {
    local arg

    for arg in "$@"; do
        printf ' %q' "$arg"
    done
    printf '\n'
}

explain_sudo_before_first_use() {
    [ "$SUDO_EXPLANATION_SHOWN" -eq 0 ] || return 0
    SUDO_EXPLANATION_SHOWN=1

    echo ""
    echo "==> Administrator privileges required"
    echo "This script is about to run the following command with sudo:"
    printf '    sudo'
    print_command_quoted "$@"
    echo ""
    echo "The macOS build script uses sudo only for system setup steps that"
    echo "macOS itself requires to be administrator-controlled, such as:"
    echo "  - installing Apple Command Line Tools with softwareupdate"
    echo "  - switching xcode-select to /Library/Developer/CommandLineTools"
    echo ""
    echo "The FreeBASIC build, staging tree, tarball creation, and pkgbuild"
    echo "packaging steps run as your normal user."
    echo ""
}

copy_tree_preserve() {
    local src="$1"
    local dst="$2"

    [ -d "$src" ] || die "missing source directory: $src"

    run mkdir -p "$dst"
    (
        cd "$src"
        tar "${TAR_METADATA_ARGS[@]}" -cf - .
    ) | (
        cd "$dst"
        tar "${TAR_METADATA_ARGS[@]}" -xpf -
    )
}

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        explain_sudo_before_first_use "$@"
        run sudo "$@"
    else
        die "this step requires administrator privileges; rerun as root or install sudo"
    fi
}

activate_homebrew() {
    local brew_cmd

    if command -v brew >/dev/null 2>&1; then
        return 0
    fi

    for brew_cmd in /opt/homebrew/bin/brew /usr/local/bin/brew; do
        if [ -x "$brew_cmd" ]; then
            eval "$("$brew_cmd" shellenv)"
            command -v brew >/dev/null 2>&1 && return 0
        fi
    done

    return 1
}

version_sort() {
    if printf '2\n10\n' | sort -V >/dev/null 2>&1; then
        sort -V
    elif command -v gsort >/dev/null 2>&1 && printf '2\n10\n' | gsort -V >/dev/null 2>&1; then
        gsort -V
    else
        sort
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/macos-build-freebasic.sh [options]

Options:
  --arch <all|both|arm64|aarch64|x86_64|native>
                                Build architectures (default: all)
  --skip-deps                   Skip Command Line Tools/Homebrew dependency installation
  --no-build                    Skip compilation and reuse staged artifacts
  --no-package                  Skip package creation
  -h, --help                    Show this help text

Environment:
  BUILDROOT                     Temporary build root. In all mode this is the
                                parent of one directory per architecture.
  OUTBASE                       Output root. In all mode this is the parent of
                                one directory per architecture.
  PREFIX                        Install prefix inside the package (default: /usr/local)
  JOBS                          Parallel make job count (default: sysctl hw.ncpu)
  DARWIN_CROSS_PREFIX           Optional GCC cross prefix for opposite-arch Darwin builds
                                Example: aarch64-apple-darwin or arm64-apple-darwin

Artifacts:
  out/macos/x86_64/freebasic-<version>-<rev>-macos-x86_64.tar.xz
  out/macos/arm64/freebasic-<version>-<rev>-macos-arm64.tar.xz
  out/macos/<arch>/freebasic-<version>-<rev>-macos-<arch>.tar.xz
  out/macos/<arch>/freebasic-<version>-<rev>-macos-<arch>.pkg  (when pkgbuild exists)
EOF
}

##############################################################################
# Options
##############################################################################

TARGET_ARCH="all"
SKIP_DEPS=0
DO_BUILD=1
DO_PACKAGE=1

while [ $# -gt 0 ]; do
    case "$1" in
        --arch)
            [ $# -ge 2 ] || die "--arch requires a value"
            TARGET_ARCH="$2"
            shift
            ;;
        --skip-deps) SKIP_DEPS=1 ;;
        --no-build) DO_BUILD=0 ;;
        --no-package) DO_PACKAGE=0 ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
    shift
done

##############################################################################
# Version / architecture
##############################################################################

FBVERSION="$(awk -F':=' '/^[[:space:]]*FBVERSION/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
REV="$(awk -F':=' '/^[[:space:]]*REV/ {gsub(/[[:space:]]/,"",$2); print $2}' mk/version.mk | head -n1)"
[ -n "$FBVERSION" ] || die "missing FBVERSION"
[ -n "$REV" ] || die "missing REV"

HOST_ARCH_RAW="$(uname -m)"
case "$HOST_ARCH_RAW" in
    arm64|aarch64) HOST_ARCH="arm64" ;;
    x86_64|amd64) HOST_ARCH="x86_64" ;;
    *) die "unsupported host architecture: $HOST_ARCH_RAW" ;;
esac

run_arch_matrix() {
    local other_arch matrix_buildroot matrix_outbase host_cache status
    local -a common_args host_args cross_args

    if [ "$HOST_ARCH" = "arm64" ]; then
        other_arch="x86_64"
    else
        other_arch="arm64"
    fi

    matrix_buildroot="${BUILDROOT:-$ROOT/.build-macos}"
    matrix_outbase="${OUTBASE:-$ROOT/out/macos}"
    host_cache="${matrix_buildroot}/host-tools/${HOST_ARCH}/fbc"

    common_args=()
    [ "$DO_BUILD" -eq 1 ] || common_args+=(--no-build)
    [ "$DO_PACKAGE" -eq 1 ] || common_args+=(--no-package)

    host_args=("${common_args[@]}")
    if [ "$SKIP_DEPS" -eq 1 ]; then
        host_args+=(--skip-deps)
    fi

    cross_args=("${common_args[@]}" --skip-deps)

    msg "building macOS package matrix"
    echo "Host architecture: $HOST_ARCH"
    echo "Second architecture: $other_arch"

    run env \
        BUILDROOT="${matrix_buildroot}/${HOST_ARCH}" \
        OUTBASE="${matrix_outbase}/${HOST_ARCH}" \
        HOST_FBC_CACHE="$host_cache" \
        "$ROOT/build_scripts/macos-build-freebasic.sh" \
        --arch "$HOST_ARCH" "${host_args[@]}"

    if [ "$DO_BUILD" -eq 1 ]; then
        [ -x "$ROOT/bin/fbc" ] || die "native matrix build did not produce bin/fbc"
        run mkdir -p "$(dirname "$host_cache")"
        run cp "$ROOT/bin/fbc" "$host_cache"
        run chmod 755 "$host_cache"
    fi

    if env \
        BUILDROOT="${matrix_buildroot}/${other_arch}" \
        OUTBASE="${matrix_outbase}/${other_arch}" \
        HOST_FBC_CACHE="$host_cache" \
        "$ROOT/build_scripts/macos-build-freebasic.sh" \
        --arch "$other_arch" "${cross_args[@]}"; then
        status=0
    else
        status=$?
    fi

    if [ "$DO_BUILD" -eq 1 ] && [ -x "$host_cache" ]; then
        run mkdir -p "$ROOT/bin"
        run cp "$host_cache" "$ROOT/bin/fbc"
        run chmod 755 "$ROOT/bin/fbc"
    fi

    [ "$status" -eq 0 ] || return "$status"

    if [ "$DO_PACKAGE" -eq 1 ]; then
        msg "matrix artifacts"
        echo "${matrix_outbase}/${HOST_ARCH}"
        echo "${matrix_outbase}/${other_arch}"
    fi
}

case "$TARGET_ARCH" in
    all|both)
        run_arch_matrix
        exit 0
        ;;
esac

if [ "$TARGET_ARCH" = "native" ]; then
    TARGET_ARCH="$HOST_ARCH"
fi

case "$TARGET_ARCH" in
    arm64|aarch64)
        TARGET_ARCH="arm64"
        FBC_TARGET="darwin-aarch64"
        TARGET_TRIPLET="aarch64-apple-darwin"
        CLANG_ARCH="arm64"
        CLANG_TARGET="arm64-apple-macos11"
        ;;
    x86_64)
        FBC_TARGET="darwin-x86_64"
        TARGET_TRIPLET="x86_64-apple-darwin"
        CLANG_ARCH="x86_64"
        CLANG_TARGET="x86_64-apple-macos10.13"
        ;;
    *)
        die "unsupported target architecture: $TARGET_ARCH"
        ;;
esac

VERSION_FULL="${FBVERSION}-${REV}"
PKG_BASENAME="freebasic-${VERSION_FULL}-macos-${TARGET_ARCH}"

##############################################################################
# Paths / tools
##############################################################################

BUILDROOT="${BUILDROOT:-$ROOT/.build-macos/$TARGET_ARCH}"
STAGE="${BUILDROOT}/stage"
PKGROOT="${BUILDROOT}/pkgroot"
PKGSCRIPTS="${BUILDROOT}/pkgscripts"
OUTBASE="${OUTBASE:-$ROOT/out/macos/$TARGET_ARCH}"
PREFIX="${PREFIX:-/usr/local}"
HOST_FBC_CACHE="${HOST_FBC_CACHE:-$ROOT/.build-macos/host-tools/$HOST_ARCH/fbc}"
CROSS_TOOL_ROOT="${CROSS_TOOL_ROOT:-$ROOT/.build-macos/cross-tools/${HOST_ARCH}-to-${TARGET_ARCH}}"
RESTORE_HOST_FBC=0

mkdir -p "$BUILDROOT" "$OUTBASE"

if command -v gmake >/dev/null 2>&1; then
    MAKE_CMD="gmake"
else
    MAKE_CMD="make"
fi

if command -v sysctl >/dev/null 2>&1; then
    JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 1)}"
else
    JOBS="${JOBS:-1}"
fi

ensure_clt_active() {
    xcode-select -p >/dev/null 2>&1 || return 1
    command -v clang >/dev/null 2>&1 || return 1
    command -v xcrun >/dev/null 2>&1 || return 1
    return 0
}

switch_to_clt() {
    [ -d /Library/Developer/CommandLineTools ] || return 1
    run_root xcode-select --switch /Library/Developer/CommandLineTools
}

find_clt_label() {
    softwareupdate --list 2>/dev/null \
        | sed -n 's/^[[:space:]]*[*-][[:space:]]*Label:[[:space:]]*//p' \
        | grep -E 'Command Line Tools|Command Line Developer Tools' \
        | tail -n1
}

install_clt_softwareupdate() {
    local label

    command -v softwareupdate >/dev/null 2>&1 || return 1

    label="$(find_clt_label || true)"
    if [ -z "$label" ]; then
        run touch /tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
        label="$(find_clt_label || true)"
        run rm -f /tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
    fi

    [ -n "$label" ] || return 1

    msg "installing Command Line Tools via softwareupdate"
    run_root softwareupdate --install "$label" --verbose
    switch_to_clt || true
    ensure_clt_active
}

ensure_clt() {
    if ensure_clt_active; then
        return 0
    fi

    if install_clt_softwareupdate; then
        return 0
    fi

    msg "requesting Apple Command Line Tools installer"
    if xcode-select --install >/dev/null 2>&1; then
        echo "Follow the macOS installer dialog, then re-run this script."
    else
        echo "Automatic Command Line Tools install was not available on this machine."
    fi

    die "Apple Command Line Tools are required before continuing"
}

find_macos_sdkroot() {
    local sdkroot

    sdkroot="$(
        xcrun --sdk macosx --show-sdk-path 2>/dev/null \
            || xcrun --show-sdk-path 2>/dev/null \
            || true
    )"

    [ -n "$sdkroot" ] || return 1
    [ -d "$sdkroot/usr/lib" ] || return 1

    printf '%s\n' "$sdkroot"
}

ensure_clt
BREW_PREFIX=""
LIBFFI_PREFIX=""
NCURSES_PREFIX=""
BOOT_FBC_RESULT=""
TOOL_CC=""
TOOL_CXX=""
TOOL_AS=""
TOOL_AR=""
TOOL_RANLIB=""
TOOL_DARWIN_CLANG=""
HOST_TRIPLET=""

resolve_gcc_toolchain() {
    local brew_bin gcc_bin gxx_bin

    brew_bin=""
    gcc_bin=""
    gxx_bin=""

    if activate_homebrew; then
        brew_bin="$(brew --prefix 2>/dev/null || true)/bin"
        if [ -d "$brew_bin" ]; then
            gcc_bin="$(find "$brew_bin" -maxdepth 1 \( -type f -o -type l \) -name 'gcc-*' | grep -E '/gcc-[0-9]+$' | version_sort | tail -n1 || true)"
            gxx_bin="$(find "$brew_bin" -maxdepth 1 \( -type f -o -type l \) -name 'g++-*' | grep -E '/g[+][+]-[0-9]+$' | version_sort | tail -n1 || true)"
        fi
    fi

    if [ -z "$gcc_bin" ] || [ -z "$gxx_bin" ]; then
        return 1
    fi

    TOOL_CC="$gcc_bin"
    TOOL_CXX="$gxx_bin"
    HOST_TRIPLET="$("$TOOL_CC" -dumpmachine 2>/dev/null || true)"
    if [ -z "$HOST_TRIPLET" ]; then
        HOST_TRIPLET="${HOST_ARCH_RAW}-apple-darwin"
    fi
}

resolve_cross_gcc_toolchain() {
    local prefix gcc_bin gxx_bin

    prefix="${DARWIN_CROSS_PREFIX:-}"
    [ -n "$prefix" ] || return 1

    gcc_bin="$(command -v "${prefix}-gcc" 2>/dev/null || true)"
    gxx_bin="$(command -v "${prefix}-g++" 2>/dev/null || true)"

    [ -n "$gcc_bin" ] || return 1
    [ -n "$gxx_bin" ] || return 1

    TOOL_CC="$gcc_bin"
    TOOL_CXX="$gxx_bin"
    TOOL_AS="$(command -v "${prefix}-as" 2>/dev/null || true)"
    TOOL_AR="$(command -v "${prefix}-ar" 2>/dev/null || true)"
    TOOL_RANLIB="$(command -v "${prefix}-ranlib" 2>/dev/null || true)"

    [ -n "$TOOL_AS" ] || return 1
    [ -n "$TOOL_AR" ] || return 1
    [ -n "$TOOL_RANLIB" ] || return 1

    DARWIN_CROSS_PREFIX="$prefix"
    return 0
}

create_clang_cross_toolchain() {
    local clang clangxx ar ranlib cross_cc cross_cxx cross_as

    clang="$(xcrun --find clang 2>/dev/null || true)"
    clangxx="$(xcrun --find clang++ 2>/dev/null || true)"
    ar="$(xcrun --find ar 2>/dev/null || true)"
    ranlib="$(xcrun --find ranlib 2>/dev/null || true)"

    [ -x "$clang" ] || die "Apple clang was not found through xcrun"
    [ -x "$clangxx" ] || die "Apple clang++ was not found through xcrun"
    [ -x "$ar" ] || die "Apple ar was not found through xcrun"
    [ -x "$ranlib" ] || die "Apple ranlib was not found through xcrun"

    cross_cc="${CROSS_TOOL_ROOT}/cc"
    cross_cxx="${CROSS_TOOL_ROOT}/cxx"
    cross_as="${CROSS_TOOL_ROOT}/as"

    run mkdir -p "$CROSS_TOOL_ROOT"

    cat > "$cross_cc" <<EOF
#!/usr/bin/env bash

set -euo pipefail

exec "$clang" -target "$CLANG_TARGET" "\$@"

# end of cc
EOF

    cat > "$cross_cxx" <<EOF
#!/usr/bin/env bash

set -euo pipefail

exec "$clangxx" -target "$CLANG_TARGET" "\$@"

# end of cxx
EOF

    cat > "$cross_as" <<EOF
#!/usr/bin/env bash

set -euo pipefail

# fbc normally invokes the host assembler directly.  Compile the emitted
# assembly through clang so the target triple is explicit during a cross build.
exec "$clang" -target "$CLANG_TARGET" -c "\$@"

# end of as
EOF

    run chmod 755 "$cross_cc" "$cross_cxx" "$cross_as"

    TOOL_CC="$cross_cc"
    TOOL_CXX="$cross_cxx"
    TOOL_AS="$cross_as"
    TOOL_AR="$ar"
    TOOL_RANLIB="$ranlib"
    TOOL_DARWIN_CLANG="$cross_cc"
}

select_build_toolchain() {
    if ! resolve_gcc_toolchain; then
        TOOL_CC="$(xcrun --find clang)"
        TOOL_CXX="$(xcrun --find clang++)"
        HOST_TRIPLET="$HOST_ARCH_RAW-apple-darwin"
    fi

    if [ "$TARGET_ARCH" = "$HOST_ARCH" ]; then
        return 0
    fi

    create_clang_cross_toolchain

    if [ -n "${DARWIN_CROSS_PREFIX:-}" ]; then
        resolve_cross_gcc_toolchain || die "DARWIN_CROSS_PREFIX does not provide a complete GCC, G++, assembler, archiver, and ranlib toolchain"
    fi
}

refresh_make_vars() {

    BREW_PREFIX=""
    LIBFFI_PREFIX=""
    NCURSES_PREFIX=""

    if [ "$TARGET_ARCH" = "$HOST_ARCH" ] && activate_homebrew; then
        BREW_PREFIX="$(brew --prefix 2>/dev/null || true)"
        LIBFFI_PREFIX="$(brew --prefix libffi 2>/dev/null || true)"
        NCURSES_PREFIX="$(brew --prefix ncurses 2>/dev/null || true)"
    fi

    BASE_PKG_CONFIG_PATH=""

    if [ -n "$LIBFFI_PREFIX" ]; then
        BASE_PKG_CONFIG_PATH="${LIBFFI_PREFIX}/lib/pkgconfig"
    fi

    if [ -n "$NCURSES_PREFIX" ]; then
        if [ -n "$BASE_PKG_CONFIG_PATH" ]; then
            BASE_PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}:"
        fi
        BASE_PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}${NCURSES_PREFIX}/lib/pkgconfig"
    fi

    MAKE_VARS=(
        "CC=${TOOL_CC}"
        "CXX=${TOOL_CXX}"
        "HOST_TRIPLET=${HOST_TRIPLET}"
    )

    if [ "$TARGET_ARCH" = "$HOST_ARCH" ]; then
        MAKE_VARS+=("TARGET_TRIPLET=${HOST_TRIPLET}")
    else
        MAKE_VARS+=(
            "TARGET_TRIPLET=${TARGET_TRIPLET}"
            "AS=${TOOL_AS}"
            "AR=${TOOL_AR}"
            "RANLIB=${TOOL_RANLIB}"
            "LD=${TOOL_CC}"
            "CLANG=${TOOL_DARWIN_CLANG}"
            "DARWIN_CLANG=${TOOL_DARWIN_CLANG}"
        )
        if [ "$DO_BUILD" -eq 1 ]; then
            MAKE_VARS+=("CPPFLAGS=${CPPFLAGS:-} -I$(find_macos_sdkroot)/usr/include/ffi")
        fi
        if [ -n "${DARWIN_CROSS_PREFIX:-}" ]; then
            MAKE_VARS+=("BUILD_PREFIX=${DARWIN_CROSS_PREFIX}-")
        else
            MAKE_VARS+=("BUILD_PREFIX=")
        fi
    fi
}

# Tool selection is repeated after dependency setup in the build path.  These
# placeholders let package-only runs inspect an existing stage without
# requiring Command Line Tools before the script has had a chance to install
# them.
TOOL_CC="${CC:-cc}"
TOOL_CXX="${CXX:-c++}"
TOOL_AS="${AS:-as}"
TOOL_AR="${AR:-ar}"
TOOL_RANLIB="${RANLIB:-ranlib}"
HOST_TRIPLET="${HOST_ARCH_RAW}-apple-darwin"
MAKE_VARS=()
if [ "$DO_BUILD" -eq 0 ]; then
    refresh_make_vars
fi

if [ "$TARGET_ARCH" != "$HOST_ARCH" ]; then
    run mkdir -p "$CROSS_TOOL_ROOT/pkgconfig-empty"
    export PKG_CONFIG_PATH=""
    export PKG_CONFIG_LIBDIR="$CROSS_TOOL_ROOT/pkgconfig-empty"
fi

##############################################################################
# Dependencies
##############################################################################

ensure_homebrew() {
    if activate_homebrew; then
        return 0
    fi

    msg "installing Homebrew"
    NONINTERACTIVE=1 /bin/bash -c \
        "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    if [ -x /opt/homebrew/bin/brew ]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [ -x /usr/local/bin/brew ]; then
        eval "$(/usr/local/bin/brew shellenv)"
    fi

    command -v brew >/dev/null 2>&1 || die "Homebrew installation failed"
}

install_deps() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "checking Apple command line tools"
    ensure_clt

    msg "checking Homebrew"
    ensure_homebrew

    msg "installing macOS build dependencies"
    run brew update
    run brew install make pkg-config xz dos2unix gnu-sed coreutils rsync gcc libffi ncurses

    if command -v gmake >/dev/null 2>&1; then
        MAKE_CMD="gmake"
    fi

    activate_homebrew || die "Homebrew was installed but is not available on PATH"
}

##############################################################################
# Bootstrap helper
##############################################################################

detect_boot_fbc() {
    local candidate

    for candidate in "$ROOT/bootstrap/fbc" "$ROOT/bin/fbc"; do
        if [ -x "$candidate" ] && "$candidate" -version >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    if command -v fbc >/dev/null 2>&1; then
        candidate="$(command -v fbc)"
        if "$candidate" -version >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    fi

    return 1
}

preserve_host_fbc() {
    local candidate

    if [ -x "$HOST_FBC_CACHE" ] && "$HOST_FBC_CACHE" -version >/dev/null 2>&1; then
        return 0
    fi

    candidate="$(detect_boot_fbc)" || die "a host-runnable fbc is required before cross-building ${TARGET_ARCH}"

    run mkdir -p "$(dirname "$HOST_FBC_CACHE")"
    run cp "$candidate" "$HOST_FBC_CACHE"
    run chmod 755 "$HOST_FBC_CACHE"

    "$HOST_FBC_CACHE" -version >/dev/null 2>&1 || die "preserved compiler is not runnable on the host"
}

restore_host_fbc_on_exit() {
    local status

    status=$?
    trap - EXIT

    if [ "$RESTORE_HOST_FBC" -eq 1 ]; then
        echo "==> restoring host compiler to $ROOT/bin/fbc"
        mkdir -p "$ROOT/bin" || status=1
        cp "$HOST_FBC_CACHE" "$ROOT/bin/fbc" || status=1
        chmod 755 "$ROOT/bin/fbc" || status=1
    fi

    exit "$status"
}

have_bootstrap_sources() {
    [ -d "$ROOT/bootstrap/$FBC_TARGET" ] || return 1
    find "$ROOT/bootstrap/$FBC_TARGET" -maxdepth 1 \
        \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .
}

bootstrap_if_needed() {
    local boot_fbc

    if boot_fbc="$(detect_boot_fbc)"; then
        if ! have_bootstrap_sources; then
            msg "emitting Darwin bootstrap sources with ${boot_fbc}"
            run "$MAKE_CMD" -f GNUmakefile "${MAKE_VARS[@]}" "BOOT_FBC=${boot_fbc}" "BUILD_FBC=${boot_fbc}" bootstrap-emit
        fi
        BOOT_FBC_RESULT="$boot_fbc"
        return 0
    fi

    if have_bootstrap_sources; then
        msg "using existing Darwin bootstrap sources in bootstrap/${FBC_TARGET}"
        BOOT_FBC_RESULT="$ROOT/bootstrap/fbc"
        return 0
    fi

    msg "no runnable fbc found, seeding Darwin bootstrap from peer sources"
    run "$MAKE_CMD" -f GNUmakefile "${MAKE_VARS[@]}" \
        "BOOT_FBC=$ROOT/bootstrap/fbc" \
        "BUILD_FBC=$ROOT/bootstrap/fbc" \
        bootstrap-seed-peer

    boot_fbc="$(detect_boot_fbc)" || die "bootstrap compiler still unavailable after peer seeding"
    BOOT_FBC_RESULT="$boot_fbc"
}

##############################################################################
# Packaging helpers
##############################################################################

bundle_toolchain_into_stage() {
    local stage_prefix stage_libexec bundle_root ncurses_root sdk_root
    local real_fbc wrapper_fbc helper_script old_ncurses bundled_ncurses

    stage_prefix="${STAGE}${PREFIX}"
    stage_libexec="${stage_prefix}/lib/freebasic/libexec"
    bundle_root="${stage_prefix}/lib/freebasic/toolchain"

    [ -x "${stage_prefix}/bin/fbc" ] || die "staged compiler missing before toolchain bundling"
    sdk_root="$(find_macos_sdkroot)" || die "macOS SDK not found through xcrun"

    msg "bundling macOS toolchain into staged package"
    run rm -rf "$bundle_root"
    run mkdir -p "$bundle_root"

    # Homebrew's GCC formula is not a self-contained directory.  Its internal
    # compiler programs load libraries from separate Homebrew formulas, and a
    # copy made on one architecture cannot serve the other architecture.  The
    # installed wrapper therefore uses the target Mac's Apple clang through
    # xcrun.  The package installer already checks for Command Line Tools.
    if [ "$TARGET_ARCH" = "$HOST_ARCH" ]; then
        [ -n "$NCURSES_PREFIX" ] || die "ncurses prefix unavailable for native macOS packaging"

        ncurses_root="$(cd "$NCURSES_PREFIX" && pwd -P)"

        copy_tree_preserve "$ncurses_root" "${bundle_root}/ncurses"
    else
        echo "Cross-architecture package will use the target Mac's Apple clang and system libraries."
    fi

    copy_tree_preserve "$sdk_root" "${bundle_root}/sdk/MacOSX.sdk"

    real_fbc="${stage_libexec}/fbc-real"
    wrapper_fbc="${stage_prefix}/bin/fbc"
    helper_script="${stage_prefix}/bin/fbc-setup-darwin"

    run mkdir -p "$stage_libexec"
    if [ ! -x "$real_fbc" ] || ! sed -n '1p' "$wrapper_fbc" 2>/dev/null | grep -q '^#!/usr/bin/env bash'; then
        run mv "$wrapper_fbc" "$real_fbc"
    fi

    cat > "$wrapper_fbc" <<EOF
#!/usr/bin/env bash
set -euo pipefail

SELF_DIR="\$(CDPATH= cd -- "\$(dirname "\$0")" && pwd)"
PREFIX_ROOT="\$(cd "\$SELF_DIR/.." && pwd)"
FBROOT="\$PREFIX_ROOT/lib/freebasic"
TOOLCHAIN_ROOT="\$FBROOT/toolchain"
NCURSES_ROOT="\$TOOLCHAIN_ROOT/ncurses"
SDK_ROOT="\$TOOLCHAIN_ROOT/sdk/MacOSX.sdk"

if command -v xcrun >/dev/null 2>&1; then
    export GCC="\$(xcrun --find clang)"
    export CLANG="\$GCC"
    export AS="\$(xcrun --find as)"
    export AR="\$(xcrun --find ar)"
    export LD="\$(xcrun --find ld)"
else
    echo "ERROR: Apple Command Line Tools are required. Run fbc-setup-darwin first." >&2
    exit 1
fi

if [ -d "\$NCURSES_ROOT/lib" ]; then
    export DYLD_LIBRARY_PATH="\$NCURSES_ROOT/lib\${DYLD_LIBRARY_PATH:+:\$DYLD_LIBRARY_PATH}"
fi

if [ -d "\$SDK_ROOT/usr/lib" ]; then
    export SDKROOT="\$SDK_ROOT"
elif command -v xcrun >/dev/null 2>&1; then
    SDKROOT_CANDIDATE="\$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || xcrun --show-sdk-path 2>/dev/null || true)"
    if [ -n "\$SDKROOT_CANDIDATE" ] && [ -d "\$SDKROOT_CANDIDATE/usr/lib" ]; then
        export SDKROOT="\$SDKROOT_CANDIDATE"
    fi
fi

exec "\$FBROOT/libexec/fbc-real" -prefix "\$PREFIX_ROOT" "\$@"
EOF
    run chmod 755 "$wrapper_fbc"

    cat > "$helper_script" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

run() { echo "==> $*"; "$@"; }

if xcode-select -p >/dev/null 2>&1 && command -v xcrun >/dev/null 2>&1; then
    echo "Apple Command Line Tools already installed."
    exit 0
fi

if command -v softwareupdate >/dev/null 2>&1; then
    label="$(
        softwareupdate --list 2>/dev/null \
            | sed -n 's/^[[:space:]]*[*-][[:space:]]*Label:[[:space:]]*//p' \
            | grep -E 'Command Line Tools|Command Line Developer Tools' \
            | tail -n1
    )"
    if [ -n "${label:-}" ]; then
        run sudo softwareupdate --install "$label" --verbose
        [ -d /Library/Developer/CommandLineTools ] && run sudo xcode-select --switch /Library/Developer/CommandLineTools
    fi
fi

if ! xcode-select -p >/dev/null 2>&1; then
    echo "Requesting Apple Command Line Tools installer..."
    xcode-select --install || true
    echo "Finish the Apple installer, then rerun your FreeBASIC build."
fi
EOF
    run chmod 755 "$helper_script"

    if [ -n "$NCURSES_PREFIX" ]; then
        old_ncurses="${NCURSES_PREFIX}/lib/libncursesw.6.dylib"
    else
        old_ncurses=""
    fi
    bundled_ncurses="@executable_path/../toolchain/ncurses/lib/libncursesw.6.dylib"
    if [ -n "$old_ncurses" ] && command -v install_name_tool >/dev/null 2>&1 && [ -f "${bundle_root}/ncurses/lib/libncursesw.6.dylib" ]; then
        echo "==> install_name_tool -change $old_ncurses $bundled_ncurses $real_fbc"
        if ! install_name_tool -change "$old_ncurses" "$bundled_ncurses" "$real_fbc" 2>"${BUILDROOT}/install_name_tool-fbc-real.log"; then
            echo "NOTE: install_name_tool could not rewrite fbc-real's ncurses load path; the public fbc wrapper will use the bundled ncurses via DYLD_LIBRARY_PATH" >&2
        fi
    fi
}

verify_staged_architecture() {
    local stage_prefix real_fbc runtime_dir file actual checked

    stage_prefix="${STAGE}${PREFIX}"
    real_fbc="${stage_prefix}/lib/freebasic/libexec/fbc-real"
    runtime_dir="${stage_prefix}/lib/freebasic/${FBC_TARGET}"
    checked=0

    [ -x "$real_fbc" ] || die "staged real compiler missing: $real_fbc"
    [ -d "$runtime_dir" ] || die "staged runtime directory missing: $runtime_dir"

    while IFS= read -r file; do
        actual="$(lipo -archs "$file" 2>/dev/null || true)"
        [ "$actual" = "$CLANG_ARCH" ] || die "wrong architecture in staged file $file: expected $CLANG_ARCH, found ${actual:-unknown}"
        checked=$((checked + 1))
    done < <(
        printf '%s\n' "$real_fbc"
        find "$runtime_dir" -maxdepth 1 -type f \( -name '*.o' -o -name '*.a' \) -print | sort
    )

    [ "$checked" -gt 1 ] || die "no staged runtime objects were available for architecture verification"

    if [ "$TARGET_ARCH" != "$HOST_ARCH" ]; then
        if otool -L "$real_fbc" | sed '1d' | grep -E '/usr/local/|/opt/homebrew/' >/dev/null 2>&1; then
            die "cross-built compiler links against host Homebrew libraries"
        fi
    fi

    echo "Verified $checked staged compiler/runtime files as $CLANG_ARCH"
}

create_pkg_scripts() {
    local postinstall

    run rm -rf "$PKGSCRIPTS"
    run mkdir -p "$PKGSCRIPTS"
    postinstall="${PKGSCRIPTS}/postinstall"

    cat > "$postinstall" <<EOF
#!/usr/bin/env bash
set -euo pipefail

PREFIX_ROOT="${PREFIX}"
HELPER="\${PREFIX_ROOT}/bin/fbc-setup-darwin"

if xcode-select -p >/dev/null 2>&1 && command -v xcrun >/dev/null 2>&1; then
    exit 0
fi

if [ -x "\$HELPER" ]; then
    "\$HELPER" || true
fi

exit 0
EOF

    run chmod 755 "$postinstall"
}

##############################################################################
# Build
##############################################################################

if [ "$DO_BUILD" -eq 1 ]; then
    BUILD_COMPILER=""
    BUILD_COMPILER_ARGS=()

    install_deps
    ensure_clt
    select_build_toolchain
    refresh_make_vars
    if [ -n "$BASE_PKG_CONFIG_PATH" ]; then
        export PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    fi

    if [ "$TARGET_ARCH" != "$HOST_ARCH" ]; then
        preserve_host_fbc
        RESTORE_HOST_FBC=1
        trap restore_host_fbc_on_exit EXIT
    fi

    msg "cleaning previous Darwin build artifacts"
    run rm -rf "$STAGE" "$PKGROOT"
    run rm -rf "$BUILDROOT"
    run mkdir -p "$BUILDROOT" "$OUTBASE"
    run rm -rf "$ROOT/bin" "$ROOT/obj"
    run rm -rf "$ROOT/src/compiler/obj/$FBC_TARGET"
    run rm -rf "$ROOT/src/rtlib/obj/$FBC_TARGET"
    run rm -rf "$ROOT/src/gfxlib2/obj/$FBC_TARGET"
    run rm -rf "$ROOT/src/sfxlib/obj/$FBC_TARGET"
    run rm -rf "$ROOT/lib/freebasic/$FBC_TARGET"

    if [ "$TARGET_ARCH" = "$HOST_ARCH" ]; then
        bootstrap_if_needed
        BOOT_FBC="$BOOT_FBC_RESULT"

        msg "building bootstrap compiler for ${FBC_TARGET}"
        run "$MAKE_CMD" -f GNUmakefile -j"$JOBS" "${MAKE_VARS[@]}" "BOOT_FBC=${BOOT_FBC}" "BUILD_FBC=${BOOT_FBC}" bootstrap-minimal

        BUILD_COMPILER="$ROOT/bootstrap/fbc"
        [ -x "$BUILD_COMPILER" ] || die "bootstrap compiler was not produced at $BUILD_COMPILER"
        BUILD_COMPILER_ARGS=(
            "FBC=${BUILD_COMPILER}"
            "BUILD_FBC=${BUILD_COMPILER}"
        )
    else
        BUILD_COMPILER="$HOST_FBC_CACHE"
        BUILD_COMPILER_ARGS=(
            "FBC=${BUILD_COMPILER}"
            "BUILD_FBC=${BUILD_COMPILER}"
            "BUILD_FBC_TARGET=${FBC_TARGET}"
            "BUILD_FBC_BUILDPREFIX="
        )
    fi

    msg "building FreeBASIC for ${FBC_TARGET}"
    run "$MAKE_CMD" -f GNUmakefile -j"$JOBS" "${MAKE_VARS[@]}" "${BUILD_COMPILER_ARGS[@]}" all

    msg "staging install tree"
    run mkdir -p "$STAGE"
    run "$MAKE_CMD" -f GNUmakefile "${MAKE_VARS[@]}" "${BUILD_COMPILER_ARGS[@]}" install "DESTDIR=${STAGE}" "prefix=${PREFIX}"
    bundle_toolchain_into_stage
    verify_staged_architecture
fi

##############################################################################
# Package
##############################################################################

if [ "$DO_PACKAGE" -eq 1 ]; then
    [ -x "$STAGE$PREFIX/bin/fbc" ] || die "staged compiler missing: $STAGE$PREFIX/bin/fbc"

    if [ ! -x "$STAGE$PREFIX/lib/freebasic/libexec/fbc-real" ] || \
        [ ! -d "$STAGE$PREFIX/lib/freebasic/toolchain" ] || \
        [ ! -d "$STAGE$PREFIX/lib/freebasic/toolchain/sdk/MacOSX.sdk/usr/lib" ]; then
        bundle_toolchain_into_stage
    fi

    verify_staged_architecture

    TAR_FILE="$OUTBASE/${PKG_BASENAME}.tar.xz"
    PKG_FILE="$OUTBASE/${PKG_BASENAME}.pkg"
    INSTALL_SH="$OUTBASE/install.sh"

    if command -v pkgbuild >/dev/null 2>&1; then
        msg "creating tar.xz package"
        run rm -f "$TAR_FILE"
        run tar "${TAR_METADATA_ARGS[@]}" -C "$STAGE" -cJf "$TAR_FILE" .
        [ -f "$TAR_FILE" ] || die "tar package was not created: $TAR_FILE"

        msg "creating macOS installer package"
        run rm -f "$PKG_FILE"
        run rm -rf "$PKGROOT"
        copy_tree_preserve "$STAGE" "$PKGROOT"
        create_pkg_scripts
        PKGBUILD_ARGS=(
            --root "$PKGROOT"
            --scripts "$PKGSCRIPTS"
            --identifier "org.freebasic.compiler"
            --version "$VERSION_FULL"
            --install-location "/"
            "$PKG_FILE"
        )
        run pkgbuild "${PKGBUILD_ARGS[@]}"
        [ -f "$PKG_FILE" ] || die "macOS installer package was not created: $PKG_FILE"

        msg "writing installer helper script"
        cat > "$INSTALL_SH" <<EOF
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="\$(CDPATH= cd -- "\$(dirname "\$0")" && pwd)"
PKG_FILE="\$SCRIPT_DIR/${PKG_BASENAME}.pkg"

[ -f "\$PKG_FILE" ] || {
    echo "ERROR: package not found: \$PKG_FILE" >&2
    exit 1
}

echo "This helper is about to run the following command with sudo:"
echo "    sudo installer -pkg \"\$PKG_FILE\" -target /"
echo ""
echo "That installs the FreeBASIC macOS package into the package's"
echo "declared system install location. It does not rebuild FreeBASIC"
echo "or modify the package artifact."
echo ""

exec sudo installer -pkg "\$PKG_FILE" -target /
EOF
        run chmod 755 "$INSTALL_SH"
    else
        msg "creating tar.xz package"
        run tar "${TAR_METADATA_ARGS[@]}" -C "$STAGE" -cJf "$TAR_FILE" .
        [ -f "$TAR_FILE" ] || die "tar package was not created: $TAR_FILE"
        echo "WARNING: pkgbuild not found; skipped .pkg creation"
    fi

    msg "artifacts"
    [ -f "$TAR_FILE" ] && echo "$TAR_FILE"
    [ -f "$PKG_FILE" ] && echo "$PKG_FILE"
    [ -f "$INSTALL_SH" ] && echo "$INSTALL_SH"
fi

# end of macos-build-freebasic.sh
