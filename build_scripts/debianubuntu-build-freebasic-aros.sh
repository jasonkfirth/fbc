#!/usr/bin/env bash
#
# Project: FreeBASIC AROS release workflow
# ----------------------------------------
#
# File: debianubuntu-build-freebasic-aros.sh
#
# Purpose:
#
#     Install host prerequisites and build complete FreeBASIC packages for the
#     supported AROS m68k, ARM hard-float, and x86_64 ports.
#
# Responsibilities:
#
#     - prepare a pinned AROS source and submodule checkout
#     - configure and build each AROS cross toolchain and SDK
#     - build the matching native AROS GCC, binutils, and linker wrapper
#     - build and stage architecture-matched static libffi archives
#     - stage CyberGraphX, AHI, and diskfont development dependencies
#     - build FreeBASIC rtlib, gfxlib2, sfxlib, and native compiler binaries
#     - create verified installable AROS packages
#     - optionally build the corresponding boot media
#
# This file intentionally does NOT contain:
#
#     - emulator control or guest test result parsing
#     - fbctests or Exampleageddon batching
#     - OMA source discovery and game packaging
#
# Architecture policy:
#
#     Generic m68k support belongs to the FreeBASIC architecture model.  The
#     68000 and software-float baseline used here is specifically the contract
#     of AROS's amiga-m68k SDK and remains in AROS platform configuration.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
AROS_SOURCE="${AROS_SOURCE:-}"
AROS_REPOSITORY="${AROS_REPOSITORY:-https://github.com/aros-development-team/AROS.git}"
AROS_REVISION="${AROS_REVISION:-b224b192a50d406820c822b6e619d6d1bbadc221}"
AROS_CONTRIB_REPOSITORY="${AROS_CONTRIB_REPOSITORY:-https://github.com/aros-development-team/contrib.git}"
AROS_CONTRIB_REVISION="${AROS_CONTRIB_REVISION:-f6e32c8f30f0be7ee4a02ff01d3deb221d8b1ab6}"
TARGETS="${AROS_TARGETS:-x86_64,m68k,arm}"
SKIP_DEPS=0
SKIP_AROS_BUILD=0
SKIP_FREEBASIC_BUILD=0
SKIP_NATIVE_TOOLCHAIN=0
NO_IMAGES=0
NO_PACKAGE=0
UPDATE_AROS=0
PATCH_WAS_APPLIED=0
PACKAGE_REVISION="${AROS_PACKAGE_REVISION:-1}"

##############################################################################
# Helpers
##############################################################################

die() {
    echo "ERROR: $*" >&2
    exit 1
}

msg() {
    echo
    echo "==> $*"
}

run() {
    echo "==> $*"
    "$@"
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

detect_jobs() {
    local jobs=1

    if command -v nproc >/dev/null 2>&1; then
        jobs="$(nproc)"
    elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
        jobs="$(getconf _NPROCESSORS_ONLN)"
    fi

    case "$jobs" in
        ''|*[!0-9]*|0) jobs=1 ;;
    esac

    printf '%s\n' "$jobs"
}

require_value() {
    local option="$1"
    local value="${2-}"

    [ -n "$value" ] || die "$option requires a value"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic-aros.sh [options]

Options:
  --targets LIST       Comma-separated x86_64,m68k,arm list. Default: all
  --skip-deps          Do not install Debian/Ubuntu packages.
  --skip-aros-build    Reuse configured AROS SDKs and cross toolchains.
  --skip-fbc-build     Reuse existing target libraries and native compilers.
  --skip-native-tools  Do not build GCC/binutils that run inside AROS.
  --no-images          Do not create AROS boot media.
  --no-package         Do not create FreeBASIC .pkg and .zip files.
  --aros-root DIR      AROS workspace. Default: out/aros
  --aros-source DIR    AROS source checkout. Default: AROS_ROOT/source
  --aros-revision REV  AROS commit or tag. Default: $AROS_REVISION
  --contrib-revision R AROS contrib commit or tag. Default: $AROS_CONTRIB_REVISION
  --update-aros        Fetch the selected revision in an existing checkout.
  --jobs N             Parallel build jobs. Default: detected CPU count
  -h, --help           Show this help.

Outputs:
  out/aros/freebasic/{x86_64,m68k,arm}/fbc
  out/aros/build-*/bin/*/AROS/Developer/bin/{gcc,as,ld,collect-aros}
  out/aros/packages/FreeBASIC-Developer-*-aros-{x86_64,m68k,arm}.pkg
  out/aros/packages/FreeBASIC-Developer-*-aros-{x86_64,m68k,arm}.zip
  out/aros/packages/FreeBASIC-*-aros-{x86_64,m68k,arm}.pkg
  out/aros/packages/FreeBASIC-*-aros-{x86_64,m68k,arm}.zip

The m68k build uses AROS's amiga-m68k 68000/soft-float SDK.  The ARM build
uses raspi-armhf ARMv7-A hard-float.  The x86_64 build uses pc-x86_64.
EOF
}

map_target() {
    local target="$1"

    case "$target" in
        x86_64)
            MAP_AROS_TARGET="pc-x86_64"
            MAP_AROS_SYSROOT_KEY="pc-x86_64"
            MAP_TOOL_PREFIX="x86_64-aros"
            MAP_FBC_TARGET="aros-x86_64"
            MAP_GCC_VERSION="10.5.0"
            MAP_CONFIG_EXTRA=""
            ;;
        m68k)
            MAP_AROS_TARGET="amiga-m68k"
            MAP_AROS_SYSROOT_KEY="amiga-m68k"
            MAP_TOOL_PREFIX="m68k-aros"
            MAP_FBC_TARGET="aros-m68k"
            MAP_GCC_VERSION="6.5.0"
            MAP_CONFIG_EXTRA="--with-serial-debug=yes"
            ;;
        arm)
            MAP_AROS_TARGET="raspi-armhf"
            MAP_AROS_SYSROOT_KEY="raspi-arm"
            MAP_TOOL_PREFIX="arm-aros"
            MAP_FBC_TARGET="aros-arm"
            MAP_GCC_VERSION="6.5.0"
            MAP_CONFIG_EXTRA=""
            ;;
        *)
            die "unsupported AROS target: $target"
            ;;
    esac
}

for_each_target() {
    local function_name="$1"
    local target

    for target in "${SELECTED_TARGETS[@]}"; do
        "$function_name" "$target"
    done
}

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --targets)
            require_value "$1" "${2-}"
            TARGETS="$2"
            shift 2
            ;;
        --skip-deps)
            SKIP_DEPS=1
            shift
            ;;
        --skip-aros-build)
            SKIP_AROS_BUILD=1
            shift
            ;;
        --skip-fbc-build)
            SKIP_FREEBASIC_BUILD=1
            shift
            ;;
        --skip-native-tools)
            SKIP_NATIVE_TOOLCHAIN=1
            shift
            ;;
        --no-images)
            NO_IMAGES=1
            shift
            ;;
        --no-package)
            NO_PACKAGE=1
            shift
            ;;
        --aros-root)
            require_value "$1" "${2-}"
            AROS_ROOT="$2"
            shift 2
            ;;
        --aros-source)
            require_value "$1" "${2-}"
            AROS_SOURCE="$2"
            shift 2
            ;;
        --aros-revision)
            require_value "$1" "${2-}"
            AROS_REVISION="$2"
            shift 2
            ;;
        --contrib-revision)
            require_value "$1" "${2-}"
            AROS_CONTRIB_REVISION="$2"
            shift 2
            ;;
        --update-aros)
            UPDATE_AROS=1
            shift
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

# Resolve dependent defaults after command-line parsing.  Otherwise changing
# --aros-root would leave the source checkout under the original workspace.
AROS_SOURCE="${AROS_SOURCE:-$AROS_ROOT/source}"
AROS_CONTRIB_SOURCE="$AROS_SOURCE/contrib"

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

IFS=',' read -r -a REQUESTED_TARGETS <<< "$TARGETS"
SELECTED_TARGETS=()
for target in "${REQUESTED_TARGETS[@]}"; do
    case "$target" in
        x86_64|m68k|arm) ;;
        '') continue ;;
        *) die "unsupported AROS target in --targets: $target" ;;
    esac

    if [[ " ${SELECTED_TARGETS[*]} " != *" $target "* ]]; then
        SELECTED_TARGETS+=("$target")
    fi
done
[ "${#SELECTED_TARGETS[@]}" -gt 0 ] || die "--targets selected no targets"

command -v apt-get >/dev/null 2>&1 ||
    die "this script requires an APT-based Debian or Ubuntu host"

##############################################################################
# Host prerequisites
##############################################################################

install_dependencies() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Debian/Ubuntu AROS build dependencies"
    export DEBIAN_FRONTEND=noninteractive
    run_root apt-get update
    # The ARM cross-GCC build configures a 32-bit x86 host zlib multilib.  A
    # compiler-only -m32 probe is not enough; its configure test must also link.
    run_root apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        bc \
        bison \
        build-essential \
        byacc \
        bzip2 \
        ca-certificates \
        ccache \
        cmake \
        device-tree-compiler \
        dosfstools \
        dos2unix \
        e2tools \
        fdisk \
        file \
        flex \
        fs-uae \
        g++ \
        g++-multilib \
        gawk \
        gcc-multilib \
        genisoimage \
        git \
        gperf \
        jlha-utils \
        libfl-dev \
        libgmp-dev \
        libisl-dev \
        liblzo2-dev \
        libmpc-dev \
        libmpfr-dev \
        libncurses-dev \
        libpng-dev \
        libsdl1.2-dev \
        libssl-dev \
        libswitch-perl \
        libxcursor-dev \
        libxext-dev \
        libx11-dev \
        libxxf86vm-dev \
        liblzma-dev \
        m4 \
        mtools \
        nasm \
        netpbm \
        patch \
        python3 \
        python3-crcmod \
        python3-mako \
        qemu-system-arm \
        qemu-system-x86 \
        rsync \
        texinfo \
        unzip \
        wget \
        xz-utils \
        xorriso \
        zlib1g-dev \
        zip
}

##############################################################################
# AROS checkout and SDK construction
##############################################################################

prepare_aros_source() {
    if [ ! -d "$AROS_SOURCE/.git" ]; then
        [ ! -e "$AROS_SOURCE" ] ||
            die "AROS source path exists but is not a Git checkout: $AROS_SOURCE"
        run mkdir -p "$(dirname "$AROS_SOURCE")"
        run git clone "$AROS_REPOSITORY" "$AROS_SOURCE"
    fi

    if [ "$UPDATE_AROS" -eq 1 ]; then
        run git -C "$AROS_SOURCE" fetch --tags origin
    fi

    if ! git -C "$AROS_SOURCE" rev-parse --verify "$AROS_REVISION^{commit}" \
        >/dev/null 2>&1; then
        run git -C "$AROS_SOURCE" fetch origin "$AROS_REVISION"
    fi

    if [ "$(git -C "$AROS_SOURCE" rev-parse HEAD)" != \
         "$(git -C "$AROS_SOURCE" rev-parse "$AROS_REVISION^{commit}")" ]; then
        [ -z "$(git -C "$AROS_SOURCE" status --short)" ] ||
            die "AROS checkout has local changes; refusing to switch revisions"
        run git -C "$AROS_SOURCE" checkout --detach "$AROS_REVISION"
    fi

    run git -C "$AROS_SOURCE" submodule update --init --recursive

    # AROS keeps the third-party ports used for native GCC and binutils in a
    # separate official repository.  Old build trees already had this checkout,
    # which hid the dependency until the release was built in a clean workspace.
    if [ ! -d "$AROS_CONTRIB_SOURCE/.git" ]; then
        [ ! -e "$AROS_CONTRIB_SOURCE" ] ||
            die "AROS contrib path exists but is not a Git checkout: $AROS_CONTRIB_SOURCE"
        run mkdir -p "$(dirname "$AROS_CONTRIB_SOURCE")"
        run git clone "$AROS_CONTRIB_REPOSITORY" "$AROS_CONTRIB_SOURCE"
    fi

    if [ "$UPDATE_AROS" -eq 1 ]; then
        run git -C "$AROS_CONTRIB_SOURCE" fetch --tags origin
    fi

    if ! git -C "$AROS_CONTRIB_SOURCE" rev-parse --verify \
        "$AROS_CONTRIB_REVISION^{commit}" >/dev/null 2>&1; then
        run git -C "$AROS_CONTRIB_SOURCE" fetch origin "$AROS_CONTRIB_REVISION"
    fi

    if [ "$(git -C "$AROS_CONTRIB_SOURCE" rev-parse HEAD)" != \
         "$(git -C "$AROS_CONTRIB_SOURCE" rev-parse "$AROS_CONTRIB_REVISION^{commit}")" ]; then
        [ -z "$(git -C "$AROS_CONTRIB_SOURCE" status --short)" ] ||
            die "AROS contrib checkout has local changes; refusing to switch revisions"
        run git -C "$AROS_CONTRIB_SOURCE" checkout --detach "$AROS_CONTRIB_REVISION"
    fi

    run git -C "$AROS_CONTRIB_SOURCE" submodule update --init --recursive
}

apply_patch_once() {
    local patch_file="$1"
    local source_dir="$2"

    if patch -d "$source_dir" -p1 --forward --dry-run \
        < "$patch_file" >/dev/null 2>&1; then
        msg "applying $(basename "$patch_file")"
        patch -d "$source_dir" -p1 --forward < "$patch_file"
        PATCH_WAS_APPLIED=1
    elif patch -d "$source_dir" -p1 --reverse --dry-run \
        < "$patch_file" >/dev/null 2>&1; then
        msg "$(basename "$patch_file") is already applied"
        PATCH_WAS_APPLIED=0
    else
        die "cannot apply or recognize patch state: $patch_file"
    fi
}

native_gcc_runtime_patch_is_applied() {
    local source_dir="$1"

    [ -f "$source_dir/gcc/config/x-aros" ] &&
        grep -q 'host_xmake_file=.*x-aros' "$source_dir/gcc/config.host" &&
        grep -q 'pex_aros_filename' "$source_dir/libiberty/pex-unix.c" &&
        grep -q '#undef HAVE_TIMES' "$source_dir/gcc/timevar.c" &&
        grep -q '#undef HAVE_CLOCK' "$source_dir/libiberty/getruntime.c"
}

native_gcc_process_patch_is_applied() {
    local source_dir="$1"

    grep -q 'struct pex_aros_child_result' \
        "$source_dir/libiberty/pex-unix.c" &&
        grep -q 'NP_NotifyOnDeath' "$source_dir/libiberty/pex-unix.c" &&
        grep -q 'gcc_libexec_prefix = "Developer:libexec/gcc/"' \
            "$source_dir/gcc/gcc.c"
}

prepare_native_prerequisites() {
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/unpack-headless-cli.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-autotools-explicit-nix-layout.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-binutils-m68k-bundled-zlib.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-binutils-m68k-no-posix-startup.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-posixc-path-buffer.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-posixc-m68k-seek.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-posixc-m68k-transfer.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-posixc-vfork-child-id.patch" \
        "$AROS_SOURCE"
    if grep -q 'static char \*aros_command_name' \
        "$AROS_SOURCE/tools/collect-aros/docommand-exec.c" &&
        grep -q 'NP_NotifyOnDeath' \
            "$AROS_SOURCE/tools/collect-aros/docommand-exec.c"; then
        msg "native-collect-aros-runcommand.patch is already applied"
    else
        apply_patch_once \
            "$SCRIPT_DIR/aros-patches/native-collect-aros-runcommand.patch" \
            "$AROS_SOURCE"
    fi
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-collect-aros-arm-process-boundary.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-collect-aros-m68k-temp-path.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-gcc-gc-cmake-policy.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-gcc-gc-target-cxxflags.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-gcc-no-common.patch" \
        "$AROS_SOURCE"
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-gcc-m68k-no-posix-startup.patch" \
        "$AROS_SOURCE"
}

configure_aros_target() {
    local target="$1"
    local build_dir
    local toolchain_dir
    local configure_args

    map_target "$target"
    build_dir="$AROS_ROOT/build-$MAP_AROS_TARGET"
    toolchain_dir="$AROS_ROOT/toolchain-$MAP_AROS_TARGET"

    if [ -f "$build_dir/config.status" ]; then
        return 0
    fi

    run mkdir -p "$build_dir" "$toolchain_dir"
    configure_args=(
        "--target=$MAP_AROS_TARGET"
        --enable-ccache
        "--with-aros-toolchain-install=$toolchain_dir"
    )
    if [ -n "$MAP_CONFIG_EXTRA" ]; then
        configure_args+=("$MAP_CONFIG_EXTRA")
    fi

    msg "configuring AROS $target ($MAP_AROS_TARGET)"
    (
        cd "$build_dir"
        run "$AROS_SOURCE/configure" "${configure_args[@]}"
    )
}

build_aros_target() {
    local target="$1"
    local build_dir
    local sdk_root
    local toolchain_dir

    map_target "$target"
    build_dir="$AROS_ROOT/build-$MAP_AROS_TARGET"
    sdk_root="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/AROS"
    toolchain_dir="$AROS_ROOT/toolchain-$MAP_AROS_TARGET"

    msg "building AROS toolchain and SDK for $target"
    if [ ! -x "$toolchain_dir/$MAP_TOOL_PREFIX-gcc" ]; then
        run make -C "$build_dir" -j"$JOBS" tools-crosstools
    fi
    # Build the link-time and guest-side dependencies explicitly.  Workbench
    # currently reaches most of these transitively, but native FreeBASIC and
    # fbctests must not depend on that implementation detail of AROS media.
    # Keep the targets in separate invocations: the generated top-level
    # Makefile starts one mmake scanner per goal, and parallel scanners cannot
    # safely share mmake's generated state.
    run make -C "$build_dir" -j"$JOBS" includes-copy
    run make -C "$build_dir" -j"$JOBS" linklibs-cybergraphics
    run make -C "$build_dir" -j"$JOBS" workbench-devs-AHI-subsystem
    run make -C "$build_dir" -j"$JOBS" workbench-libs-diskfont
    run make -C "$build_dir" -j"$JOBS" kernel-dos64
    run make -C "$build_dir" -j"$JOBS" compiler-posixc
    run make -C "$build_dir" -j"$JOBS" compiler-stdcio
    run make -C "$build_dir" -j"$JOBS" workbench-libs-cgfx
    run make -C "$build_dir" -j"$JOBS" workbench-libs-iffparse
    run make -C "$build_dir" -j"$JOBS" workbench-libs-locale
    run make -C "$build_dir" -j"$JOBS" workbench-libs-zstd

    [ -f "$sdk_root/Developer/include/cybergraphx/cybergraphics.h" ] ||
        die "CyberGraphX SDK header was not staged for $target"
    [ -f "$sdk_root/Developer/include/devices/ahi.h" ] ||
        die "AHI SDK header was not staged for $target"
    for runtime_library in \
        dos64.library \
        posixc.library \
        stdcio.library \
        cybergraphics.library \
        iffparse.library \
        locale.library \
        zstd.library; do
        [ -f "$sdk_root/Libs/$runtime_library" ] ||
            die "$runtime_library was not staged for AROS $target"
    done
}

build_native_toolchain_target() {
    local target="$1"
    local binutils_build_dir
    local binutils_build_recipe
    local binutils_configured
    local binutils_installed
    local binutils_source
    local build_dir
    local gcc_build_dir
    local gcc_build_recipe
    local gcc_configured
    local gcc_source
    local gcc_files_touched
    local gcc_jobs
    local gcc_patches_applied=0
    local gcc_runtime_patch
    local gcc_installed
    local host_elf2hunk
    local native_elf2hunk
    local native_collect_aros
    local native_collect_aros_description
    local patches_applied=0
    local sdk_root
    local stripped_elf2hunk

    [ "$SKIP_NATIVE_TOOLCHAIN" -eq 0 ] || return 0
    map_target "$target"
    build_dir="$AROS_ROOT/build-$MAP_AROS_TARGET"
    sdk_root="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/AROS"
    gcc_jobs="$JOBS"
    if [ "$target" = "m68k" ]; then
        # AROS's GCC port invokes overlapping recursive makes in one build
        # directory.  Serialize this target so newly added host objects cannot
        # be linked while another recursive make is still writing them.
        gcc_jobs=1
    fi
    binutils_build_dir="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/gen/contrib/gnu/binutils/binutils"
    binutils_build_recipe="$AROS_SOURCE/contrib/gnu/binutils/mmakefile.src"
    binutils_configured="$binutils_build_dir/.configured"
    binutils_installed="$binutils_build_dir/.installed"
    binutils_source="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/Ports/binutils/binutils-2.32"

    msg "preparing native AROS binutils for $target"
    run make -C "$build_dir" -j"$JOBS" development-binutils-fetch
    [ -d "$binutils_source" ] ||
        die "native AROS binutils source is unavailable: $binutils_source"
    PATCH_WAS_APPLIED=0
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-binutils-paths-osabi.patch" \
        "$binutils_source"
    [ "$PATCH_WAS_APPLIED" -eq 0 ] || patches_applied=1
    apply_patch_once \
        "$SCRIPT_DIR/aros-patches/native-binutils-aros-host-runtime.patch" \
        "$binutils_source"
    [ "$PATCH_WAS_APPLIED" -eq 0 ] || patches_applied=1
    if [ "$target" = "m68k" ]; then
        apply_patch_once \
            "$SCRIPT_DIR/aros-patches/native-binutils-m68k-local-environment.patch" \
            "$binutils_source"
        [ "$PATCH_WAS_APPLIED" -eq 0 ] || patches_applied=1
    fi
    if [ "$target" = "m68k" ] && [ -e "$binutils_configured" ] &&
        [ "$binutils_build_recipe" -nt "$binutils_configured" ]; then
        patches_applied=1
    fi
    if [ "$patches_applied" -eq 1 ]; then
        # AROS's port machinery deliberately does not track every upstream
        # source file or a changed configure policy. Reconfigure when needed so
        # an existing workspace acquires both kinds of native-host fix.
        if [ -f "$binutils_build_dir/Makefile" ]; then
            run make -C "$binutils_build_dir" clean
        fi
        run rm -f "$binutils_configured" "$binutils_installed"
    fi

    msg "preparing native AROS GCC host support for $target"
    run make -C "$build_dir" -j"$JOBS" development-gcc-fetch
    gcc_build_dir="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/gen/contrib/gnu/gcc/gcc"
    gcc_build_recipe="$AROS_SOURCE/contrib/gnu/gcc/mmakefile.src"
    gcc_configured="$gcc_build_dir/.configured"
    gcc_source="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/Ports/gcc/gcc-$MAP_GCC_VERSION"
    gcc_files_touched="$gcc_build_dir/.files-touched"
    gcc_installed="$gcc_build_dir/.installed"
    [ -d "$gcc_source" ] ||
        die "native AROS GCC source is unavailable: $gcc_source"
    gcc_runtime_patch="$SCRIPT_DIR/aros-patches/native-gcc-aros-host-runtime-gcc${MAP_GCC_VERSION%%.*}.patch"
    [ -f "$gcc_runtime_patch" ] ||
        die "native AROS GCC host patch is unavailable: $gcc_runtime_patch"
    PATCH_WAS_APPLIED=0
    if native_gcc_runtime_patch_is_applied "$gcc_source"; then
        msg "$(basename "$gcc_runtime_patch") is already applied"
        PATCH_WAS_APPLIED=0
    else
        apply_patch_once "$gcc_runtime_patch" "$gcc_source"
        [ "$PATCH_WAS_APPLIED" -eq 0 ] || gcc_patches_applied=1
    fi
    if [ "$target" = "m68k" ] || [ "$target" = "arm" ]; then
        if native_gcc_process_patch_is_applied "$gcc_source"; then
            msg "native-gcc-m68k-process-boundary.patch is already applied"
        else
            apply_patch_once \
                "$SCRIPT_DIR/aros-patches/native-gcc-m68k-process-boundary.patch" \
                "$gcc_source"
            [ "$PATCH_WAS_APPLIED" -eq 0 ] || gcc_patches_applied=1
        fi
    fi
    if [ "$target" = "m68k" ]; then
        run mkdir -p "$gcc_source/gcc/config/aros"
        apply_patch_once \
            "$SCRIPT_DIR/aros-patches/native-gcc-m68k-host-runtime-gcc6.patch" \
            "$gcc_source"
        [ "$PATCH_WAS_APPLIED" -eq 0 ] || gcc_patches_applied=1
    fi
    if [ "$target" = "arm" ]; then
        apply_patch_once \
            "$SCRIPT_DIR/aros-patches/native-gcc-arm-process-boundary.patch" \
            "$gcc_source"
        [ "$PATCH_WAS_APPLIED" -eq 0 ] || gcc_patches_applied=1
        apply_patch_once \
            "$SCRIPT_DIR/aros-patches/native-gcc-arm-directory-paths.patch" \
            "$gcc_source"
        [ "$PATCH_WAS_APPLIED" -eq 0 ] || gcc_patches_applied=1
    fi
    if [ -e "$gcc_configured" ] &&
        [ "$gcc_build_recipe" -nt "$gcc_configured" ]; then
        gcc_patches_applied=1
    fi
    if [ "$target" = "m68k" ] && [ -f "$gcc_build_dir/gcc/Makefile" ] &&
        ! grep -q 'config/aros/x-m68k' "$gcc_build_dir/gcc/Makefile"; then
        # The m68k host fragment is consumed only during GCC configuration.
        # A workspace configured before the fragment was installed links the
        # runtime object without a rule to build it, so force a reconfigure.
        gcc_patches_applied=1
    fi
    if [ "$gcc_patches_applied" -eq 1 ]; then
        if [ -f "$gcc_build_dir/Makefile" ]; then
            run make -C "$gcc_build_dir" clean
        fi
        run rm -f "$gcc_configured" "$gcc_installed"
    fi

    # The AROS autoconf template always names this sentinel as a dependency.
    # Its recipe is disabled because it launches one process per imported GCC
    # source file.  Create the sentinel once without making an installed GCC
    # appear stale on every subsequent build-script invocation.
    if [ ! -e "$gcc_files_touched" ]; then
        run mkdir -p "$(dirname "$gcc_files_touched")"
        run touch "$gcc_files_touched"
    fi

    if [ "$target" = "m68k" ]; then
        native_collect_aros="$sdk_root/Developer/bin/collect-aros"
        if [ -f "$native_collect_aros" ] &&
            ! readelf -h "$native_collect_aros" >/dev/null 2>&1; then
            native_collect_aros_description="$(file -b "$native_collect_aros")"
            [[ "$native_collect_aros_description" == \
                *"AmigaOS loadseg()ble executable/binary"* ]] ||
                die "unexpected native m68k collect-aros format: $native_collect_aros_description"

            # Final packaging converts the installed collector from ELF to
            # Hunk.  A later AROS make pass strips this target before checking
            # whether it needs relinking, so remove the delivered Hunk copy
            # and let its normal recipe regenerate the ELF intermediate.
            run rm -f "$native_collect_aros"
        fi
    fi

    msg "building native AROS development supply chain for $target"
    run make -C "$build_dir" -j"$JOBS" development-binutils
    run make -C "$build_dir" -j"$gcc_jobs" development-gcc
    # Let the AROS dependency graph decide whether collect-aros is current.
    # Skipping the target merely because its installed m68k copy is already
    # Hunk loadable hides newer source or patch changes from incremental runs.
    run make -C "$build_dir" -j"$JOBS" development-collect-aros

    if [ "$target" = "m68k" ]; then
        # AROS builds this utility as a target ELF program.  Convert the
        # utility itself once on the host so the installed copy can perform
        # the same ELF-to-Hunk delivery step for programs compiled on AROS.
        run make -C "$build_dir" -j"$JOBS" tools-elf2hunk
        host_elf2hunk="$build_dir/bin/linux-x86_64/tools/elf2hunk"
        native_elf2hunk="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/AROS/Extras/Developer/Build/elf2hunk"
        [ -x "$host_elf2hunk" ] ||
            die "host AROS elf2hunk is unavailable: $host_elf2hunk"
        [ -f "$native_elf2hunk" ] ||
            die "native AROS elf2hunk is unavailable: $native_elf2hunk"
    fi

    compact_native_gcc_programs "$target"

    if [ "$target" = "m68k" ]; then
        stripped_elf2hunk="$(mktemp "$native_elf2hunk.stripped.XXXXXX")"
        run "$AROS_ROOT/toolchain-$MAP_AROS_TARGET/$MAP_TOOL_PREFIX-strip" \
            -g -o "$stripped_elf2hunk" "$native_elf2hunk"
        run "$host_elf2hunk" \
            "$stripped_elf2hunk" "$sdk_root/Developer/bin/elf2hunk"
        run rm -f "$stripped_elf2hunk"
        run chmod 755 "$sdk_root/Developer/bin/elf2hunk"
    fi

    for tool in gcc as ld elfedit collect-aros; do
        [ -f "$sdk_root/Developer/bin/$tool" ] ||
            die "native AROS $tool was not staged for $target"
    done

    if [ "$target" = "m68k" ]; then
        [ -f "$sdk_root/Developer/bin/elf2hunk" ] ||
            die "native AROS elf2hunk was not staged for m68k"
    fi
}

compact_native_gcc_programs() {
    local target="$1"
    local build_dir
    local collector
    local description
    local host_elf2hunk
    local hunk_layout
    local inode
    local layout_program
    local program
    local section_count
    local sdk_root
    local stripped_program
    local temporary_program
    local toolchain_dir
    local -a program_roots
    declare -A processed_inodes=()

    map_target "$target"
    build_dir="$AROS_ROOT/build-$MAP_AROS_TARGET"
    collector="$build_dir/bin/linux-x86_64/tools/collect-aros"
    host_elf2hunk="$build_dir/bin/linux-x86_64/tools/elf2hunk"
    hunk_layout="$SCRIPT_DIR/aros-patches/m68k-native-hunk.ld"
    sdk_root="$build_dir/bin/$MAP_AROS_SYSROOT_KEY/AROS"
    toolchain_dir="$AROS_ROOT/toolchain-$MAP_AROS_TARGET"

    [ -x "$toolchain_dir/$MAP_TOOL_PREFIX-ld" ] ||
        die "AROS cross linker is unavailable: $toolchain_dir/$MAP_TOOL_PREFIX-ld"
    [ -x "$toolchain_dir/$MAP_TOOL_PREFIX-strip" ] ||
        die "AROS target strip is unavailable: $toolchain_dir/$MAP_TOOL_PREFIX-strip"

    if [ "$target" = "m68k" ]; then
        [ -x "$host_elf2hunk" ] ||
            die "host AROS elf2hunk is unavailable: $host_elf2hunk"
        [ -f "$hunk_layout" ] ||
            die "AROS m68k Hunk layout is unavailable: $hunk_layout"
        [ -x "$collector" ] ||
            die "host AROS collect-aros is unavailable: $collector"
    else
        [ -x "$collector" ] ||
            die "host AROS collect-aros is unavailable: $collector"
    fi

    msg "finalizing native AROS development programs for $target"

    # GCC 6 and 10 produce AROS-hosted programs as relocatable ELF images.
    # ARM and x86_64 LoadSeg cannot reliably process thousands of COMDAT
    # sections, so collect-aros first folds those groups into normal loadable
    # sections.  Debug sections are removed without discarding the relocations
    # consumed by LoadSeg.  The classic m68k loader instead needs its command-
    # line development programs converted to Hunk at this delivery boundary.
    # Large m68k GCC front ends need one additional layout pass.  Hunk's
    # PC-relative relocation records use 16-bit source offsets, so keeping
    # unwind metadata with the code it describes makes those references local
    # and removes the need to encode them in the load-time relocation table.
    program_roots=("$sdk_root/Developer/bin")
    program_roots+=("$sdk_root/Developer/libexec/gcc")

    while IFS= read -r -d '' program; do
        readelf -h "$program" >/dev/null 2>&1 || continue
        readelf -h "$program" | grep -q 'AROS' || continue

        inode="$(stat -c '%d:%i' "$program")"
        [ -z "${processed_inodes[$inode]+set}" ] || continue
        processed_inodes[$inode]=1

        section_count="$(
            readelf -SW "$program" |
                sed -n 's/^There are \([0-9][0-9]*\) section headers.*/\1/p'
        )"
        [ -n "$section_count" ] ||
            die "cannot determine ELF section count: $program"

        temporary_program="$(mktemp "$program.final.XXXXXX")"
        if [ "$target" = "m68k" ]; then
            stripped_program="$(mktemp "$program.stripped.XXXXXX")"
            if [ "$section_count" -gt 128 ]; then
                layout_program="$(mktemp "$program.layout.XXXXXX")"
                PATH="$toolchain_dir:$PATH" run "$collector" \
                    --force-group-allocation \
                    -d \
                    -o "$stripped_program" \
                    "$program"
            else
                run cp "$program" "$stripped_program"
            fi
            run "$toolchain_dir/$MAP_TOOL_PREFIX-strip" \
                -g "$stripped_program"
            if [ "$section_count" -gt 128 ]; then
                run "$toolchain_dir/$MAP_TOOL_PREFIX-ld" \
                    -r \
                    --force-group-allocation \
                    -T "$hunk_layout" \
                    -o "$layout_program" \
                    "$stripped_program"
                run "$host_elf2hunk" "$layout_program" "$temporary_program"
                run rm -f "$layout_program"
            else
                run "$host_elf2hunk" "$stripped_program" "$temporary_program"
            fi
            run rm -f "$stripped_program"

            description="$(file -b "$temporary_program")"
            [[ "$description" == *"AmigaOS loadseg()ble executable/binary"* ]] ||
                die "native AROS m68k program is not Hunk loadable: $program"
        else
            if [ "$section_count" -gt 128 ]; then
                PATH="$toolchain_dir:$PATH" run "$collector" \
                    --force-group-allocation \
                    -d \
                    -o "$temporary_program" \
                    "$program"
            else
                run cp "$program" "$temporary_program"
            fi
            run "$toolchain_dir/$MAP_TOOL_PREFIX-strip" \
                -g "$temporary_program"

            section_count="$(
                readelf -SW "$temporary_program" |
                    sed -n 's/^There are \([0-9][0-9]*\) section headers.*/\1/p'
            )"
            [ -n "$section_count" ] && [ "$section_count" -le 128 ] ||
                die "native AROS executable remains fragmented: $program"
        fi

        # Copy through the existing inode so GCC's installed hard-linked
        # driver aliases continue to refer to the same finalized program.
        run cp "$temporary_program" "$program"
        run chmod 755 "$program"
        run rm -f "$temporary_program"
    done < <(
        find "${program_roots[@]}" -type f -perm /111 -print0
    )
}

build_aros_image() {
    local target="$1"
    local build_dir

    [ "$NO_IMAGES" -eq 0 ] || return 0
    map_target "$target"
    build_dir="$AROS_ROOT/build-$MAP_AROS_TARGET"

    msg "building AROS boot media for $target"
    # distfiles only collects targets that already exist.  Building Workbench
    # first guarantees that command-line tools, graphics support, AHI, and the
    # SDK are present on media used for native compilation and guest testing.
    run make -C "$build_dir" -j"$JOBS" workbench
    run make -C "$build_dir" -j"$JOBS" distfiles
}

##############################################################################
# Target libffi construction
##############################################################################

build_libffi_target() {
    local target="$1"
    local architecture_flags
    local build_dir
    local libffi_source
    local m68k_source
    local runtime_dir
    local toolchain_dir

    map_target "$target"
    build_dir="$AROS_ROOT/libffi-build/$target"
    libffi_source="$AROS_ROOT/build-$MAP_AROS_TARGET/bin/linux-x86_64/Ports/host/gcc/gcc-$MAP_GCC_VERSION/libffi"
    runtime_dir="$ROOT/lib/freebasic/aros-$target"
    toolchain_dir="$AROS_ROOT/toolchain-$MAP_AROS_TARGET"

    [ -x "$libffi_source/configure" ] ||
        die "AROS GCC libffi source is unavailable for $target: $libffi_source"

    case "$target" in
        x86_64)
            architecture_flags="-m64 -mcmodel=large -mno-red-zone"
            ;;
        m68k)
            architecture_flags="-march=68000 -msoft-float"
            m68k_source="$libffi_source/src/m68k/ffi.c"
            if ! grep -q 'defined(__AROS__)' "$m68k_source"; then
                msg "patching m68k libffi for the AROS cache API"
                run patch -d "$libffi_source" -p1 \
                    < "$SCRIPT_DIR/aros-patches/libffi-m68k-cache.patch"
            fi
            ;;
        arm)
            architecture_flags="-march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4"
            ;;
    esac

    if [ ! -f "$build_dir/Makefile" ]; then
        run mkdir -p "$build_dir"
        msg "configuring static libffi for AROS $target"
        (
            cd "$build_dir"
            run "$libffi_source/configure" \
                "--host=$MAP_TOOL_PREFIX" \
                --disable-multilib \
                --disable-shared \
                --enable-static \
                "CC=$toolchain_dir/$MAP_TOOL_PREFIX-gcc" \
                "AR=$toolchain_dir/$MAP_TOOL_PREFIX-ar" \
                "RANLIB=$toolchain_dir/$MAP_TOOL_PREFIX-ranlib" \
                "CFLAGS=-O2 -fno-common $architecture_flags"
        )
    fi

    msg "building static libffi for AROS $target"
    run make -C "$build_dir" -j"$JOBS"
    [ -s "$build_dir/.libs/libffi.a" ] ||
        die "AROS $target libffi archive was not created"
    [ -s "$build_dir/include/ffi.h" ] ||
        die "AROS $target libffi header was not created"
    [ -s "$build_dir/include/ffitarget.h" ] ||
        die "AROS $target libffi target header was not created"

    run mkdir -p "$runtime_dir/include"
    run cp "$build_dir/.libs/libffi.a" "$runtime_dir/libffi.a"
    run cp "$build_dir/include/ffi.h" "$runtime_dir/include/ffi.h"
    run cp "$build_dir/include/ffitarget.h" "$runtime_dir/include/ffitarget.h"
}

##############################################################################
# FreeBASIC construction and packaging
##############################################################################

build_host_compiler() {
    local host_feature_args=(
        DISABLE_GPM=YesPlease
        DISABLE_X11=YesPlease
        DISABLE_OPENGL=YesPlease
        DISABLE_ALSA=YesPlease
        DISABLE_PULSE=YesPlease
    )

    if [ ! -x "$ROOT/bin/fbc" ]; then
        msg "building bootstrap host FreeBASIC compiler"
        run make -C "$ROOT" -j"$JOBS" bootstrap-minimal \
            "${host_feature_args[@]}"
    fi

    [ -x "$ROOT/bin/fbc" ] || die "host FreeBASIC compiler is unavailable"

    # The release bootstrap compiler predates the type alias syntax in the
    # current headers. Build the current compiler through the compatibility
    # definitions before asking it to self-host without them.
    msg "building source-compatible host FreeBASIC compiler"
    run make -C "$ROOT" -j"$JOBS" compiler \
        BUILD_FBC="$ROOT/bin/fbc" \
        BUILD_FBCFLAGS="-d __FB_BOOTSTRAP_COMPAT__" \
        "${host_feature_args[@]}"
    run make -C "$ROOT" clean-compiler

    msg "refreshing host FreeBASIC compiler"
    run make -C "$ROOT" -j"$JOBS" compiler \
        BUILD_FBC="$ROOT/bin/fbc" \
        "${host_feature_args[@]}"
}

build_freebasic_target() {
    local target="$1"
    local compiler_output
    local elf2hunk
    local toolchain_dir
    local native_compiler

    map_target "$target"
    toolchain_dir="$AROS_ROOT/toolchain-$MAP_AROS_TARGET"
    native_compiler="$AROS_ROOT/freebasic/$target/fbc"
    compiler_output="$native_compiler"

    if [ "$target" = "m68k" ]; then
        compiler_output="$native_compiler.elf"
        elf2hunk="$AROS_ROOT/build-$MAP_AROS_TARGET/bin/linux-x86_64/tools/elf2hunk"
        [ -x "$elf2hunk" ] ||
            die "AROS m68k ELF-to-Hunk converter is unavailable: $elf2hunk"
    fi

    [ -x "$toolchain_dir/$MAP_TOOL_PREFIX-gcc" ] ||
        die "AROS cross compiler is unavailable: $toolchain_dir/$MAP_TOOL_PREFIX-gcc"
    run mkdir -p "$(dirname "$native_compiler")"

    # Make does not record CPPFLAGS in dependency files. A previous build that
    # disabled libffi would otherwise leave the THREADCALL stub object current
    # even after the target headers and archive have been staged.
    msg "clearing stale AROS $target library objects"
    PATH="$toolchain_dir:$PATH" run make -C "$ROOT" \
        "TARGET_TRIPLET=$MAP_TOOL_PREFIX" \
        clean-libs

    msg "building complete FreeBASIC libraries for AROS $target"
    PATH="$toolchain_dir:$PATH" run make -C "$ROOT" -j"$JOBS" \
        "TARGET_TRIPLET=$MAP_TOOL_PREFIX" \
        "BUILD_FBC=$ROOT/bin/fbc" \
        libs

    msg "building native FreeBASIC compiler for AROS $target"
    # FBC_EXE can name an output outside bin/. Remove it explicitly because
    # make's normal compiler target may otherwise leave that existing file in
    # place even after target runtime objects have changed.
    run rm -f -- "$compiler_output"
    PATH="$toolchain_dir:$PATH" run make -C "$ROOT" -j"$JOBS" \
        "TARGET_TRIPLET=$MAP_TOOL_PREFIX" \
        "BUILD_FBC=$ROOT/bin/fbc" \
        "FBC_EXE=$compiler_output" \
        compiler

    [ -f "$compiler_output" ] ||
        die "native FreeBASIC compiler was not created: $compiler_output"

    # AROS m68k's classic loader consumes Hunk files. Keep the compiler's
    # generic m68k linker output as ELF, then perform this AROS-only delivery
    # conversion at the same boundary used for guest test executables.
    if [ "$target" = "m68k" ]; then
        msg "converting native AROS m68k compiler to Hunk format"
        run "$elf2hunk" "$compiler_output" "$native_compiler"
        run chmod 755 "$native_compiler"
    fi

    [ -f "$native_compiler" ] ||
        die "installable FreeBASIC compiler was not created: $native_compiler"
}

package_freebasic_target() {
    local target="$1"

    [ "$NO_PACKAGE" -eq 0 ] || return 0
    msg "packaging FreeBASIC for AROS $target"
    AROS_ROOT="$AROS_ROOT" run \
        "$SCRIPT_DIR/aros-package-freebasic.sh" --target "$target"
}

package_developer_target() {
    local target="$1"

    [ "$NO_PACKAGE" -eq 0 ] || return 0
    msg "packaging AROS Developer support for $target"
    AROS_ROOT="$AROS_ROOT" run \
        "$SCRIPT_DIR/aros-package-developer.sh" --target "$target"
}

write_package_checksums() {
    local package_dir="$AROS_ROOT/packages"
    local package_version
    local manifest
    local temporary_manifest

    [ "$NO_PACKAGE" -eq 0 ] || return 0
    [ -d "$package_dir" ] || die "AROS package directory is unavailable: $package_dir"

    package_version="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' \
        "$ROOT/mk/version.mk")"
    [ -n "$package_version" ] || die "could not read FreeBASIC version"

    manifest="$package_dir/FreeBASIC-${package_version}-r${PACKAGE_REVISION}-aros-SHA256SUMS"
    temporary_manifest="$(mktemp "$package_dir/.FreeBASIC-aros-SHA256SUMS.XXXXXX")"

    # The manifest is written through a temporary file so a failed hash pass
    # cannot replace the last complete release checksum list with a partial
    # one.  Restrict the input to this release's top-level package downloads;
    # OMA game archives have their own package directory and release cadence.
    if ! (
        cd "$package_dir"
        find . -maxdepth 1 -type f \
            \( -name "FreeBASIC-${package_version}-r${PACKAGE_REVISION}-aros-*.pkg" \
               -o -name "FreeBASIC-${package_version}-r${PACKAGE_REVISION}-aros-*.zip" \
               -o -name "FreeBASIC-Developer-${package_version}-r${PACKAGE_REVISION}-aros-*.pkg" \
               -o -name "FreeBASIC-Developer-${package_version}-r${PACKAGE_REVISION}-aros-*.zip" \) \
            -printf '%f\0' | sort -z | xargs -0 -r sha256sum
    ) > "$temporary_manifest"; then
        rm -f -- "$temporary_manifest"
        die "could not calculate AROS package checksums"
    fi

    [ -s "$temporary_manifest" ] || {
        rm -f -- "$temporary_manifest"
        die "no AROS package archives were available for checksumming"
    }

    msg "writing AROS package checksum manifest"
    run chmod 644 "$temporary_manifest"
    run mv "$temporary_manifest" "$manifest"
}

##############################################################################
# Main workflow
##############################################################################

install_dependencies

if [ "$SKIP_AROS_BUILD" -eq 0 ]; then
    prepare_aros_source
    if [ "$SKIP_NATIVE_TOOLCHAIN" -eq 0 ]; then
        prepare_native_prerequisites
    fi
    for_each_target configure_aros_target
    for_each_target build_aros_target
    for_each_target build_native_toolchain_target
fi

if [ "$SKIP_FREEBASIC_BUILD" -eq 0 ]; then
    build_host_compiler
    for_each_target build_libffi_target
    for_each_target build_freebasic_target
fi

# Reuse mode skips construction of the SDKs, but the existing Developer trees
# are still release inputs and must be packaged alongside the compiler.
for_each_target package_developer_target

for_each_target package_freebasic_target

if [ "$SKIP_AROS_BUILD" -eq 0 ]; then
    for_each_target build_aros_image
fi

write_package_checksums

msg "AROS FreeBASIC build complete"
if [ "$NO_PACKAGE" -eq 0 ]; then
    find "$AROS_ROOT/packages" -maxdepth 1 -type f \
        \( -name 'FreeBASIC-*-aros-*.pkg' -o -name 'FreeBASIC-*-aros-*.zip' \) \
        -printf '%f\n' | sort
fi

# end of debianubuntu-build-freebasic-aros.sh
