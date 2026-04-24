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

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

copy_tree_preserve() {
    local src="$1"
    local dst="$2"

    [ -d "$src" ] || die "missing source directory: $src"

    run mkdir -p "$dst"
    (
        cd "$src"
        tar -cf - .
    ) | (
        cd "$dst"
        tar -xpf -
    )
}

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires administrator privileges; rerun as root or install sudo"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/macos-build-freebasic.sh [options]

Options:
  --arch <arm64|x86_64|native>  Build architecture (default: native host arch)
  --skip-deps                   Skip Command Line Tools/Homebrew dependency installation
  --no-build                    Skip compilation and reuse staged artifacts
  --no-package                  Skip package creation
  -h, --help                    Show this help text

Environment:
  BUILDROOT                     Temporary build root (default: <repo>/.build-macos/<arch>)
  OUTBASE                       Output root (default: <repo>/out/macos/<arch>)
  PREFIX                        Install prefix inside the package (default: /usr/local)
  JOBS                          Parallel make job count (default: sysctl hw.ncpu)
  DARWIN_CROSS_PREFIX           Optional GCC cross prefix for opposite-arch Darwin builds
                                Example: aarch64-apple-darwin or arm64-apple-darwin

Artifacts:
  out/macos/<arch>/freebasic-<version>-<rev>-macos-<arch>.tar.xz
  out/macos/<arch>/freebasic-<version>-<rev>-macos-<arch>.pkg  (when pkgbuild exists)
EOF
}

##############################################################################
# Options
##############################################################################

TARGET_ARCH="native"
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

if [ "$TARGET_ARCH" = "native" ]; then
    TARGET_ARCH="$HOST_ARCH"
fi

case "$TARGET_ARCH" in
    arm64)
        FBC_TARGET="darwin-aarch64"
        TARGET_TRIPLET="aarch64-apple-darwin"
        CLANG_ARCH="arm64"
        ;;
    x86_64)
        FBC_TARGET="darwin-x86_64"
        TARGET_TRIPLET="x86_64-apple-darwin"
        CLANG_ARCH="x86_64"
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

ensure_clt
SDK_PATH="$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || true)"

BREW_PREFIX=""
LIBFFI_PREFIX=""
NCURSES_PREFIX=""
BOOT_FBC_RESULT=""
TOOL_CC=""
TOOL_CXX=""
HOST_TRIPLET=""

resolve_gcc_toolchain() {
    local brew_bin gcc_bin gxx_bin

    brew_bin=""
    gcc_bin=""
    gxx_bin=""

    if command -v brew >/dev/null 2>&1; then
        brew_bin="$(brew --prefix 2>/dev/null || true)/bin"
        if [ -d "$brew_bin" ]; then
            gcc_bin="$(find "$brew_bin" -maxdepth 1 \( -type f -o -type l \) -name 'gcc-*' | grep -E '/gcc-[0-9]+$' | sort -V | tail -n1 || true)"
            gxx_bin="$(find "$brew_bin" -maxdepth 1 \( -type f -o -type l \) -name 'g++-*' | grep -E '/g[+][+]-[0-9]+$' | sort -V | tail -n1 || true)"
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
    local prefix gcc_bin gxx_bin triplet_guess

    prefix="${DARWIN_CROSS_PREFIX:-}"
    if [ -z "$prefix" ]; then
        if [ "$TARGET_ARCH" = "arm64" ]; then
            for triplet_guess in aarch64-apple-darwin arm64-apple-darwin; do
                if command -v "${triplet_guess}-gcc" >/dev/null 2>&1 && command -v "${triplet_guess}-g++" >/dev/null 2>&1; then
                    prefix="$triplet_guess"
                    break
                fi
            done
        elif [ "$TARGET_ARCH" = "x86_64" ]; then
            for triplet_guess in x86_64-apple-darwin amd64-apple-darwin; do
                if command -v "${triplet_guess}-gcc" >/dev/null 2>&1 && command -v "${triplet_guess}-g++" >/dev/null 2>&1; then
                    prefix="$triplet_guess"
                    break
                fi
            done
        fi
    fi

    [ -n "$prefix" ] || return 1

    gcc_bin="$(command -v "${prefix}-gcc" 2>/dev/null || true)"
    gxx_bin="$(command -v "${prefix}-g++" 2>/dev/null || true)"

    [ -n "$gcc_bin" ] || return 1
    [ -n "$gxx_bin" ] || return 1

    TOOL_CC="$gcc_bin"
    TOOL_CXX="$gxx_bin"
    HOST_TRIPLET="$("$TOOL_CC" -dumpmachine 2>/dev/null || true)"
    [ -n "$HOST_TRIPLET" ] || HOST_TRIPLET="${prefix}"
    DARWIN_CROSS_PREFIX="$prefix"
    return 0
}

refresh_make_vars() {
    local cc_basename cxx_basename

    BREW_PREFIX=""
    LIBFFI_PREFIX=""
    NCURSES_PREFIX=""

    if command -v brew >/dev/null 2>&1; then
        BREW_PREFIX="$(brew --prefix 2>/dev/null || true)"
        LIBFFI_PREFIX="$(brew --prefix libffi 2>/dev/null || true)"
        NCURSES_PREFIX="$(brew --prefix ncurses 2>/dev/null || true)"
    fi

    BASE_CPPFLAGS=""
    BASE_CFLAGS=""
    BASE_CXXFLAGS=""
    BASE_LDFLAGS=""
    BASE_PKG_CONFIG_PATH=""

    cc_basename="$(basename "$TOOL_CC" 2>/dev/null || printf '%s' "$TOOL_CC")"
    cxx_basename="$(basename "$TOOL_CXX" 2>/dev/null || printf '%s' "$TOOL_CXX")"

    # Homebrew GCC already carries a configured macOS sysroot. Passing an
    # extra -isysroot here breaks Apple headers during rtlib/bootstrap builds.
    if [ -n "$SDK_PATH" ] && [[ ! "$cc_basename" =~ ^gcc(-[0-9]+)?$ ]] && [[ ! "$cxx_basename" =~ ^g\+\+(-[0-9]+)?$ ]]; then
        BASE_CFLAGS="${BASE_CFLAGS} -isysroot ${SDK_PATH}"
        BASE_CXXFLAGS="${BASE_CXXFLAGS} -isysroot ${SDK_PATH}"
        BASE_LDFLAGS="${BASE_LDFLAGS} -isysroot ${SDK_PATH}"
    fi

    if [ -n "$LIBFFI_PREFIX" ]; then
        BASE_CPPFLAGS="${BASE_CPPFLAGS} -I${LIBFFI_PREFIX}/include"
        BASE_CFLAGS="${BASE_CFLAGS} -I${LIBFFI_PREFIX}/include"
        BASE_CXXFLAGS="${BASE_CXXFLAGS} -I${LIBFFI_PREFIX}/include"
        BASE_LDFLAGS="${BASE_LDFLAGS} -L${LIBFFI_PREFIX}/lib"
        BASE_PKG_CONFIG_PATH="${LIBFFI_PREFIX}/lib/pkgconfig"
    fi

    if [ -n "$NCURSES_PREFIX" ]; then
        BASE_CPPFLAGS="${BASE_CPPFLAGS} -I${NCURSES_PREFIX}/include"
        BASE_CFLAGS="${BASE_CFLAGS} -I${NCURSES_PREFIX}/include"
        BASE_CXXFLAGS="${BASE_CXXFLAGS} -I${NCURSES_PREFIX}/include"
        BASE_LDFLAGS="${BASE_LDFLAGS} -L${NCURSES_PREFIX}/lib"
        if [ -n "$BASE_PKG_CONFIG_PATH" ]; then
            BASE_PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}:"
        fi
        BASE_PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}${NCURSES_PREFIX}/lib/pkgconfig"
    fi

    MAKE_VARS=(
        "CC=${TOOL_CC}"
        "CXX=${TOOL_CXX}"
        "CPPFLAGS=${BASE_CPPFLAGS}"
        "CFLAGS=${BASE_CFLAGS}"
        "CXXFLAGS=${BASE_CXXFLAGS}"
        "LDFLAGS=${BASE_LDFLAGS}"
    )

    if [ "$TARGET_ARCH" = "$HOST_ARCH" ]; then
        MAKE_VARS+=("TARGET_TRIPLET=${HOST_TRIPLET}")
    else
        MAKE_VARS+=("TARGET_TRIPLET=${TARGET_TRIPLET}")
        if [ -n "${DARWIN_CROSS_PREFIX:-}" ]; then
            MAKE_VARS+=("BUILD_PREFIX=${DARWIN_CROSS_PREFIX}-")
        else
            MAKE_VARS+=("BUILD_PREFIX=")
        fi
    fi
}

if ! resolve_gcc_toolchain; then
    TOOL_CC="gcc"
    TOOL_CXX="g++"
    HOST_TRIPLET="${HOST_ARCH_RAW}-apple-darwin"
fi

refresh_make_vars

##############################################################################
# Dependencies
##############################################################################

ensure_homebrew() {
    if command -v brew >/dev/null 2>&1; then
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

    resolve_gcc_toolchain || die "Homebrew GCC toolchain was not installed correctly"
    refresh_make_vars
}

resolve_gcc_toolchain || true
refresh_make_vars

if [ "$TARGET_ARCH" != "$HOST_ARCH" ]; then
    if ! resolve_cross_gcc_toolchain; then
        die "Cross-arch Darwin builds require a real GCC cross toolchain for ${TARGET_ARCH}. Install one and set DARWIN_CROSS_PREFIX (for example aarch64-apple-darwin), or run this script natively on ${TARGET_ARCH} hardware."
    fi
    refresh_make_vars
fi

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
    local stage_prefix stage_libexec bundle_root gcc_root libffi_root ncurses_root
    local real_fbc wrapper_fbc helper_script old_ncurses bundled_ncurses

    stage_prefix="${STAGE}${PREFIX}"
    stage_libexec="${stage_prefix}/lib/freebasic/libexec"
    bundle_root="${stage_prefix}/lib/freebasic/toolchain"

    [ -x "${stage_prefix}/bin/fbc" ] || die "staged compiler missing before toolchain bundling"
    [ -n "$BREW_PREFIX" ] || die "Homebrew prefix unavailable for macOS packaging"
    [ -n "$LIBFFI_PREFIX" ] || die "libffi prefix unavailable for macOS packaging"
    [ -n "$NCURSES_PREFIX" ] || die "ncurses prefix unavailable for macOS packaging"

    gcc_root="$(cd "$(brew --prefix gcc)" && pwd -P)"
    libffi_root="$(cd "$LIBFFI_PREFIX" && pwd -P)"
    ncurses_root="$(cd "$NCURSES_PREFIX" && pwd -P)"

    msg "bundling macOS toolchain into staged package"
    run rm -rf "$bundle_root"
    copy_tree_preserve "$gcc_root" "${bundle_root}/gcc"
    copy_tree_preserve "$libffi_root" "${bundle_root}/libffi"
    copy_tree_preserve "$ncurses_root" "${bundle_root}/ncurses"

    real_fbc="${stage_libexec}/fbc-real"
    wrapper_fbc="${stage_prefix}/bin/fbc"
    helper_script="${stage_prefix}/bin/fbc-setup-darwin"

    run mkdir -p "$stage_libexec"
    run mv "$wrapper_fbc" "$real_fbc"

    cat > "$wrapper_fbc" <<EOF
#!/usr/bin/env bash
set -euo pipefail

SELF_DIR="\$(CDPATH= cd -- "\$(dirname "\$0")" && pwd)"
PREFIX_ROOT="\$(cd "\$SELF_DIR/.." && pwd)"
FBROOT="\$PREFIX_ROOT/lib/freebasic"
TOOLCHAIN_ROOT="\$FBROOT/toolchain"
GCC_ROOT="\$TOOLCHAIN_ROOT/gcc"
LIBFFI_ROOT="\$TOOLCHAIN_ROOT/libffi"
NCURSES_ROOT="\$TOOLCHAIN_ROOT/ncurses"

export PATH="\$GCC_ROOT/bin:\$PATH"
export GCC="\$GCC_ROOT/bin/$(basename "$TOOL_CC")"
export LIBRARY_PATH="\$LIBFFI_ROOT/lib:\$NCURSES_ROOT/lib\${LIBRARY_PATH:+:\$LIBRARY_PATH}"
export CPATH="\$LIBFFI_ROOT/include:\$NCURSES_ROOT/include\${CPATH:+:\$CPATH}"
export PKG_CONFIG_PATH="\$LIBFFI_ROOT/lib/pkgconfig:\$NCURSES_ROOT/lib/pkgconfig\${PKG_CONFIG_PATH:+:\$PKG_CONFIG_PATH}"
export DYLD_LIBRARY_PATH="\$NCURSES_ROOT/lib\${DYLD_LIBRARY_PATH:+:\$DYLD_LIBRARY_PATH}"

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

    old_ncurses="${NCURSES_PREFIX}/lib/libncursesw.6.dylib"
    bundled_ncurses="@executable_path/../toolchain/ncurses/lib/libncursesw.6.dylib"
    if command -v install_name_tool >/dev/null 2>&1 && [ -f "${bundle_root}/ncurses/lib/libncursesw.6.dylib" ]; then
        run install_name_tool -change "$old_ncurses" "$bundled_ncurses" "$real_fbc"
    fi
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
    install_deps
    ensure_clt
    if [ -n "$BASE_PKG_CONFIG_PATH" ]; then
        export PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
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

    bootstrap_if_needed
    BOOT_FBC="$BOOT_FBC_RESULT"

    msg "building bootstrap compiler for ${FBC_TARGET}"
    run "$MAKE_CMD" -f GNUmakefile -j"$JOBS" "${MAKE_VARS[@]}" "BOOT_FBC=${BOOT_FBC}" "BUILD_FBC=${BOOT_FBC}" bootstrap-minimal

    BOOT_FBC="$ROOT/bootstrap/fbc"
    [ -x "$BOOT_FBC" ] || die "bootstrap compiler was not produced at $BOOT_FBC"

    msg "building FreeBASIC for ${FBC_TARGET}"
    run "$MAKE_CMD" -f GNUmakefile -j"$JOBS" "${MAKE_VARS[@]}" "FBC=${BOOT_FBC}" all

    msg "staging install tree"
    run mkdir -p "$STAGE"
    run "$MAKE_CMD" -f GNUmakefile "${MAKE_VARS[@]}" "FBC=${BOOT_FBC}" install "DESTDIR=${STAGE}" "prefix=${PREFIX}"
    bundle_toolchain_into_stage
fi

##############################################################################
# Package
##############################################################################

if [ "$DO_PACKAGE" -eq 1 ]; then
    [ -x "$STAGE$PREFIX/bin/fbc" ] || die "staged compiler missing: $STAGE$PREFIX/bin/fbc"

    if [ ! -x "$STAGE$PREFIX/lib/freebasic/libexec/fbc-real" ] || [ ! -d "$STAGE$PREFIX/lib/freebasic/toolchain" ]; then
        bundle_toolchain_into_stage
    fi

    TAR_FILE="$OUTBASE/${PKG_BASENAME}.tar.xz"
    PKG_FILE="$OUTBASE/${PKG_BASENAME}.pkg"
    INSTALL_SH="$OUTBASE/install.sh"

    msg "creating tar.xz package"
    run tar -C "$STAGE" -cJf "$TAR_FILE" .

    if command -v pkgbuild >/dev/null 2>&1; then
        msg "creating macOS installer package"
        run rm -rf "$PKGROOT"
        run mkdir -p "$PKGROOT"
        run cp -R "$STAGE"/. "$PKGROOT"/
        create_pkg_scripts
        run pkgbuild \
            --root "$PKGROOT" \
            --scripts "$PKGSCRIPTS" \
            --identifier "org.freebasic.compiler" \
            --version "$VERSION_FULL" \
            --install-location "/" \
            "$PKG_FILE"

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

exec sudo installer -pkg "\$PKG_FILE" -target /
EOF
        run chmod 755 "$INSTALL_SH"

        msg "removing redundant tar.xz package"
        run rm -f "$TAR_FILE"
    else
        echo "WARNING: pkgbuild not found; skipped .pkg creation"
    fi

    msg "artifacts"
    [ -f "$TAR_FILE" ] && echo "$TAR_FILE"
    [ -f "$PKG_FILE" ] && echo "$PKG_FILE"
    [ -f "$INSTALL_SH" ] && echo "$INSTALL_SH"
fi
