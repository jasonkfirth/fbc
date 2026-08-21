#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS release workflow
# -------------------------------------------
#
# File: riscos-build-support-packages.sh
#
# Purpose:
#
#     Create the locally-built RiscPkg archives required to compile and run
#     FreeBASIC programs on a clean RISC OS installation.
#
# Responsibilities:
#
#     - convert GCCSDK's native static ARM tools to RISC OS AIF files
#     - package the native GCC4 application and its command-line bootstrap
#     - package the SharedUnixLibrary system module
#     - package the DigitalRenderer system module used by sfxlib
#     - verify archive file types and required installed payloads
#
# This file intentionally does NOT contain:
#
#     - GCCSDK or FreeBASIC compilation
#     - FreeBASIC application package construction
#     - RISC OS desktop automation or package installation
#
# Output ownership:
#
#     The three named archives in the selected package directory are replaced.
#     Temporary staging directories below that directory are removed on exit.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

NATIVE_ROOT="${GCCSDK_NATIVE_ROOT:-$ROOT/out/riscos/gccsdk/gcc4/release-area/full}"
TOOLCHAIN_BIN="${GCCSDK_INSTALL_CROSSBIN:-$ROOT/out/riscos/gccsdk/cross/bin}"
HOSTFS_ROOT="${RISCOS_HOSTFS_ROOT:-$ROOT/out/riscos/hostfs}"
PACKAGE_OUTDIR="${RISCOS_PACKAGE_OUTDIR:-$ROOT/out/riscos/packages}"
PACKAGE_REVISION="${RISCOS_PACKAGE_REVISION:-1}"
PACKAGE_MAINTAINER="${RISCOS_PACKAGE_MAINTAINER:-SJ_Zero <sj@fbxl.net>}"

ELF2AIF=""
NATIVE_ELF2AIF=""
TEMP_ROOT=""

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

require_value() {
    local option="$1"
    local value="${2-}"

    [ -n "$value" ] || die "$option requires a value"
}

cleanup() {
    if [ -n "$TEMP_ROOT" ] &&
       [[ "$TEMP_ROOT" == "$PACKAGE_OUTDIR"/.support-package.* ]] &&
       [ -d "$TEMP_ROOT" ]; then
        rm -rf -- "$TEMP_ROOT"
    fi
}

suffix_swap_tree() {
    local tree="$1"
    shift

    local directory
    local file
    local leaf
    local stem
    local suffix

    for suffix in "$@"; do
        while IFS= read -r -d '' file; do
            directory="$(dirname "$file")"
            leaf="$(basename "$file")"
            stem="${leaf%.$suffix}"
            mkdir -p "$directory/$suffix"
            mv "$file" "$directory/$suffix/$stem"
        done < <(find "$tree" -type f -name "*.$suffix" -print0)
    done
}

convert_static_programs() {
    local tree="$1"
    local file
    local description

    while IFS= read -r -d '' file; do
        description="$(file -b "$file")"
        if [[ "$description" == *"ELF 32-bit"* ]] &&
           [[ "$description" == *"ARM"* ]] &&
           [[ "$description" == *"statically linked"* ]]; then
            cp "$file" "$file,ff8"
            "$ELF2AIF" "$file,ff8"
            rm -f -- "$file"
        fi
    done < <(find "$tree" -type f -print0)
}

add_gccsdk_suffix_mappings() {
    local run_file="$1"

    [ -s "$run_file" ] || die "GCCSDK !Run file is missing: $run_file"
    grep -q 'UnixEnv\$gcc\$sfix' "$run_file" ||
        die "GCCSDK !Run file has no gcc suffix mapping"

    # GCCSDK's stock launcher does not list archive or linker-script suffixes.
    # FreeBASIC stores its RISC OS libraries in UnixLib suffix directories
    # (a/libfb and x/*), so every linker process that can open those files must
    # receive the same two mappings after the upstream defaults are installed.
    printf '%s\n' \
        '' \
        '| FreeBASIC RISC OS package suffix additions' \
        '| -------------------------------------------' \
        '|' \
        '| Static FreeBASIC libraries are installed under the UnixLib a' \
        "| suffix directory.  GCCSDK's stock list omits a and x, which" \
        '| prevents gcc, collect2, and ld from locating libfb and scripts.' \
        'Set UnixEnv$gcc$sfix "<UnixEnv$gcc$sfix>:a:x"' \
        'Set UnixEnv$collect2$sfix "<UnixEnv$collect2$sfix>:a:x"' \
        'Set UnixEnv$as$sfix "<UnixEnv$as$sfix>:a:x"' \
        'Set UnixEnv$ld$sfix "<UnixEnv$ld$sfix>:a:x"' \
        '' \
        '| end of FreeBASIC RISC OS package suffix additions' \
        >> "$run_file"
}

write_copyright() {
    local destination="$1"
    local title="$2"
    local licence="$3"

    printf '%s\n' \
        "$title" \
        "$(printf '=%.0s' $(seq 1 "${#title}"))" \
        '' \
        'This archive contains binaries built from the GCCSDK source tree.' \
        'The GCCSDK project distributes the corresponding source and licence' \
        'information at:' \
        '' \
        'https://www.riscos.info/index.php/GCCSDK' \
        '' \
        "The included components are distributed under the $licence." \
        > "$destination"
}

archive_package() {
    local stage="$1"
    local archive="$2"
    shift 2

    python3 "$SCRIPT_DIR/riscos-zip.py" create "$stage" "$archive"
    python3 "$SCRIPT_DIR/riscos-zip.py" check "$archive" "$@"
}

find_native_elf2aif() {
    local candidate
    local freebasic_archive

    for candidate in \
        "$HOSTFS_ROOT/FreeBASIC/bin/elf2aif,ff8" \
        "$ROOT/out/riscos/programs/elf2aif,ff8"; do
        if [ -s "$candidate" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    freebasic_archive="$(find "$PACKAGE_OUTDIR" -maxdepth 1 -type f \
        -name 'FreeBASIC_*.zip' -print | sort | tail -n 1)"
    if [ -n "$freebasic_archive" ]; then
        candidate="$TEMP_ROOT/elf2aif,ff8"
        unzip -p "$freebasic_archive" \
            'Apps/Development/!FreeBASIC/bin/elf2aif' > "$candidate"
        if [ -s "$candidate" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    fi

    return 1
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-build-support-packages.sh [options]

Options:
  --native-root DIR    GCCSDK native release root containing !GCC.
  --toolchain-bin DIR  GCCSDK cross-tool directory containing elf2aif.
  --hostfs-root DIR    Native FreeBASIC staging root used for native elf2aif.
  --package-dir DIR    RiscPkg archive output directory.
  --revision N         Package revision. Default: 1
  -h, --help           Show this help.

The command writes GCC4_4.7.4-Rel6-N.zip,
SharedUnixLibrary_1.16-N.zip, and DRenderer_0.56-r-N.zip.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing and input validation
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --native-root)
            require_value "$1" "${2-}"
            NATIVE_ROOT="$2"
            shift 2
            ;;
        --toolchain-bin)
            require_value "$1" "${2-}"
            TOOLCHAIN_BIN="$2"
            shift 2
            ;;
        --hostfs-root)
            require_value "$1" "${2-}"
            HOSTFS_ROOT="$2"
            shift 2
            ;;
        --package-dir)
            require_value "$1" "${2-}"
            PACKAGE_OUTDIR="$2"
            shift 2
            ;;
        --revision)
            require_value "$1" "${2-}"
            PACKAGE_REVISION="$2"
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

case "$PACKAGE_REVISION" in
    ''|*[!0-9]*) die "--revision must contain only digits" ;;
esac

for tool in file find grep python3 unzip; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

[ -d "$NATIVE_ROOT/!GCC" ] ||
    die "native GCCSDK tree not found: $NATIVE_ROOT/!GCC"
[ -x "$TOOLCHAIN_BIN/elf2aif" ] ||
    die "GCCSDK cross elf2aif not found: $TOOLCHAIN_BIN/elf2aif"
[ -s "$NATIVE_ROOT/!GCC/bin/sul" ] ||
    die "SharedUnixLibrary module not found in the native GCCSDK tree"
[ -s "$TOOLCHAIN_BIN/../module/DRenderer,ffa" ] ||
    die "DigitalRenderer module not found beside the GCCSDK cross tools"
grep -a -q 'SharedUnixLibrary' "$NATIVE_ROOT/!GCC/bin/sul" ||
    die "native GCCSDK sul file is not a SharedUnixLibrary module"
grep -a -q 'DigitalRenderer' "$TOOLCHAIN_BIN/../module/DRenderer,ffa" ||
    die "GCCSDK module is not a DigitalRenderer module"

mkdir -p "$PACKAGE_OUTDIR"
PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
TEMP_ROOT="$(mktemp -d "$PACKAGE_OUTDIR/.support-package.XXXXXX")"
ELF2AIF="$TOOLCHAIN_BIN/elf2aif"
NATIVE_ELF2AIF="$(find_native_elf2aif)" ||
    die "native elf2aif is unavailable; build FreeBASIC before packaging GCC4"

##############################################################################
# GCC4 package
##############################################################################

msg "preparing the native GCC4 package payload"
GCC_STAGE="$TEMP_ROOT/gcc"
GCC_APP="$GCC_STAGE/Apps/Development/!GCC"
GCC_SKELETON="$NATIVE_ROOT/../../riscos/dist/!GCC"

[ -d "$GCC_SKELETON" ] ||
    die "GCCSDK !GCC skeleton not found: $GCC_SKELETON"
mkdir -p "$GCC_APP" "$GCC_STAGE/RiscPkg"
cp -a "$NATIVE_ROOT/!GCC/." "$GCC_APP/"
cp -a "$GCC_SKELETON/." "$GCC_APP/"
add_gccsdk_suffix_mappings "$GCC_APP/!Run,feb"
convert_static_programs "$GCC_APP"
# UnixLib's pathname translation represents a Unix suffix as a RISC OS
# directory.  The native GCCSDK release tree still has its host-facing
# libgcc.a and libunixlib.a leaf names, while the package launcher advertises
# the a suffix mapping.  Keep the package payload and launcher consistent so
# that collect2 and ld can resolve -lgcc and -lunixlib after a clean install.
suffix_swap_tree "$GCC_APP" c cc tcc cmhg s h o adb ads ali a x
cp "$NATIVE_ELF2AIF" "$GCC_APP/bin/elf2aif,ff8"

printf '%s\n' \
    'Package: GCC4' \
    "Version: 4.7.4-Rel6-$PACKAGE_REVISION" \
    'Priority: Optional' \
    'Section: Development' \
    "Maintainer: $PACKAGE_MAINTAINER" \
    'Standards-Version: 0.4.0' \
    'Licence: Free' \
    'Environment: arm' \
    'Depends: SharedUnixLibrary (>= 1.12)' \
    'Components: Apps.Development.!GCC (Movable LookAt)' \
    'Description: GCCSDK native C/C++ compiler and core build tools' \
    'Homepage: https://www.riscos.info/index.php/GCCSDK' \
    > "$GCC_STAGE/RiscPkg/Control,fff"
write_copyright "$GCC_STAGE/RiscPkg/Copyright,fff" \
    'GCC4 native compiler and build tools' 'GNU General Public License'

archive_package "$GCC_STAGE" \
    "$PACKAGE_OUTDIR/GCC4_4.7.4-Rel6-$PACKAGE_REVISION.zip" \
    --require 'RiscPkg/Control=fff' \
    --require 'RiscPkg/Copyright=fff' \
    --require 'Apps/Development/!GCC/!Boot=feb' \
    --require 'Apps/Development/!GCC/!Run=feb' \
    --require 'Apps/Development/!GCC/bin/gcc=ff8' \
    --require 'Apps/Development/!GCC/bin/elf2aif=ff8' \
    --require 'Apps/Development/!GCC/lib/a/libunixlib=fff' \
    --require 'Apps/Development/!GCC/lib/gcc/arm-unknown-riscos/4.7.4/a/libgcc=fff'

##############################################################################
# SharedUnixLibrary and DigitalRenderer packages
##############################################################################

msg "packaging the SharedUnixLibrary module"
SUL_STAGE="$TEMP_ROOT/shared-unix-library"
mkdir -p "$SUL_STAGE/System/310/Modules" "$SUL_STAGE/RiscPkg"
cp "$NATIVE_ROOT/!GCC/bin/sul" \
    "$SUL_STAGE/System/310/Modules/SharedULib,ffa"
printf '%s\n' \
    'Package: SharedUnixLibrary' \
    "Version: 1.16-$PACKAGE_REVISION" \
    'Priority: Required' \
    'Section: Library' \
    "Maintainer: $PACKAGE_MAINTAINER" \
    'Standards-Version: 0.4.0' \
    'Licence: Free' \
    'Environment: arm' \
    'Description: C runtime support module required by UnixLib applications' \
    'Homepage: https://www.riscos.info/index.php/GCCSDK' \
    > "$SUL_STAGE/RiscPkg/Control,fff"
write_copyright "$SUL_STAGE/RiscPkg/Copyright,fff" \
    'SharedUnixLibrary module' 'GNU Lesser General Public License'

archive_package "$SUL_STAGE" \
    "$PACKAGE_OUTDIR/SharedUnixLibrary_1.16-$PACKAGE_REVISION.zip" \
    --require 'RiscPkg/Control=fff' \
    --require 'RiscPkg/Copyright=fff' \
    --require 'System/310/Modules/SharedULib=ffa'

msg "packaging the DigitalRenderer module"
DRENDERER_STAGE="$TEMP_ROOT/drenderer"
mkdir -p "$DRENDERER_STAGE/System/310/Modules" "$DRENDERER_STAGE/RiscPkg"
cp "$TOOLCHAIN_BIN/../module/DRenderer,ffa" \
    "$DRENDERER_STAGE/System/310/Modules/DRenderer,ffa"
printf '%s\n' \
    'Package: DRenderer' \
    "Version: 0.56-r-$PACKAGE_REVISION" \
    'Priority: Optional' \
    'Section: Library' \
    "Maintainer: $PACKAGE_MAINTAINER" \
    'Standards-Version: 0.4.0' \
    'Licence: Free' \
    'Environment: arm' \
    'Description: Digital audio playback module for UnixLib applications' \
    'Homepage: https://www.riscos.info/index.php/GCCSDK' \
    > "$DRENDERER_STAGE/RiscPkg/Control,fff"
write_copyright "$DRENDERER_STAGE/RiscPkg/Copyright,fff" \
    'DigitalRenderer module' 'GNU General Public License'

archive_package "$DRENDERER_STAGE" \
    "$PACKAGE_OUTDIR/DRenderer_0.56-r-$PACKAGE_REVISION.zip" \
    --require 'RiscPkg/Control=fff' \
    --require 'RiscPkg/Copyright=fff' \
    --require 'System/310/Modules/DRenderer=ffa'

echo
echo '==> RISC OS support packages ready'
echo "    GCC4:              $PACKAGE_OUTDIR/GCC4_4.7.4-Rel6-$PACKAGE_REVISION.zip"
echo "    SharedUnixLibrary: $PACKAGE_OUTDIR/SharedUnixLibrary_1.16-$PACKAGE_REVISION.zip"
echo "    DRenderer:         $PACKAGE_OUTDIR/DRenderer_0.56-r-$PACKAGE_REVISION.zip"

# end of riscos-build-support-packages.sh
