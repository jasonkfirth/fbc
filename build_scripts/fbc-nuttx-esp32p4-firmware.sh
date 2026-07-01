#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX ESP32-P4 firmware workflow
# ----------------------------------------------------
#
# File: fbc-nuttx-esp32p4-firmware.sh
#
# Purpose:
#
#     Fetch Apache NuttX, configure the ESP32-P4 Ethernet firmware profile
#     used by the FreeBASIC NuttX module workflow, build it, and optionally
#     flash it to a board in programming mode.
#
# Responsibilities:
#
#     - clone or update the external NuttX and apps repositories
#     - apply the small FreeBASIC/NuttX compatibility patches idempotently
#     - configure NuttX for Ethernet DHCP, telnet NSH, and loadable ELF modules
#     - build the firmware with the packaged RISC-V toolchain
#     - invoke the board flash target only when explicitly requested
#
# This file intentionally does NOT contain:
#
#     - a vendored copy of Apache NuttX
#     - FreeBASIC program/module upload logic
#     - board-specific LCD, camera, USB host, or audio driver bring-up
#     - partitioning or formatting of removable SD media
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project or installed SDK root
##############################################################################

START_DIR="$(pwd)"
SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT=""

find_root_from() {
    local search_dir="$1"

    while :; do
        if [ -d "$search_dir/build_scripts" ] &&
           { [ -f "$search_dir/GNUmakefile" ] ||
             [ -f "$search_dir/makefile" ] ||
             [ -f "$search_dir/Makefile" ] ||
             [ -f "$search_dir/.nuttx-sdk-root" ]; }; then
            ROOT="$search_dir"
            return 0
        fi

        [ "$search_dir" = "/" ] && break
        search_dir="$(dirname "$search_dir")"
    done

    return 1
}

for root_candidate in \
    "${FB_NUTTX_SDK_ROOT:-}" \
    "$START_DIR" \
    "$SCRIPT_DIR" \
    "$(dirname "$SCRIPT_DIR")" \
    /usr/share/freebasic/nuttx-sdk; do
    [ -n "$root_candidate" ] || continue
    find_root_from "$root_candidate" && break
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC NuttX SDK root" >&2; exit 1; }

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

max_jobs() {
    local n=1

    if command -v nproc >/dev/null 2>&1; then
        n="$(nproc)"
    elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
        n="$(getconf _NPROCESSORS_ONLN)"
    fi

    case "$n" in
        ''|*[!0-9]*) n=1 ;;
    esac

    [ "$n" -ge 1 ] || n=1
    echo "$n"
}

default_workdir() {
    if [ -n "${FB_NUTTX_ESP32P4_WORKDIR:-}" ]; then
        printf '%s\n' "$FB_NUTTX_ESP32P4_WORKDIR"
    elif [ -n "${NUTTX_WORKDIR:-}" ]; then
        printf '%s\n' "$NUTTX_WORKDIR"
    else
        printf '%s\n' "${HOME:-/tmp}/fbxl-nuttx-esp32p4"
    fi
}

default_out_dir() {
    if [ -w "$ROOT" ]; then
        printf '%s\n' "$ROOT/build/nuttx-esp32p4-firmware"
        return 0
    fi

    if [ -n "${XDG_CACHE_HOME:-}" ]; then
        printf '%s\n' "$XDG_CACHE_HOME/freebasic/nuttx-esp32p4-firmware"
    else
        printf '%s\n' "${HOME:-/tmp}/.cache/freebasic/nuttx-esp32p4-firmware"
    fi
}

safe_remove_temp_dir() {
    local target="$1"
    local parent="$2"

    case "$target" in
        "$parent"/.clone-*)
            rm -rf "$target"
            ;;
        *)
            die "refusing to remove unexpected temporary directory: $target"
            ;;
    esac
}

clone_repo() {
    local url="$1"
    local ref="$2"
    local target="$3"
    local parent
    local temp_target

    if [ -d "$target/.git" ]; then
        echo "==> repository exists: $target"
        return 0
    fi

    if [ -e "$target" ]; then
        die "$target exists but is not a git repository"
    fi

    parent="$(dirname "$target")"
    mkdir -p "$parent"
    temp_target="$parent/.clone-$(basename "$target").$$"

    safe_remove_temp_dir "$temp_target" "$parent" >/dev/null 2>&1 || true

    if git clone --depth 1 --branch "$ref" "$url" "$temp_target"; then
        mv "$temp_target" "$target"
        return 0
    fi

    safe_remove_temp_dir "$temp_target" "$parent"

    #
    # A commit hash is not accepted by "git clone --branch" on every git
    # version.  Fall back to a full clone and then check out the requested ref.
    #
    run git clone "$url" "$temp_target"
    (
        cd "$temp_target"
        run git checkout "$ref"
    )
    mv "$temp_target" "$target"
}

update_repo() {
    local target="$1"
    local ref="$2"

    [ -d "$target/.git" ] || die "$target is not a git repository"

    (
        cd "$target"
        run git fetch --tags --prune origin
        run git checkout "$ref"

        if git rev-parse --abbrev-ref --symbolic-full-name '@{u}' >/dev/null 2>&1; then
            run git pull --ff-only
        fi
    )
}

apply_patch_if_needed() {
    local patch_file="$1"
    local patch_output
    local strip

    [ -f "$patch_file" ] || die "missing NuttX patch: $patch_file"

    case "$(basename "$patch_file")" in
        riscv-elf-lo12-s-rela-addend.patch)
            if grep -Fq 'insn = (insn & 0x01fff07f) | val;' \
                libs/libc/machine/risc-v/arch_elf.c 2>/dev/null; then
                echo "==> already applied: $patch_file"
                return 0
            fi
            ;;
    esac

    if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        for strip in 0 1; do
            if git apply "-p$strip" --check "$patch_file" 2>/dev/null; then
                run git apply "-p$strip" "$patch_file"
                return 0
            fi

            if git apply "-p$strip" --reverse --check "$patch_file" 2>/dev/null; then
                echo "==> already applied: $patch_file"
                return 0
            fi
        done
    fi

    for strip in 0 1; do
        patch_output="$(patch "-p$strip" --dry-run -N < "$patch_file" 2>&1)" && {
            echo "==> patch -p$strip -N < $patch_file"
            patch "-p$strip" -N < "$patch_file"
            return 0
        }

        if echo "$patch_output" | grep -q "Reversed (or previously applied)"; then
            echo "==> already applied: $patch_file"
            return 0
        fi
    done

    die "could not apply NuttX patch: $patch_file"
}

kconfig_enable() {
    run kconfig-tweak --enable "$1"
}

kconfig_disable() {
    run kconfig-tweak --disable "$1"
}

kconfig_set_val() {
    run kconfig-tweak --set-val "$1" "$2"
}

kconfig_set_str() {
    run kconfig-tweak --set-str "$1" "$2"
}

config_enabled() {
    grep -q "^$1=y$" "$NUTTX_DIR/.config"
}

require_config_enabled() {
    config_enabled "$1" || die "$1 did not remain enabled after olddefconfig"
}

append_extra_flags_once() {
    local flags="$1"

    case " ${EXTRAFLAGS:-} " in
        *" $flags "*)
            ;;
        *)
            EXTRAFLAGS="${EXTRAFLAGS:-} $flags"
            export EXTRAFLAGS
            ;;
    esac
}

add_packaged_toolchain_includes() {
    local candidate
    local shim_dir

    candidate="$NUTTX_DIR/arch/risc-v/src/chip/esp-hal-3rdparty/components/esp_libc/platform_include"
    if [ -f "$candidate/sys/reent.h" ]; then
        append_extra_flags_once "-idirafter $candidate"
        echo "==> using ESP libc fallback headers: $candidate"

        #
        # The ESP HAL compatibility headers still expect a few newlib reent
        # declarations.  Ubuntu's packaged RISC-V embedded toolchain can be
        # paired with picolibc instead, so provide the small declarations needed
        # for prototypes without pretending to be a full newlib implementation.
        #
        shim_dir="$WORKDIR/toolchain-include"
        mkdir -p "$shim_dir/sys"
        cat > "$shim_dir/reent.h" <<'EOF'
#ifndef FB_NUTTX_GENERATED_REENT_H
#define FB_NUTTX_GENERATED_REENT_H

#include "sys/reent.h"

#endif
EOF
        cat > "$shim_dir/sys/reent.h" <<'EOF'
#ifndef FB_NUTTX_GENERATED_SYS_REENT_H
#define FB_NUTTX_GENERATED_SYS_REENT_H

struct _reent
{
    int _reserved_0;
};

#ifndef _REENT_INIT
#define _REENT_INIT(var) { 0 }
#endif

struct _glue
{
    int _dummy;
};

#endif
EOF
        append_extra_flags_once "-idirafter $shim_dir"
    fi

    command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 || return 0

    if printf '#include <sys/lock.h>\n' |
        riscv64-unknown-elf-gcc -x c -E - >/dev/null 2>&1; then
        return 0
    fi

    for candidate in /usr/lib/picolibc/riscv64-unknown-elf/include; do
        [ -f "$candidate/sys/lock.h" ] || continue

        if printf '#include <sys/lock.h>\n' |
            riscv64-unknown-elf-gcc -isystem "$candidate" -x c -E - >/dev/null 2>&1; then
            append_extra_flags_once "-isystem $candidate"
            echo "==> using picolibc headers: $candidate"
            return 0
        fi
    done
}

usage() {
    cat <<EOF
Usage: fbc-nuttx-esp32p4-firmware [options]

Options:
  --workdir DIR          Directory that will contain nuttx/ and apps/
  --nuttx-workdir DIR    Alias for --workdir
  --nuttx-url URL        NuttX repository URL
  --apps-url URL         NuttX apps repository URL
  --nuttx-ref REF        NuttX branch/tag/commit, default: master
  --apps-ref REF         apps branch/tag/commit, default: same as --nuttx-ref
  --update              Fetch and fast-forward existing repositories
  --board-config NAME    NuttX board config, default:
                         esp32p4-function-ev-board:ethernet
  --out-dir DIR          Directory for copied image artifacts
  --skip-patches         Do not apply bundled compatibility patches
  --skip-config          Reuse the existing .config
  --no-build             Stop after clone/patch/configure
  --flash                Run the NuttX flash target after a successful build
  --port DEVICE          Serial/programming port for make flash
  --baud RATE            Flash baud rate, default: 921600
  --with-ftp             Enable NuttX ftpd for file upload experiments
  --no-telnet            Do not enable telnet NSH
  --menuconfig           Run menuconfig after scripted configuration
  --help                 Show this help text

Environment:
  FB_NUTTX_ESP32P4_WORKDIR
                         Default workdir when --workdir is omitted
  NUTTX_WORKDIR          Alternate default workdir
  ESPTOOL_PORT           Default flash port when --port is omitted
  ESPTOOL_BAUD           Default flash baud when --baud is omitted
  JOBS                   Parallel make jobs

Typical board preparation:

  fbc-nuttx-esp32p4-firmware \\
    --workdir \$HOME/fbxl-nuttx-esp32p4 \\
    --flash --port /dev/ttyACM0

After the firmware is running, upload FreeBASIC programs with:

  fbc-nuttx-esp32p4 \\
    --nuttx-workdir \$HOME/fbxl-nuttx-esp32p4 \\
    --serial-port /dev/ttyACM0 --run
EOF
}

##############################################################################
# Options
##############################################################################

WORKDIR="$(default_workdir)"
NUTTX_URL="${NUTTX_URL:-https://github.com/apache/nuttx.git}"
APPS_URL="${APPS_URL:-https://github.com/apache/nuttx-apps.git}"
NUTTX_REF="${NUTTX_REF:-master}"
APPS_REF="${APPS_REF:-}"
BOARD_CONFIG="${NUTTX_BOARD_CONFIG:-esp32p4-function-ev-board:ethernet}"
OUT_DIR="${FB_NUTTX_FIRMWARE_OUT_DIR:-$(default_out_dir)}"
UPDATE_REPOS=0
APPLY_PATCHES=1
SKIP_CONFIG=0
DO_BUILD=1
DO_FLASH=0
FLASH_PORT="${ESPTOOL_PORT:-}"
FLASH_BAUD="${ESPTOOL_BAUD:-921600}"
WITH_FTP=0
WITH_TELNET=1
RUN_MENUCONFIG=0
JOBS="${JOBS:-$(max_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --workdir|--nuttx-workdir)
            [ "$#" -ge 2 ] || die "$1 requires a directory"
            WORKDIR="$2"
            shift 2
            ;;
        --nuttx-url)
            [ "$#" -ge 2 ] || die "--nuttx-url requires a URL"
            NUTTX_URL="$2"
            shift 2
            ;;
        --apps-url)
            [ "$#" -ge 2 ] || die "--apps-url requires a URL"
            APPS_URL="$2"
            shift 2
            ;;
        --nuttx-ref)
            [ "$#" -ge 2 ] || die "--nuttx-ref requires a ref"
            NUTTX_REF="$2"
            shift 2
            ;;
        --apps-ref)
            [ "$#" -ge 2 ] || die "--apps-ref requires a ref"
            APPS_REF="$2"
            shift 2
            ;;
        --update)
            UPDATE_REPOS=1
            shift
            ;;
        --board-config)
            [ "$#" -ge 2 ] || die "--board-config requires a config name"
            BOARD_CONFIG="$2"
            shift 2
            ;;
        --out-dir)
            [ "$#" -ge 2 ] || die "--out-dir requires a directory"
            OUT_DIR="$2"
            shift 2
            ;;
        --skip-patches)
            APPLY_PATCHES=0
            shift
            ;;
        --skip-config)
            SKIP_CONFIG=1
            shift
            ;;
        --no-build)
            DO_BUILD=0
            shift
            ;;
        --flash)
            DO_FLASH=1
            shift
            ;;
        --port)
            [ "$#" -ge 2 ] || die "--port requires a device"
            FLASH_PORT="$2"
            shift 2
            ;;
        --baud)
            [ "$#" -ge 2 ] || die "--baud requires a rate"
            FLASH_BAUD="$2"
            shift 2
            ;;
        --with-ftp)
            WITH_FTP=1
            shift
            ;;
        --no-telnet)
            WITH_TELNET=0
            shift
            ;;
        --menuconfig)
            RUN_MENUCONFIG=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

[ -n "$APPS_REF" ] || APPS_REF="$NUTTX_REF"

case "$WORKDIR" in
    "") die "workdir must not be empty" ;;
esac

case "$JOBS" in
    ''|*[!0-9]*) die "invalid JOBS value: $JOBS" ;;
esac

case "$FLASH_BAUD" in
    ''|*[!0-9]*) die "invalid flash baud rate: $FLASH_BAUD" ;;
esac

if [ "$DO_FLASH" -eq 1 ] && [ -z "$FLASH_PORT" ]; then
    die "--flash requires --port or ESPTOOL_PORT"
fi

NUTTX_DIR="$WORKDIR/nuttx"
APPS_DIR="$WORKDIR/apps"

##############################################################################
# Fetch NuttX
##############################################################################

clone_repo "$NUTTX_URL" "$NUTTX_REF" "$NUTTX_DIR"
clone_repo "$APPS_URL" "$APPS_REF" "$APPS_DIR"

if [ "$UPDATE_REPOS" -eq 1 ]; then
    update_repo "$NUTTX_DIR" "$NUTTX_REF"
    update_repo "$APPS_DIR" "$APPS_REF"
fi

##############################################################################
# Patch NuttX
##############################################################################

if [ "$APPLY_PATCHES" -eq 1 ]; then
    (
        cd "$WORKDIR"
        apply_patch_if_needed \
            "$ROOT/build_scripts/nuttx-patches/apps-fbcon-unique-custom-bpp-choice.patch"
    )

    (
        cd "$NUTTX_DIR"
        apply_patch_if_needed \
            "$ROOT/build_scripts/nuttx-patches/riscv-elf-lo12-s-rela-addend.patch"
    )
fi

##############################################################################
# Configure firmware
##############################################################################

cd "$NUTTX_DIR"

add_packaged_toolchain_includes

if [ "$SKIP_CONFIG" -eq 0 ]; then
    if [ -f Make.defs ]; then
        run make distclean
    fi

    run ./tools/configure.sh "$BOARD_CONFIG"

    #
    # Loadable ELF modules are the handoff point between the firmware image and
    # FreeBASIC programs.  The module upload wrapper builds programs into the
    # NuttX apps/bin directory and then asks NSH to fetch and run them.
    #
    kconfig_enable CONFIG_SYSTEM_NSH
    kconfig_enable CONFIG_NSH_BUILTIN_APPS
    kconfig_enable CONFIG_NSH_READLINE
    kconfig_disable CONFIG_NSH_DISABLE_WGET
    kconfig_set_val CONFIG_NSH_WGET_BUFF_SIZE 1024
    kconfig_enable CONFIG_ELF
    kconfig_enable CONFIG_MODULES
    kconfig_enable CONFIG_ARCH_SETJMP_H
    kconfig_enable CONFIG_LIBC_EXECFUNCS
    kconfig_enable CONFIG_EXECFUNCS_HAVE_SYMTAB
    kconfig_enable CONFIG_EXECFUNCS_SYSTEM_SYMTAB
    kconfig_enable CONFIG_LIBC_ENVPATH
    kconfig_enable CONFIG_NSH_FILE_APPS
    kconfig_set_str CONFIG_PATH_INITIAL "/data/fb:/mnt/sd0/fb:/bin"
    kconfig_set_val CONFIG_ELF_STACKSIZE 32768
    kconfig_enable CONFIG_EXAMPLES_RUNELF
    kconfig_set_str CONFIG_EXAMPLES_RUNELF_PROGNAME runelf
    kconfig_set_val CONFIG_EXAMPLES_RUNELF_PRIORITY 100
    kconfig_set_val CONFIG_EXAMPLES_RUNELF_STACKSIZE 8192

    #
    # The ESP32-P4 board image should come up as a remotely reachable target.
    # DHCP keeps the first board-prep path simple on a normal LAN, while telnet
    # gives us a fallback console after USB serial is repurposed or unplugged.
    #
    kconfig_enable CONFIG_NET
    kconfig_enable CONFIG_NETDEVICES
    kconfig_enable CONFIG_NETDEV_LATEINIT
    kconfig_enable CONFIG_NET_IPv4
    kconfig_enable CONFIG_NET_ETHERNET
    kconfig_enable CONFIG_NET_TCP
    kconfig_enable CONFIG_NET_UDP
    kconfig_enable CONFIG_NET_ICMP
    kconfig_enable CONFIG_NET_ARP
    kconfig_enable CONFIG_NET_BROADCAST
    kconfig_enable CONFIG_NET_SOCKOPTS
    kconfig_enable CONFIG_NET_TCPBACKLOG
    kconfig_set_val CONFIG_NET_TCP_PREALLOC_CONNS 8
    kconfig_set_val CONFIG_NET_MAX_LISTENPORTS 8
    kconfig_enable CONFIG_NETINIT_DHCPC
    kconfig_enable CONFIG_NETINIT_THREAD
    kconfig_enable CONFIG_NETUTILS_DHCPC
    kconfig_enable CONFIG_SYSTEM_DHCPC_RENEW
    kconfig_enable CONFIG_NETUTILS_WEBCLIENT
    kconfig_enable CONFIG_SCHED_WORKQUEUE
    kconfig_enable CONFIG_SCHED_LPWORK
    kconfig_set_val CONFIG_SCHED_LPWORKSTACKSIZE 8192

    if [ "$WITH_TELNET" -eq 1 ]; then
        kconfig_enable CONFIG_NETUTILS_TELNETD
        kconfig_enable CONFIG_SYSTEM_TELNETD
        kconfig_set_val CONFIG_SYSTEM_TELNETD_PORT 23
        kconfig_set_val CONFIG_SYSTEM_TELNETD_STACKSIZE 8192
        kconfig_set_val CONFIG_SYSTEM_TELNETD_SESSION_STACKSIZE 8192
        kconfig_enable CONFIG_NSH_TELNET
        kconfig_disable CONFIG_NSH_DISABLE_TELNETSTART
    fi

    if [ "$WITH_FTP" -eq 1 ]; then
        kconfig_enable CONFIG_NETUTILS_FTPD
        kconfig_enable CONFIG_EXAMPLES_FTPD
        kconfig_set_val CONFIG_EXAMPLES_FTPD_PORT 21
        kconfig_set_val CONFIG_EXAMPLES_FTPD_STACKSIZE 8192
    fi

    #
    # Keep the board useful for diagnostics and data files without hiding
    # partitioning or formatting policy inside the firmware builder.
    #
    kconfig_enable CONFIG_FS_PROCFS
    kconfig_enable CONFIG_FS_PROCFS_REGISTER
    kconfig_enable CONFIG_FS_TMPFS
    kconfig_enable CONFIG_BCH
    kconfig_enable CONFIG_MBR_PARTITION
    kconfig_enable CONFIG_FS_FAT
    kconfig_enable CONFIG_FAT_LFN
    kconfig_enable CONFIG_FAT_LCNAMES
    kconfig_enable CONFIG_SYSTEM_DD
    kconfig_enable CONFIG_SYSTEM_HEXDUMP
    kconfig_enable CONFIG_NSH_CMDOPT_HEXDUMP
    kconfig_disable CONFIG_STACK_USAGE
    kconfig_set_val CONFIG_ARCH_INTERRUPTSTACK 4096

    yes "" | make olddefconfig >/dev/null || true

    require_config_enabled CONFIG_ELF
    require_config_enabled CONFIG_MODULES
    require_config_enabled CONFIG_LIBC_EXECFUNCS
    require_config_enabled CONFIG_EXAMPLES_RUNELF
    require_config_enabled CONFIG_NET
    require_config_enabled CONFIG_NET_IPv4
    require_config_enabled CONFIG_NET_TCP
    require_config_enabled CONFIG_NETUTILS_DHCPC
    require_config_enabled CONFIG_NETUTILS_WEBCLIENT

    if [ "$WITH_TELNET" -eq 1 ]; then
        require_config_enabled CONFIG_NETUTILS_TELNETD
        require_config_enabled CONFIG_SYSTEM_TELNETD
        require_config_enabled CONFIG_NSH_TELNET
    fi
fi

if [ "$RUN_MENUCONFIG" -eq 1 ]; then
    run make menuconfig
    yes "" | make olddefconfig >/dev/null || true
fi

##############################################################################
# Build and flash
##############################################################################

if [ "$DO_BUILD" -eq 1 ]; then
    #
    # NuttX regenerates dependency state during the build.  Clearing stale
    # fragments avoids carrying absolute paths from previous containers or SDK
    # revisions into this firmware build.
    #
    find "$NUTTX_DIR" "$APPS_DIR" \
        \( -name '.depend' -o -name 'Make.dep' -o -name '*.ddc' \) \
        -type f -delete

    run make EXTRALINKCMDS="${EXTRALINKCMDS:-} --no-warn-rwx-segments" -j"$JOBS"

    mkdir -p "$OUT_DIR"

    for artifact in nuttx nuttx.bin nuttx.hex bootloader.bin partition-table.bin; do
        if [ -f "$NUTTX_DIR/$artifact" ]; then
            cp "$NUTTX_DIR/$artifact" "$OUT_DIR/"
        fi
    done

    if [ -f "$OUT_DIR/nuttx.bin" ]; then
        (
            cd "$OUT_DIR"
            sha256sum nuttx.bin > nuttx.bin.sha256
        )
    fi
fi

if [ "$DO_FLASH" -eq 1 ]; then
    run make flash \
        ESPTOOL_PORT="$FLASH_PORT" \
        ESPTOOL_BAUD="$FLASH_BAUD" \
        ESPTOOL_BINDIR=./
fi

echo "FREEBASIC_NUTTX_ESP32P4_FIRMWARE_READY"
echo "WORKDIR: $WORKDIR"
echo "NUTTX:   $NUTTX_DIR"
echo "APPS:    $APPS_DIR"
echo "OUT:     $OUT_DIR"
echo "NEXT:    fbc-nuttx-esp32p4 --nuttx-workdir '$WORKDIR' --serial-port '${FLASH_PORT:-/dev/ttyACM0}' --run"

# end of fbc-nuttx-esp32p4-firmware.sh
