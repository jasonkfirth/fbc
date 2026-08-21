#!/usr/bin/env bash
#
# Project: FreeBASIC AROS Exampleageddon workflow
# ------------------------------------------------
#
# File: aros-run-exampleageddon.sh
#
# Purpose:
#
#     Compile the complete example tree and run every self-contained example
#     on AROS x86_64, m68k, and ARM.
#
# Responsibilities:
#
#     - refresh each target's rtlib, gfxlib2, and sfxlib
#     - produce a separate complete compile inventory per architecture
#     - convert only AROS m68k guest executables from ELF to Amiga Hunk
#     - execute self-contained examples sequentially in bounded batches
#     - preserve per-example return codes and per-batch emulator logs
#     - merge compile and guest evidence into CSV and Markdown reports
#
# This file intentionally does NOT contain:
#
#     - AROS SDK or emulator construction
#     - Exampleageddon classification policy
#     - generic m68k compiler or ABI policy
#     - interactive example automation
#
# Architecture policy:
#
#     Generic m68k compilation remains in the shared compiler backend. The
#     68000 soft-float baseline, Hunk conversion, AROS SDK paths, and FS-UAE
#     transport below are AROS-specific concerns.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults and process state
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
OUTPUT_ROOT="${AROS_EXAMPLEAGEDDON_OUTDIR:-$AROS_ROOT/exampleageddon}"
TARGETS="${AROS_TARGETS:-x86_64,m68k,arm}"

BATCH_SIZE_OVERRIDE=""
COMPILE_ONLY=0
COMPILE_TIMEOUT=180
JOBS=""
RESUME=0
SKIP_COMPILE=0
SKIP_LIBS=0
TIMEOUT_SECONDS=300
X86_MEMORY_MIB=2048
M68K_BOOT_ATTEMPTS=3
START_BATCH=1

CURRENT_TARGET=""
M68K_EMULATOR_PID=""
M68K_READER_PID=""
QEMU_PID=""
TEMP_STAGE=""

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

    if [ -n "$TEMP_STAGE" ] &&
       [[ "$TEMP_STAGE" == "$OUTPUT_ROOT"/*/.stage.* ]] &&
       [ -d "$TEMP_STAGE" ]; then
        rm -rf -- "$TEMP_STAGE"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-run-exampleageddon.sh [options]

Options:
  --targets LIST       Comma-separated x86_64,m68k,arm list. Default: all
  --batch-size N       Override target batch size. Defaults: x86_64/ARM=25,
                       m68k=10
  --start-batch N      Begin guest execution at this one-based batch number
  --compile-timeout N  Per-example compile timeout. Default: 180 seconds
  --timeout N          Per-batch emulator timeout. Default: 300 seconds
  --memory MIB         x86_64 QEMU memory. Default: 2048; minimum: 256
  --compile-only       Compile and classify without launching emulators
  --skip-compile       Reuse each target's existing compile inventory
  --skip-libs          Reuse existing target libraries
  --resume             Reuse completed batch logs with matching manifests
  --jobs N             Parallel compiler jobs. Default: detected CPU count
  --aros-root DIR      AROS SDK/build workspace. Default: out/aros
  --out-dir DIR        Results directory. Default: out/aros/exampleageddon
  -h, --help           Show this help

Reports, CSV results, batch logs, and native binaries are separated below the
output directory by architecture.
EOF
}

trap cleanup EXIT

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
        --batch-size)
            require_value "$1" "${2-}"
            BATCH_SIZE_OVERRIDE="$2"
            shift 2
            ;;
        --start-batch)
            require_value "$1" "${2-}"
            START_BATCH="$2"
            shift 2
            ;;
        --compile-timeout)
            require_value "$1" "${2-}"
            COMPILE_TIMEOUT="$2"
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
        --compile-only)
            COMPILE_ONLY=1
            shift
            ;;
        --skip-compile)
            SKIP_COMPILE=1
            shift
            ;;
        --skip-libs)
            SKIP_LIBS=1
            shift
            ;;
        --resume)
            RESUME=1
            shift
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        --aros-root)
            require_value "$1" "${2-}"
            AROS_ROOT="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
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

for positive_integer in \
    "$COMPILE_TIMEOUT" "$TIMEOUT_SECONDS" "$X86_MEMORY_MIB" "$JOBS" \
    "$START_BATCH"; do
    case "$positive_integer" in
        ''|*[!0-9]*|0) die "timeouts, memory, and jobs must be positive integers" ;;
    esac
done
if [ -n "$BATCH_SIZE_OVERRIDE" ]; then
    case "$BATCH_SIZE_OVERRIDE" in
        ''|*[!0-9]*|0) die "--batch-size must be a positive integer" ;;
    esac
fi
[ "$X86_MEMORY_MIB" -ge 256 ] || die "--memory must be at least 256 MiB"

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

##############################################################################
# Target mapping and prerequisites
##############################################################################

map_target() {
    local target="$1"

    case "$target" in
        x86_64)
            MAP_FBC_TARGET="aros-x86_64"
            MAP_TARGET_TRIPLET="x86_64-aros"
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-pc-x86_64"
            MAP_TOOL_PREFIX="x86_64-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-pc-x86_64"
            ;;
        m68k)
            MAP_FBC_TARGET="aros-m68k"
            MAP_TARGET_TRIPLET="m68k-aros"
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-amiga-m68k"
            MAP_TOOL_PREFIX="m68k-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-amiga-m68k"
            ;;
        arm)
            MAP_FBC_TARGET="aros-arm"
            MAP_TARGET_TRIPLET="arm-aros"
            MAP_TOOLCHAIN="$AROS_ROOT/toolchain-raspi-armhf"
            MAP_TOOL_PREFIX="arm-aros"
            MAP_AROS_BUILD="$AROS_ROOT/build-raspi-armhf"
            ;;
    esac
}

default_batch_size() {
    case "$1" in
        m68k) printf '%s\n' 10 ;;
        x86_64|arm) printf '%s\n' 25 ;;
    esac
}

for tool in awk cp file find grep make mkfifo python3 sha256sum sort timeout xorriso; do
    require_command "$tool"
done
if [ "$COMPILE_ONLY" -eq 0 ]; then
    require_command mcopy
    require_command mmd
    require_command mkfs.vfat
    require_command qemu-system-arm
    require_command qemu-system-x86_64
    require_command sfdisk
    require_command fs-uae
fi

FBC="$ROOT/bin/fbc"
[ -x "$FBC" ] || die "host FreeBASIC compiler not found: $FBC"
mkdir -p "$OUTPUT_ROOT"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
[ "$OUTPUT_ROOT" != "/" ] || die "refusing to use / as the output root"

for target in "${SELECTED_TARGETS[@]}"; do
    map_target "$target"
    [ -x "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" ] ||
        die "AROS $target cross compiler not found"
    if [ "$target" = m68k ]; then
        [ -x "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" ] ||
            die "AROS m68k elf2hunk is missing"
    fi
done

##############################################################################
# Compile inventories and manifests
##############################################################################

refresh_target_libraries() {
    local target="$1"

    [ "$SKIP_LIBS" -eq 0 ] || return 0
    map_target "$target"
    msg "refreshing AROS $target rtlib, gfxlib2, and sfxlib"
    env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
        make -s -C "$ROOT" -j"$JOBS" \
            TARGET_TRIPLET="$MAP_TARGET_TRIPLET" \
            BUILD_FBC="$FBC" \
            libs
}

compile_inventory() {
    local target="$1"
    local target_output="$OUTPUT_ROOT/$target"

    map_target "$target"
    mkdir -p "$target_output"
    if [ "$SKIP_COMPILE" -eq 0 ]; then
        msg "compiling all examples for AROS $target"
        env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
            python3 "$SCRIPT_DIR/exampleageddon-freebasic.py" \
                --root "$ROOT" \
                --outdir "$target_output" \
                --fbc "$FBC -target $MAP_FBC_TARGET -mt -i $ROOT/inc/aros" \
                --prefix "$ROOT" \
                --include-dir "$ROOT/inc" \
                --target-os aros \
                --jobs "$JOBS" \
                --compile-timeout "$COMPILE_TIMEOUT" \
                --no-run \
                --fail-on-self-contained
    fi

    [ -s "$target_output/results.csv" ] ||
        die "AROS $target compile results are missing"

    python3 - "$target_output/results.csv" "$target_output" <<'PY'
import csv
import sys
from pathlib import Path

results_path = Path(sys.argv[1])
target_output = Path(sys.argv[2])
manifest_path = target_output / "manifest.tsv"

with results_path.open(newline="", encoding="utf-8") as stream:
    rows = list(csv.DictReader(stream))

selected = [
    row
    for row in rows
    if row["group"] == "self-contained" and row["compile_status"] == "pass"
]

with manifest_path.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(("index", "path", "output", "compile_directory"))
    for index, row in enumerate(selected, start=1):
        output = Path(row["output"])
        compile_directory = target_output / "work" / output.name / "compile"
        writer.writerow((f"{index:04d}", row["path"], output, compile_directory))
PY
}

##############################################################################
# Batch staging and startup generation
##############################################################################

stage_case() {
    local case_id="$1"
    local source_path="$2"
    local binary="$3"
    local compile_directory="$4"
    local case_directory="$TEMP_STAGE/$case_id"

    [ -s "$binary" ] || die "compiled example is missing: $binary"
    [ -d "$compile_directory" ] ||
        die "example resource directory is missing: $compile_directory"

    mkdir -p "$case_directory"
    cp -a "$compile_directory/." "$case_directory/"
    find "$case_directory" -type f \
        \( -name '*.o' -o -name '*.obj' -o -name '*.asm' \) -delete

    if [ "$CURRENT_TARGET" = m68k ]; then
        "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" \
            "$binary" "$case_directory/runner"
        [[ "$(file -b "$case_directory/runner")" == *"AmigaOS loadseg"* ]] ||
            die "m68k example conversion did not produce a Hunk executable"
    else
        cp "$binary" "$case_directory/runner"
    fi
    chmod u+x "$case_directory/runner"
    printf '%s\t%s\n' "$case_id" "$source_path" >> "$TEMP_STAGE/cases.tsv"
}

install_batch_stage() {
    local row
    local case_index
    local source_path
    local binary
    local compile_directory

    TEMP_STAGE="$(mktemp -d "$OUTPUT_ROOT/$CURRENT_TARGET/.stage.XXXXXX")"
    : > "$TEMP_STAGE/cases.tsv"
    for row in "${BATCH_ROWS[@]}"; do
        IFS=$'\t' read -r \
            case_index source_path binary compile_directory <<< "$row"
        stage_case "case$case_index" "$source_path" "$binary" "$compile_directory"
    done
}

write_startup() {
    local batch_id="$1"
    local destination="$2"
    local case_index
    local source_path
    local binary
    local compile_directory
    local guest_root="SYS:Tests/Examples"
    local row

    if [ "$CURRENT_TARGET" = m68k ]; then
        guest_root="ExampleDrive:"
    fi

    cat > "$destination" <<EOF
FailAt 2147483647
MakeDir RAM:ExampleT
MakeDir RAM:ExampleENV
Assign T: RAM:ExampleT
Assign ENV: RAM:ExampleENV
EOF

    for row in "${BATCH_ROWS[@]}"; do
        IFS=$'\t' read -r \
            case_index source_path binary compile_directory <<< "$row"
        if [ "$CURRENT_TARGET" = m68k ]; then
            cat >> "$destination" <<EOF
CD ${guest_root}case$case_index
Stack 1048576
runner >NIL:
Set ExampleRC \$RC
Echo "EXAMPLE_RESULT_$case_index \$ExampleRC" >ExampleDrive:results/case$case_index.log
EOF
        else
            cat >> "$destination" <<EOF
CD ${guest_root}case$case_index
Stack 1048576
runner >NIL:
Set ExampleRC \$RC
SYS:Tests/Examples/KEcho "EXAMPLE_RESULT_$case_index \$ExampleRC"
EOF
        fi
    done

    if [ "$CURRENT_TARGET" = m68k ]; then
        cat >> "$destination" <<EOF
Echo "EXAMPLEAGEDDON_DONE_$batch_id" >ExampleDrive:done.log
Wait 2
C:Shutdown
EOF
    else
        cat >> "$destination" <<EOF
SYS:Tests/Examples/KEcho "EXAMPLEAGEDDON_DONE_$batch_id"
Wait 2
C:Shutdown
EOF
    fi
}

write_x86_grub_config() {
    local destination="$1"

    cat > "$destination" <<'EOF'
set timeout=0
set default=0

menuentry "FreeBASIC AROS Exampleageddon" {
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

##############################################################################
# Guest execution transports
##############################################################################

wait_for_done_marker() {
    local batch_id="$1"
    local elapsed=0
    local host_marker="$4"
    local log_file="$2"
    local process_id="$3"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        if grep -a -F -q "EXAMPLEAGEDDON_DONE_$batch_id" \
            "$log_file" 2>/dev/null; then
            return 0
        fi
        if [ -n "$host_marker" ] &&
           grep -a -F -q "EXAMPLEAGEDDON_DONE_$batch_id" \
               "$host_marker" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$process_id" 2>/dev/null; then
            return 1
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

run_x86_batch() {
    local base_iso="$MAP_AROS_BUILD/distfiles/aros-pc-x86_64.iso"
    local batch_id="$1"
    local current_iso="$OUTPUT_ROOT/x86_64/current.iso"
    local grub_config="$OUTPUT_ROOT/x86_64/x86_64-grub.cfg"
    local log_file="$2"
    local startup="$OUTPUT_ROOT/x86_64/$batch_id-Startup-Sequence"

    [ -f "$base_iso" ] || die "x86_64 AROS ISO is missing"
    cp "$MAP_AROS_BUILD/bin/pc-x86_64/AROS/Developer/Debug/KEcho" \
        "$TEMP_STAGE/KEcho"
    write_startup "$batch_id" "$startup"
    write_x86_grub_config "$grub_config"
    cp --reflink=auto --sparse=always "$base_iso" "$current_iso"
    if ! xorriso -indev "$current_iso" -outdev "$current_iso" \
        -boot_image any keep \
        -map "$startup" /S/Startup-Sequence \
        -map "$grub_config" /boot/grub/grub.cfg \
        -map "$TEMP_STAGE" /Tests/Examples \
        -commit -end > "$OUTPUT_ROOT/x86_64/xorriso.log" 2>&1; then
        tail -80 "$OUTPUT_ROOT/x86_64/xorriso.log" >&2
        die "could not construct the x86_64 Exampleageddon ISO"
    fi

    qemu-system-x86_64 \
        -M pc -m "$X86_MEMORY_MIB" \
        -cdrom "$current_iso" -boot d \
        -serial stdio -display none -monitor none \
        -audiodev driver=none,id=example_audio \
        -device AC97,audiodev=example_audio \
        -no-reboot > "$log_file" 2>&1 &
    QEMU_PID="$!"
    wait_for_done_marker "$batch_id" "$log_file" "$QEMU_PID" "" || true
    stop_qemu
}

prepare_arm_base_image() {
    local aros_system="$MAP_AROS_BUILD/bin/raspi-arm/AROS"
    local base_image="$OUTPUT_ROOT/arm/base.img"
    local boot_architecture="$OUTPUT_ROOT/arm/AROS.boot"

    msg "building reusable 512 MiB ARM Exampleageddon image"
    truncate -s 0 "$base_image"
    truncate -s 512M "$base_image"
    printf '%s\n' 'label: dos' 'unit: sectors' 'first-lba: 2048' \
        '1 : start=2048, size=1046528, type=c, bootable' |
        sfdisk "$base_image" >/dev/null
    mkfs.vfat -F 32 -n AROS --offset=2048 "$base_image" 523264 >/dev/null
    mcopy -i "$base_image@@1M" -spm "$aros_system"/* ::
    mmd -i "$base_image@@1M" ::/S
    mmd -i "$base_image@@1M" ::/Tests
    mmd -i "$base_image@@1M" ::/Tests/Examples
    printf 'arm\n' > "$boot_architecture"
    mcopy -i "$base_image@@1M" "$boot_architecture" ::/AROS.boot
}

run_arm_batch() {
    local aros_system="$MAP_AROS_BUILD/bin/raspi-arm/AROS"
    local base_image="$OUTPUT_ROOT/arm/base.img"
    local batch_id="$1"
    local current_image="$OUTPUT_ROOT/arm/current.img"
    local log_file="$2"
    local startup="$OUTPUT_ROOT/arm/$batch_id-Startup-Sequence"

    cp "$MAP_AROS_BUILD/bin/raspi-arm/AROS/Developer/Debug/KEcho" \
        "$TEMP_STAGE/KEcho"
    write_startup "$batch_id" "$startup"
    cp --reflink=auto --sparse=always "$base_image" "$current_image"
    mcopy -o -i "$current_image@@1M" "$startup" ::/S/Startup-Sequence
    mcopy -o -i "$current_image@@1M" -spm "$TEMP_STAGE"/* ::/Tests/Examples

    qemu-system-arm \
        -M raspi2b -m 1G -nographic -monitor none -serial stdio -no-reboot \
        -kernel "$aros_system/aros-arm-raspi.img" \
        -initrd "$aros_system/aros-arm-bsp.rom" \
        -dtb "$aros_system/bcm2709-rpi-2-b.dtb" \
        -drive file="$current_image",format=raw,if=sd \
        > "$log_file" 2>&1 &
    QEMU_PID="$!"
    wait_for_done_marker "$batch_id" "$log_file" "$QEMU_PID" "" || true
    stop_qemu
}

run_m68k_batch() {
    local base_iso="$MAP_AROS_BUILD/distfiles/aros-amiga-m68k.iso"
    local batch_id="$1"
    local config_file="$OUTPUT_ROOT/m68k/$batch_id.fs-uae"
    local current_iso="$OUTPUT_ROOT/m68k/current.iso"
    local emulator_log="$OUTPUT_ROOT/m68k/logs/$batch_id.fs-uae.log"
    local fifo="$OUTPUT_ROOT/m68k/serial.fifo"
    local host_directory="$OUTPUT_ROOT/m68k/host"
    local log_file="$2"
    local marker_path
    local startup="$OUTPUT_ROOT/m68k/$batch_id-Startup-Sequence"

    [ "$host_directory" = "$OUTPUT_ROOT/m68k/host" ] ||
        die "refusing to replace an unexpected m68k host directory"
    rm -rf -- "$host_directory"
    mkdir -p "$host_directory" "$host_directory/results"
    cp -a "$TEMP_STAGE/." "$host_directory/"
    marker_path="$host_directory/done.log"

    write_startup "$batch_id" "$startup"
    cp --reflink=auto --sparse=always "$base_iso" "$current_iso"
    if ! xorriso -dev "$current_iso" -boot_image any replay \
        -map "$startup" /S/Startup-Sequence \
        -commit -end > "$OUTPUT_ROOT/m68k/xorriso.log" 2>&1; then
        tail -80 "$OUTPUT_ROOT/m68k/xorriso.log" >&2
        die "could not construct the m68k Exampleageddon ISO"
    fi

    cat > "$config_file" <<EOF
[fs-uae]
amiga_model = A4000/040
kickstart_file = $MAP_AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-rom.bin
kickstart_ext_file = $MAP_AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-ext.bin
cdrom_drive_0 = $current_iso
hard_drive_0 = $host_directory
hard_drive_0_label = ExampleDrive
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
    wait_for_done_marker \
        "$batch_id" "$log_file" "$M68K_EMULATOR_PID" "$marker_path" || true
    if [ -f "$marker_path" ]; then
        sleep 2
    fi
    stop_m68k_processes
    if [ -d "$host_directory/results" ]; then
        find "$host_directory/results" -maxdepth 1 -type f -name 'case*.log' \
            -print0 | sort -z |
            while IFS= read -r -d '' result_file; do
                cat "$result_file" >> "$log_file"
            done
    fi
    if [ -f "$marker_path" ]; then
        cat "$marker_path" >> "$log_file"
    fi
}

##############################################################################
# Result validation and orchestration
##############################################################################

record_batch_results() {
    local batch_id="$1"
    local default_status="$3"
    local log_file="$2"
    local row
    local case_index
    local source_path
    local binary
    local compile_directory
    local result_line
    local return_code
    local run_status

    for row in "${BATCH_ROWS[@]}"; do
        IFS=$'\t' read -r \
            case_index source_path binary compile_directory <<< "$row"
        result_line="$(
            grep -a -o "EXAMPLE_RESULT_$case_index [^[:space:]]*" \
                "$log_file" | tail -n 1 || true
        )"
        if [ -n "$result_line" ]; then
            return_code="${result_line#EXAMPLE_RESULT_$case_index }"
            if [ "$return_code" = 0 ]; then
                run_status="pass"
            else
                run_status="fail"
            fi
        else
            return_code=""
            run_status="$default_status"
        fi
        printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
            "$source_path" "$run_status" "$return_code" "$batch_id" \
            "logs/$batch_id.serial.log" "" >> "$RUN_STATUS"
    done
}

run_target_batches() {
    local attempt
    local batch_id
    local batch_index
    local batch_key
    local batch_size
    local batch_start
    local done_file
    local log_file
    local target="$1"
    local target_output="$OUTPUT_ROOT/$target"
    local total_batches
    local transport_version="result-transport-v1"

    CURRENT_TARGET="$target"
    map_target "$target"
    if [ "$target" = m68k ]; then
        transport_version="result-transport-v2"
    fi
    mapfile -t MANIFEST_ROWS < <(tail -n +2 "$target_output/manifest.tsv")
    [ "${#MANIFEST_ROWS[@]}" -gt 0 ] ||
        die "AROS $target manifest contains no self-contained examples"

    batch_size="${BATCH_SIZE_OVERRIDE:-$(default_batch_size "$target")}"
    total_batches=$(( (${#MANIFEST_ROWS[@]} + batch_size - 1) / batch_size ))
    [ "$START_BATCH" -le "$total_batches" ] ||
        die "--start-batch exceeds AROS $target's $total_batches batches"
    mkdir -p "$target_output/logs" "$target_output/state"
    RUN_STATUS="$target_output/run-status.tsv"
    printf 'path\trun_status\treturn_code\tbatch\trun_log\trun_seconds\n' \
        > "$RUN_STATUS"

    if [ "$target" = arm ]; then
        prepare_arm_base_image
    fi

    for ((batch_index = 0; batch_index < total_batches; batch_index++)); do
        batch_id="$(printf 'batch%03d' "$((batch_index + 1))")"
        batch_start=$((batch_index * batch_size))
        BATCH_ROWS=("${MANIFEST_ROWS[@]:batch_start:batch_size}")
        batch_key="$(printf '%s\n' "$transport_version" "${BATCH_ROWS[@]}" |
            sha256sum | awk '{print $1}')"
        done_file="$target_output/state/$batch_id.done"
        log_file="$target_output/logs/$batch_id.serial.log"

        if [ "$((batch_index + 1))" -lt "$START_BATCH" ]; then
            [ -f "$done_file" ] ||
                die "AROS $target $batch_id has no result before --start-batch"
            record_batch_results "$batch_id" "$log_file" fail
            continue
        fi

        if [ "$RESUME" -eq 1 ] && [ -f "$done_file" ] &&
           grep -F -x -q "key=$batch_key" "$done_file" &&
           grep -a -F -q "EXAMPLEAGEDDON_DONE_$batch_id" "$log_file"; then
            echo "==> reusing AROS $target $batch_id"
            record_batch_results "$batch_id" "$log_file" fail
            continue
        fi

        msg "staging AROS $target $batch_id of $total_batches"
        install_batch_stage
        : > "$log_file"
        case "$target" in
            x86_64) run_x86_batch "$batch_id" "$log_file" ;;
            arm) run_arm_batch "$batch_id" "$log_file" ;;
            m68k)
                for ((attempt = 1; attempt <= M68K_BOOT_ATTEMPTS; attempt++)); do
                    run_m68k_batch "$batch_id" "$log_file"
                    if grep -a -F -q \
                        "EXAMPLEAGEDDON_DONE_$batch_id" "$log_file"; then
                        break
                    fi
                    if [ "$attempt" -lt "$M68K_BOOT_ATTEMPTS" ]; then
                        echo "==> retrying AROS m68k $batch_id after boot " \
                            "$attempt of $M68K_BOOT_ATTEMPTS" >&2
                    fi
                done
                ;;
        esac

        if grep -a -F -q "EXAMPLEAGEDDON_DONE_$batch_id" "$log_file"; then
            printf 'key=%s\n' "$batch_key" > "$done_file"
            record_batch_results "$batch_id" "$log_file" fail
        else
            echo "==> AROS $target $batch_id timed out; continuing" >&2
            record_batch_results "$batch_id" "$log_file" timeout
        fi

        rm -rf -- "$TEMP_STAGE"
        TEMP_STAGE=""
    done

    if python3 "$SCRIPT_DIR/aros-exampleageddon-report.py" \
        --target "$target" \
        --compile-results "$target_output/results.csv" \
        --run-status "$RUN_STATUS" \
        --output-csv "$target_output/aros-results.csv" \
        --report "$target_output/aros-report.md"; then
        echo "==> AROS $target Exampleageddon passed: ${#MANIFEST_ROWS[@]} examples"
    else
        die "AROS $target Exampleageddon found self-contained problems"
    fi
}

for target in "${SELECTED_TARGETS[@]}"; do
    refresh_target_libraries "$target"
    compile_inventory "$target"
done

if [ "$COMPILE_ONLY" -eq 1 ]; then
    echo
    echo "==> AROS Exampleageddon inventories compiled for: ${SELECTED_TARGETS[*]}"
    exit 0
fi

for target in "${SELECTED_TARGETS[@]}"; do
    run_target_batches "$target"
done

echo
echo "==> AROS Exampleageddon passed for: ${SELECTED_TARGETS[*]}"
echo "    Reports: $OUTPUT_ROOT/{x86_64,m68k,arm}/aros-report.md"

# end of aros-run-exampleageddon.sh
