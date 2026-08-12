#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS release workflow
# -------------------------------------------
#
# File: debianubuntu-build-freebasic-riscos.sh
#
# Purpose:
#
#     Install host prerequisites, build FreeBASIC for RISC OS, and create an
#     installable RiscPkg archive on Debian or Ubuntu.
#
# Responsibilities:
#
#     - install the GCCSDK, FreeBASIC, package, and RPCEmu host prerequisites
#     - build the GCCSDK cross and native compiler toolchains
#     - build and stage native FreeBASIC, its libraries, headers, and examples
#     - build the open RISC OS Open RPCEmu environment used by fbctests
#     - create and validate an Acorn-metadata RiscPkg ZIP archive
#
# This file intentionally does NOT contain:
#
#     - RISC OS desktop automation
#     - fbctests source selection or result parsing
#     - system-wide host compiler installation
#
# Output ownership:
#
#     Package staging uses a private temporary directory below the selected
#     package output directory. Existing GCCSDK, RPCEmu, and HostFS trees are
#     managed by their dedicated scripts.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_DEPS=0
NO_BUILD=0
NO_PACKAGE=0
PREPARE_EMULATOR=1
GCCSDK_REVISION=""
UPDATE_GCCSDK=0

HOSTFS_ROOT="${RISCOS_HOSTFS_ROOT:-$ROOT/out/riscos/hostfs}"
PACKAGE_OUTDIR="${RISCOS_PACKAGE_OUTDIR:-$ROOT/out/riscos/packages}"
PACKAGE_REVISION="${RISCOS_PACKAGE_REVISION:-1}"
PACKAGE_MAINTAINER="${RISCOS_PACKAGE_MAINTAINER:-SJ_Zero <sj@fbxl.net>}"
PACKAGE_STAGE=""

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

cleanup() {
    if [ -n "$PACKAGE_STAGE" ] &&
       [[ "$PACKAGE_STAGE" == "$PACKAGE_OUTDIR"/.package-stage.* ]] &&
       [ -d "$PACKAGE_STAGE" ]; then
        rm -rf -- "$PACKAGE_STAGE"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic-riscos.sh [options]

Options:
  --skip-deps       Skip Debian/Ubuntu prerequisite installation.
  --no-build        Reuse the existing out/riscos/hostfs/FreeBASIC staging.
  --no-package      Stop after building and staging FreeBASIC.
  --no-emulator     Do not build or refresh the RPCEmu test environment.
  --revision REV    Pin the GCCSDK Subversion checkout to REV.
  --update          Update an existing unpinned GCCSDK checkout.
  --jobs N          Parallel build jobs. Default: detected CPU count
  -h, --help        Show this help.

Environment:
  RISCOS_HOSTFS_ROOT          Native build staging root.
  RISCOS_PACKAGE_OUTDIR      RiscPkg archive output directory.
  RISCOS_PACKAGE_REVISION    Package revision. Default: 1
  RISCOS_PACKAGE_MAINTAINER  RiscPkg maintainer field.
  JOBS                        Parallel build jobs.

The installable archive is written to:
  out/riscos/packages/FreeBASIC_<version>-<revision>.zip

The package installs Apps.Development.!FreeBASIC and declares its native GCC4
dependency. RPCEmu uses the open RISC OS Open 5.30 ROM and HardDisc4 image.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --skip-deps)
            SKIP_DEPS=1
            shift
            ;;
        --no-build)
            NO_BUILD=1
            shift
            ;;
        --no-package)
            NO_PACKAGE=1
            shift
            ;;
        --no-emulator)
            PREPARE_EMULATOR=0
            shift
            ;;
        --revision)
            require_value "$1" "${2-}"
            GCCSDK_REVISION="$2"
            shift 2
            ;;
        --update)
            UPDATE_GCCSDK=1
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

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

case "$PACKAGE_REVISION" in
    ''|*[!0-9]*) die "RISCOS_PACKAGE_REVISION must contain only digits" ;;
esac

command -v apt-get >/dev/null 2>&1 ||
    die "this script requires an APT-based Debian or Ubuntu host"

##############################################################################
# Prerequisites and build
##############################################################################

install_dependencies() {
    [ "$SKIP_DEPS" -eq 0 ] || return 0

    msg "installing Debian/Ubuntu RISC OS build dependencies"
    export DEBIAN_FRONTEND=noninteractive
    run_root apt-get update
    run_root apt-get install -y --no-install-recommends \
        autogen \
        bison \
        build-essential \
        bzip2 \
        ca-certificates \
        cmake \
        curl \
        file \
        flex \
        git \
        gperf \
        help2man \
        libasound2-dev \
        libgmp-dev \
        libmpc-dev \
        libmpfr-dev \
        libncurses-dev \
        libqt5multimedia5-plugins \
        m4 \
        patch \
        pkg-config \
        procps \
        python3 \
        qt5-qmake \
        qtbase5-dev \
        qtmultimedia5-dev \
        rsync \
        subversion \
        texinfo \
        unzip \
        wget \
        xsltproc \
        zip
}

build_freebasic() {
    local gccsdk_args=(--with-native --jobs "$JOBS")

    if [ -n "$GCCSDK_REVISION" ]; then
        gccsdk_args+=(--revision "$GCCSDK_REVISION")
    elif [ "$UPDATE_GCCSDK" -eq 1 ]; then
        gccsdk_args+=(--update)
    fi

    msg "building the GCCSDK cross and native toolchains"
    run "$SCRIPT_DIR/riscos-gccsdk.sh" "${gccsdk_args[@]}"

    if [ ! -x "$ROOT/bin/fbc" ]; then
        msg "bootstrapping the host FreeBASIC compiler"
        run make -C "$ROOT" -j"$JOBS" bootstrap-minimal
    fi

    msg "refreshing the host FreeBASIC compiler"
    run make -C "$ROOT" -j"$JOBS" compiler FBC="$ROOT/bin/fbc"

    msg "building and staging native FreeBASIC"
    run "$SCRIPT_DIR/riscos-build-native.sh" \
        --hostfs-root "$HOSTFS_ROOT" \
        --with-libs \
        --jobs "$JOBS"
}

prepare_emulator() {
    [ "$PREPARE_EMULATOR" -eq 1 ] || return 0

    msg "preparing the 256 MiB RPCEmu test environment"
    run "$SCRIPT_DIR/riscos-rpcemu.sh" \
        --hostfs "$HOSTFS_ROOT" \
        --memory 256 \
        --jobs "$JOBS"
}

##############################################################################
# RiscPkg construction
##############################################################################

package_freebasic() {
    local version
    local package_version
    local package_path
    local source_stage="$HOSTFS_ROOT/FreeBASIC"
    local app_stage

    [ -d "$source_stage" ] ||
        die "native FreeBASIC staging tree not found: $source_stage"
    [ -s "$source_stage/fbc,ff8" ] ||
        die "native FreeBASIC AIF not found: $source_stage/fbc,ff8"
    [ -s "$source_stage/bin/elf2aif,ff8" ] ||
        die "patched native elf2aif not found: $source_stage/bin/elf2aif,ff8"
    [ -s "$source_stage/lib/freebasic/riscos-arm/a/libffi" ] ||
        die "RISC OS libffi not found in the native staging tree"
    [ -s "$source_stage/doc/libffi-license,fff" ] ||
        die "libffi licence not found in the native staging tree"

    version="$(sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' \
        "$ROOT/mk/version.mk" | head -n 1)"
    [ -n "$version" ] || die "could not determine the FreeBASIC version"
    package_version="$version-$PACKAGE_REVISION"

    mkdir -p "$PACKAGE_OUTDIR"
    PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
    PACKAGE_STAGE="$(mktemp -d "$PACKAGE_OUTDIR/.package-stage.XXXXXX")"
    app_stage="$PACKAGE_STAGE/Apps/Development/!FreeBASIC"

    mkdir -p "$PACKAGE_STAGE/Apps/Development" "$PACKAGE_STAGE/RiscPkg"
    cp -a "$source_stage" "$app_stage"

    # HostFS diagnostic helpers are useful while developing the port but are
    # replaced by the application's normal boot, run, and help files here.
    rm -f -- \
        "$app_stage/Compile,feb" \
        "$app_stage/SetPaths,feb" \
        "$app_stage/fbc-elf,e1f"

    cp "$ROOT/src/tools/riscos/!Boot" "$app_stage/!Boot,feb"
    cp "$ROOT/src/tools/riscos/!Run" "$app_stage/!Run,feb"
    cp "$ROOT/src/tools/riscos/!Help" "$app_stage/!Help,fff"
    cp "$ROOT/src/compiler/license.txt" "$app_stage/License,fff"

    printf '%s\n' \
        'Package: FreeBASIC' \
        "Version: $package_version" \
        'Priority: Optional' \
        'Section: Development' \
        "Maintainer: $PACKAGE_MAINTAINER" \
        'Standards-Version: 0.4.0' \
        'Licence: Free' \
        'Environment: arm' \
        'Depends: GCC4' \
        'Components: Apps.Development.!FreeBASIC (Movable LookAt)' \
        'Description: FreeBASIC compiler, runtime, and headers for RISC OS' \
        'Homepage: https://www.freebasic.net/' \
        > "$PACKAGE_STAGE/RiscPkg/Control,fff"

    printf '%s\n' \
        'FreeBASIC for RISC OS' \
        '=====================' \
        '' \
        'Copyright (C) 2004-2025 The FreeBASIC development team.' \
        '' \
        'FreeBASIC is distributed under the GNU General Public License,' \
        'version 2 or, at your option, any later version. A copy is included' \
        'as Apps.Development.!FreeBASIC.License and may also be found at:' \
        '' \
        '<Common_Licences$Dir>.GPL-2' \
        '' \
        'The bundled elf2aif converter is built from the GCCSDK source tree.' \
        'Its source and copyright information are available from GCCSDK:' \
        '' \
        'https://www.riscos.info/index.php/GCCSDK' \
        '' \
        'The bundled static ARM libffi is distributed under its permissive' \
        'license in Apps.Development.!FreeBASIC.doc.libffi-license.' \
        > "$PACKAGE_STAGE/RiscPkg/Copyright,fff"

    if find "$app_stage" -type f -printf '%f\n' | grep -q '\.'; then
        die "package staging contains a leaf with a RISC OS path separator"
    fi

    package_path="$PACKAGE_OUTDIR/FreeBASIC_${package_version}.zip"
    msg "creating RiscPkg archive"
    run python3 "$SCRIPT_DIR/riscos-zip.py" create \
        "$PACKAGE_STAGE" "$package_path"
    run python3 "$SCRIPT_DIR/riscos-zip.py" check "$package_path" \
        --require 'RiscPkg/Control=fff' \
        --require 'RiscPkg/Copyright=fff' \
        --require 'Apps/Development/!FreeBASIC/!Boot=feb' \
        --require 'Apps/Development/!FreeBASIC/!Run=feb' \
        --require 'Apps/Development/!FreeBASIC/fbc=ff8' \
        --require 'Apps/Development/!FreeBASIC/bin/elf2aif=ff8' \
        --require 'Apps/Development/!FreeBASIC/doc/libffi-license=fff' \
        --require 'Apps/Development/!FreeBASIC/lib/freebasic/riscos-arm/a/libffi=fff'

    echo
    echo "==> RISC OS package ready: $package_path"
}

##############################################################################
# Main workflow
##############################################################################

install_dependencies

if [ "$NO_BUILD" -eq 0 ]; then
    build_freebasic
fi

if [ "$NO_PACKAGE" -eq 0 ]; then
    package_freebasic
fi

prepare_emulator

echo
echo "==> FreeBASIC RISC OS build workflow completed"
if [ "$PREPARE_EMULATOR" -eq 1 ]; then
    echo "    Run fbctests with: ./build_scripts/riscos-run-fbctests.sh"
    echo "    Run examples with: ./build_scripts/riscos-run-exampleageddon.sh"
fi

# end of debianubuntu-build-freebasic-riscos.sh
