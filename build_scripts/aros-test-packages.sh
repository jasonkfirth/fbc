#!/usr/bin/env bash
#
# Project: FreeBASIC AROS release workflow
# ----------------------------------------
#
# File: aros-test-packages.sh
#
# Purpose:
#
#     Install the published AROS package pair in clean emulated systems and
#     prove that the installed compiler can build and run a BASIC program.
#
# Responsibilities:
#
#     - remove the build tree's Developer directory from each boot medium
#     - install Developer.pkg and FreeBASIC.pkg with the native C:Unpack tool
#     - run the installed fbc, GCC, assembler, linker, and m68k Hunk converter
#     - retain guest logs, result markers, executable details, and checksums
#
# This file intentionally does NOT contain:
#
#     - AROS source, SDK, or FreeBASIC construction
#     - host-side package extraction as a substitute for guest installation
#     - fbctests, Exampleageddon, or OMA game testing
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
DEVELOPER_PACKAGE_DIR="${AROS_DEVELOPER_PACKAGE_DIR:-$AROS_ROOT/packages}"
FREEBASIC_PACKAGE_DIR="${AROS_FREEBASIC_PACKAGE_DIR:-$AROS_ROOT/packages}"
OUTPUT_ROOT="${AROS_PACKAGE_TEST_OUTDIR:-$AROS_ROOT/package-test}"
TARGETS="${AROS_TARGETS:-x86_64,m68k,arm}"
TIMEOUT_SECONDS="${AROS_PACKAGE_TEST_TIMEOUT:-3600}"
X86_MEMORY_MIB="${AROS_PACKAGE_TEST_X86_MEMORY:-2048}"
DISK_SIZE_MIB="${AROS_PACKAGE_TEST_DISK_SIZE:-4096}"
KEEP_WORK=0

WORK_ROOT=""
RUN_ROOT=""
QEMU_PID=""
M68K_READER_PID=""
M68K_EMULATOR_PID=""

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

require_file() {
    [ -f "$1" ] || die "required file not found: $1"
}

stop_process() {
    local process_id="$1"
    local waited=0

    [ -n "$process_id" ] || return 0
    if kill -0 "$process_id" 2>/dev/null; then
        kill "$process_id" 2>/dev/null || true
    fi
    while kill -0 "$process_id" 2>/dev/null && [ "$waited" -lt 5 ]; do
        sleep 1
        waited=$((waited + 1))
    done
    if kill -0 "$process_id" 2>/dev/null; then
        kill -KILL "$process_id" 2>/dev/null || true
    fi
    wait "$process_id" 2>/dev/null || true
}

wait_for_natural_exit() {
    local process_id="$1"
    local waited=0

    while kill -0 "$process_id" 2>/dev/null && [ "$waited" -lt 15 ]; do
        sleep 1
        waited=$((waited + 1))
    done
}

cleanup() {
    local status=$?

    stop_process "$QEMU_PID"
    stop_process "$M68K_EMULATOR_PID"
    stop_process "$M68K_READER_PID"

    if [ -n "$WORK_ROOT" ] &&
       [[ "$WORK_ROOT" == "$OUTPUT_ROOT"/.package-test.* ]] &&
       [ -d "$WORK_ROOT" ]; then
        if [ "$KEEP_WORK" -eq 0 ] && [ "$status" -eq 0 ]; then
            rm -rf -- "$WORK_ROOT"
        else
            echo "Package test staging retained at: $WORK_ROOT" >&2
        fi
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-test-packages.sh [options]

Options:
  --aros-root DIR       AROS workspace. Default: out/aros
  --developer-dir DIR   Directory containing Developer packages.
  --freebasic-dir DIR   Directory containing FreeBASIC compiler packages.
  --output-dir DIR      Persistent evidence directory.
  --targets LIST        Comma-separated x86_64,m68k,arm list. Default: all
  --timeout SEC         Per-emulator timeout. Default: 3600
  --memory MIB          x86_64 QEMU memory. Default: 2048
  --disk-size MIB       Writable FAT image size. Default: 4096
  --keep-work           Retain temporary disk images and host volumes.
  -h, --help            Show this help.

The Developer and FreeBASIC package directories may be different. This is
useful when qualifying a newly built Developer package against the compiler
package already published in the repository.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --aros-root)
            require_value "$1" "${2-}"
            AROS_ROOT="$2"
            shift 2
            ;;
        --developer-dir)
            require_value "$1" "${2-}"
            DEVELOPER_PACKAGE_DIR="$2"
            shift 2
            ;;
        --freebasic-dir)
            require_value "$1" "${2-}"
            FREEBASIC_PACKAGE_DIR="$2"
            shift 2
            ;;
        --output-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --targets)
            require_value "$1" "${2-}"
            TARGETS="$2"
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
        --disk-size)
            require_value "$1" "${2-}"
            DISK_SIZE_MIB="$2"
            shift 2
            ;;
        --keep-work)
            KEEP_WORK=1
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

for numeric_value in "$TIMEOUT_SECONDS" "$X86_MEMORY_MIB" "$DISK_SIZE_MIB"; do
    case "$numeric_value" in
        ''|*[!0-9]*|0) die "timeout, memory, and disk size must be positive integers" ;;
    esac
done

[ "$DISK_SIZE_MIB" -ge 2048 ] || die "--disk-size must be at least 2048 MiB"

IFS=',' read -r -a SELECTED_TARGETS <<< "$TARGETS"
[ "${#SELECTED_TARGETS[@]}" -gt 0 ] || die "no AROS targets selected"
for target in "${SELECTED_TARGETS[@]}"; do
    case "$target" in
        x86_64|m68k|arm) ;;
        *) die "unsupported AROS target: $target" ;;
    esac
done

##############################################################################
# Host prerequisites and input selection
##############################################################################

for command_name in cp file find grep mcopy mdir mkfs.vfat mktemp \
    qemu-system-arm qemu-system-x86_64 rsync sed sfdisk sha256sum \
    timeout truncate xorriso
do
    require_command "$command_name"
done

if printf '%s\n' "${SELECTED_TARGETS[@]}" | grep -q '^m68k$'; then
    require_command fs-uae
fi

find_one_package() {
    local directory="$1"
    local pattern="$2"
    local exclude_pattern="${3-}"
    local -a matches=()

    if [ -n "$exclude_pattern" ]; then
        while IFS= read -r package; do
            matches+=("$package")
        done < <(find "$directory" -maxdepth 1 -type f -name "$pattern" \
            ! -name "$exclude_pattern" -print | sort)
    else
        while IFS= read -r package; do
            matches+=("$package")
        done < <(find "$directory" -maxdepth 1 -type f -name "$pattern" \
            -print | sort)
    fi

    [ "${#matches[@]}" -eq 1 ] ||
        die "expected one $pattern below $directory, found ${#matches[@]}"
    printf '%s\n' "${matches[0]}"
}

map_target() {
    local target="$1"

    case "$target" in
        x86_64)
            BUILD_ROOT="$AROS_ROOT/build-pc-x86_64"
            SYSROOT_KEY="pc-x86_64"
            TOOLCHAIN="$AROS_ROOT/toolchain-pc-x86_64"
            TOOL_PREFIX="x86_64-aros"
            BASE_ISO="$BUILD_ROOT/distfiles/aros-pc-x86_64.iso"
            ;;
        arm)
            BUILD_ROOT="$AROS_ROOT/build-raspi-armhf"
            SYSROOT_KEY="raspi-arm"
            TOOLCHAIN="$AROS_ROOT/toolchain-raspi-armhf"
            TOOL_PREFIX="arm-aros"
            BASE_ISO=""
            ;;
        m68k)
            BUILD_ROOT="$AROS_ROOT/build-amiga-m68k"
            SYSROOT_KEY="amiga-m68k"
            TOOLCHAIN="$AROS_ROOT/toolchain-amiga-m68k"
            TOOL_PREFIX="m68k-aros"
            BASE_ISO="$BUILD_ROOT/distfiles/aros-amiga-m68k.iso"
            ;;
    esac

    SYSROOT="$BUILD_ROOT/bin/$SYSROOT_KEY/AROS"
    DEVELOPER_PACKAGE="$(find_one_package \
        "$DEVELOPER_PACKAGE_DIR" "FreeBASIC-Developer-*-aros-$target.pkg")"
    FREEBASIC_PACKAGE="$(find_one_package \
        "$FREEBASIC_PACKAGE_DIR" "FreeBASIC-*-aros-$target.pkg" \
        'FreeBASIC-Developer-*')"

    require_file "$DEVELOPER_PACKAGE"
    require_file "$FREEBASIC_PACKAGE"
}

build_guest_helper() {
    local target="$1"
    local source="$2"
    local destination="$3"
    local temporary_elf="$destination.elf"

    case "$target" in
        x86_64)
            "$TOOLCHAIN/$TOOL_PREFIX-gcc" -O2 -fno-common \
                "$source" -o "$destination" -ldebug
            ;;
        arm)
            "$TOOLCHAIN/$TOOL_PREFIX-gcc" -O2 -fno-common \
                -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4 \
                "$source" -o "$destination" -ldebug
            ;;
        m68k)
            "$TOOLCHAIN/$TOOL_PREFIX-gcc" -O2 -fno-common \
                -march=68000 -msoft-float \
                "$source" -o "$temporary_elf" -ldebug
            "$BUILD_ROOT/bin/linux-x86_64/tools/elf2hunk" \
                "$temporary_elf" "$destination"
            rm -f -- "$temporary_elf"
            ;;
    esac

    require_file "$destination"
}

build_serial_relay() {
    build_guest_helper "$1" "$ROOT/tests/aros/log-relay.c" "$2"
}

build_file_copy() {
    build_guest_helper "$1" "$ROOT/tests/aros/file-copy.c" "$2"
}

##############################################################################
# Guest control files
##############################################################################

write_smoke_program() {
    local destination="$1"

    cat > "$destination" <<'EOF'
''
'' FreeBASIC AROS installed-package proof
'' --------------------------------------
''
'' The host checks both this marker and the distinctive return status. This
'' prevents a stale executable or an fbc command that did not really run from
'' being accepted as a successful package installation.
''

const EXPECTED_RESULT = 73

open "FBCINST:program.ok" for output as #1
print #1, "FREEBASIC_INSTALLED_PACKAGE_PROGRAM_EXECUTED"
close #1

end EXPECTED_RESULT

'' end of package-smoke.bas
EOF
}

write_startup() {
    local target="$1"
    local install_volume="$2"
    local destination="$3"
    local marker_assign=""
    local m68k_setup=""
    local compiler_path_command='Path ADD FreeBASIC:bin'
    local compile_directory='RAM:'
    local compile_file_prefix='RAM:'
    local arm_tool_environment=""
    local compiler_stack=1048576
    local fbc_command='FreeBASIC:bin/fbc'

    if [ "$target" = arm ]; then
        marker_assign='Assign FBCINST: SYS:'
        compiler_path_command=""
        compiler_stack=16777216
        fbc_command='SYS:FreeBASIC/bin/fbc'
        arm_tool_environment="$(cat <<'EOF'
SetEnv GCC SYS:Developer/bin/gcc
SetEnv AS SYS:Developer/bin/as
SetEnv LD SYS:Developer/bin/ld
SetEnv AR SYS:Developer/bin/ar
SetEnv GCC_EXEC_PREFIX SYS:Developer/libexec/gcc/arm-aros/6.5.0/
SetEnv COMPILER_PATH SYS:Developer/bin
EOF
)"
    fi

    if [ "$target" = m68k ]; then
        compile_directory='RAM:NativeBuild'
        compile_file_prefix='RAM:NativeBuild/'
        m68k_setup="$(cat <<'EOF'
MakeDir RAM:NativeBuild
MakeDir RAM:NativeBuild/T
MakeDir RAM:NativeBuild/ENV
MakeDir RAM:NativeBuild/Developer
MakeDir RAM:NativeBuild/Developer/bin
MakeDir RAM:NativeBuild/Developer/lib
MakeDir RAM:NativeBuild/Developer/libexec
MakeDir RAM:NativeBuild/Developer/libexec/gcc
MakeDir RAM:NativeBuild/Developer/libexec/gcc/m68k-aros
MakeDir RAM:NativeBuild/Developer/libexec/gcc/m68k-aros/6.5.0
MakeDir RAM:NativeBuild/FreeBASIC
MakeDir RAM:NativeBuild/FreeBASIC/bin
MakeDir RAM:NativeBuild/FreeBASIC/lib
Assign T: RAM:NativeBuild/T
Assign ENV: RAM:NativeBuild/ENV
Copy Developer:bin/gcc RAM:NativeBuild/Developer/bin/gcc CLONE QUIET
Copy Developer:bin/as RAM:NativeBuild/Developer/bin/as CLONE QUIET
Copy Developer:bin/ld RAM:NativeBuild/Developer/bin/ld CLONE QUIET
Copy Developer:bin/ar RAM:NativeBuild/Developer/bin/ar CLONE QUIET
Copy Developer:bin/collect-aros RAM:NativeBuild/Developer/bin/collect-aros CLONE QUIET
Copy Developer:bin/elf2hunk RAM:NativeBuild/Developer/bin/elf2hunk CLONE QUIET
Copy Developer:lib/#?.a RAM:NativeBuild/Developer/lib CLONE QUIET
Copy Developer:lib/#?.o RAM:NativeBuild/Developer/lib CLONE QUIET
Copy Developer:lib/gcc/m68k-aros/6.5.0/libgcc.a RAM:NativeBuild/Developer/lib/libgcc.a CLONE QUIET
Copy Developer:libexec/gcc/m68k-aros/6.5.0/cc1 RAM:NativeBuild/Developer/libexec/gcc/m68k-aros/6.5.0/cc1 CLONE QUIET
Copy FreeBASIC:bin RAM:NativeBuild/FreeBASIC/bin ALL CLONE QUIET
Copy FreeBASIC:lib RAM:NativeBuild/FreeBASIC/lib ALL CLONE QUIET
Assign Developer: RAM:NativeBuild/Developer
Assign FreeBASIC: RAM:NativeBuild/FreeBASIC
Path C: SYS:System S: SYS:Prefs SYS:Tools SYS:Utilities Developer:bin FreeBASIC:bin QUIET
SetEnv GCC Developer:bin/gcc
SetEnv AS Developer:bin/as
SetEnv LD Developer:bin/ld
SetEnv AR Developer:bin/ar
SetEnv ELF2HUNK Developer:bin/elf2hunk
SetEnv GCC_EXEC_PREFIX Developer:libexec/gcc/m68k-aros/6.5.0/
SetEnv COMPILER_PATH Developer:bin
SetEnv FBC_AROS_NATIVE_LINK_PREFIX "RAM Disk:NativeBuild/Developer/lib/"
EOF
)"
    fi

    cat > "$destination" <<EOF
;=============================================================================
;
; FreeBASIC AROS $target clean package installation test
;
; The boot medium has no Developer or FreeBASIC tree. Both trees used below
; must therefore have been created by C:Unpack during this boot.
;
;=============================================================================

FailAt 2147483647

If NOT EXISTS "RAM:T"
    MakeDir "RAM:T"
EndIf
If NOT EXISTS "RAM:ENV"
    MakeDir "RAM:ENV"
EndIf
Assign T: RAM:T
Assign ENV: RAM:ENV
Assign KEYMAPS: SYS:Devs/Keymaps
Assign LOCALE: SYS:Locale
Assign LIBS: SYS:Classes ADD
Assign FONTS: SYS:Fonts
Assign WANDERER: SYS:System/Wanderer
Path C: SYS:System S: SYS:Prefs SYS:Tools SYS:Utilities QUIET
Automount >NIL:
Wait 3
$marker_assign

Echo "FBC_AROS_PACKAGE_TEST: $target START" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
Set CleanRC 0
If EXISTS "$install_volume:Developer"
    Set CleanRC 1
EndIf
If EXISTS "$install_volume:FreeBASIC"
    Set CleanRC 1
EndIf
Echo "clean_rc=\$CleanRC" >"$install_volume:clean.log"

Echo "FBC_AROS_PACKAGE_TEST: $target BEFORE_WANDERER_1" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
Run <NIL: >NIL: WANDERER:Wanderer
Echo "FBC_AROS_PACKAGE_TEST: $target AFTER_WANDERER_1" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
Wait 5
Echo "FBC_AROS_PACKAGE_TEST: $target BEFORE_UNPACK_1" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
C:Unpack "$install_volume:Developer.pkg" TO "$install_volume:" >"$install_volume:developer-unpack.log"
Set DeveloperRC \$RC
Echo "FBC_AROS_PACKAGE_TEST: $target AFTER_UNPACK_1" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
Echo "developer_unpack_rc=\$DeveloperRC" >"$install_volume:developer-result.log"

; C:Unpack closes the public Workbench screen after each progress window.
; Reopen it because the second package needs another progress window.
Run <NIL: >NIL: WANDERER:Wanderer
Wait 5
Echo "FBC_AROS_PACKAGE_TEST: $target BEFORE_UNPACK_2" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
C:Unpack "$install_volume:FreeBASIC.pkg" TO "$install_volume:" >"$install_volume:freebasic-unpack.log"
Set FreeBASICRC \$RC
Echo "FBC_AROS_PACKAGE_TEST: $target AFTER_UNPACK_2" >RAM:package-test-serial.log
SYS:Tests/PackageTest/SerialRelay RAM:package-test-serial.log
Echo "freebasic_unpack_rc=\$FreeBASICRC" >"$install_volume:freebasic-result.log"

Assign Developer: "$install_volume:Developer"
Assign FreeBASIC: "$install_volume:FreeBASIC"
$compiler_path_command
SetEnv GCC Developer:bin/gcc
SetEnv ELF2HUNK Developer:bin/elf2hunk
$arm_tool_environment
Protect Developer:bin/gcc +e QUIET
Protect Developer:bin/as +e QUIET
Protect Developer:bin/ld +e QUIET
Protect Developer:bin/collect-aros +e QUIET
Protect FreeBASIC:bin/fbc +e QUIET
Stack $compiler_stack

$m68k_setup

SYS:Tests/PackageTest/FileCopy "$install_volume:package-smoke.bas" "${compile_file_prefix}package-smoke.bas"
CD "$compile_directory"
$fbc_command -v -R -C -O 0 -target aros -m package-smoke -x package-smoke package-smoke.bas >"$install_volume:compile.log"
Set BuildRC \$RC
Echo "build_rc=\$BuildRC" >"$install_volume:build-result.log"

Set ProgramRC 255
If EXISTS "${compile_file_prefix}package-smoke"
    Protect "${compile_file_prefix}package-smoke" +e QUIET
    SYS:Tests/PackageTest/FileCopy "${compile_file_prefix}package-smoke" "$install_volume:package-smoke"
    "${compile_file_prefix}package-smoke"
    Set ProgramRC \$RC
EndIf
Echo "program_rc=\$ProgramRC" >"$install_volume:program-result.log"

If 0 EQ \$CleanRC VAL
    If 0 EQ \$DeveloperRC VAL
        If 0 EQ \$FreeBASICRC VAL
            If 0 EQ \$BuildRC VAL
                If 73 EQ \$ProgramRC VAL
                    If EXISTS "$install_volume:program.ok"
                        Echo "FBC_AROS_PACKAGE_TEST: $target PASS" >"$install_volume:result.log"
                        SYS:Tests/PackageTest/SerialRelay "$install_volume:result.log"
                    EndIf
                EndIf
            EndIf
        EndIf
    EndIf
EndIf

If NOT EXISTS "$install_volume:result.log"
    Echo "FBC_AROS_PACKAGE_TEST: $target FAIL" >"$install_volume:result.log"
    SYS:Tests/PackageTest/SerialRelay "$install_volume:result.log"
EndIf

Wait 2
C:Shutdown

;=============================================================================
; End of file
;=============================================================================
EOF
}

write_x86_grub_config() {
    local destination="$1"

    cat > "$destination" <<'EOF'
set timeout=0
set default=0

menuentry "FreeBASIC AROS package test" {
    multiboot2 /boot/pc/bootstrap.xz vesa=800x600x32 ATA=32bit debug=serial:0@115200
    module2 /boot/pc/kernel.xz
    module2 /boot/pc/aros-bsp.pkg.xz
    module2 /boot/pc/aros-acpi.pkg.xz
    module2 /boot/aros-base.pkg.xz
    module2 /boot/aros-fs.pkg.xz
    module2 /boot/poseidon.pkg.xz
}
EOF
}

wait_for_marker() {
    local target="$1"
    local log_file="$2"
    local process_id="$3"
    local host_marker="${4-}"
    local elapsed=0
    local pattern="FBC_AROS_PACKAGE_TEST: $target (PASS|FAIL)"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        if grep -a -E -q "$pattern" "$log_file" 2>/dev/null; then
            return 0
        fi
        if [ -n "$host_marker" ] &&
           grep -a -E -q "$pattern" "$host_marker" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$process_id" 2>/dev/null; then
            return 1
        fi
        sleep 2
        elapsed=$((elapsed + 2))
    done

    return 1
}

##############################################################################
# Writable FAT media and evidence
##############################################################################

create_fat_image() {
    local image="$1"
    local label="$2"
    local total_sectors=$((DISK_SIZE_MIB * 2048))
    local partition_sectors=$((total_sectors - 2048))
    local filesystem_blocks=$((partition_sectors / 2))

    truncate -s 0 "$image"
    truncate -s "${DISK_SIZE_MIB}M" "$image"
    printf '%s\n' \
        'label: dos' \
        'unit: sectors' \
        'first-lba: 2048' \
        "1 : start=2048, size=$partition_sectors, type=c, bootable" |
        sfdisk "$image" >/dev/null
    mkfs.vfat -F 32 -n "$label" --offset=2048 \
        "$image" "$filesystem_blocks" >/dev/null
}

stage_packages() {
    local destination="$1"

    cp "$DEVELOPER_PACKAGE" "$destination/Developer.pkg"
    cp "$FREEBASIC_PACKAGE" "$destination/FreeBASIC.pkg"
}

collect_fat_evidence() {
    local image="$1"
    local evidence="$2"

    mkdir -p "$evidence/guest-files"
    mdir -i "$image@@1M" -/ :: >"$evidence/filesystem.txt"
    mcopy -i "$image@@1M" -n '::/*.log' "$evidence/guest-files/" \
        >/dev/null 2>&1 || true
    mcopy -i "$image@@1M" -n ::/package-smoke "$evidence/guest-files/" \
        >/dev/null 2>&1 || true
    mcopy -i "$image@@1M" -n ::/program.ok "$evidence/guest-files/" \
        >/dev/null 2>&1 || true
}

validate_evidence() {
    local target="$1"
    local evidence="$2"
    local result="$evidence/guest-files/result.log"
    local executable="$evidence/guest-files/package-smoke"

    require_file "$result"
    grep -a -F -q "FBC_AROS_PACKAGE_TEST: $target PASS" "$result" ||
        die "$target guest did not record a package-test pass"
    require_file "$evidence/guest-files/program.ok"
    grep -a -F -q 'FREEBASIC_INSTALLED_PACKAGE_PROGRAM_EXECUTED' \
        "$evidence/guest-files/program.ok" ||
        die "$target program marker is invalid"
    require_file "$executable"

    file "$executable" >"$evidence/executable-file.txt"
    if [ "$target" = m68k ]; then
        grep -F -q 'AmigaOS loadseg()ble executable/binary' \
            "$evidence/executable-file.txt" ||
            die "m68k installed compiler did not produce a Hunk executable"
    else
        grep -F -q 'AROS Research Operating System' \
            "$evidence/executable-file.txt" ||
            die "$target installed compiler did not produce an AROS ELF executable"
    fi

    sha256sum "$DEVELOPER_PACKAGE" "$FREEBASIC_PACKAGE" "$executable" \
        >"$evidence/SHA256SUMS"
}

prepare_clean_iso_tree() {
    local base_iso="$1"
    local iso_root="$2"
    local log_file="$3"

    [ -d "$iso_root" ] || die "clean ISO staging directory is missing"
    [[ "$iso_root" == "$WORK_ROOT"/*/iso-root ]] ||
        die "refusing to prepare unexpected ISO staging directory: $iso_root"

    xorriso -osirrox on -indev "$base_iso" -extract / "$iso_root" \
        >"$log_file" 2>&1
    [ -d "$iso_root/Developer" ] ||
        die "base ISO did not contain the Developer tree expected by the clean test"
    # Rock Ridge records make the extracted CD tree read-only. The staging
    # directory is private to this run, so make it writable before replacing
    # the startup files and deleting the preinstalled toolchain.
    chmod -R u+w "$iso_root"
    rm -rf -- "$iso_root/Developer"
    [ ! -e "$iso_root/Developer" ] ||
        die "clean ISO staging tree still contains Developer"
}

##############################################################################
# x86_64 QEMU test
##############################################################################

run_x86_test() {
    local target=x86_64
    local target_work="$WORK_ROOT/$target"
    local evidence="$RUN_ROOT/$target"
    local disk="$target_work/install.img"
    local iso="$target_work/clean.iso"
    local iso_root="$target_work/iso-root"
    local package_stage="$target_work/package-stage"
    local serial_relay="$target_work/SerialRelay"
    local file_copy="$target_work/FileCopy"
    local startup="$target_work/Startup-Sequence"
    local serial_log="$evidence/serial.log"
    local grub_config="$target_work/grub.cfg"
    local -a accelerator=(-accel tcg)

    map_target "$target"
    require_file "$BASE_ISO"
    mkdir -p "$target_work" "$evidence" "$package_stage" "$iso_root"
    build_serial_relay "$target" "$serial_relay"
    build_file_copy "$target" "$file_copy"
    stage_packages "$package_stage"
    write_smoke_program "$package_stage/package-smoke.bas"
    write_startup "$target" FBCINST "$startup"
    write_x86_grub_config "$grub_config"

    prepare_clean_iso_tree "$BASE_ISO" "$iso_root" \
        "$evidence/xorriso-extract.log"
    mkdir -p "$iso_root/Tests/PackageTest"
    cp "$startup" "$iso_root/S/Startup-Sequence"
    cp "$grub_config" "$iso_root/boot/grub/grub.cfg"
    cp "$serial_relay" "$iso_root/Tests/PackageTest/SerialRelay"
    cp "$file_copy" "$iso_root/Tests/PackageTest/FileCopy"
    xorriso -as mkisofs -R -J -joliet-long -V 'AROS Live CD' \
        -c boot.catalog \
        -b boot/grub/i386-pc/eltorito.img \
        -no-emul-boot -boot-load-size 4 -boot-info-table --grub2-boot-info \
        -o "$iso" "$iso_root" >"$evidence/xorriso.log" 2>&1
    if xorriso -indev "$iso" -find /Developer -maxdepth 0 -print \
        2>/dev/null | grep -q .; then
        die "x86_64 clean ISO still contains /Developer"
    fi

    create_fat_image "$disk" FBCINST
    mcopy -i "$disk@@1M" -spm "$package_stage"/* ::

    if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
        accelerator=(-accel kvm -cpu host)
    fi

    msg "installing x86_64 packages in clean QEMU guest"
    : >"$serial_log"
    qemu-system-x86_64 \
        -M pc "${accelerator[@]}" -m "$X86_MEMORY_MIB" \
        -cdrom "$iso" -boot d \
        -drive "file=$disk,format=raw,if=ide" \
        -serial stdio -display none -monitor none \
        -audiodev driver=none,id=package_audio \
        -device AC97,audiodev=package_audio \
        -no-reboot >"$serial_log" 2>&1 &
    QEMU_PID=$!
    wait_for_marker "$target" "$serial_log" "$QEMU_PID" || true
    wait_for_natural_exit "$QEMU_PID"
    stop_process "$QEMU_PID"
    QEMU_PID=""

    collect_fat_evidence "$disk" "$evidence"
    cp "$startup" "$evidence/Startup-Sequence"
    validate_evidence "$target" "$evidence"
}

##############################################################################
# ARM QEMU test
##############################################################################

run_arm_test() {
    local target=arm
    local target_work="$WORK_ROOT/$target"
    local evidence="$RUN_ROOT/$target"
    local disk="$target_work/clean.img"
    local system_stage="$target_work/system"
    local serial_relay="$target_work/SerialRelay"
    local file_copy="$target_work/FileCopy"
    local startup="$target_work/Startup-Sequence"
    local serial_log="$evidence/serial.log"

    map_target "$target"
    require_file "$SYSROOT/aros-arm-raspi.img"
    require_file "$SYSROOT/aros-arm-bsp.rom"
    require_file "$SYSROOT/bcm2709-rpi-2-b.dtb"
    mkdir -p "$target_work" "$evidence" "$system_stage"
    build_serial_relay "$target" "$serial_relay"
    build_file_copy "$target" "$file_copy"

    rsync -a --exclude='/Developer/***' --exclude='/FreeBASIC/***' \
        "$SYSROOT/" "$system_stage/"
    [ ! -e "$system_stage/Developer" ] ||
        die "ARM clean filesystem still contains Developer"
    mkdir -p "$system_stage/Tests/PackageTest" "$system_stage/S"
    cp "$serial_relay" "$system_stage/Tests/PackageTest/SerialRelay"
    cp "$file_copy" "$system_stage/Tests/PackageTest/FileCopy"
    stage_packages "$system_stage"
    write_smoke_program "$system_stage/package-smoke.bas"
    write_startup "$target" SYS "$startup"
    cp "$startup" "$system_stage/S/Startup-Sequence"
    printf 'arm\n' >"$system_stage/AROS.boot"

    create_fat_image "$disk" AROS
    mcopy -i "$disk@@1M" -spm "$system_stage"/* ::

    msg "installing ARM packages in clean Raspberry Pi 2 QEMU guest"
    : >"$serial_log"
    qemu-system-arm \
        -M raspi2b -m 1G -nographic -monitor none -serial stdio -no-reboot \
        -icount auto,sleep=off \
        -kernel "$SYSROOT/aros-arm-raspi.img" \
        -initrd "$SYSROOT/aros-arm-bsp.rom" \
        -dtb "$SYSROOT/bcm2709-rpi-2-b.dtb" \
        -drive "file=$disk,format=raw,if=sd" \
        >"$serial_log" 2>&1 &
    QEMU_PID=$!
    wait_for_marker "$target" "$serial_log" "$QEMU_PID" || true
    wait_for_natural_exit "$QEMU_PID"
    stop_process "$QEMU_PID"
    QEMU_PID=""

    collect_fat_evidence "$disk" "$evidence"
    cp "$startup" "$evidence/Startup-Sequence"
    validate_evidence "$target" "$evidence"
}

##############################################################################
# m68k FS-UAE test
##############################################################################

run_m68k_test() {
    local target=m68k
    local target_work="$WORK_ROOT/$target"
    local evidence="$RUN_ROOT/$target"
    local host_root="$target_work/host"
    local iso="$target_work/clean.iso"
    local iso_root="$target_work/iso-root"
    local serial_relay="$target_work/SerialRelay"
    local file_copy="$target_work/FileCopy"
    local startup="$target_work/Startup-Sequence"
    local config="$target_work/package-test.fs-uae"
    local fifo="$target_work/serial.fifo"
    local serial_log="$evidence/serial.log"
    local emulator_log="$evidence/fs-uae.log"

    map_target "$target"
    require_file "$BASE_ISO"
    require_file "$SYSROOT/boot/amiga/aros-rom.bin"
    require_file "$SYSROOT/boot/amiga/aros-ext.bin"
    mkdir -p "$target_work" "$evidence" "$host_root" "$iso_root"
    build_serial_relay "$target" "$serial_relay"
    build_file_copy "$target" "$file_copy"
    stage_packages "$host_root"
    write_smoke_program "$host_root/package-smoke.bas"
    write_startup "$target" FBCINST "$startup"

    prepare_clean_iso_tree "$BASE_ISO" "$iso_root" \
        "$evidence/xorriso-extract.log"
    mkdir -p "$iso_root/Tests/PackageTest"
    cp "$startup" "$iso_root/S/Startup-Sequence"
    cp "$serial_relay" "$iso_root/Tests/PackageTest/SerialRelay"
    cp "$file_copy" "$iso_root/Tests/PackageTest/FileCopy"
    xorriso -as mkisofs -R -J -joliet-long -V AROS \
        -o "$iso" "$iso_root" >"$evidence/xorriso.log" 2>&1
    if xorriso -indev "$iso" -find /Developer -maxdepth 0 -print \
        2>/dev/null | grep -q .; then
        die "m68k clean ISO still contains /Developer"
    fi

    cat >"$config" <<EOF
[fs-uae]
amiga_model = A4000/040
kickstart_file = $SYSROOT/boot/amiga/aros-rom.bin
kickstart_ext_file = $SYSROOT/boot/amiga/aros-ext.bin
cdrom_drive_0 = $iso
hard_drive_0 = $host_root
hard_drive_0_label = FBCINST
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

    mkfifo "$fifo"
    : >"$serial_log"
    : >"$emulator_log"
    msg "installing m68k packages in clean FS-UAE guest"
    timeout "$TIMEOUT_SECONDS" dd if="$fifo" of="$serial_log" status=none &
    M68K_READER_PID=$!
    timeout "$TIMEOUT_SECONDS" fs-uae "$config" --no-gui \
        >"$emulator_log" 2>&1 &
    M68K_EMULATOR_PID=$!

    wait_for_marker "$target" "$serial_log" "$M68K_EMULATOR_PID" \
        "$host_root/result.log" || true
    wait_for_natural_exit "$M68K_EMULATOR_PID"
    stop_process "$M68K_EMULATOR_PID"
    stop_process "$M68K_READER_PID"
    M68K_EMULATOR_PID=""
    M68K_READER_PID=""

    mkdir -p "$evidence/guest-files"
    find "$host_root" -maxdepth 1 -type f \
        \( -name '*.log' -o -name 'program.ok' -o -name 'package-smoke' \) \
        -exec cp -a -t "$evidence/guest-files" {} +
    cp "$startup" "$evidence/Startup-Sequence"
    validate_evidence "$target" "$evidence"
}

##############################################################################
# Test execution
##############################################################################

mkdir -p "$OUTPUT_ROOT"
WORK_ROOT="$(mktemp -d "$OUTPUT_ROOT/.package-test.XXXXXX")"
RUN_ROOT="$OUTPUT_ROOT/run-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$RUN_ROOT"

for target in "${SELECTED_TARGETS[@]}"; do
    case "$target" in
        x86_64) run_x86_test ;;
        arm) run_arm_test ;;
        m68k) run_m68k_test ;;
    esac
done

printf '%s\n' "${SELECTED_TARGETS[@]}" >"$RUN_ROOT/passed-targets.txt"
msg "all selected AROS package installation tests passed"
echo "Evidence: $RUN_ROOT"

# end of aros-test-packages.sh
