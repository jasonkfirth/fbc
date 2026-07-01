#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX/RISC-V device lab
# ------------------------------------------
#
# File: nuttx-riscv32-qemu-device-lab.sh
#
# Purpose:
#
#     Run the emulator-side device checks that are useful before spending a
#     scarce RP2350-PiZero hardware write cycle.
#
# Responsibilities:
#
#     - run the core FreeBASIC NuttX smoke program under qemu-system-riscv32
#     - require QEMU-visible gfxlib framebuffer presentation signatures
#     - run the board SD smoke against a QEMU virtio block disk mounted at
#       the board-style storage root
#     - require QEMU-visible sfxlib audio buffer checksum traces
#     - boot the virtio-net NSH profile far enough to prove telnet/FTP setup
#     - run a saved-log audit that separates emulator proof from hardware gaps
#     - keep all writes inside the NuttX workdir, QEMU tmpfs, or build output
#
# This file intentionally does NOT contain:
#
#     - RP2350 UF2 image generation
#     - controller flashing or mounted-drive writes
#     - claims that qemu-system-riscv32 emulates RP2350 HSTX, PIO, or USB
#       controller hardware
#

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
smoke_script="$script_dir/nuttx-riscv32-qemu-smoke.sh"
audit_script="$script_dir/nuttx-riscv32-qemu-device-audit.py"
rtlib_audit_script="$script_dir/nuttx-riscv32-rtlib-audit.py"
source_audit_script="$script_dir/nuttx-rp2350-device-source-audit.py"
memory_budget_script="$script_dir/nuttx-rp2350-memory-budget-check.py"
sram_budget_script="$script_dir/nuttx-rp2350-sram-profile-check.py"

nuttx_workdir=${NUTTX_WORKDIR:-/work}
generated_dir=${FB_NUTTX_GENERATED_C_DIR:-"$repo_root/.build-rp2350-pizero-generated"}
fbc_bin=${FBC:-"$repo_root/bin/fbc"}
memory=${QEMU_MEMORY:-512M}
timeout=${QEMU_TIMEOUT:-45}
network_timeout=${QEMU_NETWORK_TIMEOUT:-20}
lab_log_dir=${NUTTX_DEVICE_LAB_LOG_DIR:-"$repo_root/.build-nuttx-device-lab-logs"}
lab_verbose=${NUTTX_DEVICE_LAB_VERBOSE:-0}
only_cases=${NUTTX_DEVICE_LAB_ONLY:-}
run_network=1
skip_missing=0
skip_nuttx_config=0
run_audit=1

usage() {
    cat <<'EOF'
Usage:
  nuttx-riscv32-qemu-device-lab.sh [options]

Options:
  --nuttx-workdir DIR     Directory containing nuttx/ and apps/.
                          Default: $NUTTX_WORKDIR or /work

  --generated-dir DIR     Directory containing generated NuttX C examples.
                          Default: $FB_NUTTX_GENERATED_C_DIR or
                          <repo>/.build-rp2350-pizero-generated

  --fbc FILE              FreeBASIC compiler used when a lab case is generated
                          from .bas source. Default: $FBC or <repo>/bin/fbc

  --memory SIZE           QEMU RAM setting. Default: $QEMU_MEMORY or 512M

  --timeout SECONDS       Normal app timeout. Default: $QEMU_TIMEOUT or 45

  --network-timeout SECS  Network shell timeout. Default:
                          $QEMU_NETWORK_TIMEOUT or 20

  --log-dir DIR           Per-case smoke harness log directory. Default:
                          $NUTTX_DEVICE_LAB_LOG_DIR or
                          <repo>/.build-nuttx-device-lab-logs

  --verbose               Stream full per-case build and QEMU output while
                          also saving it to the case log.

  --only CASES            Run only the named comma-separated cases. Valid
                          cases are rtlibaudit, sourceaudit, core, gosub, fre,
                          filecopy, mathcvn, frac, log10, sign, assert,
                          membudget, srambudget, dviparity, dvisolid, gfx,
                          gfxhid,
                          usbcombo,
                          allcombo, storage, usbstore, usbhub, usbnet, audio,
                          sfx, and network.

  --without-network       Skip the virtio-net/telnet/FTP boot check.

  --skip-missing          Skip cases whose generated C source is missing.

  --skip-nuttx-config     Keep the existing NuttX board configuration and let
                          each case only update the app-specific settings.

  --no-audit              Do not run the saved-log device audit after the
                          selected cases finish.

  -h, --help              Show this help.

The lab currently proves these emulator surfaces:

  rtlibaudit
            source-side audit proving approved generic rtlib helpers are
            staged by default, while NuttX-only shims are explicitly listed
  sourceaudit
            source-side audit proving the RP2350 image path, native RP23xx
            USB-host patch, and QEMU USB topology still cover the same
            hub/HID/storage/USB-net plan
  core      fbhello smoke app, including console input and file I/O on tmpfs
  gfx       SCREEN 1/7/13 plus QEMU framebuffer visual signatures
  gfxhid    gfxlib opens NuttX USB HID nodes and reads an injected key event
  usbcombo  hub-attached storage, keyboard, mouse, USB Ethernet, and gfxlib
            in one QEMU boot
  allcombo  usbcombo plus the QEMU/NuttX virtio-sound PCM playback node in
            the same boot
  fre       NuttX heap availability through the FreeBASIC FRE() runtime
  fix       generic rtlib FIX() helper, covered by the core app
  filecopy  generic rtlib FILECOPY helper, covered by a BASIC readback check
  mathcvn   generic rtlib CV numeric bit-conversion helpers
  frac      generic rtlib FRAC() helper
  log10     generic rtlib integer base-10 helpers
  sign      generic rtlib SGN() helpers
  assert    generic rtlib ASSERT/ASSERTWARN helpers, including WSTRING
            expression diagnostics and hard assertion exit status
  membudget RP2350 gfxlib framebuffer and DVI scanout memory budget
  srambudget
            built NuttX image budget against 512 KB SRAM, separating the
            QEMU all-device stress build from flash/XIP controller profiles
  dviparity gfxlib2 DVI constants match the standalone DVI smoke app
  dvisolid  standalone DVI smoke app's QEMU model boots and checks timing
  storage   /mnt/sd0 smoke test backed by a QEMU virtio block disk
  usbstore  /mnt/sd0 smoke test backed by QEMU xHCI usb-storage
  sfx       sfxlib command path plus real NuttX/QEMU PCM audio writes
  network   rv-virt virtio-net NSH profile reaches telnet/FTP setup
  usbnet    QEMU usb-net enumerates on xHCI and binds as a CDC Ethernet
            network device
  usbhub    /mnt/sd0 smoke test backed by QEMU usb-storage behind an xHCI
            usb-hub, while also requiring hub-attached keyboard and mouse
            HID device nodes
  audio     QEMU virtio-sound enumerates through the NuttX audio framework
            and registers /dev/audio/pcm0p

Storage, graphics presentation, and audio are still emulator surfaces here,
not RP2350 electrical proofs. The virtio block storage case does prove the
NuttX block-driver, FAT, mount, and FreeBASIC file-I/O path under QEMU before
moving the image to hardware.
EOF
}

die() {
    printf 'nuttx-device-lab: %s\n' "$*" >&2
    exit 1
}

case_enabled() {
    local label=$1
    local item

    if [ -z "$only_cases" ]; then
        return 0
    fi

    for item in ${only_cases//,/ }; do
        if [ "$item" = "$label" ]; then
            return 0
        fi
    done

    return 1
}

needs_nuttx_runtime() {
    local item

    if [ -z "$only_cases" ]; then
        return 0
    fi

    for item in ${only_cases//,/ }; do
        case "$item" in
            membudget | dviparity | rtlibaudit | sourceaudit)
                ;;
            *)
                return 0
                ;;
        esac
    done

    return 1
}

run_smoke() {
    local label=$1
    local log_file=$lab_log_dir/$label.log
    local status=0
    shift

    mkdir -p "$lab_log_dir"

    printf 'nuttx-device-lab: log %-8s %s\n' "$label" "$log_file"

    if [ "$lab_verbose" -ne 0 ]; then
        set +e
        "$@" 2>&1 | tee "$log_file"
        status=${PIPESTATUS[0]}
        set -e
    else
        set +e
        "$@" >"$log_file" 2>&1
        status=$?
        set -e

        grep -aE \
            '^(NUTTX_RISCV32|FB_NUTTX|NuttX USB|NuttX audio|NuttX telnet|NuttX FTP|nuttx-memory-budget|nuttx-dvi-parity|nuttx-rtlib-audit|nuttx-device-source-audit|fb-nuttx-status=|hello from FreeBASIC)' \
            "$log_file" | tail -n 80 || true
    fi

    if [ "$status" -ne 0 ]; then
        printf 'nuttx-device-lab: failed %-8s status %d\n' "$label" "$status" >&2
        tail -n 120 "$log_file" >&2 || true
        return "$status"
    fi
}

run_case() {
    local label=$1
    local c_file=$2
    local config_args=()
    shift 2

    if ! case_enabled "$label"; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    if [ ! -f "$c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' "$label" "$c_file"
            return 0
        fi

        die "missing generated C for $label: $c_file"
    fi

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' "$label" "$c_file"

    run_smoke "$label" env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        "${config_args[@]}" --generated-c "$c_file" "$@"

    printf 'nuttx-device-lab: ok  %-8s\n' "$label"
}

run_bas_case() {
    local label=$1
    local bas_file=$2
    local config_args=()
    shift 2

    if ! case_enabled "$label"; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    if [ ! -f "$bas_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' "$label" "$bas_file"
            return 0
        fi

        die "missing BASIC source for $label: $bas_file"
    fi

    [ -x "$fbc_bin" ] || die "fbc is not executable: $fbc_bin"

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' "$label" "$bas_file"

    run_smoke "$label" env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        "${config_args[@]}" --bas "$bas_file" --fbc "$fbc_bin" "$@"

    printf 'nuttx-device-lab: ok  %-8s\n' "$label"
}

run_fre_case() {
    local generated_c=$generated_dir/fbfre_smoke.c
    local bas_src=$repo_root/examples/nuttx/fbfre_smoke.bas

    if ! case_enabled fre; then
        return 0
    fi

    if [ -f "$generated_c" ]; then
        run_case fre "$generated_c" --app-name fbfre
    else
        run_bas_case fre "$bas_src" --app-name fbfre
    fi
}

run_filecopy_case() {
    local generated_c=$generated_dir/fbfilecopy_smoke.c
    local bas_src=$repo_root/examples/nuttx/fbfilecopy_smoke.bas

    if ! case_enabled filecopy; then
        return 0
    fi

    if [ -f "$generated_c" ]; then
        run_case filecopy "$generated_c" --app-name fbfilecopy
    else
        run_bas_case filecopy "$bas_src" --app-name fbfilecopy
    fi
}

run_dvi_parity_case() {
    local parity_script=$repo_root/build_scripts/nuttx-rp2350-dvi-parity-check.py

    if ! case_enabled dviparity; then
        return 0
    fi

    [ -f "$parity_script" ] || die "missing DVI parity checker: $parity_script"

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' dviparity "$parity_script"

    run_smoke dviparity python3 "$parity_script" --self-test

    printf 'nuttx-device-lab: ok  %-8s\n' dviparity
}

run_memory_budget_case() {
    if ! case_enabled membudget; then
        return 0
    fi

    [ -f "$memory_budget_script" ] ||
        die "missing memory budget checker: $memory_budget_script"

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' membudget "$memory_budget_script"

    run_smoke membudget python3 "$memory_budget_script"

    printf 'nuttx-device-lab: ok  %-8s\n' membudget
}

run_rtlib_audit_case() {
    if ! case_enabled rtlibaudit; then
        return 0
    fi

    [ -f "$rtlib_audit_script" ] ||
        die "missing rtlib audit checker: $rtlib_audit_script"

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' rtlibaudit "$rtlib_audit_script"

    run_smoke rtlibaudit python3 "$rtlib_audit_script" --root "$repo_root"

    printf 'nuttx-device-lab: ok  %-8s\n' rtlibaudit
}

run_source_audit_case() {
    if ! case_enabled sourceaudit; then
        return 0
    fi

    [ -f "$source_audit_script" ] ||
        die "missing device source audit checker: $source_audit_script"

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' sourceaudit "$source_audit_script"

    run_smoke sourceaudit python3 "$source_audit_script" --root "$repo_root"

    printf 'nuttx-device-lab: ok  %-8s\n' sourceaudit
}

run_dvi_solid_case() {
    local c_file=$repo_root/examples/nuttx/fbdvi_solid.c

    if ! case_enabled dvisolid; then
        return 0
    fi

    if [ ! -f "$c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' dvisolid "$c_file"
            return 0
        fi

        die "missing standalone DVI smoke source: $c_file"
    fi

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' dvisolid "$c_file"

    run_smoke dvisolid env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
        FB_NUTTX_EXTRA_CFLAGS="-DFB_NUTTX_QEMU_DVI_SOLID_MODEL=1" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        --skip-nuttx-config --generated-c "$c_file" --app-name fbdvisolid

    printf 'nuttx-device-lab: ok  %-8s\n' dvisolid
}

run_core_case() {
    local generated_c=$generated_dir/fbhello_smoke.c
    local bas_src=$repo_root/examples/nuttx/fbhello_smoke.bas
    local config_args=()

    if ! case_enabled core; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    printf '\n'

    if [ -f "$generated_c" ] && [ "$generated_c" -nt "$bas_src" ]; then
        printf 'nuttx-device-lab: run %-8s %s\n' core "$generated_c"

        run_smoke core env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
            "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
            "${config_args[@]}" --generated-c "$generated_c" --app-name fbhello
    else
        [ -f "$bas_src" ] || die "missing core BASIC source: $bas_src"
        [ -x "$fbc_bin" ] || die "fbc is not executable: $fbc_bin"

        printf 'nuttx-device-lab: run %-8s %s\n' core "$bas_src"

        run_smoke core env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
            "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
            "${config_args[@]}" --bas "$bas_src" --fbc "$fbc_bin" \
            --app-name fbhello
    fi

    printf 'nuttx-device-lab: ok  %-8s\n' core
}

run_network_case() {
    local c_file=$generated_dir/fbhello.c
    local config_args=()

    if ! case_enabled network; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    if [ ! -f "$c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' network "$c_file"
            return 0
        fi

        die "missing generated C for network: $c_file"
    fi

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' network "$c_file"

    run_smoke network env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$network_timeout" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        "${config_args[@]}" --generated-c "$c_file" --app-name fbnetlab \
        --network-shell

    printf 'nuttx-device-lab: ok  %-8s\n' network
}

run_usbnet_case() {
    local c_file=$generated_dir/fbhello.c
    local config_args=()

    if ! case_enabled usbnet; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    if [ ! -f "$c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' usbnet "$c_file"
            return 0
        fi

        die "missing generated C for usbnet: $c_file"
    fi

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' usbnet "$c_file"

    run_smoke usbnet env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$network_timeout" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        "${config_args[@]}" --generated-c "$c_file" --app-name fbusbnet \
        --network-shell --qemu-usb-root-devices \
        --expect-qemu-usb-net-supported

    printf 'nuttx-device-lab: ok  %-8s\n' usbnet
}

run_usbhub_case() {
    local c_file=$generated_dir/fbsd_smoke.c
    local config_args=()

    if ! case_enabled usbhub; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    if [ ! -f "$c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' usbhub "$c_file"
            return 0
        fi

        die "missing generated C for usbhub: $c_file"
    fi

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' usbhub "$c_file"

    run_smoke usbhub env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        "${config_args[@]}" --generated-c "$c_file" --app-name fbusbhub \
        --qemu-usb-hub-devices --qemu-usb-storage-root /mnt/sd0 \
        --expect-qemu-usb-hub-supported --expect-qemu-usb-net-supported

    printf 'nuttx-device-lab: ok  %-8s\n' usbhub
}

run_usbcombo_case() {
    local bas_src=$repo_root/examples/nuttx/fbdevice_combo_smoke.bas
    local generated_c=$generated_dir/fbdevice_combo_smoke.c

    if ! case_enabled usbcombo; then
        return 0
    fi

    if [ -f "$generated_c" ]; then
        run_case usbcombo "$generated_c" --app-name fbcombo \
            --with-gfxlib --qemu-mock-devices \
            --qemu-usb-hub-devices --qemu-usb-storage-root /mnt/sd0 \
            --expect-qemu-usb-hub-supported --expect-qemu-usb-net-supported \
            --qemu-inject-hid-events
        return 0
    fi

    run_bas_case usbcombo "$bas_src" --app-name fbcombo \
        --with-gfxlib --qemu-mock-devices \
        --qemu-usb-hub-devices --qemu-usb-storage-root /mnt/sd0 \
        --expect-qemu-usb-hub-supported --expect-qemu-usb-net-supported \
        --qemu-inject-hid-events
}

run_allcombo_case() {
    local bas_src=$repo_root/examples/nuttx/fbdevice_combo_smoke.bas
    local generated_c=$generated_dir/fbdevice_combo_smoke.c

    if ! case_enabled allcombo; then
        return 0
    fi

    if [ -f "$generated_c" ]; then
        run_case allcombo "$generated_c" --app-name fbcombo \
            --with-gfxlib --qemu-mock-devices \
            --qemu-usb-hub-devices --qemu-usb-storage-root /mnt/sd0 \
            --expect-qemu-usb-hub-supported --expect-qemu-usb-net-supported \
            --qemu-inject-hid-events --qemu-virtio-sound
        return 0
    fi

    run_bas_case allcombo "$bas_src" --app-name fbcombo \
        --with-gfxlib --qemu-mock-devices \
        --qemu-usb-hub-devices --qemu-usb-storage-root /mnt/sd0 \
        --expect-qemu-usb-hub-supported --expect-qemu-usb-net-supported \
        --qemu-inject-hid-events --qemu-virtio-sound
}

run_audio_case() {
    local c_file=$generated_dir/fbhello.c
    local config_args=()

    if ! case_enabled audio; then
        return 0
    fi

    if [ "$skip_nuttx_config" -ne 0 ]; then
        config_args+=(--skip-nuttx-config)
    fi

    if [ ! -f "$c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-device-lab: skip %-8s missing %s\n' audio "$c_file"
            return 0
        fi

        die "missing generated C for audio: $c_file"
    fi

    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' audio "$c_file"

    run_smoke audio env QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" \
        "$smoke_script" --nuttx-workdir "$nuttx_workdir" \
        "${config_args[@]}" --generated-c "$c_file" --app-name fbaudio \
        --qemu-virtio-sound

    printf 'nuttx-device-lab: ok  %-8s\n' audio
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --nuttx-workdir)
            [ "$#" -ge 2 ] || die "--nuttx-workdir requires a directory"
            nuttx_workdir=$2
            shift 2
            ;;
        --generated-dir)
            [ "$#" -ge 2 ] || die "--generated-dir requires a directory"
            generated_dir=$2
            shift 2
            ;;
        --fbc)
            [ "$#" -ge 2 ] || die "--fbc requires a file"
            fbc_bin=$2
            shift 2
            ;;
        --memory)
            [ "$#" -ge 2 ] || die "--memory requires a QEMU memory size"
            memory=$2
            shift 2
            ;;
        --timeout)
            [ "$#" -ge 2 ] || die "--timeout requires seconds"
            timeout=$2
            shift 2
            ;;
        --network-timeout)
            [ "$#" -ge 2 ] || die "--network-timeout requires seconds"
            network_timeout=$2
            shift 2
            ;;
        --log-dir)
            [ "$#" -ge 2 ] || die "--log-dir requires a directory"
            lab_log_dir=$2
            shift 2
            ;;
        --verbose)
            lab_verbose=1
            shift
            ;;
        --only)
            [ "$#" -ge 2 ] || die "--only requires a case list"
            if [ -n "$only_cases" ]; then
                only_cases=$only_cases,$2
            else
                only_cases=$2
            fi
            shift 2
            ;;
        --without-network)
            run_network=0
            shift
            ;;
        --skip-missing)
            skip_missing=1
            shift
            ;;
        --skip-nuttx-config)
            skip_nuttx_config=1
            shift
            ;;
        --no-audit)
            run_audit=0
            shift
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

[ -f "$smoke_script" ] || die "missing smoke harness: $smoke_script"
[ -f "$audit_script" ] || die "missing device audit: $audit_script"
[ -f "$rtlib_audit_script" ] ||
    die "missing rtlib audit checker: $rtlib_audit_script"
[ -f "$source_audit_script" ] ||
    die "missing device source audit checker: $source_audit_script"
[ -f "$memory_budget_script" ] ||
    die "missing memory budget checker: $memory_budget_script"
[ -f "$sram_budget_script" ] ||
    die "missing SRAM budget checker: $sram_budget_script"

if needs_nuttx_runtime; then
    [ -d "$nuttx_workdir" ] || die "missing NuttX workdir: $nuttx_workdir"
    [ -d "$generated_dir" ] ||
        die "missing generated C directory: $generated_dir"
fi

printf 'nuttx-device-lab: workdir       %s\n' "$nuttx_workdir"
printf 'nuttx-device-lab: generated dir %s\n' "$generated_dir"
printf 'nuttx-device-lab: fbc           %s\n' "$fbc_bin"
printf 'nuttx-device-lab: qemu memory   %s\n' "$memory"
printf 'nuttx-device-lab: timeout       %s\n' "$timeout"
printf 'nuttx-device-lab: log dir       %s\n' "$lab_log_dir"
if [ -n "$only_cases" ]; then
    printf 'nuttx-device-lab: only          %s\n' "$only_cases"
fi
if [ "$skip_nuttx_config" -ne 0 ]; then
    printf 'nuttx-device-lab: config        reuse existing\n'
fi

run_rtlib_audit_case
run_source_audit_case
run_core_case
run_case gosub "$generated_dir/fbgosub_smoke.c" --app-name fbgosub
run_fre_case
run_filecopy_case
run_case mathcvn "$generated_dir/fbmathcvn_smoke.c" --app-name fbmathcvn
run_case frac "$generated_dir/fbfrac_smoke.c" --app-name fbfrac
run_case log10 "$repo_root/examples/nuttx/fblog10_smoke.c" --app-name fblog10
run_case sign "$generated_dir/fbsign_smoke.c" --app-name fbsign
run_case assert "$repo_root/examples/nuttx/fbassert_smoke.c" \
    --app-name fbassert --expect-fail
run_memory_budget_case

if case_enabled srambudget; then
    printf '\n'
    printf 'nuttx-device-lab: run %-8s %s\n' srambudget "$sram_budget_script"
    run_smoke srambudget python3 "$sram_budget_script" \
        "$nuttx_workdir/nuttx/nuttx"
    printf 'nuttx-device-lab: ok  %-8s\n' srambudget
fi

run_dvi_parity_case
run_dvi_solid_case
run_case gfx "$generated_dir/fbgfx_smoke.c" --app-name fbgfx \
    --with-gfxlib --qemu-mock-devices
run_case gfxhid "$generated_dir/fbgfx_hid_smoke.c" --app-name fbgfxhid \
    --with-gfxlib --qemu-mock-devices \
    --qemu-usb-hub-devices --qemu-usb-storage-root /mnt/sd0 \
    --expect-qemu-usb-hub-supported --expect-qemu-usb-net-supported \
    --qemu-inject-hid-events
run_usbcombo_case
run_allcombo_case
run_case storage "$generated_dir/fbsd_smoke.c" --app-name fbsd \
    --qemu-storage-root /mnt/sd0 --qemu-storage-backend virtio-blk
run_case usbstore "$generated_dir/fbsd_smoke.c" --app-name fbusbstore \
    --qemu-usb-storage-root /mnt/sd0

if [ "$run_network" -eq 1 ]; then
    run_usbhub_case
    run_usbnet_case
fi

run_case sfx "$generated_dir/fbsfx_smoke.c" --app-name fbsfx \
    --qemu-mock-devices --qemu-virtio-sound
run_audio_case

if [ "$run_network" -eq 1 ]; then
    run_network_case
fi

if [ "$run_audit" -ne 0 ] && [ "$run_network" -eq 1 ] &&
   [ -z "$only_cases" ] && [ "$skip_missing" -eq 0 ]; then
    printf '\n'
    python3 "$audit_script" "$lab_log_dir" \
        --write-evidence "$lab_log_dir/audit-evidence.tsv"
fi

printf '\n'
printf 'nuttx-device-lab: all requested emulator checks passed\n'

# end of nuttx-riscv32-qemu-device-lab.sh
