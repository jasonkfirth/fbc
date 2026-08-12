#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS emulator workflow
# --------------------------------------------
#
# File: riscos-rpcemu.sh
#
# Purpose:
#
#     Build and optionally launch RPCEmu with the FreeBASIC HostFS staging
#     tree.
#
# Responsibilities:
#
#     - download and authenticate a pinned RPCEmu source archive
#     - build the Qt 5 interpreter or dynamic recompiler
#     - install the pinned RISC OS Open 5.30 IOMD ROM by default
#     - install the matching RISC OS Open HardDisc4 boot tree in HostFS
#     - validate and install an optional user-supplied RISC OS ROM
#     - select a CPU model compatible with the FreeBASIC ARM baseline
#     - select enough emulated RAM for the native compiler toolchain
#     - copy FreeBASIC build products into RPCEmu HostFS
#     - launch RPCEmu only when explicitly requested
#
# This file intentionally does NOT contain:
#
#     - unattended guest configuration
#     - FreeBASIC runtime compilation
#     - Raspberry Pi or QEMU machine setup
#
# Runtime directory ownership:
#
#     The selected work directory contains a private RPCEmu source/runtime
#     tree. HardDisc4 is installed once because RISC OS changes its attributes
#     and choices in place. The generated FreeBASIC and !GCC directories are
#     replaced as units so stale filetype variants cannot hide current build
#     products. The source HostFS directory is never modified.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults and pinned upstream input
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

WORK_DIR="$ROOT/out/riscos/rpcemu"
HOSTFS_DIR="$ROOT/out/riscos/hostfs"
ROM_FILE=""
RUN=0
INTERPRETER=0
RPCEMU_MODEL="RPCSA"
RPCEMU_MEMORY=256

RPCEMU_VERSION="0.9.5"
RPCEMU_URL="https://www.marutan.net/rpcemu/0.9.5/rpcemu-0.9.5.tar.gz"
RPCEMU_SHA256="4e4624641cd1a83af275bd6da8765d4a0e5e0c83f008f059014cc24c5c33d59e"

RISCOS_ROM_VERSION="5.30"
RISCOS_ROM_URL="https://www.riscosopen.org/zipfiles/platform/riscpc/IOMD-Soft.5.30.zip"
RISCOS_ROM_SHA256="69defdaa0429c0cb22432eb5df648cde51a73b1810cf77e6ccaded2418853dfa"
RISCOS_ROM_MEMBER="soft/!Boot/Resources/SoftLoad/riscos"

RISCOS_HARDDISC_VERSION="5.30"
RISCOS_HARDDISC_URL="https://www.riscosopen.org/zipfiles/platform/common/HardDisc4.5.30.zip"
RISCOS_HARDDISC_SHA256="2d0ae90df9412622950b05d1b95dbb07ed95c144213e7d677401a75c330c570e"
RISCOS_HARDDISC_ROOT="HardDisc4"

TEMP_DOWNLOAD=""
TEMP_ROM=""
EXTRACT_DIR=""
HARDDISC_EXTRACT_DIR=""

##############################################################################
# Helpers
##############################################################################

die() {
    echo "ERROR: $*" >&2
    exit 1
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

download_verified() {
    local url="$1"
    local destination="$2"
    local expected_sha256="$3"

    if [ ! -f "$destination" ]; then
        TEMP_DOWNLOAD="$(mktemp "$WORK_DIR/.download.XXXXXX")"
        curl -fL "$url" -o "$TEMP_DOWNLOAD"
        printf '%s  %s\n' "$expected_sha256" "$TEMP_DOWNLOAD" |
            sha256sum -c -
        mv "$TEMP_DOWNLOAD" "$destination"
        TEMP_DOWNLOAD=""
    fi

    printf '%s  %s\n' "$expected_sha256" "$destination" | sha256sum -c -
}

cleanup() {
    if [ -n "$TEMP_DOWNLOAD" ] &&
       [[ "$TEMP_DOWNLOAD" == "$WORK_DIR"/.download.* ]] &&
       [ -f "$TEMP_DOWNLOAD" ]; then
        rm -f -- "$TEMP_DOWNLOAD"
    fi

    if [ -n "$TEMP_ROM" ] &&
       [[ "$TEMP_ROM" == "$SOURCE_DIR"/roms/.riscos-open.* ]] &&
       [ -f "$TEMP_ROM" ]; then
        rm -f -- "$TEMP_ROM"
    fi

    if [ -n "$EXTRACT_DIR" ] &&
       [[ "$EXTRACT_DIR" == "$WORK_DIR"/extract.* ]] &&
       [ -d "$EXTRACT_DIR" ]; then
        rm -rf -- "$EXTRACT_DIR"
    fi

    if [ -n "$HARDDISC_EXTRACT_DIR" ] &&
       [[ "$HARDDISC_EXTRACT_DIR" == "$WORK_DIR"/harddisc.* ]] &&
       [ -d "$HARDDISC_EXTRACT_DIR" ]; then
        rm -rf -- "$HARDDISC_EXTRACT_DIR"
    fi
}

valid_rom_size() {
    local rom="$1"
    local size

    [ -f "$rom" ] || return 1
    size="$(wc -c < "$rom")"

    # RPCEmu accepts combined ROM images in two-MiB increments from 2 to 8 MiB.
    case "$size" in
        2097152|4194304|6291456|8388608) return 0 ;;
        *) return 1 ;;
    esac
}

has_valid_installed_rom() {
    local candidate

    for candidate in "$SOURCE_DIR"/roms/*; do
        if valid_rom_size "$candidate"; then
            return 0
        fi
    done

    return 1
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-rpcemu.sh [options]

Options:
  --workdir DIR     Managed RPCEmu source/runtime directory.
                    Default: out/riscos/rpcemu
  --hostfs DIR      HostFS tree copied into RPCEmu.
                    Default: out/riscos/hostfs
  --rom FILE        Install this ROM instead of RISC OS Open 5.30.
                    The ROM must be a combined 2, 4, 6, or 8 MiB image.
  --model MODEL     RPCEmu machine model. Default: RPCSA
                    Values: RPC610, RPC710, RPCSA, A7000, A7000+, RPC810
  --memory MIB      Emulated RAM. Default: 256 (RPCEmu maximum)
                    Values: 4, 8, 16, 32, 64, 128, 256
  --interpreter     Build/use the interpreter instead of the recompiler.
  --run             Launch RPCEmu after preparing it.
  --jobs N          Parallel build jobs.
  -h, --help        Show this help.

RPCEmu 0.9.5 needs Qt 5 development tools, including qmake and Qt Multimedia.
By default, this script downloads and verifies the official RISC OS Open 5.30
IOMD soft-load ROM and matching HardDisc4 boot tree. No separately supplied
ROM or system image is required.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

JOBS="$(detect_jobs)"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --workdir)
            require_value "$1" "${2-}"
            WORK_DIR="$2"
            shift 2
            ;;
        --hostfs)
            require_value "$1" "${2-}"
            HOSTFS_DIR="$2"
            shift 2
            ;;
        --rom)
            require_value "$1" "${2-}"
            ROM_FILE="$2"
            shift 2
            ;;
        --model)
            require_value "$1" "${2-}"
            RPCEMU_MODEL="$2"
            shift 2
            ;;
        --memory)
            require_value "$1" "${2-}"
            RPCEMU_MEMORY="$2"
            shift 2
            ;;
        --interpreter)
            INTERPRETER=1
            shift
            ;;
        --run)
            RUN=1
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

case "$RPCEMU_MODEL" in
    RPC610|RPC710|RPCSA|A7000|A7000+|RPC810) ;;
    *) die "unsupported RPCEmu model: $RPCEMU_MODEL" ;;
esac

case "$RPCEMU_MEMORY" in
    4|8|16|32|64|128|256) ;;
    *) die "unsupported RPCEmu memory size: $RPCEMU_MEMORY MiB" ;;
esac

##############################################################################
# Host tools and paths
##############################################################################

for tool in curl sha256sum tar unzip find make sed; do
    command -v "$tool" >/dev/null 2>&1 || die "required host tool not found: $tool"
done

mkdir -p "$WORK_DIR"
WORK_DIR="$(cd "$WORK_DIR" && pwd)"
[ "$WORK_DIR" != "/" ] || die "refusing to use the filesystem root as --workdir"
[ "$HOSTFS_DIR" != "/" ] || die "refusing to copy the filesystem root as HostFS"

SOURCE_DIR="$WORK_DIR/source"
ARCHIVE="$WORK_DIR/rpcemu-$RPCEMU_VERSION.tar.gz"
ROM_ARCHIVE="$WORK_DIR/riscos-open-iomd-$RISCOS_ROM_VERSION.zip"
OPEN_ROM_FILE="$SOURCE_DIR/roms/RISCOS-Open-$RISCOS_ROM_VERSION"
HARDDISC_ARCHIVE="$WORK_DIR/riscos-open-harddisc4-$RISCOS_HARDDISC_VERSION.zip"
HARDDISC_MARKER="$WORK_DIR/harddisc4-$RISCOS_HARDDISC_VERSION.ready"

##############################################################################
# Authenticated source acquisition
##############################################################################

if [ ! -f "$SOURCE_DIR/src/qt5/rpcemu.pro" ]; then
    [ ! -e "$SOURCE_DIR" ] ||
        die "$SOURCE_DIR exists but is not a complete RPCEmu source tree"

    download_verified "$RPCEMU_URL" "$ARCHIVE" "$RPCEMU_SHA256"

    EXTRACT_DIR="$(mktemp -d "$WORK_DIR/extract.XXXXXX")"
    tar -xzf "$ARCHIVE" -C "$EXTRACT_DIR"

    mapfile -t extracted_roots < <(
        find "$EXTRACT_DIR" -mindepth 1 -maxdepth 1 -type d -print
    )
    [ "${#extracted_roots[@]}" -eq 1 ] ||
        die "RPCEmu archive did not contain exactly one source directory"
    [ -f "${extracted_roots[0]}/src/qt5/rpcemu.pro" ] ||
        die "RPCEmu archive did not contain the expected Qt 5 project"

    mv "${extracted_roots[0]}" "$SOURCE_DIR"
    rmdir "$EXTRACT_DIR"
    EXTRACT_DIR=""
fi

##############################################################################
# Emulator build
##############################################################################

if [ "$INTERPRETER" -eq 1 ]; then
    RPCEMU_CONFIG=interpreter
    BINARY="$SOURCE_DIR/rpcemu-interpreter"
else
    RPCEMU_CONFIG=dynarec
    BINARY="$SOURCE_DIR/rpcemu-recompiler"
fi

if [ ! -x "$BINARY" ]; then
    QMAKE=""
    for candidate in qmake qmake-qt5 qmake5; do
        if command -v "$candidate" >/dev/null 2>&1; then
            QMAKE="$(command -v "$candidate")"
            break
        fi
    done
    [ -n "$QMAKE" ] || die "Qt 5 qmake was not found"

    QT_VERSION="$("$QMAKE" -query QT_VERSION 2>/dev/null || true)"
    case "$QT_VERSION" in
        5.*) ;;
        *) die "$QMAKE reports Qt $QT_VERSION; RPCEmu 0.9.5 requires Qt 5" ;;
    esac

    (
        cd "$SOURCE_DIR/src/qt5"

        # RPCEmu 0.9.5 defines a compatibility typedef named bool.  GCC 15
        # treats bool as a C23 keyword, so retain the C dialect used when this
        # release was published instead of depending on the host GCC default.
        "$QMAKE" rpcemu.pro \
            CONFIG+=release \
            CONFIG+="$RPCEMU_CONFIG" \
            "QMAKE_CFLAGS+=-std=gnu17"
        make -j"$JOBS"
    )
fi

[ -x "$BINARY" ] || die "RPCEmu build did not produce $BINARY"

RPCEMU_CONFIG_FILE="$SOURCE_DIR/rpc.cfg"
[ -f "$RPCEMU_CONFIG_FILE" ] ||
    die "RPCEmu source tree does not contain rpc.cfg"
grep -q '^model=' "$RPCEMU_CONFIG_FILE" ||
    die "RPCEmu rpc.cfg does not contain a model setting"
sed -i "s/^model=.*/model=$RPCEMU_MODEL/" "$RPCEMU_CONFIG_FILE"
grep -q "^model=$RPCEMU_MODEL$" "$RPCEMU_CONFIG_FILE" ||
    die "failed to select RPCEmu model $RPCEMU_MODEL"
grep -q '^mem_size=' "$RPCEMU_CONFIG_FILE" ||
    die "RPCEmu rpc.cfg does not contain a memory setting"
sed -i "s/^mem_size=.*/mem_size=$RPCEMU_MEMORY/" "$RPCEMU_CONFIG_FILE"
grep -q "^mem_size=$RPCEMU_MEMORY$" "$RPCEMU_CONFIG_FILE" ||
    die "failed to select $RPCEMU_MEMORY MiB of emulated RAM"

##############################################################################
# ROM, boot tree, and HostFS preparation
##############################################################################

mkdir -p "$SOURCE_DIR/roms"

if [ -n "$ROM_FILE" ]; then
    [ -f "$ROM_FILE" ] || die "ROM not found: $ROM_FILE"
    valid_rom_size "$ROM_FILE" ||
        die "ROM must be a combined 2, 4, 6, or 8 MiB image"

    INSTALLED_ROM="$SOURCE_DIR/roms/$(basename "$ROM_FILE")"
    if [ ! "$ROM_FILE" -ef "$INSTALLED_ROM" ]; then
        cp "$ROM_FILE" "$INSTALLED_ROM"
    fi
else
    download_verified "$RISCOS_ROM_URL" "$ROM_ARCHIVE" "$RISCOS_ROM_SHA256"

    mapfile -t rom_members < <(
        unzip -Z1 "$ROM_ARCHIVE" |
            while IFS= read -r member; do
                if [ "$member" = "$RISCOS_ROM_MEMBER" ]; then
                    printf '%s\n' "$member"
                fi
            done
    )
    [ "${#rom_members[@]}" -eq 1 ] ||
        die "RISC OS Open archive did not contain exactly one expected ROM"

    if ! valid_rom_size "$OPEN_ROM_FILE"; then
        TEMP_ROM="$(mktemp "$SOURCE_DIR/roms/.riscos-open.XXXXXX")"
        unzip -p "$ROM_ARCHIVE" "$RISCOS_ROM_MEMBER" > "$TEMP_ROM"
        valid_rom_size "$TEMP_ROM" ||
            die "RISC OS Open archive did not produce a valid combined ROM"
        mv "$TEMP_ROM" "$OPEN_ROM_FILE"
        TEMP_ROM=""
    fi
fi

mkdir -p "$SOURCE_DIR/hostfs"
RUNTIME_BOOT_FILE="$SOURCE_DIR/hostfs/!Boot/!Run,feb"

if [ ! -f "$HARDDISC_MARKER" ]; then
    if [ ! -f "$RUNTIME_BOOT_FILE" ]; then
        download_verified \
            "$RISCOS_HARDDISC_URL" \
            "$HARDDISC_ARCHIVE" \
            "$RISCOS_HARDDISC_SHA256"

        HARDDISC_EXTRACT_DIR="$(mktemp -d "$WORK_DIR/harddisc.XXXXXX")"

        # Info-ZIP's -F option converts the Acorn filetype metadata stored in
        # the ZIP archive into the comma suffixes understood by HostFS.
        unzip -q -F "$HARDDISC_ARCHIVE" -d "$HARDDISC_EXTRACT_DIR"

        HARDDISC_TREE="$HARDDISC_EXTRACT_DIR/$RISCOS_HARDDISC_ROOT"
        [ -f "$HARDDISC_TREE/!Boot/!Run,feb" ] ||
            die "HardDisc4 archive did not contain the expected boot tree"

        cp -a "$HARDDISC_TREE/." "$SOURCE_DIR/hostfs/"
        rm -rf -- "$HARDDISC_EXTRACT_DIR"
        HARDDISC_EXTRACT_DIR=""
    fi

    # Existing work directories created by older script versions already have
    # a valid boot tree but no marker. Preserve their RISC OS choices and file
    # attributes instead of trying to overlay read-only files.
    [ -f "$RUNTIME_BOOT_FILE" ] ||
        die "HardDisc4 installation did not produce the expected boot tree"
    : > "$HARDDISC_MARKER"
fi

[ -f "$RUNTIME_BOOT_FILE" ] ||
    die "HardDisc4 marker exists but the boot tree is incomplete"

for staged_leaf in FreeBASIC '!GCC'; do
    if [ -d "$HOSTFS_DIR/$staged_leaf" ]; then
        runtime_staged_dir="$SOURCE_DIR/hostfs/$staged_leaf"
        [ "$runtime_staged_dir" != "/" ] ||
            die "refusing to replace the filesystem root"
        rm -rf -- "$runtime_staged_dir"
    fi
done

if [ -d "$HOSTFS_DIR" ]; then
    cp -a "$HOSTFS_DIR/." "$SOURCE_DIR/hostfs/"
fi

if [ "$RUN" -eq 1 ] && ! has_valid_installed_rom; then
    die "--run requires a valid ROM in $SOURCE_DIR/roms"
fi

echo "==> RPCEmu ready: $BINARY"
echo "    Runtime directory: $SOURCE_DIR"
echo "    RISC OS boot tree and FreeBASIC files are installed in HostFS."

if [ "$RUN" -eq 1 ]; then
    (
        cd "$SOURCE_DIR"
        exec "$BINARY"
    )
fi

# end of riscos-rpcemu.sh
