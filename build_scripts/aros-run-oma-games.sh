#!/usr/bin/env bash
#
# Project: FreeBASIC OMA games for AROS
# --------------------------------------
#
# File: aros-run-oma-games.sh
#
# Purpose:
#
#     Launch-qualify every staged OMA game under its target AROS emulator.
#
# Responsibilities:
#
#     - invoke the authoritative build and package matrix when requested
#     - boot x86_64 and ARM with QEMU and m68k with FS-UAE
#     - launch each game after AROS has initialized its Intuition display
#     - require the game process to remain alive for a bounded stability period
#     - save serial evidence, screenshots, and resumable completion records
#
# This file intentionally does NOT contain:
#
#     - OMA source or asset definitions
#     - AROS SDK or toolchain construction
#     - gameplay input automation
#     - generic m68k compiler policy
#
# Test boundary:
#
#     These old interactive games do not expose deterministic end-to-end test
#     APIs. A pass proves that the target executable loads on AROS, resolves its
#     staged assets, opens its gfxlib2 path, and remains alive through startup.
#     The saved screenshot is the reviewable visual artifact for each launch.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
OUTPUT_ROOT="${AROS_OMA_OUTDIR:-$AROS_ROOT/oma}"
M68K_BASE_ISO="${AROS_M68K_ISO:-}"
TARGETS="${AROS_TARGETS:-x86_64,m68k,arm}"
SELECTED_GAMES_TEXT=""
STABILITY_SECONDS=8
TIMEOUT_SECONDS=180
X86_MEMORY_MIB=2048

BUILD_GAMES=1
HEADLESS=0
RESUME=0

CURRENT_TARGET=""
GAME_LIVENESS_PASSED=0
M68K_EMULATOR_PID=""
M68K_READER_PID=""
QEMU_PID=""

##############################################################################
# General helpers
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

require_command() {
    command -v "$1" >/dev/null 2>&1 ||
        die "required host tool not found: $1"
}

stop_qemu() {
    local grace_seconds=0

    [ -n "$QEMU_PID" ] || return 0
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        kill "$QEMU_PID" 2>/dev/null || true
    fi
    while kill -0 "$QEMU_PID" 2>/dev/null && [ "$grace_seconds" -lt 5 ]; do
        sleep 1
        grace_seconds=$((grace_seconds + 1))
    done
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        kill -KILL "$QEMU_PID" 2>/dev/null || true
    fi
    wait "$QEMU_PID" 2>/dev/null || true
    QEMU_PID=""
}

stop_m68k_processes() {
    local grace_seconds
    local process_id

    for process_id in "$M68K_EMULATOR_PID" "$M68K_READER_PID"; do
        [ -n "$process_id" ] || continue
        if kill -0 "$process_id" 2>/dev/null; then
            kill "$process_id" 2>/dev/null || true
        fi
        grace_seconds=0
        while kill -0 "$process_id" 2>/dev/null &&
              [ "$grace_seconds" -lt 5 ]; do
            sleep 1
            grace_seconds=$((grace_seconds + 1))
        done
        if kill -0 "$process_id" 2>/dev/null; then
            kill -KILL "$process_id" 2>/dev/null || true
        fi
        wait "$process_id" 2>/dev/null || true
    done

    M68K_EMULATOR_PID=""
    M68K_READER_PID=""
}

cleanup() {
    stop_qemu
    stop_m68k_processes
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-run-oma-games.sh [options]

Options:
  --targets LIST      Comma-separated x86_64,m68k,arm list. Default: all
  --games LIST        Comma-separated game keys. Default: all built games
  --aros-root DIR     AROS workspace. Default: out/aros
  --m68k-iso FILE     AROS m68k ISO to customise. Default: SDK build output
  --out-dir DIR       OMA build and evidence root. Default: out/aros/oma
  --stability SEC     Required live-process interval. Default: 8
  --timeout SEC       Per-game emulator timeout. Default: 180
  --memory MIB        x86_64 QEMU memory. Default: 2048; minimum: 256
  --skip-build        Reuse staged games and packages.
  --headless          Do not open or capture emulator display windows.
  --resume            Skip games with matching successful completion records.
  -h, --help          Show this help.

A passing launch saves a serial log, a host-readable completion record, and,
unless --headless is selected, a PNG screenshot below out/aros/oma/tests.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --targets)
            require_value "$1" "${2-}"
            TARGETS="$2"
            shift 2
            ;;
        --games)
            require_value "$1" "${2-}"
            SELECTED_GAMES_TEXT="$2"
            shift 2
            ;;
        --aros-root)
            require_value "$1" "${2-}"
            AROS_ROOT="$2"
            shift 2
            ;;
        --m68k-iso)
            require_value "$1" "${2-}"
            M68K_BASE_ISO="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --stability)
            require_value "$1" "${2-}"
            STABILITY_SECONDS="$2"
            shift 2
            ;;
        --timeout)
            require_value "$1" "${2-}"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --memory)
            require_value "$1" "${2-}"
            X86_MEMORY_MIB="$2"
            shift 2
            ;;
        --skip-build)
            BUILD_GAMES=0
            shift
            ;;
        --headless)
            HEADLESS=1
            shift
            ;;
        --resume)
            RESUME=1
            shift
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

for positive_integer in "$STABILITY_SECONDS" "$TIMEOUT_SECONDS" "$X86_MEMORY_MIB"; do
    case "$positive_integer" in
        ''|*[!0-9]*|0) die "stability, timeout, and memory must be positive integers" ;;
    esac
done
[ "$X86_MEMORY_MIB" -ge 256 ] || die "--memory must be at least 256 MiB"
[ "$TIMEOUT_SECONDS" -gt "$STABILITY_SECONDS" ] ||
    die "--timeout must be greater than --stability"

IFS=',' read -r -a REQUESTED_TARGETS <<< "$TARGETS"
SELECTED_TARGETS=()
for target in "${REQUESTED_TARGETS[@]}"; do
    case "$target" in
        x86_64|m68k|arm) ;;
        '') continue ;;
        *) die "unsupported AROS target: $target" ;;
    esac
    if [[ " ${SELECTED_TARGETS[*]} " != *" $target "* ]]; then
        SELECTED_TARGETS+=("$target")
    fi
done
[ "${#SELECTED_TARGETS[@]}" -gt 0 ] || die "--targets selected no targets"

if [ -n "$SELECTED_GAMES_TEXT" ]; then
    IFS=',' read -r -a SELECTED_GAMES <<< "$SELECTED_GAMES_TEXT"
else
    SELECTED_GAMES=()
fi

##############################################################################
# Paths and prerequisites
##############################################################################

CONTROL_ROOT="$OUTPUT_ROOT/tests/control"
EVIDENCE_ROOT="$OUTPUT_ROOT/tests"
STATE_ROOT="$EVIDENCE_ROOT/state"

for tool in awk cp dd file grep mkfifo sed sort timeout xorriso; do
    require_command "$tool"
done

if [ "$HEADLESS" -eq 0 ]; then
    require_command import
    require_command xdotool
    [ -n "${DISPLAY:-}" ] || die "DISPLAY is unset; use --headless or provide an X display"
fi

for target in "${SELECTED_TARGETS[@]}"; do
    case "$target" in
        x86_64) require_command qemu-system-x86_64 ;;
        arm)
            require_command mcopy
            require_command mmd
            require_command mkfs.vfat
            require_command qemu-system-arm
            require_command sfdisk
            ;;
        m68k) require_command fs-uae ;;
    esac
done

mkdir -p "$CONTROL_ROOT" "$STATE_ROOT"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
CONTROL_ROOT="$OUTPUT_ROOT/tests/control"
EVIDENCE_ROOT="$OUTPUT_ROOT/tests"
STATE_ROOT="$EVIDENCE_ROOT/state"

##############################################################################
# Target mapping and helper construction
##############################################################################

map_target() {
    local target="$1"

    case "$target" in
        x86_64)
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-pc-x86_64"
            MAP_TOOL_PREFIX="x86_64-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-pc-x86_64"
            MAP_AROS_SYSTEM="pc-x86_64"
            ;;
        m68k)
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-amiga-m68k"
            MAP_TOOL_PREFIX="m68k-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-amiga-m68k"
            MAP_AROS_SYSTEM="amiga-m68k"
            ;;
        arm)
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-raspi-armhf"
            MAP_TOOL_PREFIX="arm-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-raspi-armhf"
            MAP_AROS_SYSTEM="raspi-arm"
            ;;
        *)
            die "internal unsupported target: $target"
            ;;
    esac
}

build_log_relay() {
    local helper_directory="$EVIDENCE_ROOT/$1/helpers"
    local helper_elf="$helper_directory/log-relay.elf"
    local target="$1"

    map_target "$target"
    mkdir -p "$helper_directory"

    case "$target" in
        x86_64)
            "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                -O2 -fno-common "$ROOT/tests/aros/log-relay.c" \
                -o "$helper_directory/log-relay" -ldebug
            ;;
        arm)
            "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                -O2 -fno-common \
                -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4 \
                "$ROOT/tests/aros/log-relay.c" \
                -o "$helper_directory/log-relay" -ldebug
            ;;
        m68k)
            "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" \
                -O2 -fno-common -march=68000 -msoft-float \
                "$ROOT/tests/aros/log-relay.c" -o "$helper_elf" -ldebug
            "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" \
                "$helper_elf" "$helper_directory/log-relay"
            ;;
    esac
}

##############################################################################
# Matrix and startup generation
##############################################################################

load_manifest() {
    local key
    local manifest="$OUTPUT_ROOT/manifest-$1.tsv"
    local status
    local target="$1"

    [ -f "$manifest" ] || die "OMA manifest not found: $manifest"
    TARGET_GAME_KEYS=()
    declare -gA TARGET_GAME_DIRECTORIES=()

    while IFS=$'\t' read -r key _ _ _ status _ game_directory _ _; do
        [ "$status" = built ] || continue
        TARGET_GAME_DIRECTORIES["$key"]="$game_directory"
        TARGET_GAME_KEYS+=("$key")
    done < "$manifest"

    if [ "${#SELECTED_GAMES[@]}" -gt 0 ]; then
        for key in "${SELECTED_GAMES[@]}"; do
            [ -n "${TARGET_GAME_DIRECTORIES[$key]:-}" ] ||
                die "selected game is not built for AROS $target: $key"
        done
        TARGET_GAME_KEYS=("${SELECTED_GAMES[@]}")
    fi

    [ "${#TARGET_GAME_KEYS[@]}" -gt 0 ] ||
        die "manifest has no built games for AROS $target"
}

write_user_startup() {
    local destination="$3"
    local key="$1"
    local target="$4"
    local game_root="SYS:Tests/OMA/"
    local log_root="SYS:Tests/OMA/"
    local game_log="RAM:AROS-OMA-game.log"
    local marker_path="RAM:AROS-OMA-marker.log"

    if [ "$target" = m68k ]; then
        game_root="GameDrive:"
        log_root="GameDrive:"
        game_log="GameDrive:game.log"
        marker_path="GameDrive:marker.log"
    fi

    cat > "$destination" <<EOF
FailAt 21
Echo "AROS_OMA_TEST: $target $key BEGIN" >$marker_path
${log_root}log-relay $marker_path
CD ${game_root}game
Stack 1048576
game >$game_log
Set OMAGameRC \$RC
${log_root}log-relay $game_log
Echo "AROS_OMA_TEST: $target $key RETURNED rc=\$OMAGameRC" >$marker_path
${log_root}log-relay $marker_path
Wait 120
Shutdown
EOF
}

write_graphics_startup() {
    local destination="$1"

    cat > "$destination" <<'EOF'
FailAt 21

MakeDir RAM:T
MakeDir RAM:ENV
Assign T: RAM:T
Assign ENV: RAM:ENV
Assign KEYMAPS: DEVS:Keymaps
Assign LOCALE: SYS:Locale
Assign LIBS: SYS:Classes ADD
Assign FONTS: SYS:Fonts
Assign THEMES: SYS:Prefs/Presets/Themes
Assign IMAGES: SYS:System/Images
Path C: SYS:System S: SYS:Prefs SYS:Tools SYS:Utilities QUIET
Copy >NIL: ENVARC: ENV: ALL NOPRO NOREQ PAT "~(def_#?.info)"

If EXISTS "ENV:SYS/theme.var"
    Assign THEME: "${SYS/theme.var}"
Else
    Assign THEME: THEMES:AROSDefault
EndIf
If EXISTS "THEME:Images"
    Assign IMAGES: THEME:Images PREPEND
EndIf

Execute S:User-Startup
EOF
}

write_arm_startup() {
    local destination="$3"
    local key="$1"

    cat > "$destination" <<EOF
FailAt 21
Assign T: SYS:Tests/OMA/T
Assign ENV: SYS:Tests/OMA/ENV
Echo "AROS_OMA_TEST: arm $key BEGIN" >SYS:Tests/OMA/marker.log
SYS:Tests/OMA/log-relay SYS:Tests/OMA/marker.log
CD SYS:Tests/OMA/game
Stack 1048576
game >SYS:Tests/OMA/game.log
Set OMAGameRC \$RC
SYS:Tests/OMA/log-relay SYS:Tests/OMA/game.log
Echo "AROS_OMA_TEST: arm $key RETURNED rc=\$OMAGameRC" >SYS:Tests/OMA/marker.log
SYS:Tests/OMA/log-relay SYS:Tests/OMA/marker.log
Wait 120
Shutdown
EOF
}

write_x86_grub_config() {
    local destination="$1"

    cat > "$destination" <<EOF
set timeout=0
set default=0

menuentry "FreeBASIC AROS OMA qualification" {
    multiboot2 /boot/pc/bootstrap.xz vesa=800x600x32 ATA=32bit debug=serial:0@115200 nomonitors
    module2 /boot/pc/kernel.xz
    module2 /boot/pc/aros-bsp.pkg.xz
    module2 /boot/pc/aros-acpi.pkg.xz
    module2 /boot/aros-base.pkg.xz
    module2 /boot/aros-fs.pkg.xz
    module2 /boot/poseidon.pkg.xz
}
EOF
}

wait_for_game_liveness() {
    local elapsed=0
    local host_marker="$4"
    local key="$1"
    local log_file="$2"
    local process_id="$3"
    local started_at=-1
    local begin_marker="AROS_OMA_TEST: $CURRENT_TARGET $key BEGIN"
    local return_marker="AROS_OMA_TEST: $CURRENT_TARGET $key RETURNED"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        if grep -a -F -q "$return_marker" "$log_file" 2>/dev/null ||
           { [ -n "$host_marker" ] &&
             grep -a -F -q "$return_marker" "$host_marker" 2>/dev/null; }; then
            return 0
        fi

        if grep -a -F -q "$begin_marker" "$log_file" 2>/dev/null ||
           { [ -n "$host_marker" ] &&
             grep -a -F -q "$begin_marker" "$host_marker" 2>/dev/null; }; then
            if [ "$started_at" -lt 0 ]; then
                started_at="$elapsed"
            fi
            if [ $((elapsed - started_at)) -ge "$STABILITY_SECONDS" ]; then
                GAME_LIVENESS_PASSED=1
                return 0
            fi
        fi

        if ! kill -0 "$process_id" 2>/dev/null; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done

    return 1
}

capture_emulator_window() {
    local description="$3"
    local elapsed=0
    local process_id="$1"
    local screenshot="$2"
    local title_pattern="${4-}"
    local window_id=""

    [ "$HEADLESS" -eq 0 ] || return 0

    while [ "$elapsed" -lt 15 ]; do
        window_id="$(xdotool search --onlyvisible --pid "$process_id" 2>/dev/null | tail -1 || true)"
        if [ -z "$window_id" ] && [ -n "$title_pattern" ]; then
            window_id="$(xdotool search --onlyvisible --name "$title_pattern" 2>/dev/null | tail -1 || true)"
        fi
        if [ -n "$window_id" ]; then
            import -window "$window_id" "$screenshot"
            [ -s "$screenshot" ] || die "empty screenshot for $description"
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done

    die "could not find the emulator display window for $description"
}

##############################################################################
# x86_64 QEMU execution
##############################################################################

run_x86_game() {
    local base_iso
    local current_iso="$EVIDENCE_ROOT/x86_64/current.iso"
    local game_directory="$2"
    local game_stage="$OUTPUT_ROOT/games/x86_64/$game_directory"
    local grub_config="$CONTROL_ROOT/x86_64-grub.cfg"
    local key="$1"
    local log_file="$3"
    local screenshot="$4"
    local system_startup="$CONTROL_ROOT/graphics-Startup-Sequence"
    local startup="$CONTROL_ROOT/x86_64-$key-User-Startup"

    map_target x86_64
    base_iso="$MAP_AROS_BUILD/distfiles/aros-pc-x86_64.iso"
    [ -f "$base_iso" ] || die "x86_64 AROS ISO not found: $base_iso"
    [ -d "$game_stage" ] || die "staged x86_64 game not found: $game_stage"

    write_user_startup "$key" "$game_directory" "$startup" x86_64
    write_graphics_startup "$system_startup"
    write_x86_grub_config "$grub_config"
    cp --reflink=auto --sparse=always "$base_iso" "$current_iso"
    if ! xorriso -indev "$current_iso" -outdev "$current_iso" \
        -boot_image any keep \
        -map "$system_startup" /S/Startup-Sequence \
        -map "$startup" /S/User-Startup \
        -map "$grub_config" /boot/grub/grub.cfg \
        -map "$EVIDENCE_ROOT/x86_64/helpers/log-relay" /Tests/OMA/log-relay \
        -map "$game_stage" /Tests/OMA/game \
        -commit -end > "$EVIDENCE_ROOT/x86_64/xorriso.log" 2>&1; then
        tail -80 "$EVIDENCE_ROOT/x86_64/xorriso.log" >&2
        die "could not construct the x86_64 OMA test ISO"
    fi

    local -a display_arguments=(-display gtk,show-cursor=on)
    if [ "$HEADLESS" -eq 1 ]; then
        display_arguments=(-display none)
    fi

    qemu-system-x86_64 \
        -M pc \
        -m "$X86_MEMORY_MIB" \
        -cdrom "$current_iso" \
        -boot d \
        -serial stdio \
        "${display_arguments[@]}" \
        -monitor none \
        -audiodev driver=none,id=oma_audio \
        -device AC97,audiodev=oma_audio \
        -no-reboot > "$log_file" 2>&1 &
    QEMU_PID="$!"
    GAME_LIVENESS_PASSED=0
    wait_for_game_liveness "$key" "$log_file" "$QEMU_PID" "" || true
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        capture_emulator_window "$QEMU_PID" "$screenshot" "AROS x86_64 $key"
    fi
    stop_qemu
    if [ "$GAME_LIVENESS_PASSED" -eq 1 ]; then
        printf '%s\n' "AROS_OMA_TEST: x86_64 $key PASS" >> "$log_file"
    fi
}

##############################################################################
# ARM QEMU execution
##############################################################################

prepare_arm_base_image() {
    local aros_system
    local base_image="$EVIDENCE_ROOT/arm/base.img"
    local boot_architecture="$CONTROL_ROOT/AROS.boot"

    map_target arm
    aros_system="$MAP_AROS_BUILD/bin/raspi-arm/AROS"
    [ -d "$aros_system" ] || die "ARM AROS Workbench tree not found: $aros_system"

    msg "building reusable 512 MiB ARM AROS OMA image"
    truncate -s 0 "$base_image"
    truncate -s 512M "$base_image"
    printf '%s\n' 'label: dos' 'unit: sectors' 'first-lba: 2048' \
        '1 : start=2048, size=1046528, type=c, bootable' |
        sfdisk "$base_image" >/dev/null
    mkfs.vfat -F 32 -n AROS --offset=2048 "$base_image" 523264 >/dev/null
    mcopy -i "$base_image@@1M" -spm "$aros_system"/* ::
    mmd -i "$base_image@@1M" ::/S
    mmd -i "$base_image@@1M" ::/Tests
    mmd -i "$base_image@@1M" ::/Tests/OMA
    mmd -i "$base_image@@1M" ::/Tests/OMA/T
    mmd -i "$base_image@@1M" ::/Tests/OMA/ENV
    mcopy -i "$base_image@@1M" \
        "$EVIDENCE_ROOT/arm/helpers/log-relay" ::/Tests/OMA/log-relay
    printf 'arm\n' > "$boot_architecture"
    mcopy -i "$base_image@@1M" "$boot_architecture" ::/AROS.boot
}

run_arm_game() {
    local aros_system
    local base_image="$EVIDENCE_ROOT/arm/base.img"
    local current_image="$EVIDENCE_ROOT/arm/current.img"
    local game_directory="$2"
    local game_stage="$OUTPUT_ROOT/games/arm/$game_directory"
    local key="$1"
    local log_file="$3"
    local screenshot="$4"
    local startup="$CONTROL_ROOT/arm-$key-Startup-Sequence"

    map_target arm
    aros_system="$MAP_AROS_BUILD/bin/raspi-arm/AROS"
    [ -f "$base_image" ] || die "ARM AROS base image not found: $base_image"
    [ -d "$game_stage" ] || die "staged ARM game not found: $game_stage"

    write_arm_startup "$key" "$game_directory" "$startup"
    cp --reflink=auto --sparse=always "$base_image" "$current_image"
    mcopy -o -i "$current_image@@1M" "$startup" ::/S/Startup-Sequence
    mcopy -o -i "$current_image@@1M" -spm "$game_stage" ::/Tests/OMA/game

    local -a display_arguments=(-display gtk,show-cursor=on)
    if [ "$HEADLESS" -eq 1 ]; then
        display_arguments=(-display none)
    fi

    qemu-system-arm \
        -M raspi2b \
        -m 1G \
        "${display_arguments[@]}" \
        -monitor none \
        -serial stdio \
        -no-reboot \
        -kernel "$aros_system/aros-arm-raspi.img" \
        -initrd "$aros_system/aros-arm-bsp.rom" \
        -dtb "$aros_system/bcm2709-rpi-2-b.dtb" \
        -drive file="$current_image",format=raw,if=sd \
        > "$log_file" 2>&1 &
    QEMU_PID="$!"
    GAME_LIVENESS_PASSED=0
    wait_for_game_liveness "$key" "$log_file" "$QEMU_PID" "" || true
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        capture_emulator_window "$QEMU_PID" "$screenshot" "AROS ARM $key"
    fi
    stop_qemu
    if [ "$GAME_LIVENESS_PASSED" -eq 1 ]; then
        printf '%s\n' "AROS_OMA_TEST: arm $key PASS" >> "$log_file"
    fi
}

##############################################################################
# m68k FS-UAE execution
##############################################################################

run_m68k_game() {
    local base_iso
    local config_file="$CONTROL_ROOT/m68k-$1.fs-uae"
    local current_iso="$EVIDENCE_ROOT/m68k/current.iso"
    local emulator_log="$EVIDENCE_ROOT/m68k/logs/$1.fs-uae.log"
    local fifo="$EVIDENCE_ROOT/m68k/serial.fifo"
    local game_directory="$2"
    local game_stage="$OUTPUT_ROOT/games/m68k/$game_directory"
    local host_directory="$EVIDENCE_ROOT/m68k/host"
    local key="$1"
    local log_file="$3"
    local screenshot="$4"
    local system_startup="$CONTROL_ROOT/graphics-Startup-Sequence"
    local startup="$CONTROL_ROOT/m68k-$key-User-Startup"

    map_target m68k
    #
    # A known-bootable m68k AROS ISO can be used while a separate SDK build is
    # still producing its distributable image. The runner only alters a copy,
    # leaving either source image intact for subsequent test runs.
    #
    base_iso="$M68K_BASE_ISO"
    if [ -z "$base_iso" ]; then
        base_iso="$MAP_AROS_BUILD/distfiles/aros-amiga-m68k.iso"
    fi
    [ -f "$base_iso" ] || die "m68k AROS ISO not found: $base_iso"
    [ -d "$game_stage" ] || die "staged m68k game not found: $game_stage"

    [ "$host_directory" = "$EVIDENCE_ROOT/m68k/host" ] ||
        die "refusing to replace unexpected m68k host-directory volume"
    rm -rf -- "$host_directory"
    mkdir -p "$host_directory/game"
    cp -a "$game_stage/." "$host_directory/game/"
    cp "$EVIDENCE_ROOT/m68k/helpers/log-relay" "$host_directory/log-relay"
    chmod u+x "$host_directory/log-relay" "$host_directory/game/game"

    write_user_startup "$key" "$game_directory" "$startup" m68k
    write_graphics_startup "$system_startup"
    cp --reflink=auto --sparse=always "$base_iso" "$current_iso"
    if ! xorriso -dev "$current_iso" \
        -boot_image any replay \
        -map "$system_startup" /S/Startup-Sequence \
        -map "$startup" /S/User-Startup \
        -commit -end > "$EVIDENCE_ROOT/m68k/xorriso.log" 2>&1; then
        tail -80 "$EVIDENCE_ROOT/m68k/xorriso.log" >&2
        die "could not construct the m68k OMA test ISO"
    fi

    cat > "$config_file" <<EOF
[fs-uae]
amiga_model = A4000/040
kickstart_file = $MAP_AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-rom.bin
kickstart_ext_file = $MAP_AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-ext.bin
cdrom_drive_0 = $current_iso
hard_drive_0 = $host_directory
hard_drive_0_label = GameDrive
hard_drive_0_priority = -128
fast_memory = 8192
motherboard_ram = 65536
zorro_iii_memory = 1048576
graphics_card = uaegfx
graphics_card_memory = 32768
serial_port = $fifo
amiga_enable_serial_port = 1
fullscreen = 0
window_width = 800
window_height = 600
sound_output = none
EOF

    if [ ! -p "$fifo" ]; then
        [ ! -e "$fifo" ] || rm -f -- "$fifo"
        mkfifo "$fifo"
    fi
    : > "$log_file"
    : > "$emulator_log"

    timeout "$TIMEOUT_SECONDS" dd if="$fifo" of="$log_file" status=none &
    M68K_READER_PID="$!"
    timeout "$TIMEOUT_SECONDS" fs-uae "$config_file" --no-gui \
        > "$emulator_log" 2>&1 &
    M68K_EMULATOR_PID="$!"

    GAME_LIVENESS_PASSED=0
    wait_for_game_liveness \
        "$key" "$log_file" "$M68K_EMULATOR_PID" \
        "$host_directory/marker.log" || true

    if kill -0 "$M68K_EMULATOR_PID" 2>/dev/null; then
        capture_emulator_window \
            "$M68K_EMULATOR_PID" "$screenshot" "AROS m68k $key" 'FS-UAE'
    fi

    if [ -f "$host_directory/marker.log" ]; then
        printf '\n' >> "$log_file"
        cat "$host_directory/marker.log" >> "$log_file"
    fi
    stop_m68k_processes
    if [ "$GAME_LIVENESS_PASSED" -eq 1 ]; then
        printf '%s\n' "AROS_OMA_TEST: m68k $key PASS" >> "$log_file"
    fi
}

##############################################################################
# Validation and orchestration
##############################################################################

validate_game_log() {
    local done_file="$2"
    local key="$1"
    local log_file="$3"
    local pass_marker="AROS_OMA_TEST: $CURRENT_TARGET $key PASS"
    local screenshot="$4"

    if ! grep -a -F -q "$pass_marker" "$log_file"; then
        tail -120 "$log_file" || true
        die "AROS $CURRENT_TARGET $key failed or timed out; see $log_file"
    fi
    if [ "$HEADLESS" -eq 0 ]; then
        [ -s "$screenshot" ] || die "missing screenshot for AROS $CURRENT_TARGET $key"
    fi

    {
        printf 'target=%s\n' "$CURRENT_TARGET"
        printf 'game=%s\n' "$key"
        printf 'stability_seconds=%s\n' "$STABILITY_SECONDS"
        printf 'marker=%s\n' "$pass_marker"
        if [ "$HEADLESS" -eq 0 ]; then
            printf 'screenshot=%s\n' "$screenshot"
        fi
    } > "$done_file"
}

game_is_complete() {
    local done_file="$2"
    local key="$1"

    [ "$RESUME" -eq 1 ] || return 1
    [ -f "$done_file" ] || return 1
    grep -F -x -q "target=$CURRENT_TARGET" "$done_file" || return 1
    grep -F -x -q "game=$key" "$done_file" || return 1
    grep -F -x -q "stability_seconds=$STABILITY_SECONDS" "$done_file"
}

if [ "$BUILD_GAMES" -eq 1 ]; then
    build_arguments=(--targets "$(IFS=,; echo "${SELECTED_TARGETS[*]}")")
    if [ "${#SELECTED_GAMES[@]}" -gt 0 ]; then
        build_arguments+=(--games "$(IFS=,; echo "${SELECTED_GAMES[*]}")")
    fi
    "$SCRIPT_DIR/aros-build-oma-games.sh" "${build_arguments[@]}"
fi

for target in "${SELECTED_TARGETS[@]}"; do
    CURRENT_TARGET="$target"
    map_target "$target"
    load_manifest "$target"
    mkdir -p \
        "$EVIDENCE_ROOT/$target/helpers" \
        "$EVIDENCE_ROOT/$target/logs" \
        "$EVIDENCE_ROOT/$target/screenshots"
    build_log_relay "$target"
    if [ "$target" = arm ]; then
        prepare_arm_base_image
    fi

    passed_count=0
    for key in "${TARGET_GAME_KEYS[@]}"; do
        game_directory="${TARGET_GAME_DIRECTORIES[$key]}"
        done_file="$STATE_ROOT/$target-$key.done"
        log_file="$EVIDENCE_ROOT/$target/logs/$key.serial.log"
        screenshot="$EVIDENCE_ROOT/$target/screenshots/$key.png"

        if game_is_complete "$key" "$done_file"; then
            echo "==> skipping passed AROS $target game: $key"
            passed_count=$((passed_count + 1))
            continue
        fi

        msg "launch-testing $key on AROS $target"
        : > "$log_file"
        case "$target" in
            x86_64) run_x86_game "$key" "$game_directory" "$log_file" "$screenshot" ;;
            arm) run_arm_game "$key" "$game_directory" "$log_file" "$screenshot" ;;
            m68k) run_m68k_game "$key" "$game_directory" "$log_file" "$screenshot" ;;
        esac
        validate_game_log "$key" "$done_file" "$log_file" "$screenshot"
        echo "==> AROS $target $key launch PASS"
        passed_count=$((passed_count + 1))
    done

    echo "==> AROS $target OMA launch tests passed: $passed_count/${#TARGET_GAME_KEYS[@]}"
done

echo
echo "==> selected AROS OMA launch tests passed for: ${SELECTED_TARGETS[*]}"
echo "    Evidence: $EVIDENCE_ROOT/{x86_64,m68k,arm}"

# end of aros-run-oma-games.sh
