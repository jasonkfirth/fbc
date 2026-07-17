#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX/RISC-V smoke harness
# ---------------------------------------------
#
# File: nuttx-riscv32-qemu-smoke.sh
#
# Purpose:
#
#     Build and run a tiny generated-FreeBASIC NuttX application under
#     qemu-system-riscv32.
#
# Responsibilities:
#
#     - locate the FreeBASIC source tree
#     - generate RISC-V 32 C from a small .bas file when fbc is available
#     - adapt the compiler's constructor-style C output to a NuttX app main
#     - stage extra generated C modules when a fbctest needs more than one
#       translation unit
#     - create or reuse the default NuttX and NuttX apps workspace
#     - install a temporary NuttX example app into that workspace
#     - build and run the selected NuttX RISC-V target
#     - post-process loadable modules for NuttX ELF loader constraints
#
# This file intentionally does NOT contain:
#
#     - a full NuttX board port
#     - a full FreeBASIC runtime
#     - hardware flashing logic
#     - package/release installation
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project root
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

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root" >&2; exit 1; }

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

default_work_root() {
    if [ -w "$ROOT" ]; then
        printf '%s\n' "$ROOT/.build-nuttx-riscv32"
        return 0
    fi

    if [ -n "${XDG_CACHE_HOME:-}" ]; then
        printf '%s\n' "$XDG_CACHE_HOME/freebasic/nuttx-riscv32"
    else
        printf '%s\n' "${HOME:-/tmp}/.cache/freebasic/nuttx-riscv32"
    fi
}

default_nuttx_workdir() {
    if [ -n "${FB_NUTTX_RISCV32_WORKDIR:-}" ]; then
        printf '%s\n' "$FB_NUTTX_RISCV32_WORKDIR"
    elif [ -n "${NUTTX_WORKDIR:-}" ]; then
        printf '%s\n' "$NUTTX_WORKDIR"
    elif [ -n "${XDG_CACHE_HOME:-}" ]; then
        printf '%s\n' "$XDG_CACHE_HOME/freebasic/nuttx-riscv32"
    else
        printf '%s\n' "${HOME:-/tmp}/.cache/freebasic/nuttx-riscv32"
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

    echo "==> cloning $url ($ref) into $target"
    if git clone --depth 1 --branch "$ref" "$url" "$temp_target"; then
        mv "$temp_target" "$target"
        return 0
    fi

    safe_remove_temp_dir "$temp_target" "$parent"

    # A commit hash is not accepted by git clone --branch on every git
    # version. Fall back to a full clone and then check out the requested ref.
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

prepare_nuttx_workspace() {
    [ "$AUTO_SETUP" -eq 0 ] && return 0
    command -v git >/dev/null 2>&1 || die "git is required to create the NuttX workspace"

    mkdir -p "$NUTTX_WORKDIR"
    clone_repo "$NUTTX_URL" "$NUTTX_REF" "$NUTTX_DIR"
    clone_repo "$APPS_URL" "$APPS_REF" "$APPS_DIR"

    if [ "$UPDATE_REPOS" -eq 1 ]; then
        update_repo "$NUTTX_DIR" "$NUTTX_REF"
        update_repo "$APPS_DIR" "$APPS_REF"
    fi
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

    if [ -n "${NUTTX_DIR:-}" ]; then
        candidate="$NUTTX_DIR/arch/risc-v/src/chip/esp-hal-3rdparty/components/esp_libc/platform_include"
        if [ -f "$candidate/sys/reent.h" ]; then
            append_extra_flags_once "-idirafter $candidate"
            echo "==> using ESP libc fallback headers: $candidate"

            #
            # Some ESP HAL sources include newlib's historical <reent.h>
            # while the available ESP compatibility header lives at
            # <sys/reent.h>.  Keep the wrapper generated and late in the
            # include order so it cannot override any real C library header.
            #
            shim_dir="$WORK_DIR/toolchain-include"
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

/*
 * The ESP HAL shim expects the newlib reentrancy structs to exist even
 * though the hosted package toolchain used here does not ship newlib's
 * sys/reent.h.  The NuttX ESP32-P4 build only needs these declarations for
 * prototypes in the ESP compatibility layer.
 */
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
    fi

    # RP2350 containers commonly mount an xPack riscv-none-elf toolchain.
    # The optional riscv64-prefixed header probe is not required there.
    command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 || return 0

    if printf '#include <sys/lock.h>\n' |
        riscv64-unknown-elf-gcc -x c -E - >/dev/null 2>&1; then
        return
    fi

    for candidate in /usr/lib/picolibc/riscv64-unknown-elf/include; do
        [ -f "$candidate/sys/lock.h" ] || continue

        if ! printf '#include <sys/lock.h>\n' |
            riscv64-unknown-elf-gcc -isystem "$candidate" -x c -E - >/dev/null 2>&1; then
            continue
        fi

        append_extra_flags_once "-isystem $candidate"
        echo "==> using picolibc headers: $candidate"
        return
    done

    return 0
}

config_has_line() {
    local line="$1"

    grep -Fxq "$line" .config
}

find_loadable_module_tool() {
    local tool="$1"
    local cross_prefix="${CROSSDEV:-}"
    local candidate

    #
    # NuttX board trees do not all use the same RISC-V toolchain prefix.
    # Prefer the configured NuttX prefix when it is available, then try the
    # common package names used by the Ubuntu cross toolchains.
    #
    if [ -n "$cross_prefix" ]; then
        candidate="${cross_prefix}${tool}"
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    fi

    for candidate in \
        "riscv-none-elf-$tool" \
        "riscv64-unknown-elf-$tool" \
        "riscv32-unknown-elf-$tool" \
        "$tool"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

allocate_loadable_module_common_symbols() {
    local module_file="$1"
    local module_ld="${NUTTX_MODULE_LD:-}"
    local module_readelf="${NUTTX_MODULE_READELF:-}"
    local temp_file

    if [ -z "$module_readelf" ]; then
        module_readelf="$(find_loadable_module_tool readelf)" ||
            die "could not find readelf for loadable module audit"
    elif ! command -v "$module_readelf" >/dev/null 2>&1; then
        die "could not find readelf for loadable module audit: $module_readelf"
    fi

    if ! "$module_readelf" -s -W "$module_file" |
        awk '$7 == "COM" { found = 1 } END { exit found ? 0 : 1 }'; then
        return
    fi

    if [ -z "$module_ld" ]; then
        module_ld="$(find_loadable_module_tool ld)" ||
            die "could not find linker for loadable module COMMON allocation"
    elif ! command -v "$module_ld" >/dev/null 2>&1; then
        die "could not find linker for loadable module COMMON allocation: $module_ld"
    fi

    temp_file="$module_file.common-alloc.$$"
    rm -f "$temp_file"

    # NuttX's ELF loader rejects SHN_COMMON symbols at load time.
    #
    # FreeBASIC intentionally emits C COMMON symbols for BASIC COMMON blocks so
    # the hosted linker can merge the same block across translation units.  For
    # NuttX loadable modules, do a final partial link with -d so those COMMON
    # symbols are allocated into the module's BSS before NuttX loads it.
    run "$module_ld" -m elf32lriscv -r -d -o "$temp_file" "$module_file"
    chmod --reference="$module_file" "$temp_file" 2>/dev/null || chmod 755 "$temp_file"
    mv "$temp_file" "$module_file"
}

apply_nuttx_patch_if_needed() {
    local patch_file="$1"
    local patch_output
    local strip

    [ -f "$patch_file" ] || die "missing NuttX patch: $patch_file"

    case "$(basename "$patch_file")" in
        riscv-elf-lo12-s-rela-addend.patch)
            if [ -n "${NUTTX_DIR:-}" ] &&
               grep -Fq 'insn = (insn & 0x01fff07f) | val;' \
                   "$NUTTX_DIR/libs/libc/machine/risc-v/arch_elf.c" 2>/dev/null; then
                echo "==> already applied: $patch_file"
                return
            fi
            ;;
    esac

    for strip in 0 1; do
        if git apply "-p$strip" --check "$patch_file" 2>/dev/null; then
            run git apply "-p$strip" "$patch_file"
            return
        fi

        if git apply "-p$strip" --reverse --check "$patch_file" 2>/dev/null; then
            echo "==> already applied: $patch_file"
            return
        fi
    done

    for strip in 0 1; do
        patch_output="$(patch "-p$strip" --dry-run -N < "$patch_file" 2>&1)" && {
            echo "==> patch -p$strip -N < $patch_file"
            patch "-p$strip" -N < "$patch_file"
            return
        }

        if echo "$patch_output" | grep -q "Reversed (or previously applied)"; then
            echo "==> already applied: $patch_file"
            return
        fi
    done

    die "could not apply NuttX patch: $patch_file"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/nuttx-riscv32-qemu-smoke.sh [options]

Options:
  --nuttx-workdir DIR   Directory containing nuttx/ and apps/
                        Default: \$XDG_CACHE_HOME/freebasic/nuttx-riscv32
                        or \$HOME/.cache/freebasic/nuttx-riscv32
  --update              Fetch and fast-forward existing NuttX repositories
  --nuttx-url URL       NuttX repository URL
  --apps-url URL        NuttX apps repository URL
  --nuttx-ref REF       NuttX branch/tag/commit, default: master
  --apps-ref REF        apps branch/tag/commit, default: same as NuttX
  --no-bootstrap        Require nuttx/ and apps/ to already exist
  --prepare-only        Create or reuse the workspace, then stop
  --bas FILE            FreeBASIC source to compile
  --fbc FILE            fbc binary to use
  --generated-c FILE    Use existing generated C instead of invoking fbc
  --extra-generated-c FILE
                        Add one extra generated C module to the same NuttX
                        app. May be passed more than once.
  --extra-c FILE        Add one ordinary C support source to the NuttX app.
                        This is for fbctests helper objects, not generated
                        FreeBASIC modules.
  --extra-cxx FILE      Add one ordinary C++ support source to the NuttX app.
  --extra-asm FILE      Add one ordinary assembly support source to the
                        NuttX app.
  --app-name NAME       NuttX app name, default: fbhello
  --with-gfxlib         Link the selected gfxlib2 smoke-test objects
  --sfx-no-media-decoders
                        Link sfxlib without bundled WAV/MP3/Ogg decoders.
                        This keeps generated tones available for small boards,
                        while media load commands fail cleanly.
  --qemu-mock-devices   Compile emulator-visible mock device traces for
                        drivers that cannot be represented by qemu-system's
                        rv-virt machine. Currently this asserts gfx presents.
  --qemu-storage-root DIR
                        Mount a tmpfs-backed storage root before running the
                        app. This lets QEMU exercise board storage smoke tests
                        such as /mnt/sd0 without writing host or controller
                        media. Default: disabled.
  --qemu-storage-backend NAME
                        Storage backing used with --qemu-storage-root:
                        tmpfs or virtio-blk. The virtio-blk backend attaches
                        a throwaway raw disk to QEMU, formats /dev/virtblk0
                        inside NuttX, then mounts it at the requested root.
                        Default: tmpfs.
  --qemu-usb-root-devices
                        Attach QEMU xHCI root-port USB devices directly:
                        mass storage, keyboard, mouse, and usb-net. This does
                        not emulate an external USB hub.
  --qemu-usb-hub-devices
                        Attach the same QEMU USB devices behind an external
                        usb-hub device.
  --expect-qemu-usb-net-unsupported
                        Require the current QEMU usb-net/NuttX class mismatch
                        to be visible in the log. This is a guardrail for the
                        device lab: virtio-net may work, but that must not be
                        mistaken for a proven USB Ethernet dongle path.
  --expect-qemu-usb-net-supported
                        Require QEMU usb-net to enumerate and bind as a NuttX
                        CDC Ethernet network device.
  --expect-qemu-usb-hub-unsupported
                        Require QEMU's usb-hub to enumerate as an unsupported
                        class device. This prevents a hub-attached topology
                        from being mistaken for a proven hardware path.
  --expect-qemu-usb-hub-supported
                        Enable the NuttX USB hub class and require QEMU's
                        hub-attached storage, keyboard, and mouse devices to
                        bind through the external hub.
  --qemu-inject-hid-events
                        Use QEMU's HMP monitor socket to inject a keyboard
                        event into the emulated USB keyboard. This is meant
                        for focused HID tests, not ordinary serial input.
  --qemu-virtio-sound   Attach QEMU's virtio sound device and require NuttX
                        to register its PCM playback node.
  --qemu-usb-storage-root DIR
                        Format the QEMU usb-storage disk at /dev/sda and mount
                        it at DIR before running the app. Implies
                        --qemu-usb-root-devices.
  --network-shell       Build a network-enabled NuttX guest with telnetd
                        and ftpd enabled, then leave QEMU running.
  --expect-fail         Treat a non-zero FreeBASIC program status as success.
                        This is for fbctests cases whose TEST_MODE expects a
                        runtime assertion or explicit failure.
  --reuse-config        Reuse the existing NuttX app/config slot. This is for
                        serial harnesses that rebuild the same app name many
                        times with different generated C sources.
  --keep-existing-apps  Keep previously staged FreeBASIC examples enabled when
                        adding this app to the NuttX image.
  --loadable-module     Build the selected app as a NuttX ELF module under
                        apps/bin/<app-name> instead of as a built-in command.
  --without-minrt       Do not compile the temporary FreeBASIC runtime into
                        this app. Use when another built-in BASIC app already
                        provides the runtime symbols.
  --local-generated-symbols
                        Keep generated startup helper symbols private to this
                        app. Use when staging several BASIC built-ins into one
                        firmware image.
  --nuttx-config NAME   NuttX board config passed to tools/configure.sh,
                        default: rv-virt:nsh
  --skip-nuttx-config   Keep the existing NuttX board configuration.
  --host-telnet-port N  Host TCP port forwarded to NuttX telnet, default: 2323
  --host-ftp-port N     Host TCP port forwarded to NuttX FTP, default: 2121
  --no-run              Build only; do not launch QEMU
  --help                Show this help text

Environment:
  NUTTX_WORKDIR         Same as --nuttx-workdir
  FB_NUTTX_RISCV32_WORKDIR
                        Alternate default workdir
  FB_NUTTX_AUTO_SETUP   Set to 0 to disable repository bootstrap
  FB_NUTTX_UPDATE_REPOS Set to 1 to update existing repositories
  FBC                   Same as --fbc
  FB_GENERATED_C        Same as --generated-c
  APP_STACKSIZE         NuttX task stack size, default: 65536
  JOBS                  Parallel make jobs, default: host CPU count
  QEMU_TIMEOUT          QEMU run timeout in seconds, default: 20
  QEMU_MEMORY           QEMU RAM size, default: 2G
  QEMU_HOSTFWD_BIND     Address used for QEMU hostfwd, default: 0.0.0.0
  NUTTX_CONFIG          Same as --nuttx-config

The script modifies the given NuttX apps tree by adding examples/<app-name>.
It also appends one source line to apps/examples/Kconfig if needed.
EOF
}

##############################################################################
# Options
##############################################################################

NUTTX_WORKDIR="${NUTTX_WORKDIR:-}"
NUTTX_URL="${NUTTX_URL:-https://github.com/apache/nuttx.git}"
APPS_URL="${APPS_URL:-https://github.com/apache/nuttx-apps.git}"
NUTTX_REF="${NUTTX_REF:-master}"
APPS_REF="${APPS_REF:-}"
BAS_SRC="$ROOT/examples/nuttx/fbhello.bas"
FBC_BIN="${FBC:-}"
GENERATED_C="${FB_GENERATED_C:-}"
EXTRA_GENERATED_C_FILES=()
EXTRA_C_FILES=()
EXTRA_CXX_FILES=()
EXTRA_ASM_FILES=()
APP_NAME="fbhello"
RUN_QEMU=1
WITH_GFXLIB=0
SFX_NO_MEDIA_DECODERS="${FB_NUTTX_SFX_NO_MEDIA_DECODERS:-0}"
QEMU_MOCK_DEVICES=0
NETWORK_SHELL=0
HOST_TELNET_PORT="${HOST_TELNET_PORT:-2323}"
HOST_FTP_PORT="${HOST_FTP_PORT:-2121}"
QEMU_HOSTFWD_BIND="${QEMU_HOSTFWD_BIND:-0.0.0.0}"
QEMU_STORAGE_ROOT="${QEMU_STORAGE_ROOT:-}"
QEMU_STORAGE_BACKEND="${QEMU_STORAGE_BACKEND:-tmpfs}"
QEMU_STORAGE_IMAGE_SIZE="${QEMU_STORAGE_IMAGE_SIZE:-16M}"
QEMU_USB_ROOT_DEVICES=0
QEMU_USB_HUB_DEVICES=0
QEMU_USB_STORAGE_ROOT="${QEMU_USB_STORAGE_ROOT:-}"
QEMU_USB_STORAGE_IMAGE_SIZE="${QEMU_USB_STORAGE_IMAGE_SIZE:-16M}"
QEMU_HID_INJECT_DELAY="${QEMU_HID_INJECT_DELAY:-3}"
QEMU_HID_SERIAL_EXIT_DELAY="${QEMU_HID_SERIAL_EXIT_DELAY:-0}"
QEMU_SERIAL_CHAR_DELAY="${QEMU_SERIAL_CHAR_DELAY:-0.02}"
QEMU_SERIAL_LINE_DELAY="${QEMU_SERIAL_LINE_DELAY:-0.25}"
EXPECT_QEMU_USB_NET_UNSUPPORTED=0
EXPECT_QEMU_USB_NET_SUPPORTED=0
EXPECT_QEMU_USB_HUB_UNSUPPORTED=0
EXPECT_QEMU_USB_HUB_SUPPORTED=0
QEMU_INJECT_HID_EVENTS=0
QEMU_VIRTIO_SOUND=0
EXPECT_FAIL=0
REUSE_CONFIG=0
KEEP_EXISTING_APPS=0
LOADABLE_MODULE=0
WITH_MINRT=1
USE_GENERIC_GOSUB="${FB_NUTTX_USE_GENERIC_GOSUB:-1}"
USE_GENERIC_MEMORY="${FB_NUTTX_USE_GENERIC_MEMORY:-1}"
USE_GENERIC_MATH_CVN="${FB_NUTTX_USE_GENERIC_MATH_CVN:-1}"
USE_GENERIC_MATH_FIX="${FB_NUTTX_USE_GENERIC_MATH_FIX:-1}"
USE_GENERIC_MATH_FRAC="${FB_NUTTX_USE_GENERIC_MATH_FRAC:-1}"
USE_GENERIC_MATH_LOG10="${FB_NUTTX_USE_GENERIC_MATH_LOG10:-1}"
USE_GENERIC_MATH_RND="${FB_NUTTX_USE_GENERIC_MATH_RND:-1}"
USE_GENERIC_MATH_SGN="${FB_NUTTX_USE_GENERIC_MATH_SGN:-1}"
USE_GENERIC_OBJECT="${FB_NUTTX_USE_GENERIC_OBJECT:-1}"
USE_GENERIC_DATETIME_MATH="${FB_NUTTX_USE_GENERIC_DATETIME_MATH:-1}"
USE_GENERIC_CLOCK="${FB_NUTTX_USE_GENERIC_CLOCK:-1}"
USE_GENERIC_ENVIRON="${FB_NUTTX_USE_GENERIC_ENVIRON:-1}"
USE_GENERIC_DIR="${FB_NUTTX_USE_GENERIC_DIR:-1}"
USE_GENERIC_FILE_COPY="${FB_NUTTX_USE_GENERIC_FILE_COPY:-1}"
USE_GENERIC_STR_BASE="${FB_NUTTX_USE_GENERIC_STR_BASE:-1}"
USE_GENERIC_STR_FILL="${FB_NUTTX_USE_GENERIC_STR_FILL:-1}"
USE_GENERIC_STR_EXTRA="${FB_NUTTX_USE_GENERIC_STR_EXTRA:-1}"
USE_GENERIC_WSTRING="${FB_NUTTX_USE_GENERIC_WSTRING:-1}"
USE_GENERIC_ASSERT="${FB_NUTTX_USE_GENERIC_ASSERT:-1}"
LOCAL_GENERATED_SYMBOLS=0
NUTTX_CONFIG="${NUTTX_CONFIG:-rv-virt:nsh}"
SKIP_NUTTX_CONFIG=0
AUTO_SETUP="${FB_NUTTX_AUTO_SETUP:-1}"
UPDATE_REPOS="${FB_NUTTX_UPDATE_REPOS:-0}"
PREPARE_ONLY=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --nuttx-workdir)
            [ "$#" -ge 2 ] || die "--nuttx-workdir requires a directory"
            NUTTX_WORKDIR="$2"
            shift 2
            ;;
        --update)
            UPDATE_REPOS=1
            shift
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
        --no-bootstrap)
            AUTO_SETUP=0
            shift
            ;;
        --prepare-only)
            PREPARE_ONLY=1
            RUN_QEMU=0
            shift
            ;;
        --bas)
            [ "$#" -ge 2 ] || die "--bas requires a file"
            BAS_SRC="$2"
            shift 2
            ;;
        --fbc)
            [ "$#" -ge 2 ] || die "--fbc requires a file"
            FBC_BIN="$2"
            shift 2
            ;;
        --generated-c)
            [ "$#" -ge 2 ] || die "--generated-c requires a file"
            GENERATED_C="$2"
            shift 2
            ;;
        --extra-generated-c)
            [ "$#" -ge 2 ] || die "--extra-generated-c requires a file"
            EXTRA_GENERATED_C_FILES+=("$2")
            shift 2
            ;;
        --extra-c)
            [ "$#" -ge 2 ] || die "--extra-c requires a file"
            EXTRA_C_FILES+=("$2")
            shift 2
            ;;
        --extra-cxx)
            [ "$#" -ge 2 ] || die "--extra-cxx requires a file"
            EXTRA_CXX_FILES+=("$2")
            shift 2
            ;;
        --extra-asm)
            [ "$#" -ge 2 ] || die "--extra-asm requires a file"
            EXTRA_ASM_FILES+=("$2")
            shift 2
            ;;
        --app-name)
            [ "$#" -ge 2 ] || die "--app-name requires a name"
            APP_NAME="$2"
            shift 2
            ;;
        --with-gfxlib)
            WITH_GFXLIB=1
            shift
            ;;
        --sfx-no-media-decoders)
            SFX_NO_MEDIA_DECODERS=1
            shift
            ;;
        --qemu-mock-devices)
            QEMU_MOCK_DEVICES=1
            shift
            ;;
        --qemu-storage-root)
            [ "$#" -ge 2 ] || die "--qemu-storage-root requires a directory"
            QEMU_STORAGE_ROOT="$2"
            shift 2
            ;;
        --qemu-storage-backend)
            [ "$#" -ge 2 ] || die "--qemu-storage-backend requires a name"
            QEMU_STORAGE_BACKEND="$2"
            shift 2
            ;;
        --qemu-usb-root-devices)
            QEMU_USB_ROOT_DEVICES=1
            shift
            ;;
        --qemu-usb-hub-devices)
            QEMU_USB_HUB_DEVICES=1
            shift
            ;;
        --expect-qemu-usb-net-unsupported)
            EXPECT_QEMU_USB_NET_UNSUPPORTED=1
            shift
            ;;
        --expect-qemu-usb-net-supported)
            EXPECT_QEMU_USB_NET_SUPPORTED=1
            shift
            ;;
        --expect-qemu-usb-hub-unsupported)
            EXPECT_QEMU_USB_HUB_UNSUPPORTED=1
            shift
            ;;
        --expect-qemu-usb-hub-supported)
            EXPECT_QEMU_USB_HUB_SUPPORTED=1
            shift
            ;;
        --qemu-inject-hid-events)
            QEMU_INJECT_HID_EVENTS=1
            shift
            ;;
        --qemu-virtio-sound)
            QEMU_VIRTIO_SOUND=1
            shift
            ;;
        --qemu-usb-storage-root)
            [ "$#" -ge 2 ] || die "--qemu-usb-storage-root requires a directory"
            if [ "$QEMU_USB_HUB_DEVICES" -eq 0 ]; then
                QEMU_USB_ROOT_DEVICES=1
            fi
            QEMU_USB_STORAGE_ROOT="$2"
            shift 2
            ;;
        --network-shell)
            NETWORK_SHELL=1
            RUN_QEMU=1
            shift
            ;;
        --expect-fail)
            EXPECT_FAIL=1
            shift
            ;;
        --reuse-config)
            REUSE_CONFIG=1
            shift
            ;;
        --keep-existing-apps)
            KEEP_EXISTING_APPS=1
            shift
            ;;
        --loadable-module)
            LOADABLE_MODULE=1
            RUN_QEMU=0
            shift
            ;;
        --without-minrt)
            WITH_MINRT=0
            shift
            ;;
        --local-generated-symbols)
            LOCAL_GENERATED_SYMBOLS=1
            shift
            ;;
        --nuttx-config)
            [ "$#" -ge 2 ] || die "--nuttx-config requires a board config"
            NUTTX_CONFIG="$2"
            shift 2
            ;;
        --skip-nuttx-config)
            SKIP_NUTTX_CONFIG=1
            shift
            ;;
        --host-telnet-port)
            [ "$#" -ge 2 ] || die "--host-telnet-port requires a port"
            HOST_TELNET_PORT="$2"
            shift 2
            ;;
        --host-ftp-port)
            [ "$#" -ge 2 ] || die "--host-ftp-port requires a port"
            HOST_FTP_PORT="$2"
            shift 2
            ;;
        --no-run)
            RUN_QEMU=0
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
[ -n "$NUTTX_WORKDIR" ] || NUTTX_WORKDIR="$(default_nuttx_workdir)"

case "$AUTO_SETUP" in
    0|1) ;;
    *) die "FB_NUTTX_AUTO_SETUP must be 0 or 1" ;;
esac

case "$UPDATE_REPOS" in
    0|1) ;;
    *) die "FB_NUTTX_UPDATE_REPOS must be 0 or 1" ;;
esac

if [ "$QEMU_USB_ROOT_DEVICES" -eq 1 ] && [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
    die "--qemu-usb-root-devices and --qemu-usb-hub-devices are separate topologies"
fi

if [ "$EXPECT_QEMU_USB_HUB_UNSUPPORTED" -eq 1 ] &&
   [ "$EXPECT_QEMU_USB_HUB_SUPPORTED" -eq 1 ]; then
    die "USB hub cannot be expected both supported and unsupported"
fi

if [ "$EXPECT_QEMU_USB_NET_UNSUPPORTED" -eq 1 ] &&
   [ "$EXPECT_QEMU_USB_NET_SUPPORTED" -eq 1 ]; then
    die "USB net cannot be expected both supported and unsupported"
fi

if [ "$EXPECT_QEMU_USB_HUB_SUPPORTED" -eq 1 ] && [ "$QEMU_USB_HUB_DEVICES" -eq 0 ]; then
    die "--expect-qemu-usb-hub-supported requires --qemu-usb-hub-devices"
fi

if [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ] &&
   [ "$QEMU_USB_ROOT_DEVICES" -eq 0 ] && [ "$QEMU_USB_HUB_DEVICES" -eq 0 ]; then
    die "--qemu-inject-hid-events requires a QEMU USB keyboard topology"
fi

if [ "$EXPECT_QEMU_USB_NET_SUPPORTED" -eq 1 ] &&
   [ "$QEMU_USB_ROOT_DEVICES" -eq 0 ] && [ "$QEMU_USB_HUB_DEVICES" -eq 0 ]; then
    die "--expect-qemu-usb-net-supported requires a QEMU USB device topology"
fi

NUTTX_DIR="$NUTTX_WORKDIR/nuttx"
APPS_DIR="$NUTTX_WORKDIR/apps"

prepare_nuttx_workspace

[ -d "$NUTTX_DIR" ] || die "missing NuttX directory: $NUTTX_DIR"
[ -d "$APPS_DIR" ] || die "missing NuttX apps directory: $APPS_DIR"

if [ "$PREPARE_ONLY" -eq 1 ]; then
    echo "NUTTX_WORKSPACE_READY: $NUTTX_WORKDIR"
    exit 0
fi

APP_DIR="$APPS_DIR/examples/$APP_NAME"
WORK_DIR="${FB_NUTTX_WORK_ROOT:-$(default_work_root)}/$APP_NAME"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
APP_PRIORITY="${APP_PRIORITY:-100}"
APP_STACKSIZE="${APP_STACKSIZE:-65536}"

if [ -z "${QEMU_TIMEOUT+x}" ]; then
    if [ "$NETWORK_SHELL" -eq 1 ]; then
        QEMU_TIMEOUT=0
    else
        QEMU_TIMEOUT=20
    fi
fi

QEMU_MEMORY="${QEMU_MEMORY:-2G}"
FB_NUTTX_SMOKE_APPS="$APP_NAME fbhello fbbuiltin fbpretest fbgfx fbmini"
FB_NUTTX_KCONFIG_OLD_APPS="$FB_NUTTX_SMOKE_APPS"

if [ "$KEEP_EXISTING_APPS" -eq 1 ]; then
    FB_NUTTX_KCONFIG_OLD_APPS="$APP_NAME"
fi

QEMU_SEED_HOST_INPUT_FILE=0


case "$APP_NAME" in
    *[!A-Za-z0-9_]*|'')
        die "app name must contain only letters, numbers, and underscores"
        ;;
esac

case "$HOST_TELNET_PORT" in
    *[!0-9]*|'')
        die "host telnet port must be numeric"
        ;;
esac

case "$HOST_FTP_PORT" in
    *[!0-9]*|'')
        die "host ftp port must be numeric"
        ;;
esac

case "$QEMU_STORAGE_ROOT" in
    "")
        ;;
    /*)
        case "$QEMU_STORAGE_ROOT" in
            *"'"* | *" "* | *";"* | *"&"* | *"|"* | *"<"* | *">"* | *"$"* | *"("* | *")"*)
                die "storage root contains unsupported shell characters: $QEMU_STORAGE_ROOT"
                ;;
        esac
        ;;
    *)
        die "--qemu-storage-root must be an absolute NuttX path"
        ;;
esac

case "$QEMU_USB_STORAGE_ROOT" in
    "")
        ;;
    /*)
        case "$QEMU_USB_STORAGE_ROOT" in
            *"'"* | *" "* | *";"* | *"&"* | *"|"* | *"<"* | *">"* | *"$"* | *"("* | *")"*)
                die "USB storage root contains unsupported shell characters: $QEMU_USB_STORAGE_ROOT"
                ;;
        esac
        ;;
    *)
        die "--qemu-usb-storage-root must be an absolute NuttX path"
        ;;
esac

case "$QEMU_STORAGE_BACKEND" in
    tmpfs|virtio-blk)
        ;;
    *)
        die "--qemu-storage-backend must be tmpfs or virtio-blk"
        ;;
esac

if [ "$QEMU_STORAGE_BACKEND" != "tmpfs" ] && [ -z "$QEMU_STORAGE_ROOT" ]; then
    die "--qemu-storage-backend requires --qemu-storage-root"
fi

case "$QEMU_STORAGE_IMAGE_SIZE" in
    *[!0-9KMGkmg]*|'')
        die "QEMU_STORAGE_IMAGE_SIZE must be a simple truncate size"
        ;;
esac

case "$QEMU_USB_STORAGE_IMAGE_SIZE" in
    *[!0-9KMGkmg]*|'')
        die "QEMU_USB_STORAGE_IMAGE_SIZE must be a simple truncate size"
        ;;
esac

APP_SYMBOL="$(printf '%s' "$APP_NAME" | tr '[:lower:]' '[:upper:]')"
APP_ENTRY_SYMBOL="fb_${APP_NAME}_nuttx_program_main"
APP_MODULE_VALUE="\$(CONFIG_EXAMPLES_${APP_SYMBOL})"

if [ "$LOADABLE_MODULE" -eq 1 ]; then
    APP_MODULE_VALUE="m"
fi

##############################################################################
# Keep the generated FreeBASIC smoke app isolated
##############################################################################

for old_app in $FB_NUTTX_SMOKE_APPS; do
    if [ "$REUSE_CONFIG" -eq 0 ] &&
       [ "$KEEP_EXISTING_APPS" -eq 0 ] &&
       [ "$old_app" != "$APP_NAME" ]; then
        rm -rf "$APPS_DIR/examples/$old_app"
    fi
done

if [ "$REUSE_CONFIG" -eq 0 ] && [ "$KEEP_EXISTING_APPS" -eq 0 ]; then
    find "$APPS_DIR/examples" -mindepth 1 -maxdepth 1 -type d -name 'eg_*' |
    while IFS= read -r old_app_dir; do
        if [ "$(basename -- "$old_app_dir")" != "$APP_NAME" ]; then
            rm -rf "$old_app_dir"
        fi
    done
fi

if [ "$REUSE_CONFIG" -eq 0 ]; then
    rm -f "$APPS_DIR/libapps.a" "$NUTTX_DIR/staging/libapps.a"
fi

##############################################################################
# Generate or import FreeBASIC C
##############################################################################

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"

if ! command -v curl >/dev/null 2>&1 && command -v wget >/dev/null 2>&1; then
    TOOLBIN_DIR="$WORK_DIR/toolbin"
    mkdir -p "$TOOLBIN_DIR"
    cat > "$TOOLBIN_DIR/curl" <<'EOF'
#!/usr/bin/env sh

output=
url=

while [ "$#" -gt 0 ]; do
    case "$1" in
        -L|-s|-S|-Ss|-sS)
            shift
            ;;
        -o)
            shift
            [ "$#" -gt 0 ] || exit 2
            output="$1"
            shift
            ;;
        -*)
            exit 2
            ;;
        *)
            url="$1"
            shift
            ;;
    esac
done

[ -n "$url" ] || exit 2

if [ -n "$output" ]; then
    exec wget -q -O "$output" "$url"
fi

exec wget -q "$url"
EOF
    chmod +x "$TOOLBIN_DIR/curl"
    PATH="$TOOLBIN_DIR:$PATH"
    export PATH
fi

if [ -z "$GENERATED_C" ]; then
    if [ -z "$FBC_BIN" ]; then
        if [ -x "$ROOT/bin/fbc" ]; then
            FBC_BIN="$ROOT/bin/fbc"
        elif [ -x "$ROOT/bin/fbc.exe" ]; then
            FBC_BIN="$ROOT/bin/fbc.exe"
        elif command -v fbc >/dev/null 2>&1; then
            FBC_BIN="$(command -v fbc)"
        fi
    fi

    [ -n "$FBC_BIN" ] || die "no fbc found; pass --fbc or --generated-c"
    [ -x "$FBC_BIN" ] || die "fbc is not executable: $FBC_BIN"
    [ -f "$BAS_SRC" ] || die "missing FreeBASIC source: $BAS_SRC"

    BAS_WORK="$WORK_DIR/$(basename "$BAS_SRC")"
    cp "$BAS_SRC" "$BAS_WORK"

    FBC_GENERATE_ARGS=(-target nuttx-riscv32 -gen gcc)
    if [ "$APP_NAME" = "fbsfx" ] && [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
        FBC_GENERATE_ARGS+=(-d FB_NUTTX_QEMU_VIRTIO_SOUND)
    fi

    run "$FBC_BIN" "${FBC_GENERATE_ARGS[@]}" -r "$BAS_WORK" -x "$WORK_DIR/$APP_NAME"
    GENERATED_C="$WORK_DIR/$(basename "$BAS_WORK" .bas).c"
fi

[ -f "$GENERATED_C" ] || die "generated C was not found: $GENERATED_C"

for extra_generated_c in "${EXTRA_GENERATED_C_FILES[@]}"; do
    [ -f "$extra_generated_c" ] ||
        die "extra generated C was not found: $extra_generated_c"
done

for extra_c in "${EXTRA_C_FILES[@]}"; do
    [ -f "$extra_c" ] ||
        die "extra C source was not found: $extra_c"
done

for extra_cxx in "${EXTRA_CXX_FILES[@]}"; do
    [ -f "$extra_cxx" ] ||
        die "extra C++ source was not found: $extra_cxx"
done

for extra_asm in "${EXTRA_ASM_FILES[@]}"; do
    [ -f "$extra_asm" ] ||
        die "extra assembly source was not found: $extra_asm"
done

add_packaged_toolchain_includes

adapt_constructor_source() {
    local src=$1
    local dst=$2
    local ctor_list=$3
    local main_list=$4
    local local_symbols=$5

    python3 - "$src" "$dst" "$ctor_list" "$main_list" "$local_symbols" <<'PY'
import re
import sys

src_path = sys.argv[1]
dst_path = sys.argv[2]
ctor_list_path = sys.argv[3]
main_list_path = sys.argv[4]
local_symbols = sys.argv[5] == "1"

with open(src_path, "r", encoding="utf-8") as f:
    text = f.read()

standard_preamble = "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n"
diagnostic_preamble = (
    "#if defined(__GNUC__)\n"
    "#  pragma GCC diagnostic ignored \"-Wunused-but-set-variable\"\n"
    "#  pragma GCC diagnostic ignored \"-Wunused-function\"\n"
    "#  pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n"
    "#  if __GNUC__ >= 12\n"
    "#    pragma GCC diagnostic ignored \"-Wdangling-pointer\"\n"
    "#  endif\n"
    "#endif\n\n"
)

if not text.startswith("#include <stdio.h>"):
    text = standard_preamble + text

if diagnostic_preamble not in text:
    if text.startswith(standard_preamble):
        text = standard_preamble + diagnostic_preamble + text[len(standard_preamble):]
    else:
        text = diagnostic_preamble + text

text = text.replace("int32 printf( const char*, ... );\n", "")
text = text.replace("int32 fprintf( void*, const char*, ... );\n", "")
text = text.replace("void* fopen( const char*, const char* );\n", "")
text = text.replace("int32 fclose( void* );\n", "")
text = text.replace("extern void* stdin;\n", "")
text = text.replace("extern void* stdout;\n", "")
text = text.replace("extern void* stderr;\n", "")
text = text.replace("fprintf( stdout, ", "printf( ")

stdio_prototypes = (
    "rename",
    "fopen",
    "freopen",
    "fflush",
    "fclose",
    "remove",
    "tmpfile",
    "tmpnam",
    "tempnam",
    "setvbuf",
    "setbuf",
    "fprintf",
    "printf",
    "sprintf",
    "vfprintf",
    "vprintf",
    "vsprintf",
    "vscanf",
    "vfscanf",
    "vsscanf",
    "fscanf",
    "scanf",
    "sscanf",
    "fgetc",
    "fgets",
    "fputc",
    "fputs",
    "getc",
    "getchar",
    "gets",
    "putc",
    "putchar",
    "puts",
    "ungetc",
    "fread",
    "fwrite",
    "fseek",
    "ftell",
    "rewind",
    "fgetpos",
    "fsetpos",
    "clearerr",
    "feof",
    "ferror",
    "perror",
)

for name in stdio_prototypes:
    text = re.sub(
        r"^\s*(?:[A-Za-z_][A-Za-z0-9_]*\s*\*?\s+)+" +
            re.escape(name) +
            r"\( [^\n;{}]* \);\n",
        "",
        text,
        flags=re.MULTILINE,
    )

decl = re.compile(r"static void (fb_ctor__[A-Za-z0-9_]+)\( void \) __attribute__\(\( constructor \)\);")
defn = re.compile(r"__attribute__\(\( constructor \)\) static void (fb_ctor__[A-Za-z0-9_]+)\( void \)")
main_defn = re.compile(r"\b(int32|int)\s+main\s*\(")

ctor_names = defn.findall(text)
if local_symbols:
    text = decl.sub(r"static void \1( void );", text)
    text = defn.sub(r"static void \1( void )", text)
else:
    text = decl.sub(r"void \1( void );", text)
    text = defn.sub(r"void \1( void )", text)

main_names = []

if main_defn.search(text):
    if local_symbols:
        text = main_defn.sub(r"static \1 fb_nuttx_generated_main(", text, count=1)
    else:
        text = main_defn.sub(r"\1 fb_nuttx_generated_main(", text, count=1)
    main_names.append("fb_nuttx_generated_main")

with open(dst_path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)

with open(ctor_list_path, "a", encoding="utf-8", newline="\n") as f:
    for name in ctor_names:
        f.write(name + "\n")

with open(main_list_path, "a", encoding="utf-8", newline="\n") as f:
    for name in main_names:
        f.write(name + "\n")
PY
}

adapt_initial_data_restore() {
    python3 - "$1" "$2" <<'PY'
import re
import sys

path = sys.argv[1]
entry_symbol = sys.argv[2]

with open(path, "r", encoding="utf-8") as f:
    text = f.read()

data_match = re.search(r"static struct [^\n;]*__FB_DATADESC[^\n;]*\s+(label\$\d+)\s*\[", text)

if data_match is None:
    raise SystemExit(0)

label = data_match.group(1)
main_pattern = re.compile(
    r"(int " + re.escape(entry_symbol) +
    r"\( int argc, char \*\*argv \)\s*\{\n)"
)
marker = "    /* Initial DATA cursor for the NuttX smoke harness. */\n"

if marker in text:
    raise SystemExit(0)

text, count = main_pattern.subn(
    r"\1" + marker + r"    fb_DataRestore( (void*)" + label + r" );\n",
    text,
    count=1)

if count != 1:
    raise SystemExit("could not insert initial DATA restore")

with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
PY
}

adapt_host_file_open_paths() {
    python3 - "$1" <<'PY'
import ast
import re
import sys

path = sys.argv[1]

with open(path, "r", encoding="utf-8") as f:
    text = f.read()

pattern = re.compile(
    r'fb_StrAllocTempDescZEx\(\s*\(char\*\)((?:"(?:\\.|[^"\\])*"\s*)+),\s*\d+\s*\)'
)
changed = False

def decode_c_strings(text):
    result = ""

    for piece in re.findall(r'"(?:\\.|[^"\\])*"', text):
        result += ast.literal_eval(piece)

    return result

def replace_path(match):
    global changed

    source_path = decode_c_strings(match.group(1))
    source_path = source_path.replace("/", "\\").lower()

    if source_path.endswith("\\tests\\qb\\file_open.bas"):
        changed = True
        return 'fb_StrAllocTempDescZEx( (char*)"fbctest-file-open.bas", 21 )'

    return match.group(0)

text = pattern.sub(replace_path, text)

if changed:
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)

raise SystemExit(0 if changed else 1)
PY
}

adapt_fbsfx_virtio_sound_hold() {
    python3 - "$1" <<'PY'
import re
import sys

path = sys.argv[1]

with open(path, "r", encoding="utf-8") as f:
    text = f.read()

marker = "/* QEMU virtio-sound keeps fbsfx alive after the success marker. */"
if marker in text:
    raise SystemExit(0)

if "FB_NUTTX_SFX_SMOKE_OK" not in text:
    raise SystemExit("fbsfx success marker was not found")

if "#include <unistd.h>" not in text:
    text = text.replace("#include <string.h>\n",
        "#include <string.h>\n#include <unistd.h>\n", 1)

pattern = re.compile(r"(\s*)fb_End\(\s*0\s*\);")
replacement = (
    r"\1" + marker + "\n"
    r"\1for (;;) {\n"
    r"\1    usleep(1000000);\n"
    r"\1}\n"
    r"\1fb_End( 0 );"
)

text, count = pattern.subn(replacement, text, count=1)
if count != 1:
    raise SystemExit("fbsfx end call was not found")

with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
PY
}

adapt_stb_vorbis_diagnostics() {
    python3 - "$1" <<'PY'
import sys

path = sys.argv[1]

with open(path, "r", encoding="utf-8") as f:
    text = f.read()

diagnostic_preamble = (
    "#if defined(__GNUC__)\n"
    "#  pragma GCC diagnostic ignored \"-Wshadow\"\n"
    "#endif\n\n"
)

if diagnostic_preamble not in text:
    text = diagnostic_preamble + text

with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
PY
}

USES_FBCUNIT=0
USES_TCP=0
USES_GOSUB=0
USES_SFX=0
EXTRA_GENERATED_CSRCS=""
FBCUNIT_TEST_CTORS="$WORK_DIR/fbcunit-test-ctors.txt"
FBCUNIT_SUPPORT_CTORS="$WORK_DIR/fbcunit-support-ctors.txt"
GENERATED_MAIN_ENTRIES="$WORK_DIR/generated-main-entries.txt"
: > "$FBCUNIT_TEST_CTORS"
: > "$FBCUNIT_SUPPORT_CTORS"
: > "$GENERATED_MAIN_ENTRIES"

if grep -q '_ZN4FBCU' "$GENERATED_C"; then
    USES_FBCUNIT=1
fi

if grep -q 'fb_FileOpenTcp\|fb_FileOpenTcpServer\|fb_TcpAccept\|fb_Eoc' "$GENERATED_C"; then
    USES_TCP=1
fi

if grep -q 'fb_Gosub\|setjmp' "$GENERATED_C"; then
    USES_GOSUB=1
fi

if grep -q 'fb_sfx' "$GENERATED_C"; then
    USES_SFX=1
fi

for extra_generated_c in "${EXTRA_GENERATED_C_FILES[@]}"; do
    if grep -q '_ZN4FBCU' "$extra_generated_c"; then
        USES_FBCUNIT=1
    fi

    if grep -q 'fb_FileOpenTcp\|fb_FileOpenTcpServer\|fb_TcpAccept\|fb_Eoc' "$extra_generated_c"; then
        USES_TCP=1
    fi

    if grep -q 'fb_Gosub\|setjmp' "$extra_generated_c"; then
        USES_GOSUB=1
    fi

    if grep -q 'fb_sfx' "$extra_generated_c"; then
        USES_SFX=1
    fi
done

if [ "$LOCAL_GENERATED_SYMBOLS" -eq 1 ] && [ "$USES_FBCUNIT" -eq 1 ]; then
    die "--local-generated-symbols is only for simple generated-C apps"
fi

if [ "$USES_FBCUNIT" -eq 1 ]; then
    adapt_constructor_source "$GENERATED_C" "$WORK_DIR/fb_program.c" \
        "$FBCUNIT_TEST_CTORS" "$GENERATED_MAIN_ENTRIES" \
        "$LOCAL_GENERATED_SYMBOLS"

    extra_index=1

    for extra_generated_c in "${EXTRA_GENERATED_C_FILES[@]}"; do
        extra_base="fb_extra_${extra_index}.c"

        adapt_constructor_source "$extra_generated_c" "$WORK_DIR/$extra_base" \
            "$FBCUNIT_TEST_CTORS" "$GENERATED_MAIN_ENTRIES" \
            "$LOCAL_GENERATED_SYMBOLS"
        EXTRA_GENERATED_CSRCS="$EXTRA_GENERATED_CSRCS $extra_base"
        extra_index=$((extra_index + 1))
    done
else
    adapt_constructor_source "$GENERATED_C" "$WORK_DIR/fb_program.c" \
        "$FBCUNIT_TEST_CTORS" "$GENERATED_MAIN_ENTRIES" \
        "$LOCAL_GENERATED_SYMBOLS"

    extra_index=1

    for extra_generated_c in "${EXTRA_GENERATED_C_FILES[@]}"; do
        extra_base="fb_extra_${extra_index}.c"

        adapt_constructor_source "$extra_generated_c" "$WORK_DIR/$extra_base" \
            "$FBCUNIT_TEST_CTORS" "$GENERATED_MAIN_ENTRIES" \
            "$LOCAL_GENERATED_SYMBOLS"
        EXTRA_GENERATED_CSRCS="$EXTRA_GENERATED_CSRCS $extra_base"
        extra_index=$((extra_index + 1))
    done

    {
        printf '\n'
        printf '/* Explicit generated-program startup for the NuttX smoke harness. */\n'
        if [ -s "$GENERATED_MAIN_ENTRIES" ] &&
           [ "$LOCAL_GENERATED_SYMBOLS" -eq 0 ]; then
            printf 'extern int fb_nuttx_generated_main( int, char** );\n'
        fi
        while IFS= read -r ctor_name; do
            [ -n "$ctor_name" ] || continue
            if [ "$LOCAL_GENERATED_SYMBOLS" -eq 0 ]; then
                printf 'extern void %s( void );\n' "$ctor_name"
            fi
        done < "$FBCUNIT_TEST_CTORS"
        printf '\n'
        printf 'int %s( int argc, char **argv )\n' "$APP_ENTRY_SYMBOL"
        printf '{\n'
        printf '    (void)argc;\n'
        printf '    (void)argv;\n'
        while IFS= read -r ctor_name; do
            [ -n "$ctor_name" ] || continue
            printf '    %s();\n' "$ctor_name"
        done < "$FBCUNIT_TEST_CTORS"
        if [ -s "$GENERATED_MAIN_ENTRIES" ]; then
            printf '    return fb_nuttx_generated_main( argc, (char**)argv );\n'
        else
            printf '    return 0;\n'
        fi
        printf '}\n'
    } >> "$WORK_DIR/fb_program.c"
fi

if [ "$APP_NAME" = "fbsfx" ] && [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
    adapt_fbsfx_virtio_sound_hold "$WORK_DIR/fb_program.c"
fi

if [ "$USES_FBCUNIT" -eq 0 ]; then
    adapt_initial_data_restore "$WORK_DIR/fb_program.c" "$APP_ENTRY_SYMBOL"
fi

if adapt_host_file_open_paths "$WORK_DIR/fb_program.c"; then
    QEMU_SEED_HOST_INPUT_FILE=1
fi

##############################################################################
# Install temporary NuttX app
##############################################################################

if [ "$REUSE_CONFIG" -eq 0 ]; then
    rm -rf "$APP_DIR"
fi

mkdir -p "$APP_DIR"
cp "$WORK_DIR/fb_program.c" "$APP_DIR/fb_program.c"

for extra_src in $EXTRA_GENERATED_CSRCS; do
    cp "$WORK_DIR/$extra_src" "$APP_DIR/$extra_src"
done

for rt_src in "$ROOT/src/rtlib/nuttx"/fb_nuttx_*.c; do
    cp "$rt_src" "$APP_DIR/"
done

#
# The public GPIO API is shared across targets, so the NuttX implementation
# uses the API name instead of the fb_nuttx_ private-runtime prefix.
#
cp "$ROOT/src/rtlib/nuttx/gpiopins.c" "$APP_DIR/"

FBCUNIT_CSRCS=""
EXTRA_CSRCS=""
EXTRA_CXXSRCS=""
EXTRA_ASRCS=""

case "$USE_GENERIC_MEMORY" in
    0)
        ;;
    1)
        #
        # fb_MemCopyClear is target-neutral in the normal runtime.  Build it
        # separately and leave the local fallback shim staged but disabled.
        #
        cp "$ROOT/src/rtlib/mem_copyclear.c" "$APP_DIR/fb_nuttx_generic_memory.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_generic_memory.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MEMORY must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_GOSUB" in
    0)
        ;;
    1)
        #
        # The NuttX mini runtime includes helper files into fb_nuttx_minrt.c.
        # Build the normal rtlib source separately so the smoke image proves
        # this target can share that implementation while keeping the fallback
        # shim in the staged tree.
        #
        cp "$ROOT/src/rtlib/gosub.c" "$APP_DIR/fb_nuttx_generic_gosub.c"
        sed -i 's|#include <setjmp.h>|#include "setjmp.h"|' \
            "$APP_DIR/fb_nuttx_generic_gosub.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_generic_gosub.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_GOSUB must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_OBJECT" in
    0)
        ;;
    1)
        #
        # Keep object layout and run-time type checks aligned with the normal
        # FreeBASIC runtime.  The local fallback shim stays in the staged tree
        # but is disabled by CFLAGS while the generic sources provide the ABI.
        #
        cp "$ROOT/src/rtlib/oop_object.c" "$APP_DIR/fb_nuttx_oop_object.c"
        cp "$ROOT/src/rtlib/oop_istypeof.c" "$APP_DIR/fb_nuttx_oop_istypeof.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_OBJECT must be 0 or 1"
        ;;
esac

SETJMP_ASRCS=""
SETJMP_HEADER="$NUTTX_DIR/arch/risc-v/include/setjmp.h"
SETJMP_ASM_HEADER="$NUTTX_DIR/libs/libc/machine/risc-v/asm.h"

[ -f "$SETJMP_HEADER" ] || die "missing RISC-V setjmp header: $SETJMP_HEADER"
[ -f "$SETJMP_ASM_HEADER" ] || die "missing RISC-V setjmp asm header: $SETJMP_ASM_HEADER"

cp "$SETJMP_HEADER" "$APP_DIR/setjmp.h"
cp "$SETJMP_ASM_HEADER" "$APP_DIR/asm.h"

if [ "$USES_GOSUB" -eq 1 ]; then
    SETJMP_SRC="$NUTTX_DIR/libs/libc/machine/risc-v/arch_setjmp.S"

    [ -f "$SETJMP_SRC" ] || die "missing RISC-V setjmp source: $SETJMP_SRC"

    cp "$SETJMP_SRC" "$APP_DIR/arch_setjmp.S"
    SETJMP_ASRCS="arch_setjmp.S"
fi

case "$USE_GENERIC_MATH_FIX" in
    0)
        ;;
    1)
        #
        # FIX() is target-neutral in the normal runtime and only needs the
        # normal SGN helpers.  Build the shared implementation as a separate
        # app source; the local compatibility body is disabled by CFLAGS.
        #
        cp "$ROOT/src/rtlib/math_fix.c" "$APP_DIR/fb_nuttx_math_fix.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_math_fix.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MATH_FIX must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_MATH_CVN" in
    0)
        ;;
    1)
        #
        # These bit-conversion helpers are target-neutral in the normal
        # runtime.  Build the shared implementation as a separate app source
        # and disable the duplicate local copies by CFLAGS.
        #
        cp "$ROOT/src/rtlib/math_cvn.c" "$APP_DIR/fb_nuttx_math_cvn.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_math_cvn.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MATH_CVN must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_MATH_FRAC" in
    0)
        ;;
    1)
        #
        # FRAC() is implemented in the normal runtime in terms of FIX().
        # Since the lab already stages the shared FIX() helper, keep FRAC()
        # shared too instead of adding another NuttX-only math shim.
        #
        cp "$ROOT/src/rtlib/math_frac.c" "$APP_DIR/fb_nuttx_math_frac.c"
        python3 - "$APP_DIR/fb_nuttx_math_frac.c" <<'PY'
import sys

path = sys.argv[1]

with open(path, "r", encoding="utf-8") as f:
    text = f.read()

needle = '#include "fb.h"\n'
prototypes = (
    '#include "fb.h"\n\n'
    'FBCALL float fb_FIXSingle( float x );\n'
    'FBCALL double fb_FIXDouble( double x );\n'
)

if needle not in text:
    raise SystemExit("could not find math_frac include marker")

text = text.replace(needle, prototypes, 1)

with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
PY
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_math_frac.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MATH_FRAC must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_MATH_LOG10" in
    0)
        ;;
    1)
        #
        # The integer base-10 helpers are small scalar routines used by the
        # normal formatting code.  They have no hosted runtime state, so keep
        # them shared instead of adding NuttX-specific copies later.
        #
        cp "$ROOT/src/rtlib/math_log10.c" "$APP_DIR/fb_nuttx_math_log10.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_math_log10.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MATH_LOG10 must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_MATH_RND" in
    0)
        ;;
    1)
        #
        # RND/RANDOMIZE are normal runtime services, and the generic helper
        # already supports the historical QB, CRT, FAST, and MT algorithms.
        # Build it separately so the mini runtime no longer owns a compact
        # NuttX-only PRNG.
        #
        cp "$ROOT/src/rtlib/math_rnd.c" "$APP_DIR/fb_nuttx_math_rnd.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_math_rnd.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MATH_RND must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_MATH_SGN" in
    0)
        ;;
    1)
        #
        # SGN() is target-neutral in the normal runtime.  Keep using the
        # shared implementation in the NuttX lab and disable the duplicate
        # local copies in the compatibility math file by CFLAGS.
        #
        cp "$ROOT/src/rtlib/math_sgn.c" "$APP_DIR/fb_nuttx_math_sgn.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_math_sgn.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_MATH_SGN must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_DATETIME_MATH" in
    0)
        ;;
    1)
        #
        # The calendar serial math is target-neutral in the normal runtime.
        # Keep NuttX-specific clock, parsing, and formatting edges local, but
        # share the parts that define DATEADD/DATEPART/DATEDIFF semantics.
        #
        cp "$ROOT/src/rtlib/time_core.c" "$APP_DIR/fb_nuttx_time_core.c"
        python3 - "$APP_DIR/fb_nuttx_time_core.c" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
replacement = '''#include "fb.h"

#ifndef DBG_ASSERT
#define DBG_ASSERT(expr) ((void)0)
#endif
'''

text = text.replace('#include "fb.h"\n', replacement, 1)
path.write_text(text, encoding="utf-8")
PY
        cp "$ROOT/src/rtlib/time_dateserial.c" "$APP_DIR/fb_nuttx_time_dateserial.c"
        cp "$ROOT/src/rtlib/time_timeserial.c" "$APP_DIR/fb_nuttx_time_timeserial.c"
        cp "$ROOT/src/rtlib/time_decodeserdate.c" "$APP_DIR/fb_nuttx_time_decodeserdate.c"
        cp "$ROOT/src/rtlib/time_decodesertime.c" "$APP_DIR/fb_nuttx_time_decodesertime.c"
        cp "$ROOT/src/rtlib/time_week.c" "$APP_DIR/fb_nuttx_time_week.c"
        cp "$ROOT/src/rtlib/time_datepart.c" "$APP_DIR/fb_nuttx_time_datepart.c"
        cp "$ROOT/src/rtlib/time_dateadd.c" "$APP_DIR/fb_nuttx_time_dateadd.c"
        cp "$ROOT/src/rtlib/time_datediff.c" "$APP_DIR/fb_nuttx_time_datediff.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_core.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_dateserial.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_timeserial.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_decodeserdate.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_decodesertime.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_week.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_datepart.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_dateadd.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_datediff.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_DATETIME_MATH must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_CLOCK" in
    0)
        ;;
    1)
        #
        # DATE$, TIME$, and TIMER are ordinary C-library backed rtlib helpers
        # on POSIX-like targets.  Share the normal implementations so the
        # NuttX target does not keep a second copy of the formatting logic.
        #
        cp "$ROOT/src/rtlib/time_date.c" "$APP_DIR/fb_nuttx_time_date.c"
        cp "$ROOT/src/rtlib/time_time.c" "$APP_DIR/fb_nuttx_time_time.c"
        cp "$ROOT/src/rtlib/unix/time_timer.c" "$APP_DIR/fb_nuttx_time_timer.c"
        python3 - "$APP_DIR/fb_nuttx_time_date.c" \
            "$APP_DIR/fb_nuttx_time_time.c" \
            "$APP_DIR/fb_nuttx_time_timer.c" <<'PY'
from pathlib import Path
import sys

for raw_path in sys.argv[1:]:
    path = Path(raw_path)
    text = path.read_text(encoding="utf-8")
    text = text.replace('#include "../fb.h"\n', '#include "fb.h"\n', 1)
    replacement = '''#include "fb.h"

#ifndef FB_LOCK
#define FB_LOCK()
#endif

#ifndef FB_UNLOCK
#define FB_UNLOCK()
#endif
'''
    text = text.replace('#include "fb.h"\n', replacement, 1)
    path.write_text(text, encoding="utf-8")
PY
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_date.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_time.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_time_timer.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_CLOCK must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_ENVIRON" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/sys_environ.c" "$APP_DIR/fb_nuttx_sys_environ.c"
        python3 - "$APP_DIR/fb_nuttx_sys_environ.c" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
replacement = '''#include "fb.h"

#ifndef FB_STRLOCK
#define FB_STRLOCK()
#endif

#ifndef FB_STRUNLOCK
#define FB_STRUNLOCK()
#endif

FBSTRING *fb_hStrAllocTemp_NoLock(FBSTRING *str, ssize_t size);
int fb_hStrDelTemp_NoLock(FBSTRING *str);
void fb_hStrDelTemp(const FBSTRING *str);
void fb_hStrCopy(char *dst, const char *src, ssize_t bytes);
'''

text = text.replace('#include "fb.h"\n', replacement, 1)
path.write_text(text, encoding="utf-8")
PY
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_sys_environ.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_ENVIRON must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_DIR" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/sys_mkdir.c" "$APP_DIR/fb_nuttx_sys_mkdir.c"
        cp "$ROOT/src/rtlib/sys_rmdir.c" "$APP_DIR/fb_nuttx_sys_rmdir.c"
        cp "$ROOT/src/rtlib/sys_chdir.c" "$APP_DIR/fb_nuttx_sys_chdir.c"
        cp "$ROOT/src/rtlib/sys_cdir.c" "$APP_DIR/fb_nuttx_sys_cdir.c"
        python3 - "$APP_DIR/fb_nuttx_sys_mkdir.c" \
            "$APP_DIR/fb_nuttx_sys_rmdir.c" \
            "$APP_DIR/fb_nuttx_sys_chdir.c" <<'PY'
from pathlib import Path
import sys

for raw_path in sys.argv[1:]:
    path = Path(raw_path)
    text = path.read_text(encoding="utf-8")
    replacement = '''#include "fb.h"

void fb_hStrDelTemp(const FBSTRING *str);
'''

    text = text.replace('#include "fb.h"\n', replacement, 1)
    path.write_text(text, encoding="utf-8")
PY
        python3 - "$APP_DIR/fb_nuttx_sys_cdir.c" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
replacement = '''#include "fb.h"

#ifndef FB_LOCK
#define FB_LOCK()
#endif

#ifndef FB_UNLOCK
#define FB_UNLOCK()
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

FBSTRING *fb_hStrAllocTemp(FBSTRING *str, ssize_t size);
ssize_t fb_hGetCurrentDir(char *dst, ssize_t maxlen);
'''

text = text.replace('#include "fb.h"\n', replacement, 1)
path.write_text(text, encoding="utf-8")
PY
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_sys_mkdir.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_sys_rmdir.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_sys_chdir.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_sys_cdir.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_DIR must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_FILE_COPY" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/file_copy_crt.c" "$APP_DIR/fb_nuttx_file_copy_crt.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_file_copy_crt.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_FILE_COPY must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_STR_BASE" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/str_base.c" "$APP_DIR/fb_nuttx_str_base.c"
        cp "$ROOT/src/rtlib/str_hex.c" "$APP_DIR/fb_nuttx_str_hex.c"
        cp "$ROOT/src/rtlib/str_hex_lng.c" "$APP_DIR/fb_nuttx_str_hex_lng.c"
        cp "$ROOT/src/rtlib/str_hex_ptr.c" "$APP_DIR/fb_nuttx_str_hex_ptr.c"
        cp "$ROOT/src/rtlib/str_oct.c" "$APP_DIR/fb_nuttx_str_oct.c"
        cp "$ROOT/src/rtlib/str_oct_lng.c" "$APP_DIR/fb_nuttx_str_oct_lng.c"
        cp "$ROOT/src/rtlib/str_oct_ptr.c" "$APP_DIR/fb_nuttx_str_oct_ptr.c"
        cp "$ROOT/src/rtlib/str_bin.c" "$APP_DIR/fb_nuttx_str_bin.c"
        cp "$ROOT/src/rtlib/str_bin_lng.c" "$APP_DIR/fb_nuttx_str_bin_lng.c"
        cp "$ROOT/src/rtlib/str_bin_ptr.c" "$APP_DIR/fb_nuttx_str_bin_ptr.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_base.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_hex.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_hex_lng.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_hex_ptr.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_oct.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_oct_lng.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_oct_ptr.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_bin.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_bin_lng.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_bin_ptr.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_STR_BASE must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_STR_FILL" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/str_misc.c" "$APP_DIR/fb_nuttx_str_misc.c"
        cp "$ROOT/src/rtlib/str_fill.c" "$APP_DIR/fb_nuttx_str_fill.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_misc.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_fill.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_STR_FILL must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_STR_EXTRA" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/str_hskip.c" "$APP_DIR/fb_nuttx_str_hskip.c"
        cp "$ROOT/src/rtlib/str_left.c" "$APP_DIR/fb_nuttx_str_left.c"
        cp "$ROOT/src/rtlib/str_right.c" "$APP_DIR/fb_nuttx_str_right.c"
        cp "$ROOT/src/rtlib/str_mid.c" "$APP_DIR/fb_nuttx_str_mid.c"
        cp "$ROOT/src/rtlib/str_instr.c" "$APP_DIR/fb_nuttx_str_instr.c"
        cp "$ROOT/src/rtlib/str_instrrev.c" "$APP_DIR/fb_nuttx_str_instrrev.c"
        cp "$ROOT/src/rtlib/str_comp.c" "$APP_DIR/fb_nuttx_str_comp.c"
        cp "$ROOT/src/rtlib/str_len.c" "$APP_DIR/fb_nuttx_str_len.c"
        cp "$ROOT/src/rtlib/str_lcase.c" "$APP_DIR/fb_nuttx_str_lcase.c"
        cp "$ROOT/src/rtlib/str_ucase.c" "$APP_DIR/fb_nuttx_str_ucase.c"
        cp "$ROOT/src/rtlib/str_ltrim.c" "$APP_DIR/fb_nuttx_str_ltrim.c"
        cp "$ROOT/src/rtlib/str_rtrim.c" "$APP_DIR/fb_nuttx_str_rtrim.c"
        cp "$ROOT/src/rtlib/str_trim.c" "$APP_DIR/fb_nuttx_str_trim.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_hskip.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_left.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_right.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_mid.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_instr.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_instrrev.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_comp.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_len.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_lcase.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_ucase.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_ltrim.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_rtrim.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_str_trim.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_STR_EXTRA must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_WSTRING" in
    0)
        ;;
    1)
        #
        # The WSTRING operators below are ordinary rtlib code.  The NuttX
        # mini runtime keeps the platform-specific byte widening/narrowing
        # helpers, but the visible assignment, compare, concat, case, fill,
        # base conversion, and delete routines now come from the shared
        # implementation.
        #
        cp "$ROOT/src/rtlib/strw_alloc.c" "$APP_DIR/fb_nuttx_strw_alloc.c"
        cp "$ROOT/src/rtlib/strw_assign.c" "$APP_DIR/fb_nuttx_strw_assign.c"
        cp "$ROOT/src/rtlib/strw_asc.c" "$APP_DIR/fb_nuttx_strw_asc.c"
        cp "$ROOT/src/rtlib/strw_len.c" "$APP_DIR/fb_nuttx_strw_len.c"
        cp "$ROOT/src/rtlib/strw_fill.c" "$APP_DIR/fb_nuttx_strw_fill.c"
        cp "$ROOT/src/rtlib/strw_space.c" "$APP_DIR/fb_nuttx_strw_space.c"
        cp "$ROOT/src/rtlib/strw_comp.c" "$APP_DIR/fb_nuttx_strw_comp.c"
        cp "$ROOT/src/rtlib/strw_lcase.c" "$APP_DIR/fb_nuttx_strw_lcase.c"
        cp "$ROOT/src/rtlib/strw_ucase.c" "$APP_DIR/fb_nuttx_strw_ucase.c"
        cp "$ROOT/src/rtlib/strw_concat.c" "$APP_DIR/fb_nuttx_strw_concat.c"
        cp "$ROOT/src/rtlib/strw_convconcat.c" "$APP_DIR/fb_nuttx_strw_convconcat.c"
        cp "$ROOT/src/rtlib/strw_bin.c" "$APP_DIR/fb_nuttx_strw_bin.c"
        cp "$ROOT/src/rtlib/strw_bin_lng.c" "$APP_DIR/fb_nuttx_strw_bin_lng.c"
        cp "$ROOT/src/rtlib/strw_hex.c" "$APP_DIR/fb_nuttx_strw_hex.c"
        cp "$ROOT/src/rtlib/strw_hex_lng.c" "$APP_DIR/fb_nuttx_strw_hex_lng.c"
        cp "$ROOT/src/rtlib/strw_oct.c" "$APP_DIR/fb_nuttx_strw_oct.c"
        cp "$ROOT/src/rtlib/strw_oct_lng.c" "$APP_DIR/fb_nuttx_strw_oct_lng.c"
        cp "$ROOT/src/rtlib/strw_del.c" "$APP_DIR/fb_nuttx_strw_del.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_alloc.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_assign.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_asc.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_len.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_fill.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_space.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_comp.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_lcase.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_ucase.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_concat.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_convconcat.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_bin.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_bin_lng.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_hex.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_hex_lng.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_oct.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_oct_lng.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_strw_del.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_WSTRING must be 0 or 1"
        ;;
esac

case "$USE_GENERIC_ASSERT" in
    0)
        ;;
    1)
        cp "$ROOT/src/rtlib/error_assert.c" "$APP_DIR/fb_nuttx_error_assert.c"
        cp "$ROOT/src/rtlib/error_assert_wstr.c" \
            "$APP_DIR/fb_nuttx_error_assert_wstr.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_error_assert.c"
        EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_error_assert_wstr.c"
        ;;
    *)
        die "FB_NUTTX_USE_GENERIC_ASSERT must be 0 or 1"
        ;;
esac

if [ "$USE_GENERIC_OBJECT" -eq 1 ]; then
    EXTRA_CSRCS="$EXTRA_CSRCS fb_nuttx_oop_object.c fb_nuttx_oop_istypeof.c"
fi

for extra_c in "${EXTRA_C_FILES[@]}"; do
    extra_base=$(basename -- "$extra_c")
    cp "$extra_c" "$APP_DIR/$extra_base"
    EXTRA_CSRCS="$EXTRA_CSRCS $extra_base"
done

for extra_cxx in "${EXTRA_CXX_FILES[@]}"; do
    extra_base=$(basename -- "$extra_cxx")

    case "$extra_base" in
        *.cxx)
            ;;
        *.*)
            extra_base="${extra_base%.*}.cxx"
            ;;
        *)
            extra_base="$extra_base.cxx"
            ;;
    esac

    cp "$extra_cxx" "$APP_DIR/$extra_base"
    EXTRA_CXXSRCS="$EXTRA_CXXSRCS $extra_base"
done

if [ -n "$EXTRA_CXXSRCS" ]; then
    cat > "$APP_DIR/fb_nuttx_cxx_support.cxx" <<'EOF'
/*
    Minimal C++ allocation support for fbctests helper objects.

    The rv-virt NuttX smoke configuration intentionally keeps the C++ runtime
    small.  Some fbctests helper modules still use C++ delete expressions, so
    provide the global allocation hooks those objects expect.
*/

#include <stddef.h>
#include <stdlib.h>

void *operator new(size_t size)
{
    void *ptr = malloc(size == 0 ? 1 : size);

    if (ptr == NULL) {
        abort();
    }

    return ptr;
}

void *operator new[](size_t size)
{
    void *ptr = malloc(size == 0 ? 1 : size);

    if (ptr == NULL) {
        abort();
    }

    return ptr;
}

void operator delete(void *ptr) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr) noexcept
{
    free(ptr);
}

void operator delete(void *ptr, size_t size) noexcept
{
    (void)size;
    free(ptr);
}

void operator delete[](void *ptr, size_t size) noexcept
{
    (void)size;
    free(ptr);
}
EOF

    EXTRA_CXXSRCS="$EXTRA_CXXSRCS fb_nuttx_cxx_support.cxx"
fi

for extra_asm in "${EXTRA_ASM_FILES[@]}"; do
    extra_base=$(basename -- "$extra_asm")
    cp "$extra_asm" "$APP_DIR/$extra_base"
    EXTRA_ASRCS="$EXTRA_ASRCS $extra_base"
done

if [ "$USES_FBCUNIT" -eq 1 ]; then
    GENERATED_DIR=$(dirname -- "$GENERATED_C")

    for support_base in fbcunit.c fbcunit_qb.c fbcunit_console.c fbcunit_report.c; do
        support_src="$GENERATED_DIR/$support_base"
        [ -f "$support_src" ] || die "missing fbcunit support source: $support_src"

        adapt_constructor_source "$support_src" "$APP_DIR/$support_base" \
            "$FBCUNIT_SUPPORT_CTORS" "$GENERATED_MAIN_ENTRIES" \
            "$LOCAL_GENERATED_SYMBOLS"
        FBCUNIT_CSRCS="$FBCUNIT_CSRCS $support_base"
    done

    {
        printf '\n'
        printf '/* Explicit fbcunit startup for the NuttX smoke harness. */\n'
        if [ -s "$GENERATED_MAIN_ENTRIES" ] &&
           [ "$LOCAL_GENERATED_SYMBOLS" -eq 0 ]; then
            printf 'extern int32 fb_nuttx_generated_main( int32, char** );\n'
        fi
        while IFS= read -r ctor_name; do
            [ -n "$ctor_name" ] || continue
            if [ "$LOCAL_GENERATED_SYMBOLS" -eq 0 ]; then
                printf 'extern void %s( void );\n' "$ctor_name"
            fi
        done < "$FBCUNIT_SUPPORT_CTORS"
        while IFS= read -r ctor_name; do
            [ -n "$ctor_name" ] || continue
            if [ "$LOCAL_GENERATED_SYMBOLS" -eq 0 ]; then
                printf 'extern void %s( void );\n' "$ctor_name"
            fi
        done < "$FBCUNIT_TEST_CTORS"
        printf 'extern boolean _ZN4FBCU9RUN_TESTSEbb( boolean, boolean );\n'
        printf '\n'
        printf 'int %s( int argc, char **argv )\n' "$APP_ENTRY_SYMBOL"
        printf '{\n'
        printf '    int status;\n'
        printf '\n'
        printf '    (void)argc;\n'
        printf '    (void)argv;\n'
        while IFS= read -r ctor_name; do
            [ -n "$ctor_name" ] || continue
            printf '    %s();\n' "$ctor_name"
        done < "$FBCUNIT_SUPPORT_CTORS"
        while IFS= read -r ctor_name; do
            [ -n "$ctor_name" ] || continue
            printf '    %s();\n' "$ctor_name"
        done < "$FBCUNIT_TEST_CTORS"
        if [ -s "$GENERATED_MAIN_ENTRIES" ]; then
            printf '    status = (int)fb_nuttx_generated_main( (int32)argc, (char**)argv );\n'
        fi
        printf '    status = _ZN4FBCU9RUN_TESTSEbb( (boolean)1, (boolean)0 ) ? 0 : 1;\n'
        printf '    return status;\n'
        printf '}\n'
    } >> "$APP_DIR/fb_program.c"
    adapt_initial_data_restore "$APP_DIR/fb_program.c" "$APP_ENTRY_SYMBOL"
fi

GFX_CSRCS=""
SFX_CSRCS=""

if [ "$USES_SFX" -eq 1 ]; then
    mkdir -p "$APP_DIR/rtlib/nuttx" "$APP_DIR/sfxlib/nuttx" "$APP_DIR/sfxlib/third_party"

    cp "$ROOT/src/rtlib"/*.h "$APP_DIR/rtlib/"
    cp "$ROOT/src/rtlib/nuttx"/*.h "$APP_DIR/rtlib/nuttx/"
    cp "$ROOT/src/sfxlib"/*.h "$APP_DIR/sfxlib/"
    if [ "$SFX_NO_MEDIA_DECODERS" -eq 0 ]; then
        cp "$ROOT/src/sfxlib/third_party"/*.c "$APP_DIR/sfxlib/third_party/"
        cp "$ROOT/src/sfxlib/third_party"/*.h "$APP_DIR/sfxlib/third_party/"
        adapt_stb_vorbis_diagnostics "$APP_DIR/sfxlib/third_party/stb_vorbis.c"
    fi

    for sfx_src in "$ROOT/src/sfxlib"/*.c; do
        sfx_base=$(basename -- "$sfx_src")

        if [ -f "$ROOT/src/sfxlib/nuttx/$sfx_base" ]; then
            continue
        fi

        if [ "$SFX_NO_MEDIA_DECODERS" -ne 0 ] && [ "$sfx_base" = "sfx_decode.c" ]; then
            continue
        fi

        cp "$sfx_src" "$APP_DIR/sfxlib/$sfx_base"
        SFX_CSRCS="$SFX_CSRCS sfxlib/$sfx_base"
    done

    if [ "$SFX_NO_MEDIA_DECODERS" -ne 0 ]; then
        cat > "$APP_DIR/sfxlib/sfx_decode_stub.c" <<'EOF'
/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_decode_stub.c

    Purpose:

        Provide the media decode entry point for small NuttX loadable
        module smoke tests that deliberately omit bundled decoders.

    Responsibilities:

        - keep generated-tone sfxlib tests linkable on small board filesystems
        - report unsupported media decoding through the normal failure path
        - clear output parameters before returning failure

    This file intentionally does NOT contain:

        - WAV, MP3, or Ogg decoding
        - filesystem probing
        - playback or mixer logic
*/

#include <stddef.h>

#include "fb_sfx_internal.h"

int fb_sfxDecodeFile(const char *filename,
                     float **samples,
                     int *frames,
                     int *channels,
                     int *sample_rate)
{
    (void)filename;

    if (samples != NULL)
        *samples = NULL;

    if (frames != NULL)
        *frames = 0;

    if (channels != NULL)
        *channels = 0;

    if (sample_rate != NULL)
        *sample_rate = 0;

    return -1;
}

/* end of sfx_decode_stub.c */
EOF
        SFX_CSRCS="$SFX_CSRCS sfxlib/sfx_decode_stub.c"
    fi

    for sfx_src in "$ROOT/src/sfxlib/nuttx"/*.c; do
        sfx_base=$(basename -- "$sfx_src")

        cp "$sfx_src" "$APP_DIR/sfxlib/nuttx/$sfx_base"
        SFX_CSRCS="$SFX_CSRCS sfxlib/nuttx/$sfx_base"
    done

    if [ "$WITH_GFXLIB" -eq 0 ]; then
        cp "$ROOT/src/rtlib/nuttx/fb_nuttx_sfx_context.c" "$APP_DIR/"
        SFX_CSRCS="$SFX_CSRCS fb_nuttx_sfx_context.c"
    fi
fi

if [ "$WITH_GFXLIB" -eq 1 ]; then
    mkdir -p "$APP_DIR/gfxlib2/nuttx" "$APP_DIR/rtlib/nuttx"
    cp "$ROOT/src/rtlib"/*.h "$APP_DIR/rtlib/"
    cp "$ROOT/src/rtlib/nuttx"/*.h "$APP_DIR/rtlib/nuttx/"
    cp "$ROOT/src/rtlib/con_print_raw.c" "$APP_DIR/rtlib/"
    cp "$ROOT/src/rtlib/con_print_raw_uni.h" "$APP_DIR/rtlib/"
    cp "$ROOT/src/rtlib/con_print_tty.c" "$APP_DIR/rtlib/"
    cp "$ROOT/src/rtlib/con_print_tty_uni.h" "$APP_DIR/rtlib/"

    cp "$ROOT/src/gfxlib2/fb_gfx.h" "$APP_DIR/gfxlib2/"
    cp "$ROOT/src/gfxlib2/fb_gfx_gl.h" "$APP_DIR/gfxlib2/"
    cp "$ROOT/src/gfxlib2/fb_gfx_lzw.h" "$APP_DIR/gfxlib2/"
    cp "$ROOT/src/gfxlib2/gfxdata_inline.h" "$APP_DIR/gfxlib2/"

    for gfx_src in \
        gfx_access.c \
        gfx_box.c \
        gfx_cls.c \
        gfx_color.c \
        gfx_core.c \
        gfx_data.c \
        gfx_driver_null.c \
        gfx_getmouse.c \
        gfx_inkey.c \
        gfx_lzw.c \
        gfx_multikey.c \
        gfx_page.c \
        gfx_palette.c \
        gfx_pmap.c \
        gfx_point.c \
        gfx_print.c \
        gfx_print_wstr.c \
        gfx_pset.c \
        gfx_readstr.c \
        gfx_readxy.c \
        gfx_screen.c \
        gfx_screeninfo.c \
        gfx_screenlist.c \
        gfx_setmouse.c \
        gfx_sleep.c \
        gfx_vars.c \
        gfx_vgaemu.c \
        gfx_view.c \
        gfx_vsync.c \
        gfx_width.c
    do
        cp "$ROOT/src/gfxlib2/$gfx_src" "$APP_DIR/gfxlib2/"
        GFX_CSRCS="$GFX_CSRCS gfxlib2/$gfx_src"
    done

    cp "$ROOT/src/gfxlib2/nuttx/gfx_driver.c" "$APP_DIR/gfxlib2/nuttx/"
    cp "$ROOT/src/gfxlib2/nuttx/gfx_rp2350_dvi.c" "$APP_DIR/gfxlib2/nuttx/"
    GFX_CSRCS="$GFX_CSRCS gfxlib2/nuttx/gfx_driver.c"
    GFX_CSRCS="$GFX_CSRCS gfxlib2/nuttx/gfx_rp2350_dvi.c"
    GFX_CSRCS="$GFX_CSRCS rtlib/con_print_raw.c rtlib/con_print_tty.c fb_nuttx_gfx_compat.c"
fi

cat > "$APP_DIR/fb.h" <<'EOF'
/*
    Minimal FreeBASIC generated-C ABI header for the NuttX smoke app.

    This is intentionally tiny. It only defines the types needed by the
    generated C program and the seed runtime shim used by this harness.
*/
#ifndef FB_NUTTX_SMOKE_FB_H
#define FB_NUTTX_SMOKE_FB_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifndef FBCALL
#  define FBCALL
#endif

typedef int32_t int32;
typedef uint32_t uint32;
typedef uint32_t FB_WCHAR;

#define _LC(ch) ((FB_WCHAR)(ch))

#ifndef FB_FALSE
#  define FB_FALSE 0
#endif

#ifndef FB_TRUE
#  define FB_TRUE (-1)
#endif

typedef struct _FBSTRING {
    char *data;
    int32 len;
    int32 size;
} FBSTRING;

typedef struct _FB_RTTI {
    struct _FB_RTTI *pRTTIBase;
    const char *id;
    void *pMeta;
} FB_RTTI;

typedef struct _FB_BASEVT {
    void *dtor;
    FB_RTTI *pRTTI;
} FB_BASEVT;

typedef struct _FB_OBJECT {
    FB_BASEVT *pVT;
} FB_OBJECT;

typedef enum _FB_RTERROR {
    FB_RTERROR_OK = 0,
    FB_RTERROR_ILLEGALFUNCTIONCALL,
    FB_RTERROR_FILENOTFOUND,
    FB_RTERROR_FILEIO,
    FB_RTERROR_OUTOFMEM,
    FB_RTERROR_ILLEGALRESUME,
    FB_RTERROR_OUTOFBOUNDS,
    FB_RTERROR_NULLPTR,
    FB_RTERROR_NOPRIVILEGES,
    FB_RTERROR_SIGINT,
    FB_RTERROR_SIGILL,
    FB_RTERROR_SIGFPE,
    FB_RTERROR_SIGSEGV,
    FB_RTERROR_SIGTERM,
    FB_RTERROR_SIGABRT,
    FB_RTERROR_SIGQUIT,
    FB_RTERROR_RETURNWITHOUTGOSUB,
    FB_RTERROR_ENDOFFILE,
    FB_RTERROR_NOTDIMENSIONED,
    FB_RTERROR_WRONGDIMENSIONS,
    FB_RTERROR_MAX
} FB_RTERROR;

#define FB_ERRMSG_SIZE 1024

enum FB_LANG {
    FB_LANG_FB,
    FB_LANG_FB_DEPRECATED,
    FB_LANG_FB_FBLITE,
    FB_LANG_QB,
    FB_LANGS
};

#define STATIC_ASSERT(expr) _Static_assert((expr), #expr)

typedef enum _FB_RND_ALGORITHMS {
    FB_RND_AUTO = 0,
    FB_RND_CRT,
    FB_RND_FAST,
    FB_RND_MTWIST,
    FB_RND_QB,
    FB_RND_REAL
} FB_RND_ALGORITHMS;

#define FB_RND_MAX_STATE 624
#define FBRNDFAST32(arg) (((arg) * 1664525u) + 1013904223u)
#define FB_MATH_LOCK()
#define FB_MATH_UNLOCK()
#define fb_hSign(x) (((x) < 0) ? -1 : 1)

#define FB_WEEK_FIRST_SYSTEM            0
#define FB_WEEK_FIRST_JAN_1             1
#define FB_WEEK_FIRST_FOUR_DAYS         2
#define FB_WEEK_FIRST_FULL_WEEK         3
#define FB_WEEK_FIRST_DEFAULT           FB_WEEK_FIRST_JAN_1

#define FB_WEEK_DAY_SYSTEM              0
#define FB_WEEK_DAY_SUNDAY              1
#define FB_WEEK_DAY_MONDAY              2
#define FB_WEEK_DAY_TUESDAY             3
#define FB_WEEK_DAY_WEDNESDAY           4
#define FB_WEEK_DAY_THURSDAY            5
#define FB_WEEK_DAY_FRIDAY              6
#define FB_WEEK_DAY_SATURDAY            7
#define FB_WEEK_DAY_DEFAULT             FB_WEEK_DAY_SUNDAY

#define FB_TIME_INTERVAL_INVALID        0
#define FB_TIME_INTERVAL_YEAR           1
#define FB_TIME_INTERVAL_QUARTER        2
#define FB_TIME_INTERVAL_MONTH          3
#define FB_TIME_INTERVAL_DAY_OF_YEAR    4
#define FB_TIME_INTERVAL_DAY            5
#define FB_TIME_INTERVAL_WEEKDAY        6
#define FB_TIME_INTERVAL_WEEK_OF_YEAR   7
#define FB_TIME_INTERVAL_HOUR           8
#define FB_TIME_INTERVAL_MINUTE         9
#define FB_TIME_INTERVAL_SECOND         10

#define fb_hTimeDaysInYear(year) (365 + fb_hTimeLeap(year))
#define FB_CHAR_TO_INT(ch) ((int)((unsigned)(unsigned char)(ch)))
#define FB_STRSIZE(s) ((ssize_t)((FBSTRING *)(s))->len)
#define FB_STRSIZEVARLEN -1
#define FB_STRSIZEMSK ((int32)0x7fffffff)
#define FB_STRLOCK()
#define FB_STRUNLOCK()

static inline ssize_t fb_nuttx_strsetup_len(const char *ptr, ssize_t size)
{
    ssize_t len;

    if (ptr == NULL)
        return 0;

    if (size < FB_STRSIZEVARLEN)
        return (ssize_t)((int32)size & FB_STRSIZEMSK);

    if (size == 0)
        return (ssize_t)strlen(ptr);

    len = 0;

    while ((len < size) && (ptr[len] != '\0'))
        len++;

    return len;
}

#define FB_STRSETUP_FIX(s, size, ptr, len) \
    do { \
        if ((size) == FB_STRSIZEVARLEN) { \
            ptr = ((FBSTRING *)(s))->data; \
            len = FB_STRSIZE(s); \
        } else { \
            ptr = (char *)(s); \
            len = fb_nuttx_strsetup_len(ptr, (ssize_t)(size)); \
        } \
    } while (0)
#define FB_MEMCPY memcpy
#define FB_MEMCPYX memcpy
#define FB_MEMCMP memcmp
#define FB_MEMCHR memchr

FB_WCHAR *fb_wstr_AllocTemp(ssize_t chars);
void fb_wstr_Del(FB_WCHAR *text);
ssize_t fb_wstr_Len(const FB_WCHAR *text);
void fb_wstr_Copy(FB_WCHAR *dst, const FB_WCHAR *src, ssize_t chars);
FB_WCHAR *fb_wstr_Move(FB_WCHAR *dst, const FB_WCHAR *src, ssize_t chars);
void fb_wstr_Fill(FB_WCHAR *dst, FB_WCHAR ch, ssize_t chars);
int fb_wstr_Compare(const FB_WCHAR *str1, const FB_WCHAR *str2,
    ssize_t chars);
ssize_t fb_wstr_CalcDiff(const FB_WCHAR *first, const FB_WCHAR *last);
const FB_WCHAR *fb_wstr_SkipChar(const FB_WCHAR *text, ssize_t chars,
    FB_WCHAR ch);
const FB_WCHAR *fb_wstr_SkipCharRev(const FB_WCHAR *text, ssize_t chars,
    FB_WCHAR ch);
int fb_wstr_IsLower(FB_WCHAR ch);
int fb_wstr_IsUpper(FB_WCHAR ch);
FB_WCHAR fb_wstr_ToLower(FB_WCHAR ch);
FB_WCHAR fb_wstr_ToUpper(FB_WCHAR ch);
ssize_t fb_wstr_ConvFromA(FB_WCHAR *dst, ssize_t dst_chars,
    const char *src);
ssize_t fb_wstr_ConvToA(char *dst, ssize_t dst_chars,
    const FB_WCHAR *src);
FBCALL FB_WCHAR *fb_WstrBinEx_l(unsigned long long num, int digits);
FBCALL FB_WCHAR *fb_WstrHexEx_l(unsigned long long num, int digits);
FBCALL FB_WCHAR *fb_WstrOctEx_l(unsigned long long num, int digits);

typedef struct _FB_RNDSTATE {
    uint32_t algorithm;
    uint32_t length;
    double (*rndproc)(float n);
    uint32_t (*rndproc32)(void);
    union {
        uint64_t iseed64;
        uint32_t iseed32;
    };
    uint32_t *index32;
    uint32_t state32[FB_RND_MAX_STATE];
} FB_RNDSTATE;

typedef struct FB_RTLIB_CTX_ {
    int argc;
    char **argv;
    FBSTRING null_desc;
    char *errmsg;
    char hooks[1];
    char fileTB[1];
    int do_file_reset;
    int lang;
    void (*exit_sfxlib)(void);
    int (*idle_sfxlib)(int msecs);
    void (*exit_gfxlib2)(void);
} FB_RTLIB_CTX;

extern FB_RTLIB_CTX __fb_ctx;
extern char __fb_errmsg[FB_ERRMSG_SIZE];
FBCALL int fb_DateSerial(int year, int month, int day);
FBCALL double fb_TimeSerial(int hour, int minute, int second);
FBCALL int fb_Weekday(double serial, int first_day_of_week);
FBCALL double fb_FIXDouble(double x);
FBCALL void fb_Randomize(double seed, int algorithm);
FBCALL double fb_Rnd(float n);
FBCALL uint32_t fb_Rnd32(void);
FBCALL double fb_Timer(void);
FBCALL int fb_ErrorSetNum(int errnum);
FILE *fb_hOpenFile(const char *path, const char *mode);
FBCALL void fb_End(int32 status);
FBCALL void fb_Assert(char *filename, int linenum, char *funcname,
    char *expression);
FBCALL void fb_AssertWarn(char *filename, int linenum, char *funcname,
    char *expression);
FBCALL void fb_AssertW(char *filename, int linenum, char *funcname,
    FB_WCHAR *expression);
FBCALL void fb_AssertWarnW(char *filename, int linenum, char *funcname,
    FB_WCHAR *expression);
FBCALL FBSTRING *fb_hStrAllocTemp(FBSTRING *str, ssize_t size);
FBCALL void fb_hStrDelTemp(const FBSTRING *str);
FBCALL FBSTRING *fb_hStrAllocTemp_NoLock(FBSTRING *str, ssize_t size);
FBCALL int fb_hStrDelTemp_NoLock(FBSTRING *str);
FBCALL void fb_hStrCopy(char *dst, const char *src, ssize_t bytes);
FBCALL char *fb_hStrSkipChar(char *s, ssize_t len, int c);
FBCALL char *fb_hStrSkipCharRev(char *s, ssize_t len, int c);
FBCALL FBSTRING *fb_SPACE(ssize_t len);
FBCALL FBSTRING *fb_StrFill1(ssize_t cnt, int fchar);
FBCALL FBSTRING *fb_StrFill2(ssize_t cnt, FBSTRING *src);
FBCALL FBSTRING *fb_LEFT(FBSTRING *src, ssize_t chars);
FBCALL FBSTRING *fb_RIGHT(FBSTRING *src, ssize_t chars);
FBCALL FBSTRING *fb_StrMid(FBSTRING *src, ssize_t start, ssize_t len);
FBCALL ssize_t fb_StrInstr(ssize_t start, FBSTRING *src, FBSTRING *patt);
FBCALL ssize_t fb_StrInstrRev(FBSTRING *src, FBSTRING *patt, ssize_t start);
FBCALL int fb_StrCompare(void *str1, ssize_t str1_size, void *str2,
    ssize_t str2_size);
FBCALL ssize_t fb_StrLen(void *str, ssize_t str_size);
FBCALL FBSTRING *fb_StrLcase2(FBSTRING *src, int mode);
FBCALL FBSTRING *fb_StrUcase2(FBSTRING *src, int mode);
FBCALL FBSTRING *fb_LTRIM(FBSTRING *src);
FBCALL FBSTRING *fb_RTRIM(FBSTRING *src);
FBCALL FBSTRING *fb_TRIM(FBSTRING *src);
FBCALL FBSTRING *fb_HEX_b(unsigned char num);
FBCALL FBSTRING *fb_HEX_s(unsigned short num);
FBCALL FBSTRING *fb_HEX_i(unsigned int num);
FBCALL FBSTRING *fb_HEX_l(unsigned long long num);
FBCALL FBSTRING *fb_HEX_p(const void *p);
FBCALL FBSTRING *fb_HEXEx_b(unsigned char num, int digits);
FBCALL FBSTRING *fb_HEXEx_s(unsigned short num, int digits);
FBCALL FBSTRING *fb_HEXEx_i(unsigned int num, int digits);
FBCALL FBSTRING *fb_HEXEx_l(unsigned long long num, int digits);
FBCALL FBSTRING *fb_HEXEx_p(const void *p, int digits);
FBCALL FBSTRING *fb_OCT_b(unsigned char num);
FBCALL FBSTRING *fb_OCT_s(unsigned short num);
FBCALL FBSTRING *fb_OCT_i(unsigned int num);
FBCALL FBSTRING *fb_OCT_l(unsigned long long num);
FBCALL FBSTRING *fb_OCT_p(const void *p);
FBCALL FBSTRING *fb_OCTEx_b(unsigned char num, int digits);
FBCALL FBSTRING *fb_OCTEx_s(unsigned short num, int digits);
FBCALL FBSTRING *fb_OCTEx_i(unsigned int num, int digits);
FBCALL FBSTRING *fb_OCTEx_l(unsigned long long num, int digits);
FBCALL FBSTRING *fb_OCTEx_p(const void *p, int digits);
FBCALL FBSTRING *fb_BIN_b(unsigned char num);
FBCALL FBSTRING *fb_BIN_s(unsigned short num);
FBCALL FBSTRING *fb_BIN_i(unsigned int num);
FBCALL FBSTRING *fb_BIN_l(unsigned long long num);
FBCALL FBSTRING *fb_BIN_p(const void *p);
FBCALL FBSTRING *fb_BINEx_b(unsigned char num, int digits);
FBCALL FBSTRING *fb_BINEx_s(unsigned short num, int digits);
FBCALL FBSTRING *fb_BINEx_i(unsigned int num, int digits);
FBCALL FBSTRING *fb_BINEx_l(unsigned long long num, int digits);
FBCALL FBSTRING *fb_BINEx_p(const void *p, int digits);
int fb_hTimeLeap(int year);
int fb_hTimeDaysInMonth(int month, int year);
void fb_hNormalizeDate(int *pDay, int *pMonth, int *pYear);
int fb_hTimeGetIntervalType(FBSTRING *interval);
void fb_hDateDecodeSerial(double serial, int *pYear, int *pMonth, int *pDay);
void fb_hTimeDecodeSerial(double serial, int *pHour, int *pMinute,
    int *pSecond, int use_qb_hack);
int fb_hGetDayOfYearEx(int year, int month, int day);
int fb_hGetWeekOfYear(int ref_year, double serial, int first_day_of_year,
    int first_day_of_week);

static inline void hRnd_FillFAST32(uint32_t *buffer, uint32_t length32,
    uint32_t iseed32)
{
    uint32_t i;

    if ((buffer == NULL) || (length32 == 0))
        return;

    buffer[0] = iseed32;

    for (i = 1; i < length32; i++)
        buffer[i] = FBRNDFAST32(buffer[i - 1]);
}

#endif
EOF

if [ "$WITH_GFXLIB" -eq 1 ]; then
    GFX_MAIN_DECLS="extern void fb_nuttx_gfx_compat_init(int argc, FAR char *argv[]);
extern void fb_nuttx_gfx_compat_exit(void);"
    GFX_MAIN_INIT="  fb_nuttx_gfx_compat_init(argc, argv);"
    GFX_MAIN_EXIT="  fb_nuttx_gfx_compat_exit();"
else
    GFX_MAIN_DECLS=""
    GFX_MAIN_INIT=""
    GFX_MAIN_EXIT=""
fi

MINRT_CSRCS=""

if [ "$WITH_MINRT" -eq 1 ]; then
    MINRT_CSRCS=" fb_nuttx_minrt.c"
fi

cat > "$APP_DIR/${APP_NAME}_main.c" <<EOF
/****************************************************************************
 * apps/examples/$APP_NAME/${APP_NAME}_main.c
 *
 * Temporary FreeBASIC smoke-test entry point.
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>

#ifndef FAR
#  define FAR
#endif

extern int $APP_ENTRY_SYMBOL(int argc, FAR char *argv[]);
extern void fb_nuttx_set_args(int argc, FAR char *argv[]);
extern void fb_nuttx_report_status(int status);
$GFX_MAIN_DECLS

int main(int argc, FAR char *argv[])
{
  int status;

  fb_nuttx_set_args(argc, argv);
$GFX_MAIN_INIT
  status = $APP_ENTRY_SYMBOL(argc, argv);
$GFX_MAIN_EXIT
  fb_nuttx_report_status(status);

  return status;
}
EOF

cat > "$APP_DIR/Makefile" <<EOF
include \$(APPDIR)/Make.defs

PROGNAME  = \$(CONFIG_EXAMPLES_${APP_SYMBOL}_PROGNAME)
PRIORITY  = \$(CONFIG_EXAMPLES_${APP_SYMBOL}_PRIORITY)
STACKSIZE = \$(CONFIG_EXAMPLES_${APP_SYMBOL}_STACKSIZE)
MODULE    = $APP_MODULE_VALUE

CSRCS = fb_program.c$EXTRA_GENERATED_CSRCS$FBCUNIT_CSRCS$EXTRA_CSRCS$MINRT_CSRCS$GFX_CSRCS$SFX_CSRCS
CXXSRCS = $EXTRA_CXXSRCS
ASRCS = $SETJMP_ASRCS$EXTRA_ASRCS
MAINSRC = ${APP_NAME}_main.c

CFLAGS += -Wno-unused-label -Wno-unused-variable
CXXFLAGS += -Wno-attributes -fno-sized-deallocation
CFLAGS += -DHOST_NUTTX -DDISABLE_OPENGL
CFLAGS += -DFB_NUTTX_USE_GENERIC_GOSUB=$USE_GENERIC_GOSUB
CFLAGS += -DFB_NUTTX_USE_GENERIC_MEMORY=$USE_GENERIC_MEMORY
CFLAGS += -DFB_NUTTX_USE_GENERIC_OBJECT=$USE_GENERIC_OBJECT
CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_FIX=$USE_GENERIC_MATH_FIX
CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_CVN=$USE_GENERIC_MATH_CVN
CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_FRAC=$USE_GENERIC_MATH_FRAC
CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_LOG10=$USE_GENERIC_MATH_LOG10
CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_RND=$USE_GENERIC_MATH_RND
CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_SGN=$USE_GENERIC_MATH_SGN
CFLAGS += -DFB_NUTTX_USE_GENERIC_DATETIME_MATH=$USE_GENERIC_DATETIME_MATH
CFLAGS += -DFB_NUTTX_USE_GENERIC_CLOCK=$USE_GENERIC_CLOCK
CFLAGS += -DFB_NUTTX_USE_GENERIC_ENVIRON=$USE_GENERIC_ENVIRON
CFLAGS += -DFB_NUTTX_USE_GENERIC_DIR=$USE_GENERIC_DIR
CFLAGS += -DFB_NUTTX_USE_GENERIC_FILE_COPY=$USE_GENERIC_FILE_COPY
CFLAGS += -DFB_NUTTX_USE_GENERIC_STR_BASE=$USE_GENERIC_STR_BASE
CFLAGS += -DFB_NUTTX_USE_GENERIC_STR_FILL=$USE_GENERIC_STR_FILL
CFLAGS += -DFB_NUTTX_USE_GENERIC_STR_EXTRA=$USE_GENERIC_STR_EXTRA
CFLAGS += -DFB_NUTTX_USE_GENERIC_WSTRING=$USE_GENERIC_WSTRING
CFLAGS += -DFB_NUTTX_USE_GENERIC_ASSERT=$USE_GENERIC_ASSERT
EOF

if [ "$WITH_GFXLIB" -eq 1 ] || [ "$USES_SFX" -eq 1 ]; then
    printf 'CFLAGS += -DFB_NUTTX_HAS_FULL_RT_CONTEXT=1\n' >> "$APP_DIR/Makefile"
else
    printf 'CFLAGS += -DFB_NUTTX_HAS_FULL_RT_CONTEXT=0\n' >> "$APP_DIR/Makefile"
fi

cat >> "$APP_DIR/Makefile" <<EOF

ifeq (\$(MODULE),m)
DO_REGISTRATION = n
endif

include \$(APPDIR)/Application.mk

ifeq (\$(MODULE),m)
\$(BINDIR)\$(DELIM)\$(PROGNAME): \$(OBJS)
PROGOBJ_\$(BINDIR)\$(DELIM)\$(PROGNAME) += \$(OBJS)
endif
EOF

if [ "$WITH_GFXLIB" -eq 1 ]; then
    printf 'CFLAGS += -DFB_NUTTX_WITH_GFXLIB=1\n' >> "$APP_DIR/Makefile"
    printf 'CFLAGS += -I$(TOPDIR)/arch/risc-v/src/common\n' >> "$APP_DIR/Makefile"
    printf 'CFLAGS += -I$(TOPDIR)/arch/risc-v/src/rp23xx-rv\n' >> "$APP_DIR/Makefile"
fi

if [ "$QEMU_MOCK_DEVICES" -eq 1 ]; then
    printf 'CFLAGS += -DFB_NUTTX_QEMU_MOCK_DEVICES=1\n' >> "$APP_DIR/Makefile"
fi

if [ -n "${FB_NUTTX_EXTRA_CFLAGS:-}" ]; then
    printf 'CFLAGS += %s\n' "$FB_NUTTX_EXTRA_CFLAGS" >> "$APP_DIR/Makefile"
fi

cat > "$APP_DIR/Make.defs" <<EOF
ifneq (\$(CONFIG_EXAMPLES_${APP_SYMBOL}),)
CONFIGURED_APPS += \$(APPDIR)/examples/$APP_NAME
endif
EOF

cat > "$APP_DIR/Kconfig" <<EOF
config EXAMPLES_${APP_SYMBOL}
	tristate "FreeBASIC hello example"
	default n
	---help---
		Build a tiny generated-FreeBASIC application using the temporary
		NuttX runtime shim.

if EXAMPLES_${APP_SYMBOL}

config EXAMPLES_${APP_SYMBOL}_PROGNAME
	string "Program name"
	default "$APP_NAME"

config EXAMPLES_${APP_SYMBOL}_PRIORITY
	int "FreeBASIC hello task priority"
	default 100

config EXAMPLES_${APP_SYMBOL}_STACKSIZE
	int "FreeBASIC hello stack size"
	default DEFAULT_TASK_STACKSIZE

endif
EOF

KCONFIG_LINE="source \"$APP_DIR/Kconfig\""
KCONFIG_TMP="$WORK_DIR/examples.Kconfig"

if [ ! -f "$APPS_DIR/examples/Kconfig" ]; then
    #
    # apps_distclean removes generated Kconfig indexes.  The temporary app is
    # staged before NuttX configure runs, so rebuild the Examples index here
    # instead of requiring an earlier build to have left one behind.
    #
    (
        cd "$APPS_DIR/examples"
        "$APPS_DIR/tools/mkkconfig.sh" -m Examples
    )
fi

awk -v line="$KCONFIG_LINE" \
    -v old_apps="$FB_NUTTX_KCONFIG_OLD_APPS" '
    BEGIN {
        count = split(old_apps, apps, " ")
    }
    {
        for( i = 1; i <= count; i++ ) {
            if( $0 == "source \"examples/" apps[i] "/Kconfig\"" ) {
                next
            }

            if( $0 ~ "source \".*/examples/" apps[i] "/Kconfig\"" ) {
                next
            }
        }

        if( $0 ~ "source \".*/examples/(fb|eg)_[^\"]*/Kconfig\"" && $0 != line ) {
            next
        }

        if( $0 ~ "source \"examples/(fb|eg)_[^\"]*/Kconfig\"" && $0 != line ) {
            next
        }
    }
    $0 == "endmenu # Examples" && inserted == 0 {
        print line
        inserted = 1
    }
    { print }
    END {
        if( inserted == 0 ) {
            print line
        }
    }
' "$APPS_DIR/examples/Kconfig" > "$KCONFIG_TMP"

if ! cmp -s "$KCONFIG_TMP" "$APPS_DIR/examples/Kconfig"; then
    cp "$KCONFIG_TMP" "$APPS_DIR/examples/Kconfig"
fi

##############################################################################
# Build and run
##############################################################################

(
    cd "$NUTTX_WORKDIR"
    apply_nuttx_patch_if_needed \
        "$ROOT/build_scripts/nuttx-patches/apps-fbcon-unique-custom-bpp-choice.patch"
)

cd "$NUTTX_DIR"

apply_nuttx_patch_if_needed \
    "$ROOT/build_scripts/nuttx-patches/riscv-elf-lo12-s-rela-addend.patch"

if [ "$QEMU_USB_ROOT_DEVICES" -eq 1 ] ||
   [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
    apply_nuttx_patch_if_needed \
        "$ROOT/build_scripts/nuttx-patches/qemu-rv-virt-pci-fdt-registration.patch"
    apply_nuttx_patch_if_needed \
        "$ROOT/build_scripts/nuttx-patches/qemu-rv-virt-pci-intx.patch"
    if grep -q "pci_get_irq(priv->pcidev)" "$NUTTX_DIR/drivers/usbhost/usbhost_xhci_pci.c" &&
       grep -q "up_addrenv_pa_to_va(priv->pg_sb" "$NUTTX_DIR/drivers/usbhost/usbhost_xhci_pci.c"; then
        echo "==> already applied: $ROOT/build_scripts/nuttx-patches/qemu-rv-virt-xhci-intx.patch"
    else
        apply_nuttx_patch_if_needed \
            "$ROOT/build_scripts/nuttx-patches/qemu-rv-virt-xhci-intx.patch"
    fi
    if grep -q "usbhost_cdcecm_initialize()" \
       "$NUTTX_DIR/drivers/usbhost/usbhost_drivers.c"; then
        echo "==> already applied: $ROOT/build_scripts/nuttx-patches/qemu-rv-virt-usbhost-net.patch"
    else
        apply_nuttx_patch_if_needed \
            "$ROOT/build_scripts/nuttx-patches/qemu-rv-virt-usbhost-net.patch"
    fi
    apply_nuttx_patch_if_needed \
        "$ROOT/build_scripts/nuttx-patches/qemu-rv-virt-usbhost-net-qemu-cdc.patch"
fi

if [ "$EXPECT_QEMU_USB_HUB_SUPPORTED" -eq 1 ]; then
    apply_nuttx_patch_if_needed \
        "$ROOT/build_scripts/nuttx-patches/qemu-rv-virt-xhci-hub.patch"
fi

if [ "$NETWORK_SHELL" -eq 1 ] && [ "$NUTTX_CONFIG" = "rv-virt:nsh" ]; then
    NUTTX_CONFIG="rv-virt:netnsh"
fi

if [ "$REUSE_CONFIG" -eq 0 ] && [ "$SKIP_NUTTX_CONFIG" -eq 0 ]; then
    run ./tools/configure.sh "$NUTTX_CONFIG"
fi

if [ -f "$NUTTX_DIR/openamp/libmetal/lib/system/nuttx/io.c" ]; then
    #
    # configure.sh materializes NuttX's OpenAMP/libmetal dependency.  Current
    # libmetal calls address-environment hooks even for flat rv-virt builds,
    # where physical and virtual addresses are identical and those optional
    # hooks do not exist.
    #
    apply_nuttx_patch_if_needed \
        "$ROOT/build_scripts/nuttx-patches/openamp-nuttx-flat-address-identity.patch"
fi

#
# Interrupted or relocated NuttX builds can leave generated dependency files
# behind that refer to stale .ddc fragments or package-cache include paths.
# Removing dependency state is safe even when reusing a board configuration:
# NuttX will regenerate it on the next make pass, while .config and the staged
# app sources remain untouched.
#
find "$NUTTX_DIR" "$APPS_DIR" \
    \( -name '.depend' -o -name 'Make.dep' -o -name '*.ddc' \) \
    -type f -delete

if [ "$REUSE_CONFIG" -eq 0 ]; then
    if [ "$KEEP_EXISTING_APPS" -eq 0 ]; then
        find "$APPS_DIR/examples" -mindepth 2 -maxdepth 2 -name Kconfig -print |
        while IFS= read -r kconfig_file; do
            sed -n 's/^[[:space:]]*config[[:space:]]*\(EXAMPLES_\(FB\|EG\)[A-Za-z0-9_]*\)$/\1/p' "$kconfig_file"
        done |
        sort -u |
        while IFS= read -r fb_example_symbol; do
            [ -n "$fb_example_symbol" ] || continue
            run kconfig-tweak --disable "CONFIG_$fb_example_symbol"
        done
    fi

    run kconfig-tweak --enable "CONFIG_EXAMPLES_${APP_SYMBOL}"
    run kconfig-tweak --set-str "CONFIG_EXAMPLES_${APP_SYMBOL}_PROGNAME" "$APP_NAME"
    run kconfig-tweak --set-val "CONFIG_EXAMPLES_${APP_SYMBOL}_PRIORITY" "$APP_PRIORITY"
    run kconfig-tweak --set-val "CONFIG_EXAMPLES_${APP_SYMBOL}_STACKSIZE" "$APP_STACKSIZE"

    if [ "$LOADABLE_MODULE" -eq 1 ]; then
        run kconfig-tweak --enable CONFIG_ELF
        run kconfig-tweak --enable CONFIG_MODULES
    fi

    run kconfig-tweak --disable CONFIG_STACK_USAGE

    # The rv-virt default interrupt stack is only 2 KiB.  At low QEMU memory sizes
    # the timer interrupt path can overflow it during boot, which makes the kernel
    # panic before the FreeBASIC program gets control.  The extra 2 KiB is cheap
    # compared with the time lost chasing false runtime failures.
    run kconfig-tweak --set-val CONFIG_ARCH_INTERRUPTSTACK 4096

    run kconfig-tweak --enable CONFIG_ARCH_RV32
    run kconfig-tweak --disable CONFIG_ARCH_RV64
    run kconfig-tweak --enable CONFIG_ARCH_RV_ISA_M
    run kconfig-tweak --enable CONFIG_ARCH_RV_ISA_A
    run kconfig-tweak --enable CONFIG_ARCH_RV_ISA_C
    run kconfig-tweak --enable CONFIG_ARCH_RV_ISA_ZICSR_ZIFENCEI
    run kconfig-tweak --enable CONFIG_RISCV_TOOLCHAIN_GNU_RV64
    run kconfig-tweak --disable CONFIG_RISCV_TOOLCHAIN_GNU_RV32
    run kconfig-tweak --enable CONFIG_ARCH_TOOLCHAIN_GNU
    run kconfig-tweak --set-val CONFIG_16550_REGINCR 1
    run kconfig-tweak --set-val CONFIG_16550_REGWIDTH 8
    run kconfig-tweak --enable CONFIG_LIBM

    run kconfig-tweak --enable CONFIG_FS_TMPFS

    if [ "$USES_SFX" -eq 1 ]; then
        # sfxlib nests its runtime lock during lazy/core/driver setup.
        run kconfig-tweak --enable CONFIG_PTHREAD_MUTEX_TYPES
    fi

    if [ "$QEMU_STORAGE_BACKEND" = "virtio-blk" ]; then
        run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO
        run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO_MMIO
        run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO_BLK
        run kconfig-tweak --enable CONFIG_BCH
        run kconfig-tweak --enable CONFIG_FS_FAT
        run kconfig-tweak --enable CONFIG_FS_FAT_WRITE
        run kconfig-tweak --enable CONFIG_SYSTEM_MKFATFS
    fi

    if [ "$QEMU_USB_ROOT_DEVICES" -eq 1 ] || [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
        run kconfig-tweak --enable CONFIG_LIBC_FDT
        run kconfig-tweak --enable CONFIG_DEVICE_TREE
        run kconfig-tweak --enable CONFIG_PCI
        run kconfig-tweak --disable CONFIG_PCI_MSIX
        run kconfig-tweak --enable CONFIG_SCHED_WORKQUEUE
        run kconfig-tweak --enable CONFIG_SCHED_HPWORK
        run kconfig-tweak --enable CONFIG_SCHED_LPWORK
        run kconfig-tweak --set-val CONFIG_SCHED_HPWORKSTACKSIZE 8192
        run kconfig-tweak --set-val CONFIG_SCHED_LPWORKSTACKSIZE 8192
        run kconfig-tweak --enable CONFIG_USBHOST
        run kconfig-tweak --enable CONFIG_USBHOST_WAITER
        if [ "$EXPECT_QEMU_USB_HUB_SUPPORTED" -eq 1 ]; then
            run kconfig-tweak --enable CONFIG_USBHOST_HUB
            run kconfig-tweak --set-val CONFIG_USBHOST_HUB_POLLMSEC 100
        else
            run kconfig-tweak --disable CONFIG_USBHOST_HUB
        fi
        run kconfig-tweak --enable CONFIG_USBHOST_XHCI_PCI
        run kconfig-tweak --enable CONFIG_USBHOST_MSC
        run kconfig-tweak --enable CONFIG_USBHOST_HIDKBD
        run kconfig-tweak --enable CONFIG_USBHOST_HIDMOUSE
        run kconfig-tweak --enable CONFIG_USBHOST_CDCECM
        run kconfig-tweak --enable CONFIG_NET
        run kconfig-tweak --enable CONFIG_NETDEVICES
        run kconfig-tweak --enable CONFIG_NET_IPv4
        run kconfig-tweak --enable CONFIG_NET_TCP
        run kconfig-tweak --enable CONFIG_BCH
        run kconfig-tweak --enable CONFIG_FS_FAT
        run kconfig-tweak --enable CONFIG_FS_FAT_WRITE
        run kconfig-tweak --enable CONFIG_SYSTEM_MKFATFS
    fi

    if [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
        run kconfig-tweak --enable CONFIG_AUDIO
        run kconfig-tweak --enable CONFIG_AUDIO_FORMAT_PCM
        run kconfig-tweak --enable CONFIG_DRIVERS_AUDIO
        run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO
        run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO_MMIO
        run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO_SOUND
        run kconfig-tweak --set-val CONFIG_AUDIO_NUM_BUFFERS 2
        run kconfig-tweak --set-val CONFIG_AUDIO_BUFFER_NUMBYTES 8192
    fi
fi

if [ "$NETWORK_SHELL" -eq 1 ]; then
    run kconfig-tweak --enable CONFIG_SCHED_WORKQUEUE
    run kconfig-tweak --set-val CONFIG_SCHED_LPWORKSTACKSIZE 8192
    run kconfig-tweak --enable CONFIG_NET
    run kconfig-tweak --enable CONFIG_NETDEVICES
    run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO
    run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO_MMIO
    run kconfig-tweak --enable CONFIG_DRIVERS_VIRTIO_NET
    run kconfig-tweak --enable CONFIG_NET_IPv4
    run kconfig-tweak --enable CONFIG_NET_TCP
    run kconfig-tweak --enable CONFIG_NET_TCPBACKLOG
    run kconfig-tweak --enable CONFIG_NET_SOCKOPTS
    run kconfig-tweak --disable CONFIG_NET_TCP_WRITE_BUFFERS
    run kconfig-tweak --set-val CONFIG_NET_MAX_LISTENPORTS 16
    run kconfig-tweak --disable CONFIG_NETUTILS_IPERF
    run kconfig-tweak --enable CONFIG_NETUTILS_TELNETD
    run kconfig-tweak --enable CONFIG_SYSTEM_TELNETD
    run kconfig-tweak --set-val CONFIG_SYSTEM_TELNETD_PORT 23
    run kconfig-tweak --set-val CONFIG_SYSTEM_TELNETD_STACKSIZE 8192
    run kconfig-tweak --set-val CONFIG_SYSTEM_TELNETD_SESSION_STACKSIZE 8192
    run kconfig-tweak --enable CONFIG_NSH_TELNET
    run kconfig-tweak --disable CONFIG_NSH_DISABLE_TELNETSTART
    run kconfig-tweak --enable CONFIG_NETUTILS_FTPD
    run kconfig-tweak --enable CONFIG_EXAMPLES_FTPD
    run kconfig-tweak --set-val CONFIG_EXAMPLES_FTPD_PORT 21
    run kconfig-tweak --set-val CONFIG_EXAMPLES_FTPD_STACKSIZE 8192
fi

if [ "$USES_TCP" -eq 1 ]; then
    run kconfig-tweak --enable CONFIG_SCHED_WORKQUEUE
    run kconfig-tweak --set-val CONFIG_SCHED_LPWORKSTACKSIZE 8192
    run kconfig-tweak --enable CONFIG_NET
    run kconfig-tweak --enable CONFIG_NETDEV_LATEINIT
    run kconfig-tweak --enable CONFIG_NET_IPv4
    run kconfig-tweak --enable CONFIG_NET_LOOPBACK
    run kconfig-tweak --set-val CONFIG_NET_LOOPBACK_PKTSIZE 65535
    run kconfig-tweak --enable CONFIG_PIPES
    run kconfig-tweak --set-val CONFIG_DEV_FIFO_SIZE 1024
    run kconfig-tweak --set-val CONFIG_DEV_PIPE_SIZE 1024
    run kconfig-tweak --set-val CONFIG_DEV_PIPE_MAXSIZE 4096
    run kconfig-tweak --set-str CONFIG_DEV_PIPE_VFS_PATH /dev/pipe
    run kconfig-tweak --enable CONFIG_NET_LOCAL
    run kconfig-tweak --set-str CONFIG_NET_LOCAL_VFS_PATH /var/run
    run kconfig-tweak --enable CONFIG_NET_LOCAL_STREAM
    run kconfig-tweak --enable CONFIG_NET_TCP
    run kconfig-tweak --enable CONFIG_NET_TCPBACKLOG
    run kconfig-tweak --enable CONFIG_NET_SOCKOPTS
    run kconfig-tweak --disable CONFIG_NET_TCP_WRITE_BUFFERS
    run kconfig-tweak --set-val CONFIG_NET_TCP_PREALLOC_CONNS 16
    run kconfig-tweak --set-val CONFIG_NET_MAX_LISTENPORTS 8
fi

if [ "$LOADABLE_MODULE" -eq 1 ] && [ "$REUSE_CONFIG" -eq 1 ]; then
    if config_has_line "CONFIG_EXAMPLES_${APP_SYMBOL}=m" &&
       config_has_line "CONFIG_EXAMPLES_${APP_SYMBOL}_PROGNAME=\"$APP_NAME\"" &&
       config_has_line "CONFIG_EXAMPLES_${APP_SYMBOL}_PRIORITY=$APP_PRIORITY" &&
       config_has_line "CONFIG_EXAMPLES_${APP_SYMBOL}_STACKSIZE=$APP_STACKSIZE" &&
       config_has_line "CONFIG_ELF=y" &&
       config_has_line "CONFIG_MODULES=y"; then
        echo "==> reusing existing loadable-module Kconfig state"
    else
        run kconfig-tweak --module "CONFIG_EXAMPLES_${APP_SYMBOL}"
        run kconfig-tweak --set-str "CONFIG_EXAMPLES_${APP_SYMBOL}_PROGNAME" "$APP_NAME"
        run kconfig-tweak --set-val "CONFIG_EXAMPLES_${APP_SYMBOL}_PRIORITY" "$APP_PRIORITY"
        run kconfig-tweak --set-val "CONFIG_EXAMPLES_${APP_SYMBOL}_STACKSIZE" "$APP_STACKSIZE"
        run kconfig-tweak --enable CONFIG_ELF
        run kconfig-tweak --enable CONFIG_MODULES
        yes "" | make olddefconfig >/dev/null || true
    fi
fi

if [ "$REUSE_CONFIG" -eq 0 ]; then
    kconfig-tweak --set-val CONFIG_STACK_USAGE_WARNING 0 >/dev/null 2>&1 || true
    yes "" | make olddefconfig >/dev/null || true
    if [ "$QEMU_USB_ROOT_DEVICES" -eq 1 ] || [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
        kconfig-tweak --disable CONFIG_PCI_MSIX
        python3 - <<'PY'
from pathlib import Path

config = Path(".config")
text = config.read_text()
text = text.replace("CONFIG_PCI_MSIX=y", "# CONFIG_PCI_MSIX is not set")
config.write_text(text)
PY
        yes "" | make olddefconfig >/dev/null || true
    fi
fi

if [ "$USES_SFX" -eq 1 ] &&
   ! config_has_line "CONFIG_PTHREAD_MUTEX_TYPES=y"; then
    die "NuttX sfxlib requires CONFIG_PTHREAD_MUTEX_TYPES=y"
fi

if [ "$LOADABLE_MODULE" -eq 1 ]; then
    MODULE_FILE="$APPS_DIR/bin/$APP_NAME"

    rm -f "$MODULE_FILE"
    run make EXTRALINKCMDS="${EXTRALINKCMDS:-} --no-warn-rwx-segments" -j"$JOBS" context pass2

    [ -f "$MODULE_FILE" ] ||
        die "loadable module was not produced: $MODULE_FILE"

    allocate_loadable_module_common_symbols "$MODULE_FILE"

    echo "NUTTX_RISCV32_FB_MODULE_OK"
    echo "MODULE: $MODULE_FILE"
    exit 0
fi

run make EXTRALINKCMDS="${EXTRALINKCMDS:-} --no-warn-rwx-segments" -j"$JOBS"

if [ "$RUN_QEMU" -eq 1 ]; then
    QEMU_LOG="$WORK_DIR/qemu.log"
    QEMU_INPUT="$WORK_DIR/qemu.input"
    QEMU_MONITOR="$WORK_DIR/qemu-monitor.sock"
    QEMU_STORAGE_IMAGE="$WORK_DIR/qemu-storage.img"
    QEMU_USB_STORAGE_IMAGE="$WORK_DIR/qemu-usb-storage.img"
    QEMU_SETUP='mkdir /ram
mount -t tmpfs /ram
cd /ram
mkdir file
mkdir sfx
'

    if [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
        QEMU_SETUP="${QEMU_SETUP}"'sleep 2
echo "fbxl QEMU virtio sound device directory follows"
ls /dev
ls /dev/audio
'

        if [ "$APP_NAME" = "fbsfx" ]; then
            QEMU_SETUP="${QEMU_SETUP}"'set FB_SFX_NUTTX_KEEP_AUDIO_OPEN 1
'
        fi
    fi

    if [ -n "$QEMU_STORAGE_ROOT" ]; then
        QEMU_STORAGE_PARENT=$(dirname -- "$QEMU_STORAGE_ROOT")
        QEMU_SETUP="${QEMU_SETUP}"'mkdir '"$QEMU_STORAGE_PARENT"'
mkdir '"$QEMU_STORAGE_ROOT"'
'
        if [ "$QEMU_STORAGE_BACKEND" = "virtio-blk" ]; then
            QEMU_SETUP="${QEMU_SETUP}"'mkfatfs /dev/virtblk0
mount -t vfat /dev/virtblk0 '"$QEMU_STORAGE_ROOT"'
echo "fbxl QEMU virtio block storage ready: '"$QEMU_STORAGE_ROOT"'"
'
        else
            QEMU_SETUP="${QEMU_SETUP}"'mount -t tmpfs '"$QEMU_STORAGE_ROOT"'
echo "fbxl QEMU tmpfs storage ready: '"$QEMU_STORAGE_ROOT"'"
'
        fi
    fi

    if [ -n "$QEMU_USB_STORAGE_ROOT" ]; then
        QEMU_USB_STORAGE_PARENT=$(dirname -- "$QEMU_USB_STORAGE_ROOT")
        QEMU_SETUP="${QEMU_SETUP}"'mkdir '"$QEMU_USB_STORAGE_PARENT"'
mkdir '"$QEMU_USB_STORAGE_ROOT"'
'
        if [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
            QEMU_SETUP="${QEMU_SETUP}"'sleep 3
echo "fbxl QEMU usb device directory follows"
ls /dev
'
        fi

        QEMU_SETUP="${QEMU_SETUP}"'mkfatfs /dev/sda
mount -t vfat /dev/sda '"$QEMU_USB_STORAGE_ROOT"'
echo "fbxl QEMU usb-storage ready: '"$QEMU_USB_STORAGE_ROOT"'"
'
    fi

    if [ "$QEMU_SEED_HOST_INPUT_FILE" -eq 1 ]; then
        QEMU_SETUP="${QEMU_SETUP}"'printf seed > fbctest-file-open.bas
'
    fi

    if [ "$NETWORK_SHELL" -eq 1 ]; then
        printf '%bifconfig\nftpd_start\necho "fbxl NuttX network shell ready"\n' \
            "$QEMU_SETUP" > "$QEMU_INPUT"
    elif [ "$APP_NAME" = "fbhello" ]; then
        printf '%b%s fbarg\nnuttx input line\n321\ninput string value\n6.25\n1234567890123\nexit\n' "$QEMU_SETUP" "$APP_NAME" > "$QEMU_INPUT"
    elif [ "$WITH_GFXLIB" -eq 1 ] && [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
        printf '%b%s\n' "$QEMU_SETUP" "$APP_NAME" > "$QEMU_INPUT"
    elif [ "$WITH_GFXLIB" -eq 1 ]; then
        printf '%b%s\nz\nexit\n' "$QEMU_SETUP" "$APP_NAME" > "$QEMU_INPUT"
    elif [ "$APP_NAME" = "fbsfx" ] && [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
        printf '%b%s\n' "$QEMU_SETUP" "$APP_NAME" > "$QEMU_INPUT"
    else
        printf '%b%s\nexit\n' "$QEMU_SETUP" "$APP_NAME" > "$QEMU_INPUT"
    fi

    QEMU_ARGS=(
        qemu-system-riscv32
        -semihosting
        -M virt,aclint=on
        -m "$QEMU_MEMORY"
        -cpu rv32
        -bios none
        -kernel nuttx
        -nographic
    )

    if [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
        rm -f "$QEMU_MONITOR"
        QEMU_ARGS+=(
            -monitor "unix:$QEMU_MONITOR,server,nowait"
        )
    fi

    if [ "$QEMU_STORAGE_BACKEND" = "virtio-blk" ]; then
        truncate -s "$QEMU_STORAGE_IMAGE_SIZE" "$QEMU_STORAGE_IMAGE"
        QEMU_ARGS+=(
            -drive "file=$QEMU_STORAGE_IMAGE,if=none,format=raw,id=fbxlblk"
            -device "virtio-blk-device,drive=fbxlblk"
        )
    fi

    if [ "$QEMU_USB_ROOT_DEVICES" -eq 1 ]; then
        truncate -s "$QEMU_USB_STORAGE_IMAGE_SIZE" "$QEMU_USB_STORAGE_IMAGE"
        QEMU_ARGS+=(
            -device "qemu-xhci,id=fbxlxhci"
            -drive "file=$QEMU_USB_STORAGE_IMAGE,if=none,format=raw,id=fbxlusb"
            -device "usb-storage,bus=fbxlxhci.0,port=1,drive=fbxlusb"
            -device "usb-kbd,bus=fbxlxhci.0,port=2"
            -device "usb-mouse,bus=fbxlxhci.0,port=3"
            -netdev "user,id=fbxlusbnet"
            -device "usb-net,bus=fbxlxhci.0,port=4,netdev=fbxlusbnet"
        )
    fi

    if [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
        truncate -s "$QEMU_USB_STORAGE_IMAGE_SIZE" "$QEMU_USB_STORAGE_IMAGE"
        QEMU_ARGS+=(
            -device "qemu-xhci,id=fbxlxhci"
            -drive "file=$QEMU_USB_STORAGE_IMAGE,if=none,format=raw,id=fbxlusb"
            -device "usb-hub,bus=fbxlxhci.0,port=1,id=fbxlhub,ports=4,port-power=on"
            -device "usb-storage,bus=fbxlxhci.0,port=1.1,drive=fbxlusb"
            -device "usb-kbd,bus=fbxlxhci.0,port=1.2"
            -device "usb-mouse,bus=fbxlxhci.0,port=1.3"
            -netdev "user,id=fbxlusbnet"
            -device "usb-net,bus=fbxlxhci.0,port=1.4,netdev=fbxlusbnet"
        )
    fi

    if [ "$NETWORK_SHELL" -eq 1 ]; then
        QEMU_ARGS+=(
            -netdev "user,id=fbxl0,hostfwd=tcp:${QEMU_HOSTFWD_BIND}:${HOST_TELNET_PORT}-:23,hostfwd=tcp:${QEMU_HOSTFWD_BIND}:${HOST_FTP_PORT}-:21"
            -device "virtio-net-device,netdev=fbxl0"
        )

        echo "NuttX telnet will listen through host port $HOST_TELNET_PORT"
        echo "NuttX FTP will listen through host port $HOST_FTP_PORT"
    fi

    if [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
        QEMU_ARGS+=(
            -audiodev "none,id=fbxlaudio"
            -device "virtio-sound-device,audiodev=fbxlaudio"
        )
    fi

    trap - ERR
    set +e
    (
        sleep 3
        if [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
            python3 - "$QEMU_INPUT" "$QEMU_SERIAL_CHAR_DELAY" "$QEMU_SERIAL_LINE_DELAY" <<'PY'
from pathlib import Path
import sys
import time

input_path = Path(sys.argv[1])
char_delay = float(sys.argv[2])
line_delay = float(sys.argv[3])

for char in input_path.read_text().replace("\r\n", "\n"):
    sys.stdout.write(char)
    sys.stdout.flush()

    if char == "\n":
        time.sleep(line_delay)
    else:
        time.sleep(char_delay)
PY
        else
            cat "$QEMU_INPUT"
        fi
        if [ "$NETWORK_SHELL" -eq 1 ]; then
            cat
        elif [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
            python3 - "$QEMU_MONITOR" "$QEMU_HID_INJECT_DELAY" <<'PY' >/dev/null 2>&1
import socket
import sys
import time

path = sys.argv[1]
inject_delay = float(sys.argv[2])
deadline = time.time() + 20.0

while True:
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(path)
        break
    except OSError:
        if time.time() >= deadline:
            raise

        time.sleep(0.1)

with sock:
    sock.settimeout(1.0)

    try:
        sock.recv(4096)
    except OSError:
        pass

    time.sleep(inject_delay)

    for command in (b"mouse_move 17 9\n", b"sendkey z\n", b"sendkey z\n"):
        sock.sendall(command)
        time.sleep(0.2)
PY
            if [ "$QEMU_HID_SERIAL_EXIT_DELAY" != "0" ]; then
                sleep "$QEMU_HID_SERIAL_EXIT_DELAY"
                printf 'exit\n'
            else
                sleep 8
            fi
        else
            sleep 1
        fi
    ) |
        if [ "$QEMU_TIMEOUT" = "0" ]; then
            "${QEMU_ARGS[@]}" > "$QEMU_LOG" 2>&1
        else
            timeout -k 5s "$QEMU_TIMEOUT" "${QEMU_ARGS[@]}" > "$QEMU_LOG" 2>&1
        fi
    QEMU_STATUS=$?
    set -e
    trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

    cat "$QEMU_LOG"

    if grep -Eq "PANIC!!!|EXCEPTION:" "$QEMU_LOG"; then
        die "NuttX reported a panic or CPU exception"
    fi

    if [ "$EXPECT_QEMU_USB_NET_UNSUPPORTED" -eq 1 ]; then
        if ! grep -Fq "class:2 subclass:0 protocol:0" "$QEMU_LOG" ||
           ! grep -Fq "usbhost_classbind: Returning: -22" "$QEMU_LOG"; then
            die "QEMU usb-net mismatch was not observed"
        fi

        echo "NuttX USB Ethernet: QEMU usb-net enumerated but did not bind"
    fi

    if [ "$EXPECT_QEMU_USB_NET_SUPPORTED" -eq 1 ]; then
        if ! grep -Fq "class:2 subclass:0 protocol:0" "$QEMU_LOG"; then
            die "QEMU usb-net device descriptor was not observed"
        fi

        if grep -Fq "usbhost_classbind: Returning: -22" "$QEMU_LOG"; then
            die "QEMU usb-net still failed class binding"
        fi

        if ! grep -Eq "Device MAC:|Using random MAC address|cdcecm_net_ifup" "$QEMU_LOG"; then
            die "QEMU usb-net did not reach the CDC Ethernet network device path"
        fi

        echo "NuttX USB Ethernet: QEMU usb-net bound as a CDC Ethernet device"
    fi

    if [ "$EXPECT_QEMU_USB_HUB_UNSUPPORTED" -eq 1 ]; then
        if ! grep -Fq "class:9 subclass:0 protocol:0" "$QEMU_LOG" ||
           ! grep -Fq "usbhost_classbind: Returning: -22" "$QEMU_LOG"; then
            die "QEMU usb-hub unsupported-class marker was not observed"
        fi

        echo "NuttX USB hub: QEMU usb-hub enumerated but did not bind"
    fi

    if [ "$EXPECT_QEMU_USB_HUB_SUPPORTED" -eq 1 ]; then
        if ! grep -Fq "class:9 subclass:0 protocol:0" "$QEMU_LOG"; then
            die "QEMU usb-hub device descriptor was not observed"
        fi

        if ! grep -Fq "class:8 subclass:6 protocol:80" "$QEMU_LOG"; then
            die "QEMU hub-attached usb-storage class was not observed"
        fi

        if ! grep -Fq "class:3 subclass:1 protocol:1" "$QEMU_LOG"; then
            die "QEMU hub-attached keyboard class was not observed"
        fi

        if ! grep -Fq "usbhost_kbdpoll: Started" "$QEMU_LOG" ||
           ! grep -Fq "usbhost_kbdpoll: Entering poll loop" "$QEMU_LOG"; then
            die "QEMU hub-attached keyboard did not start HID polling"
        fi

        if ! grep -Fq "class:3 subclass:1 protocol:2" "$QEMU_LOG"; then
            die "QEMU hub-attached mouse class was not observed"
        fi

        if ! grep -Fq "usbhost_mouse_poll: Started" "$QEMU_LOG" ||
           ! grep -Fq "usbhost_mouse_poll: Entering poll loop" "$QEMU_LOG"; then
            die "QEMU hub-attached mouse did not start HID polling"
        fi

        if ! grep -Fq "fbxl QEMU usb device directory follows" "$QEMU_LOG"; then
            die "QEMU hub-attached USB device directory was not listed"
        fi

        if ! grep -Eq "(^|[^A-Za-z0-9_])kbda([^A-Za-z0-9_]|$)" "$QEMU_LOG"; then
            die "QEMU hub-attached keyboard did not expose /dev/kbda"
        fi

        if ! grep -Eq "(^|[^A-Za-z0-9_])mouse0([^A-Za-z0-9_]|$)" "$QEMU_LOG"; then
            die "QEMU hub-attached mouse did not expose /dev/mouse0"
        fi

        if ! grep -Fq "usbhost_classbind: Returning: 0" "$QEMU_LOG"; then
            die "No successful QEMU hub-attached USB class bind was observed"
        fi

        echo "NuttX USB hub: QEMU hub-attached storage and HID device nodes bound"
    fi

    if [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
        if ! grep -Fq "fbxl QEMU virtio sound device directory follows" "$QEMU_LOG"; then
            die "QEMU virtio sound device directory was not listed"
        fi

        if ! grep -Eq "(^|[^A-Za-z0-9_])pcm0p([^A-Za-z0-9_]|$)" "$QEMU_LOG"; then
            die "QEMU virtio sound did not expose /dev/audio/pcm0p"
        fi

        echo "NuttX audio: QEMU virtio sound registered /dev/audio/pcm0p"
    fi

    if [ "$NETWORK_SHELL" -eq 1 ]; then
        if [ "$QEMU_STATUS" -ne 0 ] && [ "$QEMU_STATUS" -ne 124 ]; then
            die "QEMU failed with status $QEMU_STATUS"
        fi

        if ! grep -Fq "fbxl NuttX network shell ready" "$QEMU_LOG"; then
            die "NuttX network shell readiness marker was not observed"
        fi

        echo "NuttX telnet: telnet <docker-host> $HOST_TELNET_PORT"
        echo "NuttX FTP:    ftp -p <docker-host> $HOST_FTP_PORT"
        exit 0
    fi

    if [ "$EXPECT_FAIL" -eq 1 ]; then
        if grep -Fq "fb-nuttx-status=0" "$QEMU_LOG"; then
            die "FreeBASIC NuttX program reported success, but failure was expected"
        fi

        if ! grep -Eq "fb-nuttx-status=-?[1-9][0-9]*" "$QEMU_LOG"; then
            die "FreeBASIC NuttX program did not report an expected non-zero status"
        fi
    else
        if [ "$APP_NAME" = "fbsfx" ] && [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
            :
        elif ! grep -Fq "fb-nuttx-status=0" "$QEMU_LOG"; then
            die "FreeBASIC NuttX program did not report a zero exit status"
        fi
    fi

    if [ "$APP_NAME" = "fbhello" ]; then
    if ! grep -Fq "fbhello: FreeBASIC is running on NuttX/RISC-V" "$QEMU_LOG"; then
        die "FreeBASIC NuttX output was not observed"
    fi

    if ! grep -Fq "sum 1..5 =15" "$QEMU_LOG"; then
        die "FreeBASIC integer output was not observed"
    fi

    if ! grep -Fq "hex total = F" "$QEMU_LOG"; then
        die "FreeBASIC HEX output was not observed"
    fi

    if ! grep -Fq "double sample =3.5" "$QEMU_LOG"; then
        die "FreeBASIC double output was not observed"
    fi

    if ! grep -Fq "sin sample =1" "$QEMU_LOG"; then
        die "FreeBASIC SIN output was not observed"
    fi

    if ! grep -Fq "cos sample =1" "$QEMU_LOG"; then
        die "FreeBASIC COS output was not observed"
    fi

    if ! grep -Fq "sqr sample =1" "$QEMU_LOG"; then
        die "FreeBASIC SQR output was not observed"
    fi

    if ! grep -Fq "int sample =1" "$QEMU_LOG"; then
        die "FreeBASIC INT output was not observed"
    fi

    if ! grep -Fq "fix sample =1" "$QEMU_LOG"; then
        die "FreeBASIC FIX output was not observed"
    fi

    if ! grep -Fq "rnd range =1" "$QEMU_LOG"; then
        die "FreeBASIC RND output was not observed"
    fi

    if ! grep -Fq "byte sample =-7" "$QEMU_LOG"; then
        die "FreeBASIC BYTE print output was not observed"
    fi

    if ! grep -Fq "ubyte sample =250" "$QEMU_LOG"; then
        die "FreeBASIC UBYTE print output was not observed"
    fi

    if ! grep -Fq "short sample =-1234" "$QEMU_LOG"; then
        die "FreeBASIC SHORT print output was not observed"
    fi

    if ! grep -Fq "ushort sample =54321" "$QEMU_LOG"; then
        die "FreeBASIC USHORT print output was not observed"
    fi

    if ! grep -Fq "longint sample =1234567890123" "$QEMU_LOG"; then
        die "FreeBASIC LONGINT print output was not observed"
    fi

    if ! grep -Fq "ulongint sample =1234567890123" "$QEMU_LOG"; then
        die "FreeBASIC ULONGINT print output was not observed"
    fi

    if ! grep -Fq "bool sample =true" "$QEMU_LOG"; then
        die "FreeBASIC BOOLEAN print output was not observed"
    fi

    if ! grep -Fq "array total ok" "$QEMU_LOG"; then
        die "FreeBASIC static array output was not observed"
    fi

    if ! grep -Fq "select ok" "$QEMU_LOG"; then
        die "FreeBASIC SELECT CASE output was not observed"
    fi

    if ! grep -Fq "data read sum =30" "$QEMU_LOG"; then
        die "FreeBASIC DATA/READ output was not observed"
    fi

    if ! grep -Fq "data restore sum =30" "$QEMU_LOG"; then
        die "FreeBASIC RESTORE output was not observed"
    fi

    if ! grep -Fq "data string sample = stored text" "$QEMU_LOG"; then
        die "FreeBASIC string DATA/READ output was not observed"
    fi

    if ! grep -Fq "data double sample =6.25" "$QEMU_LOG"; then
        die "FreeBASIC double DATA/READ output was not observed"
    fi

    if ! grep -Fq "data longint sample =1234567890123" "$QEMU_LOG"; then
        die "FreeBASIC longint DATA/READ output was not observed"
    fi

    if ! grep -Fq "abs sample =42" "$QEMU_LOG"; then
        die "FreeBASIC ABS output was not observed"
    fi

    if ! grep -Fq "len sample =5" "$QEMU_LOG"; then
        die "FreeBASIC LEN output was not observed"
    fi

    if ! grep -Fq "concat sample = nuttx" "$QEMU_LOG"; then
        die "FreeBASIC string concat output was not observed"
    fi

    if ! grep -Fq "left sample = nut" "$QEMU_LOG"; then
        die "FreeBASIC LEFT output was not observed"
    fi

    if ! grep -Fq "mid sample = utt" "$QEMU_LOG"; then
        die "FreeBASIC MID output was not observed"
    fi

    if ! grep -Fq "right sample = tx" "$QEMU_LOG"; then
        die "FreeBASIC RIGHT output was not observed"
    fi

    if ! grep -Fq "instr sample =3" "$QEMU_LOG"; then
        die "FreeBASIC INSTR output was not observed"
    fi

    if ! grep -Fq "instrrev sample =4" "$QEMU_LOG"; then
        die "FreeBASIC INSTRREV output was not observed"
    fi

    if ! grep -Fq "ucase sample = NUTTX" "$QEMU_LOG"; then
        die "FreeBASIC UCASE output was not observed"
    fi

    if ! grep -Fq "lcase sample = nuttx" "$QEMU_LOG"; then
        die "FreeBASIC LCASE output was not observed"
    fi

    if ! grep -Fq "trim sample = pad" "$QEMU_LOG"; then
        die "FreeBASIC TRIM output was not observed"
    fi

    if ! grep -Fq "val sample =45.5" "$QEMU_LOG"; then
        die "FreeBASIC VAL output was not observed"
    fi

    if ! grep -Fq "valint sample =42" "$QEMU_LOG"; then
        die "FreeBASIC VALINT output was not observed"
    fi

    if ! grep -Fq "valuint sample =42" "$QEMU_LOG"; then
        die "FreeBASIC VALUINT output was not observed"
    fi

    if ! grep -Fq "vallng sample =1234567890123" "$QEMU_LOG"; then
        die "FreeBASIC VALLNG output was not observed"
    fi

    if ! grep -Fq "valulng sample =1234567890123" "$QEMU_LOG"; then
        die "FreeBASIC VALULNG output was not observed"
    fi

    if ! grep -Fq "str sample = 123" "$QEMU_LOG"; then
        die "FreeBASIC STR output was not observed"
    fi

    if ! grep -Fq "chr sample = A" "$QEMU_LOG"; then
        die "FreeBASIC CHR output was not observed"
    fi

    if ! grep -Fq "asc sample =65" "$QEMU_LOG"; then
        die "FreeBASIC ASC output was not observed"
    fi

    if ! grep -Fq "space len =3" "$QEMU_LOG"; then
        die "FreeBASIC SPACE output was not observed"
    fi

    if ! grep -Fq "string sample = ***" "$QEMU_LOG"; then
        die "FreeBASIC STRING output was not observed"
    fi

    if ! grep -Fq "tab sample below" "$QEMU_LOG"; then
        die "FreeBASIC TAB marker was not observed"
    fi

    if ! grep -Fq "spc sample below" "$QEMU_LOG"; then
        die "FreeBASIC SPC marker was not observed"
    fi

    if ! grep -Fq "C   D" "$QEMU_LOG"; then
        die "FreeBASIC SPC output was not observed"
    fi

    if ! grep -Fq "using double below" "$QEMU_LOG"; then
        die "FreeBASIC PRINT USING double marker was not observed"
    fi

    if ! grep -Fq " 12.50" "$QEMU_LOG"; then
        die "FreeBASIC PRINT USING double output was not observed"
    fi

    if ! grep -Fq "using longint below" "$QEMU_LOG"; then
        die "FreeBASIC PRINT USING longint marker was not observed"
    fi

    if ! grep -Fq "   42" "$QEMU_LOG"; then
        die "FreeBASIC PRINT USING longint output was not observed"
    fi

    if ! grep -Fq "using string below" "$QEMU_LOG"; then
        die "FreeBASIC PRINT USING string marker was not observed"
    fi

    if ! grep -Fq "fixed len =5" "$QEMU_LOG"; then
        die "FreeBASIC fixed-length string length output was not observed"
    fi

    if ! grep -Fq "fixed asc4 =32" "$QEMU_LOG"; then
        die "FreeBASIC fixed-length string padding output was not observed"
    fi

    if ! grep -Fq "lset sample = [ab    ]" "$QEMU_LOG"; then
        die "FreeBASIC LSET output was not observed"
    fi

    if ! grep -Fq "rset sample = [    cd]" "$QEMU_LOG"; then
        die "FreeBASIC RSET output was not observed"
    fi

    if ! grep -Fq "bin sample = 1010" "$QEMU_LOG"; then
        die "FreeBASIC BIN output was not observed"
    fi

    if ! grep -Fq "oct sample = 12" "$QEMU_LOG"; then
        die "FreeBASIC OCT output was not observed"
    fi

    if ! grep -Fq "hex byte sample = FF" "$QEMU_LOG"; then
        die "FreeBASIC byte HEX output was not observed"
    fi

    if ! grep -Fq "mki cvi sample =1" "$QEMU_LOG"; then
        die "FreeBASIC MKI/CVI output was not observed"
    fi

    if ! grep -Fq "mkl cvl sample =1" "$QEMU_LOG"; then
        die "FreeBASIC MKL/CVL output was not observed"
    fi

    if ! grep -Fq "mks cvs sample =1" "$QEMU_LOG"; then
        die "FreeBASIC MKS/CVS output was not observed"
    fi

    if ! grep -Fq "mkd cvd sample =1" "$QEMU_LOG"; then
        die "FreeBASIC MKD/CVD output was not observed"
    fi

    if ! grep -Fq "mid assign sample = aXYZef" "$QEMU_LOG"; then
        die "FreeBASIC MID assignment output was not observed"
    fi

    if ! grep -Fq "ltrim sample = left" "$QEMU_LOG"; then
        die "FreeBASIC LTRIM output was not observed"
    fi

    if ! grep -Fq "rtrim sample = right" "$QEMU_LOG"; then
        die "FreeBASIC RTRIM output was not observed"
    fi

    if ! grep -Fq "command zero sample =1" "$QEMU_LOG"; then
        die "FreeBASIC COMMAND(0) output was not observed"
    fi

    if ! grep -Fq "command arg sample = fbarg" "$QEMU_LOG"; then
        die "FreeBASIC COMMAND(1) output was not observed"
    fi

    if ! grep -Fq "command all sample = fbarg" "$QEMU_LOG"; then
        die "FreeBASIC COMMAND output was not observed"
    fi

    if ! grep -Fq "exepath sample =1" "$QEMU_LOG"; then
        die "FreeBASIC EXEPATH output was not observed"
    fi

    if ! grep -Fq "env missing len =0" "$QEMU_LOG"; then
        die "FreeBASIC ENVIRON output was not observed"
    fi

    if ! grep -Fq "env set sample = ok" "$QEMU_LOG"; then
        die "FreeBASIC ENVIRON assignment output was not observed"
    fi

    if ! grep -Fq "err initial =0" "$QEMU_LOG"; then
        die "FreeBASIC ERR initial output was not observed"
    fi

    if ! grep -Fq "fb-shell-ok" "$QEMU_LOG"; then
        die "FreeBASIC SHELL command output was not observed"
    fi

    if ! grep -Fq "shell sample =1" "$QEMU_LOG"; then
        die "FreeBASIC SHELL return code output was not observed"
    fi

    if ! grep -Fq "pos sample =4" "$QEMU_LOG"; then
        die "FreeBASIC POS output was not observed"
    fi

    if ! grep -Fq "csrlin sample =3" "$QEMU_LOG"; then
        die "FreeBASIC CSRLIN output was not observed"
    fi

    if ! grep -Fq "console control ok" "$QEMU_LOG"; then
        die "FreeBASIC console control output was not observed"
    fi

    if ! grep -Fq "inkey len =0" "$QEMU_LOG"; then
        die "FreeBASIC INKEY output was not observed"
    fi

    if ! grep -Fq "line input sample = nuttx input line" "$QEMU_LOG"; then
        die "FreeBASIC LINE INPUT output was not observed"
    fi

    if ! grep -Fq "input int sample =321" "$QEMU_LOG"; then
        die "FreeBASIC INPUT integer output was not observed"
    fi

    if ! grep -Fq "input string sample = input string value" "$QEMU_LOG"; then
        die "FreeBASIC INPUT string output was not observed"
    fi

    if ! grep -Fq "input double sample =6.25" "$QEMU_LOG"; then
        die "FreeBASIC INPUT double output was not observed"
    fi

    if ! grep -Fq "input longint sample =1234567890123" "$QEMU_LOG"; then
        die "FreeBASIC INPUT longint output was not observed"
    fi

    if ! grep -Fq "rnd range =1" "$QEMU_LOG"; then
        die "FreeBASIC RND output was not observed"
    fi
    if ! grep -Fq "compare eq =1" "$QEMU_LOG"; then
        die "FreeBASIC string equality output was not observed"
    fi

    if ! grep -Fq "compare ne =1" "$QEMU_LOG"; then
        die "FreeBASIC string inequality output was not observed"
    fi

    if ! grep -Fq "compare lt =1" "$QEMU_LOG"; then
        die "FreeBASIC string ordering output was not observed"
    fi

    if ! grep -Fq "redim total =30" "$QEMU_LOG"; then
        die "FreeBASIC REDIM array output was not observed"
    fi

    if ! grep -Fq "erase sample ok" "$QEMU_LOG"; then
        die "FreeBASIC ERASE array output was not observed"
    fi

    if ! grep -Fq "redim2 bounds ok" "$QEMU_LOG"; then
        die "FreeBASIC two-dimensional REDIM bounds output was not observed"
    fi

    if ! grep -Fq "redim2 total =18" "$QEMU_LOG"; then
        die "FreeBASIC two-dimensional REDIM data output was not observed"
    fi

    if ! grep -Fq "timer sample =1" "$QEMU_LOG"; then
        die "FreeBASIC TIMER/SLEEP output was not observed"
    fi

    if ! grep -Fq "date sample =1" "$QEMU_LOG"; then
        die "FreeBASIC DATE output was not observed"
    fi

    if ! grep -Fq "time sample =1" "$QEMU_LOG"; then
        die "FreeBASIC TIME output was not observed"
    fi

    if ! grep -Fq "file eof start =0" "$QEMU_LOG"; then
        die "FreeBASIC EOF start output was not observed"
    fi

    if ! grep -Fq "file size =10" "$QEMU_LOG"; then
        die "FreeBASIC LOF output was not observed"
    fi

    if ! grep -Fq "file loc start =1" "$QEMU_LOG"; then
        die "FreeBASIC LOC output was not observed"
    fi

    if ! grep -Fq "file seek =1" "$QEMU_LOG"; then
        die "FreeBASIC SEEK output was not observed"
    fi

    if ! grep -Fq "file eof end =1" "$QEMU_LOG"; then
        die "FreeBASIC EOF end output was not observed"
    fi

    if ! grep -Fq "file sample = file line" "$QEMU_LOG"; then
        die "FreeBASIC file I/O output was not observed"
    fi

    if ! grep -Fq "append sample = one:two" "$QEMU_LOG"; then
        die "FreeBASIC APPEND output was not observed"
    fi

    if ! grep -Fq "reset sample = reset ok" "$QEMU_LOG"; then
        die "FreeBASIC RESET output was not observed"
    fi

    if ! grep -Fq "lock sample ok" "$QEMU_LOG"; then
        die "FreeBASIC LOCK/UNLOCK output was not observed"
    fi

    if ! grep -Fq "input dollar sample = abc" "$QEMU_LOG"; then
        die "FreeBASIC INPUT$ output was not observed"
    fi

    if ! grep -Fq "file input string = from file" "$QEMU_LOG"; then
        die "FreeBASIC file INPUT string output was not observed"
    fi

    if ! grep -Fq "file input int =456" "$QEMU_LOG"; then
        die "FreeBASIC file INPUT integer output was not observed"
    fi

    if ! grep -Fq "file input double =7.5" "$QEMU_LOG"; then
        die "FreeBASIC file INPUT double output was not observed"
    fi

    if ! grep -Fq 'write sample = "abc",123,4.5' "$QEMU_LOG"; then
        die "FreeBASIC WRITE output was not observed"
    fi

    if ! grep -Fq "write input string = abc" "$QEMU_LOG"; then
        die "FreeBASIC INPUT from WRITE string output was not observed"
    fi

    if ! grep -Fq "write input int =123" "$QEMU_LOG"; then
        die "FreeBASIC INPUT from WRITE integer output was not observed"
    fi

    if ! grep -Fq "write input double =4.5" "$QEMU_LOG"; then
        die "FreeBASIC INPUT from WRITE double output was not observed"
    fi

    if ! grep -Fq 'write types sample = -1,2,-3,4,5,-6,7,1.25,true' "$QEMU_LOG"; then
        die "FreeBASIC typed WRITE output was not observed"
    fi

    if ! grep -Fq "write input types ok =1" "$QEMU_LOG"; then
        die "FreeBASIC typed INPUT from WRITE output was not observed"
    fi

    if ! grep -Fq "binary sample =1" "$QEMU_LOG"; then
        die "FreeBASIC binary GET/PUT output was not observed"
    fi

    if ! grep -Fq "random file sample =1" "$QEMU_LOG"; then
        die "FreeBASIC random file GET/PUT output was not observed"
    fi

    if ! grep -Fq "random string sample = nuttx" "$QEMU_LOG"; then
        die "FreeBASIC random string GET/PUT output was not observed"
    fi

    if ! grep -Fq "binary string sample = xyz" "$QEMU_LOG"; then
        die "FreeBASIC binary string GET/PUT output was not observed"
    fi

    if ! grep -Fq "curdir sample =1" "$QEMU_LOG"; then
        die "FreeBASIC CURDIR/CHDIR/MKDIR output was not observed"
    fi

    if ! grep -Fq "kill sample ok" "$QEMU_LOG"; then
        die "FreeBASIC KILL output was not observed"
    fi

    if ! grep -Fq "rename sample ok" "$QEMU_LOG"; then
        die "FreeBASIC NAME AS output was not observed"
    fi

    if ! grep -Fq "rmdir sample ok" "$QEMU_LOG"; then
        die "FreeBASIC RMDIR output was not observed"
    fi

    if ! grep -Fq "dir sample =1" "$QEMU_LOG"; then
        die "FreeBASIC DIR output was not observed"
    fi

    if ! grep -Fq "freefile sample =1" "$QEMU_LOG"; then
        die "FreeBASIC FREEFILE output was not observed"
    fi

    if ! grep -Fq "heap sum =15" "$QEMU_LOG"; then
        die "FreeBASIC ALLOCATE output was not observed"
    fi

    if ! grep -Fq "heap resize sum =24" "$QEMU_LOG"; then
        die "FreeBASIC REALLOCATE output was not observed"
    fi

    if ! grep -Fq "callocate zero =0" "$QEMU_LOG"; then
        die "FreeBASIC CALLOCATE output was not observed"
    fi
    fi

    if [ "$WITH_GFXLIB" -eq 1 ] && [ "$APP_NAME" = "fbgfx" ]; then
    if ! grep -Fq "NUTTX_GFX_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX gfx smoke output was not observed"
    fi
    if [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Fq "FB_NUTTX_QEMU_GFX_PRESENT" "$QEMU_LOG"; then
        die "FreeBASIC NuttX QEMU gfx present trace was not observed"
    fi
    if [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       { ! grep -Fq "FB_NUTTX_QEMU_HDMI_CONSOLE" "$QEMU_LOG" ||
         ! grep -Fq 'text="gfx print ok"' "$QEMU_LOG"; }; then
        die "FreeBASIC NuttX QEMU HDMI console signature was not observed"
    fi
    if [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Eq "FB_NUTTX_QEMU_DVI_SCANOUT .*sample_x=10 index=4 .*nonblack=[1-9][0-9]*" "$QEMU_LOG"; then
        die "FreeBASIC NuttX QEMU DVI scanout signature was not observed"
    fi
    if [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Eq "FB_NUTTX_QEMU_GFX_VISUAL .*p10_10=4" "$QEMU_LOG"; then
        die "FreeBASIC NuttX QEMU gfx visual signature was not observed"
    fi
    if [ "$QEMU_USB_HUB_DEVICES" -eq 1 ]; then
        if ! grep -Fq "FB_NUTTX_QEMU_HID_KEYBOARD_OPEN /dev/kbda" "$QEMU_LOG"; then
            die "FreeBASIC NuttX gfx driver did not open the USB keyboard device"
        fi
        if ! grep -Fq "FB_NUTTX_QEMU_HID_MOUSE_OPEN /dev/mouse0" "$QEMU_LOG"; then
            die "FreeBASIC NuttX gfx driver did not open the USB mouse device"
        fi
    fi
    if [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
        echo "NuttX USB HID: QEMU monitor injected keyboard event reached gfxlib"
    fi
    fi

    if [ "$WITH_GFXLIB" -eq 1 ] && [ "$APP_NAME" = "fbgfxhid" ]; then
    if ! grep -Fq "FB_NUTTX_GFX_HID_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX gfx HID smoke output was not observed"
    fi
    if ! grep -Fq "FB_NUTTX_QEMU_HID_KEYBOARD_OPEN /dev/kbda" "$QEMU_LOG"; then
        die "FreeBASIC NuttX gfx HID smoke did not open the USB keyboard device"
    fi
    if ! grep -Fq "FB_NUTTX_QEMU_HID_MOUSE_OPEN /dev/mouse0" "$QEMU_LOG"; then
        die "FreeBASIC NuttX gfx HID smoke did not open the USB mouse device"
    fi
    if [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
        echo "NuttX USB HID: QEMU monitor injected keyboard and mouse events reached gfxlib"
    fi
    fi

    if [ "$WITH_GFXLIB" -eq 1 ] && [ "$APP_NAME" = "fbcombo" ]; then
    if ! grep -Fq "FB_NUTTX_DEVICE_COMBO_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX combined device smoke output was not observed"
    fi
    if ! grep -Fq "FB_NUTTX_QEMU_HID_KEYBOARD_OPEN /dev/kbda" "$QEMU_LOG"; then
        die "FreeBASIC NuttX combined device smoke did not open the USB keyboard device"
    fi
    if ! grep -Fq "FB_NUTTX_QEMU_HID_MOUSE_OPEN /dev/mouse0" "$QEMU_LOG"; then
        die "FreeBASIC NuttX combined device smoke did not open the USB mouse device"
    fi
    if ! grep -Fq "fbxl QEMU usb-storage ready: /mnt/sd0" "$QEMU_LOG"; then
        die "FreeBASIC NuttX combined device smoke did not mount USB storage"
    fi
    if [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Fq "FB_NUTTX_QEMU_GFX_VISUAL" "$QEMU_LOG"; then
        die "FreeBASIC NuttX combined device smoke did not produce a gfx visual signature"
    fi
    if [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Fq "FB_NUTTX_QEMU_DVI_SCANOUT" "$QEMU_LOG"; then
        die "FreeBASIC NuttX combined device smoke did not produce a DVI scanout signature"
    fi
    if [ "$QEMU_INJECT_HID_EVENTS" -eq 1 ]; then
        echo "NuttX USB HID: QEMU monitor injected keyboard and mouse events reached gfxlib"
    fi
    fi

    if [ "$APP_NAME" = "fbsfx" ]; then
    if ! grep -Fq "FB_NUTTX_SFX_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX sfx smoke output was not observed"
    fi
    if [ "$QEMU_VIRTIO_SOUND" -eq 1 ]; then
    if ! grep -Fq "FB_NUTTX_QEMU_SFX_AUDIO_OPEN" "$QEMU_LOG"; then
        die "FreeBASIC NuttX sfx driver did not open the NuttX audio device"
    fi
    if ! grep -Eq "FB_NUTTX_QEMU_SFX_AUDIO_ENQUEUE .*frames=[1-9][0-9]* .*bytes=[1-9][0-9]*" "$QEMU_LOG"; then
        die "FreeBASIC NuttX sfx driver did not enqueue PCM audio"
    fi
    elif [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Fq "FB_NUTTX_QEMU_SFX_WRITE" "$QEMU_LOG"; then
        die "FreeBASIC NuttX QEMU sfx write trace was not observed"
    fi
    if [ "$QEMU_VIRTIO_SOUND" -eq 0 ] &&
       [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Eq "FB_NUTTX_QEMU_SFX_WRITE .*samples=[1-9][0-9]* .*nonzero=[1-9][0-9]*" "$QEMU_LOG"; then
        die "FreeBASIC NuttX QEMU sfx trace did not contain non-silent samples"
    fi
    if [ "$QEMU_VIRTIO_SOUND" -eq 0 ] &&
       [ "$QEMU_MOCK_DEVICES" -eq 1 ] &&
       ! grep -Eq "FB_NUTTX_QEMU_SFX_PCM .*rate=44100 .*channels=2 .*bits=16 .*active=[1-9][0-9]* .*peak=[1-9][0-9]*" "$QEMU_LOG"; then
        die "FreeBASIC NuttX QEMU sfx PCM trace did not contain active signed 16-bit samples"
    fi
    fi

    if [ "$APP_NAME" = "fbdvisolid" ]; then
    if ! grep -Fq "FB_NUTTX_QEMU_DVI_SOLID_MODEL_OK" "$QEMU_LOG"; then
        die "NuttX QEMU solid DVI model smoke output was not observed"
    fi
    if ! grep -Fq "fbdvi: QEMU solid DVI model ok htotal=800 vtotal=525 active=640x480" "$QEMU_LOG"; then
        die "NuttX QEMU solid DVI timing model output was not observed"
    fi
    if ! grep -Fq "fbdvi: QEMU solid DVI model tmds=0007fd00,0007fd00,000bfa01" "$QEMU_LOG"; then
        die "NuttX QEMU solid DVI TMDS model output was not observed"
    fi
    fi

    if [ "$APP_NAME" = "fbfre" ]; then
    if ! grep -Fq "FB_NUTTX_FRE_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX FRE smoke output was not observed"
    fi
    if ! grep -Fq "fre initial nonzero =1" "$QEMU_LOG"; then
        die "FreeBASIC NuttX FRE did not report nonzero heap availability"
    fi
    if ! grep -Fq "fre after alloc lower =1" "$QEMU_LOG"; then
        die "FreeBASIC NuttX FRE did not decrease after heap allocation"
    fi
    if ! grep -Fq "fre after free sane =1" "$QEMU_LOG"; then
        die "FreeBASIC NuttX FRE did not remain sane after heap release"
    fi
    fi

    if [ "$QEMU_STATUS" -ne 0 ] && [ "$QEMU_STATUS" -ne 124 ]; then
        die "QEMU failed with status $QEMU_STATUS"
    fi
fi

if [ "$APP_NAME" = "fbgosub" ]; then
    if ! grep -Fq "FB_NUTTX_GOSUB_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX GOSUB smoke output was not observed"
    fi
fi

if [ "$APP_NAME" = "fbmathcvn" ]; then
    if ! grep -Fq "FB_NUTTX_MATH_CVN_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX math CVN smoke output was not observed"
    fi
fi

if [ "$APP_NAME" = "fbsign" ]; then
    if ! grep -Fq "FB_NUTTX_SIGN_SMOKE_OK" "$QEMU_LOG"; then
        die "FreeBASIC NuttX SGN smoke output was not observed"
    fi
fi

echo "NUTTX_RISCV32_FB_SMOKE_OK"

# end of nuttx-riscv32-qemu-smoke.sh
