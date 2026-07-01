#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX/RP2350-PiZero hardware image
# -----------------------------------------------------
#
# File: nuttx-rp2350-pizero-image.sh
#
# Purpose:
#
#     Build a NuttX UF2 image for the Waveshare RP2350-PiZero that carries
#     small generated-FreeBASIC programs and the first board services needed
#     for USB console and MicroSD testing.
#
# Responsibilities:
#
#     - apply the small RP2350B NuttX GPIO/Kconfig patch needed by this board
#     - configure the Pico 2 RISC-V USB NSH image for the RP2350B package
#     - enable SPI1 MicroSD support on the RP2350-PiZero pins
#     - stage a first low-memory DVI signal smoke test for the board connector
#     - stage small FreeBASIC console, SD, gfxlib, and sfxlib smoke programs
#     - add and configure the first RP23xx USB host/hub controller path
#     - copy the resulting UF2 to a predictable build directory
#
# This file intentionally does NOT contain:
#
#     - a full NuttX board port for the RP2350-PiZero
#     - a full DVI/HSTX scanout driver
#     - PIO-USB host/device support for the second Type-C port
#     - hardware flashing logic
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/build_scripts" ] &&
       { [ -f "$SEARCH_DIR/GNUmakefile" ] ||
         [ -f "$SEARCH_DIR/makefile" ] ||
         [ -f "$SEARCH_DIR/Makefile" ] ||
         [ -f "$SEARCH_DIR/.nuttx-sdk-root" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi

    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root" >&2; exit 1; }

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }

find_default_fbc() {
    if [ -x "$ROOT/bin/fbc" ]; then
        printf '%s\n' "$ROOT/bin/fbc"
        return 0
    fi

    if [ -x "$ROOT/bin/fbc.exe" ]; then
        printf '%s\n' "$ROOT/bin/fbc.exe"
        return 0
    fi

    if command -v fbc >/dev/null 2>&1; then
        command -v fbc
        return 0
    fi

    return 1
}

usage() {
    cat <<EOF
Usage: ./build_scripts/nuttx-rp2350-pizero-image.sh [options]

Options:
  --nuttx-workdir DIR   Directory containing nuttx/ and apps/
  --fbc FILE            fbc binary to use, default: ./bin/fbc
  --generated-c-dir DIR Directory containing pre-generated C files. When set,
                        fbc is not invoked inside this script.
  --out-dir DIR         Output directory, default: build/nuttx-rp2350-pizero
  --skip-nuttx-patch    Do not apply the bundled NuttX RP2350B patches
  --with-i2s-audio      Enable the RP23xx I2S PCM device on configured pins
  --i2s-data-gpio N     I2S DATA pin used with --with-i2s-audio, default: 9
  --i2s-clock-gpio N    I2S clock base pin used with --with-i2s-audio,
                        default: 10
  --with-usb-host-hub   Enable USB host hub/HID/MSC class support. This
                        requires an RP23xx USB host controller driver in the
                        selected NuttX tree.
  --with-usb-ethernet   Enable USB CDC-ECM/MBIM Ethernet class support and
                        the small IPv4/TCP/DHCP network stack needed to use
                        it. This implies --with-usb-host-hub because the same
                        RP23xx host controller driver is required.
  --build-only-no-uf2   Build and collect the ELF/raw binary without running
                        the RP23xx UF2 post-build step. This is for CI and
                        emulator-side compile gates where picotool is absent.
  --without-dvi-smoke   Do not stage the first solid-field DVI smoke app
  --autorun-gfx-demo    Boot directly into the persistent SCREEN 13 demo.
                        This replaces NSH and must be paired with
                        --allow-blind-autorun.
  --autorun-dvi-smoke   Boot directly into the solid-field DVI smoke app.
                        This replaces NSH and must be paired with
                        --allow-blind-autorun.
  --allow-blind-autorun Allow an autorun image to replace the USB NSH entry.
                        Use only after a normal NSH image has proven the
                        console and recovery path on this board.
  --help                Show this help text

Environment:
  NUTTX_WORKDIR         Same as --nuttx-workdir
  FBC                   Same as --fbc
  FB_NUTTX_GENERATED_C_DIR
                        Same as --generated-c-dir
  JOBS                  Parallel make jobs

The generated image keeps the built-in USB NSH console and adds these BASIC
programs:

  fbgfx      software gfxlib2 framebuffer smoke test
  fbgfxdemo  persistent SCREEN 13 DVI demo
  fbhello    console hello world
  fbcount    second-program smoke test
  fbstrings  string runtime smoke test
  fbsd       MicroSD write/read/delete smoke test
  bootsel    reboot into RP2350 BOOTSEL USB programming mode
  fbdvi      solid DVI signal smoke test for the RP2350-PiZero connector
  fbdviregs  DVI GPIO/PIO/PWM/DMA register snapshot
  fbsfx      sfxlib SOUND command smoke test
EOF
}

apply_nuttx_patch() {
    local patch_file=$1
    local patch_log

    patch_log="$(mktemp)"

    if patch --forward --dry-run -p0 < "$patch_file" >"$patch_log" 2>&1; then
        cat "$patch_log"
        run patch --forward -p0 < "$patch_file"
    elif patch --reverse --dry-run -p0 < "$patch_file" >"$patch_log" 2>&1; then
        echo "NuttX patch is already applied: $(basename "$patch_file")"
    elif nuttx_patch_markers_present "$patch_file"; then
        echo "NuttX patch markers are already present: $(basename "$patch_file")"
    else
        cat "$patch_log" >&2
        rm -f "$patch_log"
        die "could not apply NuttX patch: $patch_file"
    fi

    rm -f "$patch_log"
}

nuttx_patch_markers_present() {
    local patch_file=$1

    case "$(basename "$patch_file")" in
        rp23xx-rv-pizero-dvi-clock.patch)
            grep -q 'config RP23XX_RV_PIZERO_DVI_CLOCK' \
                arch/risc-v/src/rp23xx-rv/Kconfig &&
            grep -q 'RP23XX_PLL_USB_VCO_FREQ' \
                arch/risc-v/src/rp23xx-rv/rp23xx_clock.c &&
            grep -q 'BOARD_HSTX_FREQ' \
                boards/risc-v/rp23xx-rv/raspberrypi-pico-2-rv/include/board.h
            ;;

        rp23xx-rv-usbhost-controller.patch)
            [ -f arch/risc-v/src/rp23xx-rv/rp23xx_usbhost.c ] &&
            grep -q 'config RP23XX_RV_USBHOST' \
                arch/risc-v/src/rp23xx-rv/Kconfig &&
            grep -q 'rp23xx_usbhost.c' arch/risc-v/src/rp23xx-rv/Make.defs
            ;;

        *)
            return 1
            ;;
    esac
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
    config_enabled "$1" ||
        die "$1 did not remain enabled after olddefconfig"
}

nuttx_has_rp23xx_usbhost_controller() {
    [ -f "$NUTTX_DIR/arch/risc-v/src/rp23xx-rv/rp23xx_usbhost.c" ] ||
    grep -Rqs '^config RP23XX_RV_.*USB.*HOST' \
        "$NUTTX_DIR/arch/risc-v/src/rp23xx-rv" \
        "$NUTTX_DIR/boards/risc-v/rp23xx-rv"
}

remove_stale_apps_kconfigs() {
    local kconfig_file

    [ -d "$APPS_DIR" ] || return 0

    while IFS= read -r kconfig_file; do
        [ -n "$kconfig_file" ] || continue

        if grep -q '^# This file is autogenerated, do not edit\.$' \
            "$kconfig_file"; then
            echo "Removing stale NuttX generated Kconfig: $kconfig_file"
            rm -f "$kconfig_file"
        else
            die "stale absolute app Kconfig source in non-generated file: $kconfig_file"
        fi
    done < <(
        grep -RIlE 'source "/.*/apps/' "$APPS_DIR" --include=Kconfig 2>/dev/null |
        while IFS= read -r kconfig_file; do
            if ! grep -Fq "source \"$APPS_DIR/" "$kconfig_file"; then
                printf '%s\n' "$kconfig_file"
            fi
        done
    )
}

report_nuttx_memory_budget() {
    local elf_file="$NUTTX_DIR/nuttx"
    local map_file="$NUTTX_DIR/nuttx.map"
    local ram_origin
    local ram_length
    local heap_start
    local heap_limit
    local static_ram
    local heap_bytes
    local screen13_bytes=64000
    local gfx_app_stack_bytes=32768
    local heap_after_screen13
    local heap_after_gfx_app

    [ -f "$elf_file" ] || return 0
    [ -f "$map_file" ] || return 0

    echo "==> NuttX memory budget"
    size "$elf_file" || true

    read -r ram_origin ram_length <<EOF
$(awk '$1 == "RAM" { print $2, $3; exit }' "$map_file")
EOF

    heap_start="$(awk '/end = __end__/ { print $1; exit }' "$map_file")"
    heap_limit="$(awk '/__HeapLimit/ { print $1; exit }' "$map_file")"

    if [ -n "$ram_origin" ] && [ -n "$ram_length" ] &&
       [ -n "$heap_start" ] && [ -n "$heap_limit" ]; then
        static_ram=$((heap_start - ram_origin))
        heap_bytes=$((heap_limit - heap_start))

        if [ "$heap_bytes" -gt "$screen13_bytes" ]; then
            heap_after_screen13=$((heap_bytes - screen13_bytes))
        else
            heap_after_screen13=0
        fi

        if [ "$heap_after_screen13" -gt "$gfx_app_stack_bytes" ]; then
            heap_after_gfx_app=$((heap_after_screen13 - gfx_app_stack_bytes))
        else
            heap_after_gfx_app=0
        fi

        printf 'RAM static image: %u bytes of %u bytes\n' \
            "$static_ram" "$((ram_length))"
        printf 'Heap before runtime stacks/apps: %u bytes\n' "$heap_bytes"
        printf 'Heap after one SCREEN 13 page: %u bytes\n' \
            "$heap_after_screen13"
        printf 'Heap after one SCREEN 13 page plus 32K app stack: %u bytes\n' \
            "$heap_after_gfx_app"
    fi
}

stage_fb_app() {
    local app_name=$1
    local bas_file=$2
    local with_gfx=$3
    local stack_size=${4:-16384}
    local with_minrt=${5:-1}
    local args=()

    args=(
        "$SMOKE_SCRIPT"
        --nuttx-workdir "$NUTTX_WORKDIR"
        --app-name "$app_name"
        --keep-existing-apps
        --skip-nuttx-config
        --no-run
        --local-generated-symbols
    )

    if [ -n "$GENERATED_C_DIR" ]; then
        local generated_c

        generated_c="$GENERATED_C_DIR/$(basename "$bas_file" .bas).c"
        [ -f "$generated_c" ] ||
            die "missing generated C for $bas_file: $generated_c"

        args+=(--generated-c "$generated_c")
    else
        args+=(--fbc "$FBC_BIN" --bas "$bas_file")
    fi

    if [ "$with_gfx" -eq 1 ]; then
        args+=(--with-gfxlib)
    fi

    if [ "$with_minrt" -eq 0 ]; then
        args+=(--without-minrt)
    fi

    (
        cd "$ROOT"
        run env APP_STACKSIZE="$stack_size" bash "${args[@]}"
    )
}

stage_c_app() {
    local app_name=$1
    local c_file=$2
    local app_title=${3:-"RP2350-PiZero DVI smoke test"}
    local app_help_1=${4:-"Start a minimal solid-field DVI signal on the Waveshare"}
    local app_help_2=${5:-"RP2350-PiZero connector."}
    local app_stack_size=${6:-4096}
    local app_symbol
    local app_dir
    local kconfig_line
    local kconfig_tmp

    case "$app_name" in
        *[!A-Za-z0-9_]*|'')
            die "app name must contain only letters, numbers, and underscores: $app_name"
            ;;
    esac

    [ -f "$c_file" ] || die "missing C app source: $c_file"

    app_symbol="$(printf '%s' "$app_name" | tr '[:lower:]' '[:upper:]')"
    app_dir="$NUTTX_WORKDIR/apps/examples/$app_name"
    kconfig_line="source \"$app_dir/Kconfig\""
    kconfig_tmp="$(mktemp)"

    rm -rf "$app_dir"
    mkdir -p "$app_dir"
    cp "$c_file" "$app_dir/${app_name}_main.c"

    cat > "$app_dir/Makefile" <<EOF
include \$(APPDIR)/Make.defs

PROGNAME  = \$(CONFIG_EXAMPLES_${app_symbol}_PROGNAME)
PRIORITY  = \$(CONFIG_EXAMPLES_${app_symbol}_PRIORITY)
STACKSIZE = \$(CONFIG_EXAMPLES_${app_symbol}_STACKSIZE)
MODULE    = \$(CONFIG_EXAMPLES_${app_symbol})

MAINSRC = ${app_name}_main.c

CFLAGS += -I\$(TOPDIR)/arch/risc-v/src/common
CFLAGS += -I\$(TOPDIR)/arch/risc-v/src/rp23xx-rv
CFLAGS += -Wno-unused-function

include \$(APPDIR)/Application.mk
EOF

    cat > "$app_dir/Make.defs" <<EOF
ifneq (\$(CONFIG_EXAMPLES_${app_symbol}),)
CONFIGURED_APPS += \$(APPDIR)/examples/$app_name
endif
EOF

    cat > "$app_dir/Kconfig" <<EOF
config EXAMPLES_${app_symbol}
	tristate "$app_title"
	default n
	---help---
		$app_help_1
		$app_help_2

if EXAMPLES_${app_symbol}

config EXAMPLES_${app_symbol}_PROGNAME
	string "Program name"
	default "$app_name"

config EXAMPLES_${app_symbol}_PRIORITY
	int "Application task priority"
	default 100

config EXAMPLES_${app_symbol}_STACKSIZE
	int "Application stack size"
	default $app_stack_size

endif
EOF

    awk -v line="$kconfig_line" '
        $0 == line { next }
        $0 == "endmenu # Examples" && inserted == 0 {
            print line
            inserted = 1
        }
        { print }
        END {
            if (inserted == 0) {
                print line
            }
        }
    ' "$NUTTX_WORKDIR/apps/examples/Kconfig" > "$kconfig_tmp"

    cp "$kconfig_tmp" "$NUTTX_WORKDIR/apps/examples/Kconfig"
    rm -f "$kconfig_tmp"

    kconfig_enable "CONFIG_EXAMPLES_${app_symbol}"
    kconfig_set_str "CONFIG_EXAMPLES_${app_symbol}_PROGNAME" "$app_name"
    kconfig_set_val "CONFIG_EXAMPLES_${app_symbol}_PRIORITY" 100
    kconfig_set_val "CONFIG_EXAMPLES_${app_symbol}_STACKSIZE" "$app_stack_size"
}

##############################################################################
# Options
##############################################################################

NUTTX_WORKDIR="${NUTTX_WORKDIR:-}"
FBC_BIN="${FBC:-}"
if [ -z "$FBC_BIN" ]; then
    FBC_BIN="$(find_default_fbc || true)"
fi
GENERATED_C_DIR="${FB_NUTTX_GENERATED_C_DIR:-}"
OUT_DIR="$ROOT/build/nuttx-rp2350-pizero"
APPLY_NUTTX_PATCH=1
WITH_I2S_AUDIO=0
I2S_DATA_GPIO=9
I2S_CLOCK_GPIO=10
WITH_USB_HOST_HUB=0
WITH_USB_ETHERNET=0
WITH_DVI_SMOKE=1
BUILD_ONLY_NO_UF2=0
AUTORUN_GFX_DEMO=0
AUTORUN_DVI_SMOKE=0
ALLOW_BLIND_AUTORUN=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --nuttx-workdir)
            [ "$#" -ge 2 ] || die "--nuttx-workdir requires a directory"
            NUTTX_WORKDIR="$2"
            shift 2
            ;;
        --fbc)
            [ "$#" -ge 2 ] || die "--fbc requires a file"
            FBC_BIN="$2"
            shift 2
            ;;
        --generated-c-dir)
            [ "$#" -ge 2 ] || die "--generated-c-dir requires a directory"
            GENERATED_C_DIR="$2"
            shift 2
            ;;
        --out-dir)
            [ "$#" -ge 2 ] || die "--out-dir requires a directory"
            OUT_DIR="$2"
            shift 2
            ;;
        --skip-nuttx-patch)
            APPLY_NUTTX_PATCH=0
            shift
            ;;
        --with-i2s-audio)
            WITH_I2S_AUDIO=1
            shift
            ;;
        --i2s-data-gpio)
            [ "$#" -ge 2 ] || die "--i2s-data-gpio requires a pin number"
            I2S_DATA_GPIO="$2"
            shift 2
            ;;
        --i2s-clock-gpio)
            [ "$#" -ge 2 ] || die "--i2s-clock-gpio requires a pin number"
            I2S_CLOCK_GPIO="$2"
            shift 2
            ;;
        --with-usb-host-hub)
            WITH_USB_HOST_HUB=1
            shift
            ;;
        --with-usb-ethernet)
            WITH_USB_HOST_HUB=1
            WITH_USB_ETHERNET=1
            shift
            ;;
        --build-only-no-uf2)
            BUILD_ONLY_NO_UF2=1
            shift
            ;;
        --without-dvi-smoke)
            WITH_DVI_SMOKE=0
            shift
            ;;
        --autorun-gfx-demo)
            AUTORUN_GFX_DEMO=1
            shift
            ;;
        --autorun-dvi-smoke)
            AUTORUN_DVI_SMOKE=1
            WITH_DVI_SMOKE=1
            shift
            ;;
        --allow-blind-autorun)
            ALLOW_BLIND_AUTORUN=1
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

[ -n "$NUTTX_WORKDIR" ] || die "set NUTTX_WORKDIR or pass --nuttx-workdir"
[ -d "$NUTTX_WORKDIR/nuttx" ] || die "missing NuttX tree: $NUTTX_WORKDIR/nuttx"
[ -d "$NUTTX_WORKDIR/apps" ] || die "missing NuttX apps tree: $NUTTX_WORKDIR/apps"

if [ -n "$GENERATED_C_DIR" ]; then
    [ -d "$GENERATED_C_DIR" ] ||
        die "missing generated C directory: $GENERATED_C_DIR"
else
    [ -x "$FBC_BIN" ] || die "fbc is not executable: $FBC_BIN"
fi

case "$I2S_DATA_GPIO" in *[!0-9]*|'') die "invalid --i2s-data-gpio: $I2S_DATA_GPIO" ;; esac
case "$I2S_CLOCK_GPIO" in *[!0-9]*|'') die "invalid --i2s-clock-gpio: $I2S_CLOCK_GPIO" ;; esac

if [ "$AUTORUN_GFX_DEMO" -eq 1 ] && [ "$AUTORUN_DVI_SMOKE" -eq 1 ]; then
    die "--autorun-gfx-demo and --autorun-dvi-smoke are mutually exclusive"
fi

if [ "$ALLOW_BLIND_AUTORUN" -eq 0 ] &&
   { [ "$AUTORUN_GFX_DEMO" -eq 1 ] || [ "$AUTORUN_DVI_SMOKE" -eq 1 ]; }; then
    die "autorun replaces NSH USB console setup; pass --allow-blind-autorun only after a normal NSH image has proven the console path"
fi

case "$OUT_DIR" in
    /*) ;;
    *) OUT_DIR="$ROOT/$OUT_DIR" ;;
esac

SMOKE_SCRIPT="$ROOT/build_scripts/nuttx-riscv32-qemu-smoke.sh"
APPS_PATCH_FILES=(
    "$ROOT/build_scripts/nuttx-patches/apps-fbcon-unique-custom-bpp-choice.patch"
)
PATCH_FILES=(
    "$ROOT/build_scripts/nuttx-patches/rp23xx-rv-rp2350b-pizero-support.patch"
    "$ROOT/build_scripts/nuttx-patches/rp23xx-rv-spi-callback-stubs.patch"
    "$ROOT/build_scripts/nuttx-patches/rp23xx-rv-spisd-partition-mount.patch"
    "$ROOT/build_scripts/nuttx-patches/rp23xx-rv-pizero-dvi-clock.patch"
    "$ROOT/build_scripts/nuttx-patches/rp2350-exec-symtab-exports.patch"
    "$ROOT/build_scripts/nuttx-patches/rp23xx-rv-usbhost-controller.patch"
)
NUTTX_DIR="$NUTTX_WORKDIR/nuttx"

[ -f "$SMOKE_SCRIPT" ] || die "missing smoke app staging script: $SMOKE_SCRIPT"
for patch_file in "${APPS_PATCH_FILES[@]}" "${PATCH_FILES[@]}"; do
    [ -f "$patch_file" ] || die "missing NuttX patch: $patch_file"
done

##############################################################################
# Patch and configure NuttX
##############################################################################

cd "$NUTTX_DIR"

APPS_DIR="${NUTTX_WORKDIR%/}/apps"

if [ -L Make.defs ] && [ ! -e Make.defs ]; then
    echo "Removing stale NuttX Make.defs symlink: $(readlink Make.defs)"
    rm -f Make.defs .config include/nuttx/config.h
fi

remove_stale_apps_kconfigs

if [ "$APPLY_NUTTX_PATCH" -eq 1 ]; then
    for patch_file in "${APPS_PATCH_FILES[@]}"; do
        (
            cd "$NUTTX_WORKDIR"
            apply_nuttx_patch "$patch_file"
        )
    done

    for patch_file in "${PATCH_FILES[@]}"; do
        apply_nuttx_patch "$patch_file"
    done
fi

if [ -f Make.defs ]; then
    run make distclean
else
    echo "NuttX tree is already unconfigured; skipping make distclean"
fi

if [ "$WITH_USB_HOST_HUB" -eq 1 ]; then
    run ./tools/configure.sh raspberrypi-pico-2-rv:nsh
else
    run ./tools/configure.sh raspberrypi-pico-2-rv:usbnsh
fi

kconfig_enable CONFIG_RP23XX_RV_RP2350B
kconfig_enable CONFIG_RP23XX_RV_DMAC
kconfig_enable CONFIG_RP23XX_RV_PIZERO_DVI_CLOCK
kconfig_set_str CONFIG_ARCH_RV_ISA_VENDOR_EXTENSIONS zba_zbb_zbs_zbkb_zcb
kconfig_disable CONFIG_RISCV_TOOLCHAIN_GNU_RV32
kconfig_enable CONFIG_RISCV_TOOLCHAIN_GNU_RV64
kconfig_disable CONFIG_LIBM_TOOLCHAIN
kconfig_enable CONFIG_LIBM

##############################################################################
# RP2350-PiZero MicroSD
##############################################################################

kconfig_enable CONFIG_RP23XX_RV_SPI
kconfig_enable CONFIG_RP23XX_RV_SPI1
kconfig_disable CONFIG_RP23XX_RV_SPI_DMA
kconfig_enable CONFIG_RP23XX_RV_SPISD
kconfig_set_val CONFIG_RP23XX_RV_SPISD_SPI_CH 1
kconfig_set_val CONFIG_RP23XX_RV_SPI1_RX_GPIO 40
kconfig_set_val CONFIG_RP23XX_RV_SPI1_TX_GPIO 31
kconfig_set_val CONFIG_RP23XX_RV_SPI1_SCK_GPIO 30
kconfig_set_val CONFIG_RP23XX_RV_SPI1_CS_GPIO 43
kconfig_enable CONFIG_SPI_EXCHANGE
kconfig_enable CONFIG_MMCSD
kconfig_set_val CONFIG_MMCSD_SPICLOCK 12500000

#
# The RP2350-PiZero image treats the 8GB card as removable data storage.
# Keep this MBR/FAT32 path small and explicit; partitioning and formatting
# are operator actions, not side effects of building the UF2 image.
#
kconfig_enable CONFIG_MBR_PARTITION
kconfig_enable CONFIG_FS_FAT
kconfig_enable CONFIG_FAT_LCNAMES
kconfig_enable CONFIG_FAT_LFN
kconfig_enable CONFIG_FSUTILS_MKMBR
kconfig_set_val CONFIG_NAME_MAX 64
kconfig_set_val CONFIG_FAT_MAXFNAME 64

##############################################################################
# Board services useful while bringing the hardware up
##############################################################################

kconfig_enable CONFIG_ELF
kconfig_enable CONFIG_ARCH_SETJMP_H
kconfig_enable CONFIG_LIBC_EXECFUNCS
kconfig_enable CONFIG_EXECFUNCS_HAVE_SYMTAB
kconfig_enable CONFIG_EXECFUNCS_SYSTEM_SYMTAB
kconfig_enable CONFIG_LIBC_ENVPATH
kconfig_enable CONFIG_NSH_FILE_APPS
kconfig_set_str CONFIG_PATH_INITIAL "/mnt/sd0/bin:/mnt/sd0:/bin"
kconfig_set_val CONFIG_ELF_STACKSIZE 32768
kconfig_enable CONFIG_BOARDCTL
kconfig_enable CONFIG_BOARDCTL_RESET
kconfig_enable CONFIG_SYSTEM_YMODEM
kconfig_set_val CONFIG_SYSTEM_YMODEM_STACKSIZE 4096
kconfig_set_val CONFIG_SYSTEM_YMODEM_PRIORITY 100
kconfig_enable CONFIG_FS_PROCFS
kconfig_enable CONFIG_FS_PROCFS_REGISTER
kconfig_enable CONFIG_MMCSD_PROCFS
kconfig_enable CONFIG_FS_TMPFS
kconfig_enable CONFIG_SYSTEM_DD
kconfig_enable CONFIG_SYSTEM_HEXDUMP
kconfig_enable CONFIG_NSH_CMDOPT_HEXDUMP
kconfig_disable CONFIG_STACK_USAGE
kconfig_set_val CONFIG_ARCH_INTERRUPTSTACK 4096

if [ "$BUILD_ONLY_NO_UF2" -eq 1 ]; then
    kconfig_disable CONFIG_RP23XX_UF2_BINARY
    kconfig_enable CONFIG_RAW_BINARY
fi

#
# The Pico 2 usbnsh defconfig enables a couple of NuttX test apps. They are
# useful while validating the upstream board port, but this image is meant to
# leave SRAM and flash for FreeBASIC programs and board services.
#
kconfig_disable CONFIG_TESTING_OSTEST
kconfig_disable CONFIG_TESTING_GETPRIME

if [ "$WITH_I2S_AUDIO" -eq 1 ]; then
    kconfig_enable CONFIG_SCHED_WORKQUEUE
    kconfig_enable CONFIG_SCHED_HPWORK
    kconfig_enable CONFIG_AUDIO
    kconfig_enable CONFIG_DRIVERS_AUDIO
    kconfig_enable CONFIG_AUDIO_FORMAT_PCM
    kconfig_disable CONFIG_AUDIO_FORMAT_MP3
    kconfig_disable CONFIG_AUDIO_FORMAT_SBC
    kconfig_disable CONFIG_AUDIO_FORMAT_AMR
    kconfig_disable CONFIG_AUDIO_FORMAT_OPUS
    kconfig_enable CONFIG_AUDIO_I2S
    kconfig_enable CONFIG_AUDIO_I2SCHAR
    kconfig_enable CONFIG_AUDIO_PCM
    kconfig_enable CONFIG_RP23XX_RV_I2S
    kconfig_set_val CONFIG_RP23XX_RV_I2S_DATA "$I2S_DATA_GPIO"
    kconfig_set_val CONFIG_RP23XX_RV_I2S_CLOCK "$I2S_CLOCK_GPIO"
fi

if [ "$WITH_USB_HOST_HUB" -eq 1 ]; then
    if ! nuttx_has_rp23xx_usbhost_controller; then
        die "USB host hub requested, but this NuttX tree has no RP23xx USB host controller driver. The generic hub/HID/MSC classes are present under drivers/usbhost, but RP23xx-RV currently only provides rp23xx_usbdev.c."
    fi

    #
    # This block prepares the class-driver side.  The low-level RP23xx host
    # controller must select USBHOST_HAVE_ASYNCH so hub interrupt polling can
    # survive olddefconfig.  HID needs BSD-compatible queue helpers.
    #
    kconfig_disable CONFIG_USBDEV
    kconfig_disable CONFIG_CDCACM
    kconfig_enable CONFIG_SCHED_WORKQUEUE
    kconfig_enable CONFIG_SCHED_HPWORK
    kconfig_set_val CONFIG_SCHED_HPWORKSTACKSIZE 4096
    kconfig_enable CONFIG_SCHED_LPWORK
    kconfig_set_val CONFIG_SCHED_LPWORKSTACKSIZE 4096
    kconfig_enable CONFIG_USBHOST
    kconfig_enable CONFIG_RP23XX_RV_USBHOST
    kconfig_set_val CONFIG_RP23XX_RV_USBHOST_DESCSIZE 512
    kconfig_enable CONFIG_USBHOST_ASYNCH
    kconfig_enable CONFIG_USBHOST_WAITER
    kconfig_set_val CONFIG_USBHOST_WAITER_PRIO 100
    kconfig_set_val CONFIG_USBHOST_WAITER_STACKSIZE 2048
    kconfig_enable CONFIG_USBHOST_HUB
    kconfig_set_val CONFIG_USBHOST_HUB_POLLMSEC 100
    kconfig_enable CONFIG_ALLOW_BSD_COMPONENTS
    kconfig_enable CONFIG_USBHOST_HID
    kconfig_enable CONFIG_USBHOST_HIDKBD
    kconfig_set_val CONFIG_HIDKBD_POLLUSEC 20000
    kconfig_set_val CONFIG_HIDKBD_STACKSIZE 1536
    kconfig_set_val CONFIG_HIDKBD_BUFSIZE 64
    kconfig_enable CONFIG_HIDKBD_NOGETREPORT
    kconfig_enable CONFIG_USBHOST_HIDMOUSE
    kconfig_enable CONFIG_USBHOST_MSC

    if [ "$WITH_USB_ETHERNET" -eq 1 ]; then
        #
        # CDC-ECM is the best first USB Ethernet target for NuttX because it
        # is a standard class driver already present in the tree.  MBIM stays
        # out of this default image because this tree hides it behind
        # EXPERIMENTAL and pulls in the separate MBIM network layer.  RNDIS is
        # intentionally not toggled here because this NuttX tree does not
        # advertise a USB host RNDIS class driver beside the ECM/MBIM drivers.
        #
        kconfig_enable CONFIG_SCHED_WORKQUEUE
        kconfig_enable CONFIG_SCHED_LPWORK
        kconfig_set_val CONFIG_SCHED_LPWORKSTACKSIZE 4096
        kconfig_enable CONFIG_NET
        kconfig_enable CONFIG_NETDEV_LATEINIT
        kconfig_enable CONFIG_NETDEVICES
        kconfig_enable CONFIG_NET_IPv4
        kconfig_enable CONFIG_NET_ETHERNET
        kconfig_enable CONFIG_NET_TCP
        kconfig_enable CONFIG_NET_UDP
        kconfig_enable CONFIG_NET_ICMP
        kconfig_enable CONFIG_NET_ARP
        kconfig_enable CONFIG_NET_SOCKOPTS
        kconfig_enable CONFIG_NET_TCPBACKLOG
        kconfig_enable CONFIG_NET_TCP_WRITE_BUFFERS
        kconfig_set_val CONFIG_NET_TCP_PREALLOC_CONNS 8
        kconfig_set_val CONFIG_NET_MAX_LISTENPORTS 8
        kconfig_enable CONFIG_NETUTILS_DHCPC
        kconfig_enable CONFIG_USBHOST_CDCECM
    fi
fi

yes "" | make olddefconfig >/dev/null || true

if [ "$WITH_USB_HOST_HUB" -eq 1 ]; then
    require_config_enabled CONFIG_RP23XX_RV_USBHOST
    require_config_enabled CONFIG_USBHOST
    require_config_enabled CONFIG_USBHOST_ASYNCH
    require_config_enabled CONFIG_USBHOST_WAITER
    require_config_enabled CONFIG_USBHOST_HUB
    require_config_enabled CONFIG_USBHOST_HID
    require_config_enabled CONFIG_USBHOST_HIDKBD
    require_config_enabled CONFIG_USBHOST_HIDMOUSE
    require_config_enabled CONFIG_USBHOST_MSC

    if [ "$WITH_USB_ETHERNET" -eq 1 ]; then
        require_config_enabled CONFIG_NET
        require_config_enabled CONFIG_NET_IPv4
        require_config_enabled CONFIG_NET_TCP
        require_config_enabled CONFIG_NET_UDP
        require_config_enabled CONFIG_NETUTILS_DHCPC
        require_config_enabled CONFIG_USBHOST_CDCECM
    fi
fi

##############################################################################
# Stage FreeBASIC programs
##############################################################################

#
# fbgfx is staged first so the single built-in FreeBASIC runtime copy is
# compiled with the gfx bridge enabled.  END and runtime-error exits then
# unwind the terminal and DVI driver even when a smoke test fails early.
#
stage_fb_app fbgfx "$ROOT/examples/nuttx/fbgfx_smoke.bas" 1 32768 1
stage_fb_app fbgfxdemo "$ROOT/examples/nuttx/fbgfx_demo.bas" 1 32768 0
stage_fb_app fbhello "$ROOT/examples/nuttx/fbhello.bas" 0 8192 0
stage_fb_app fbcount "$ROOT/examples/nuttx/fbcount.bas" 0 8192 0
stage_fb_app fbstrings "$ROOT/examples/nuttx/fbstrings.bas" 0 16384 0
stage_fb_app fbsd "$ROOT/examples/nuttx/fbsd_smoke.bas" 0 16384 0
stage_c_app bootsel "$ROOT/examples/nuttx/fbbootsel.c" \
    "RP2350 BOOTSEL programming-mode reboot" \
    "Ask the RP23xx board reset hook to reboot into BOOTSEL" \
    "USB programming mode so the next UF2 can be copied without buttons." \
    2048
if [ "$WITH_DVI_SMOKE" -eq 1 ]; then
    stage_c_app fbdvi "$ROOT/examples/nuttx/fbdvi_solid.c"
    stage_c_app fbdviregs "$ROOT/examples/nuttx/fbdvi_regs.c"
fi
stage_fb_app fbsfx "$ROOT/examples/nuttx/fbsfx_smoke.bas" 0 32768 0

if [ "$AUTORUN_GFX_DEMO" -eq 1 ]; then
    #
    # Hardware bring-up sometimes needs a display signal before the USB NSH
    # console is trustworthy.  This image variant makes the generated BASIC
    # demo the initial task and gives it the same stack budget as the normal
    # built-in app entry.
    #
    kconfig_set_str CONFIG_INIT_ENTRYPOINT fbgfxdemo_main
    kconfig_set_str CONFIG_INIT_ENTRYNAME fbgfxdemo_main
    kconfig_set_val CONFIG_INIT_STACKSIZE 32768
    yes "" | make olddefconfig >/dev/null || true
    run make EXTRALINKCMDS="${EXTRALINKCMDS:-} --no-warn-rwx-segments" \
        -j"${JOBS:-$(nproc)}"
fi

if [ "$AUTORUN_DVI_SMOKE" -eq 1 ]; then
    #
    # This path strips the bring-up problem down to the electrical DVI signal.
    # It bypasses the FreeBASIC gfxlib layer and starts the small C smoke app
    # as init so a monitor can lock before the USB console path matters.
    #
    kconfig_set_str CONFIG_INIT_ENTRYPOINT fbdvi_main
    kconfig_set_str CONFIG_INIT_ENTRYNAME fbdvi_main
    kconfig_set_val CONFIG_INIT_STACKSIZE 8192
    yes "" | make olddefconfig >/dev/null || true
    run make EXTRALINKCMDS="${EXTRALINKCMDS:-} --no-warn-rwx-segments" \
        -j"${JOBS:-$(nproc)}"
fi

##############################################################################
# Collect the UF2
##############################################################################

mkdir -p "$OUT_DIR"

if [ "$BUILD_ONLY_NO_UF2" -eq 1 ]; then
    [ -f "$NUTTX_DIR/nuttx" ] ||
        die "NuttX ELF output was not found"

    cp "$NUTTX_DIR/nuttx" "$OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.elf"

    if [ -f "$NUTTX_DIR/nuttx.bin" ]; then
        cp "$NUTTX_DIR/nuttx.bin" "$OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.bin"
    fi
elif [ -f "$NUTTX_DIR/nuttx.uf2" ]; then
    cp "$NUTTX_DIR/nuttx.uf2" "$OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.uf2"
elif [ -f "$NUTTX_DIR/nuttx.bin.uf2" ]; then
    cp "$NUTTX_DIR/nuttx.bin.uf2" "$OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.uf2"
else
    die "NuttX UF2 output was not found"
fi

report_nuttx_memory_budget

(
    cd "$OUT_DIR"
    if [ "$BUILD_ONLY_NO_UF2" -eq 1 ]; then
        sha256sum freebasic-rp2350-pizero-riscv32-usb-sd.elf \
            > freebasic-rp2350-pizero-riscv32-usb-sd.elf.sha256

        if [ -f freebasic-rp2350-pizero-riscv32-usb-sd.bin ]; then
            sha256sum freebasic-rp2350-pizero-riscv32-usb-sd.bin \
                > freebasic-rp2350-pizero-riscv32-usb-sd.bin.sha256
        fi
    else
        sha256sum freebasic-rp2350-pizero-riscv32-usb-sd.uf2 \
            > freebasic-rp2350-pizero-riscv32-usb-sd.uf2.sha256
    fi
)

echo "NUTTX_RP2350_PIZERO_IMAGE_OK"
if [ "$BUILD_ONLY_NO_UF2" -eq 1 ]; then
    echo "ELF:    $OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.elf"
    if [ -f "$OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.bin" ]; then
        echo "BIN:    $OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.bin"
    fi
    echo "SHA256: $OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.elf.sha256"
else
    echo "UF2:    $OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.uf2"
    echo "SHA256: $OUT_DIR/freebasic-rp2350-pizero-riscv32-usb-sd.uf2.sha256"
fi

# end of nuttx-rp2350-pizero-image.sh
