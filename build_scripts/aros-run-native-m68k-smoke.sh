#!/usr/bin/env bash
#
# Project: FreeBASIC AROS native compiler qualification
# -----------------------------------------------------
#
# File: aros-run-native-m68k-smoke.sh
#
# Purpose:
#
#     Prove that the packaged m68k FreeBASIC compiler can compile, link,
#     convert, and run a program inside an AROS m68k guest.
#
# Responsibilities:
#
#     - stage the AROS-hosted GCC supply chain on a fresh host volume
#     - run one normal fbc invocation with the low-memory -O0 policy
#     - verify GCC compilation, assembly, linking, and Hunk conversion
#     - verify the resulting program and its distinctive return status
#     - retain compiler, emulator, executable, and checksum evidence
#
# This file intentionally does NOT contain:
#
#     - AROS source or toolchain construction
#     - cross-compiler qualification
#     - fbctests, Exampleageddon, or OMA orchestration
#     - generic m68k compiler or ABI policy
#
# Memory policy:
#
#     FS-UAE exposes its maximum 1 GiB Zorro III allocation. The current AROS
#     m68k allocator only registers a smaller portion of that board, so the
#     guest process stack is kept at 1 MiB and generated C is compiled at -O0.
#     The prebuilt compiler and GCC executables remain optimized because making
#     those programs larger and slower would increase guest memory pressure.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults and process state
##############################################################################

SCRIPT_PATH="$0"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
OUTPUT_ROOT="${AROS_NATIVE_SMOKE_OUTDIR:-$AROS_ROOT/native-build-smoke/m68k}"
TIMEOUT_SECONDS="${AROS_NATIVE_SMOKE_TIMEOUT:-1200}"
STACK_BYTES="${AROS_NATIVE_M68K_STACK:-1048576}"
BASE_ISO_OVERRIDE=""
KEEP_WORK=0

WORK_ROOT=""
EVIDENCE_ROOT=""
EMULATOR_PID=""

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

require_command() {
    command -v "$1" >/dev/null 2>&1 ||
        die "required host tool not found: $1"
}

require_file() {
    [ -f "$1" ] || die "required file not found: $1"
}

require_directory() {
    [ -d "$1" ] || die "required directory not found: $1"
}

require_loadable_hunk() {
    local description

    description="$(file -b "$1")"
    [[ "$description" == *"AmigaOS loadseg()ble executable/binary"* ]] ||
        die "not an AROS m68k-loadable Hunk file: $1: $description"
}

stop_emulator() {
    local grace_seconds=0

    [ -n "$EMULATOR_PID" ] || return 0
    if kill -0 "$EMULATOR_PID" 2>/dev/null; then
        kill "$EMULATOR_PID" 2>/dev/null || true
    fi
    while kill -0 "$EMULATOR_PID" 2>/dev/null &&
          [ "$grace_seconds" -lt 5 ]; do
        sleep 1
        grace_seconds=$((grace_seconds + 1))
    done
    if kill -0 "$EMULATOR_PID" 2>/dev/null; then
        kill -KILL "$EMULATOR_PID" 2>/dev/null || true
    fi
    wait "$EMULATOR_PID" 2>/dev/null || true
    EMULATOR_PID=""
}

cleanup() {
    local status=$?

    stop_emulator

    if [ -n "$WORK_ROOT" ] &&
       [[ "$WORK_ROOT" == "$OUTPUT_ROOT"/.native-m68k.* ]] &&
       [ -d "$WORK_ROOT" ]; then
        if [ "$KEEP_WORK" -eq 0 ] && [ "$status" -eq 0 ]; then
            rm -rf -- "$WORK_ROOT"
        else
            echo "Native smoke staging retained at: $WORK_ROOT" >&2
        fi
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-run-native-m68k-smoke.sh [options]

Options:
  --aros-root DIR  AROS workspace. Default: out/aros
  --out-dir DIR    Evidence directory. Default: out/aros/native-build-smoke/m68k
  --timeout SEC    Bounded emulator runtime. Default: 1200
  --stack BYTES    Guest process stack. Default: 1048576
  --base-iso FILE  Existing AROS m68k boot ISO. Defaults to the build image.
  --keep-work      Retain the temporary staged host volume and ISO.
  -h, --help       Show this help.

The guest always compiles generated C at -O0. Each run creates a separate
timestamped evidence directory so an interrupted run cannot leave a stale
success marker for a later run.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --aros-root)
            [ -n "${2-}" ] || die "$1 requires a value"
            AROS_ROOT="$2"
            shift 2
            ;;
        --out-dir)
            [ -n "${2-}" ] || die "$1 requires a value"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --timeout)
            [ -n "${2-}" ] || die "$1 requires a value"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --stack)
            [ -n "${2-}" ] || die "$1 requires a value"
            STACK_BYTES="$2"
            shift 2
            ;;
        --base-iso)
            [ -n "${2-}" ] || die "$1 requires a value"
            BASE_ISO_OVERRIDE="$2"
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

case "$TIMEOUT_SECONDS" in
    ''|*[!0-9]*|0) die "--timeout must be a positive integer" ;;
esac

case "$STACK_BYTES" in
    ''|*[!0-9]*|0) die "--stack must be a positive integer" ;;
esac

##############################################################################
# Input validation
##############################################################################

for command_name in cp file find fs-uae grep mktemp sed sha256sum timeout xorriso
do
    require_command "$command_name"
done

AROS_BUILD="$AROS_ROOT/build-amiga-m68k"
AROS_SYSROOT="$AROS_BUILD/bin/amiga-m68k/AROS"
BASE_ISO="$AROS_BUILD/distfiles/aros-amiga-m68k.iso"

# A release build may intentionally omit image generation after the native
# toolchain is ready.  An explicitly supplied ISO lets the qualification use
# a separately verified AROS guest without changing the build workspace.
if [ -n "$BASE_ISO_OVERRIDE" ]; then
    BASE_ISO="$BASE_ISO_OVERRIDE"
fi

ROM_FILE="$AROS_SYSROOT/boot/amiga/aros-rom.bin"
ROM_EXTENSION_FILE="$AROS_SYSROOT/boot/amiga/aros-ext.bin"
DEVELOPER_ROOT="$AROS_SYSROOT/Developer"
DEVELOPER_BIN="$DEVELOPER_ROOT/bin"
DEVELOPER_LIB="$DEVELOPER_ROOT/lib"
CC1="$DEVELOPER_ROOT/libexec/gcc/m68k-aros/6.5.0/cc1"
LIBGCC="$DEVELOPER_ROOT/lib/gcc/m68k-aros/6.5.0/libgcc.a"
COMPILER="$AROS_ROOT/freebasic/m68k/fbc"
RUNTIME_DIR="$ROOT/lib/freebasic/aros-m68k"

require_file "$BASE_ISO"
require_file "$ROM_FILE"
require_file "$ROM_EXTENSION_FILE"
require_directory "$DEVELOPER_BIN"
require_directory "$DEVELOPER_LIB"
require_file "$CC1"
require_file "$LIBGCC"
require_file "$COMPILER"
require_directory "$RUNTIME_DIR"

for runtime_file in fbrt0.o libfb.a
do
    require_file "$RUNTIME_DIR/$runtime_file"
done

require_file "$DEVELOPER_LIB/libpthread.a"

for tool_name in gcc as ld collect-aros elf2hunk
do
    require_file "$DEVELOPER_BIN/$tool_name"
    require_loadable_hunk "$DEVELOPER_BIN/$tool_name"
done

require_loadable_hunk "$CC1"
require_loadable_hunk "$COMPILER"

##############################################################################
# Fresh host volume and guest files
##############################################################################

mkdir -p "$OUTPUT_ROOT"
WORK_ROOT="$(mktemp -d "$OUTPUT_ROOT/.native-m68k.XXXXXX")"
RUN_NAME="run-$(date +%Y%m%d-%H%M%S)-$$"
EVIDENCE_ROOT="$OUTPUT_ROOT/$RUN_NAME"
HOST_ROOT="$WORK_ROOT/host"
CURRENT_ISO="$WORK_ROOT/aros-native-m68k.iso"
STARTUP_FILE="$WORK_ROOT/Startup-Sequence"
FSUAE_CONFIG="$WORK_ROOT/native-m68k.fs-uae"

mkdir -p \
    "$EVIDENCE_ROOT" \
    "$HOST_ROOT/Work" \
    "$HOST_ROOT/Developer/bin" \
    "$HOST_ROOT/Developer/libexec/gcc/m68k-aros/6.5.0" \
    "$HOST_ROOT/LinkStage/Developer/lib" \
    "$HOST_ROOT/FreeBASIC/bin" \
    "$HOST_ROOT/FreeBASIC/lib/freebasic/aros-m68k"

msg "staging the AROS-hosted m68k supply chain"
cp -a "$DEVELOPER_BIN/." "$HOST_ROOT/Developer/bin/"
cp -a "$CC1" \
    "$HOST_ROOT/Developer/libexec/gcc/m68k-aros/6.5.0/cc1"

# The native linker receives a single RAM-resident -B/-L prefix. Keeping only
# top-level archives and startup objects avoids copying the unrelated 164 MiB
# SDK tree into AROS memory while preserving the ordinary FreeBASIC link set.
find "$DEVELOPER_LIB" -maxdepth 1 -type f \
    \( -name '*.a' -o -name '*.o' \) \
    -exec cp -a -t "$HOST_ROOT/LinkStage/Developer/lib" {} +
cp -a "$LIBGCC" "$HOST_ROOT/LinkStage/Developer/lib/libgcc.a"

cp -a "$COMPILER" "$HOST_ROOT/FreeBASIC/bin/fbc"
cp -a "$RUNTIME_DIR/." \
    "$HOST_ROOT/FreeBASIC/lib/freebasic/aros-m68k/"

cat > "$HOST_ROOT/Work/native-build-smoke.bas" <<'EOF'
''
'' FreeBASIC AROS m68k native-compilation proof
'' ------------------------------------------------
''
'' File: native-build-smoke.bas
''
'' Purpose:
''
''     Prove that a program compiled inside AROS m68k can execute there.
''
'' Responsibilities:
''
''     - write a host-visible success marker
''     - return a distinctive status to the startup harness
''
'' This file intentionally does NOT exercise graphics or sound. Those
'' subsystems are covered by fbctests, Exampleageddon, and the OMA matrix.
''

const EXPECTED_RESULT = 73

open "Native:program.ok" for output as #1
print #1, "FREEBASIC_NATIVE_PROGRAM_EXECUTED"
close #1

end EXPECTED_RESULT

'' end of native-build-smoke.bas
EOF

cat > "$STARTUP_FILE" <<EOF
;=============================================================================
;
; FreeBASIC AROS m68k native-compilation smoke test
;
; PURPOSE
;   Compile, link, convert, and run one BASIC program through the ordinary
;   native fbc command in a single fresh AROS boot.
;
;=============================================================================

FailAt 2147483647
MakeDir RAM:NativeBuild
MakeDir RAM:NativeBuild/T
MakeDir RAM:NativeBuild/ENV
MakeDir RAM:NativeBuild/Developer
MakeDir RAM:NativeBuild/Developer/lib
MakeDir RAM:NativeBuild/FreeBASIC
MakeDir RAM:NativeBuild/FreeBASIC/bin
MakeDir RAM:NativeBuild/FreeBASIC/lib
Assign T: RAM:NativeBuild/T
Assign ENV: RAM:NativeBuild/ENV

Copy Native:LinkStage/Developer/lib RAM:NativeBuild/Developer/lib ALL CLONE QUIET
Copy Native:FreeBASIC/bin RAM:NativeBuild/FreeBASIC/bin ALL CLONE QUIET
Copy Native:FreeBASIC/lib RAM:NativeBuild/FreeBASIC/lib ALL CLONE QUIET
Assign Developer: Native:Developer
Assign FreeBASIC: RAM:NativeBuild/FreeBASIC
Path ADD Developer:bin
Path ADD FreeBASIC:bin
SetEnv GCC Developer:bin/gcc
SetEnv ELF2HUNK Developer:bin/elf2hunk
SetEnv FBC_AROS_NATIVE_LINK_PREFIX "RAM Disk:NativeBuild/Developer/lib/"
Stack $STACK_BYTES

Copy Native:Work/native-build-smoke.bas RAM:NativeBuild/native-build-smoke.bas QUIET
CD RAM:NativeBuild

FreeBASIC:bin/fbc -v -R -C -O 0 -target aros -m native-build-smoke -x native-build-smoke native-build-smoke.bas >Native:oneshot.log
Set BuildRC \$RC
Echo "build_rc=\$BuildRC" >Native:build-result.log

If EXISTS "RAM:NativeBuild/native-build-smoke"
    Protect RAM:NativeBuild/native-build-smoke +e
    Copy RAM:NativeBuild/native-build-smoke Native:native-build-smoke QUIET
    RAM:NativeBuild/native-build-smoke
    Set ProgramRC \$RC
    Echo "program_rc=\$ProgramRC" >Native:program-result.log
Else
    Echo "AROS_NATIVE_EXECUTABLE_MISSING" >Native:program-result.log
EndIf

If EXISTS "Native:program.ok"
    Echo "AROS_NATIVE_M68K_ONESHOT_PASS" >Native:marker.log
Else
    Echo "AROS_NATIVE_M68K_ONESHOT_FAIL" >Native:marker.log
EndIf

Wait 2
C:Shutdown

;=============================================================================
; End of file
;=============================================================================
EOF

cp "$HOST_ROOT/Work/native-build-smoke.bas" \
    "$EVIDENCE_ROOT/native-build-smoke.bas"
cp "$STARTUP_FILE" "$EVIDENCE_ROOT/Startup-Sequence"

##############################################################################
# Boot image and emulator configuration
##############################################################################

msg "creating the isolated AROS boot image"
cp --reflink=auto "$BASE_ISO" "$CURRENT_ISO"
xorriso -dev "$CURRENT_ISO" \
    -boot_image any replay \
    -map "$STARTUP_FILE" /S/Startup-Sequence \
    -commit -end >"$EVIDENCE_ROOT/xorriso.log" 2>&1

cat > "$FSUAE_CONFIG" <<EOF
[fs-uae]
amiga_model = A4000/040
kickstart_file = $ROM_FILE
kickstart_ext_file = $ROM_EXTENSION_FILE
cdrom_drive_0 = $CURRENT_ISO
hard_drive_0 = $HOST_ROOT
hard_drive_0_label = Native
hard_drive_0_priority = -128
fast_memory = 8192
motherboard_ram = 65536
zorro_iii_memory = 1048576
graphics_card = uaegfx
graphics_card_memory = 32768
fullscreen = 0
window_width = 800
window_height = 600
sound_output = none
EOF

cp "$FSUAE_CONFIG" "$EVIDENCE_ROOT/native-m68k.fs-uae"

##############################################################################
# Bounded guest execution
##############################################################################

msg "running native fbc at -O0 inside AROS m68k"
set +e
timeout --signal=TERM --kill-after=10 "$TIMEOUT_SECONDS" \
    fs-uae "$FSUAE_CONFIG" --no-gui \
    >"$EVIDENCE_ROOT/fs-uae.log" 2>&1 &
EMULATOR_PID=$!
wait "$EMULATOR_PID"
EMULATOR_STATUS=$?
EMULATOR_PID=""
set -e

if [ "$EMULATOR_STATUS" -eq 124 ] || [ "$EMULATOR_STATUS" -eq 137 ]; then
    die "AROS m68k native smoke test exceeded $TIMEOUT_SECONDS seconds"
fi
[ "$EMULATOR_STATUS" -eq 0 ] ||
    die "FS-UAE exited with status $EMULATOR_STATUS"

##############################################################################
# Evidence validation
##############################################################################

require_file "$HOST_ROOT/marker.log"
require_file "$HOST_ROOT/build-result.log"
require_file "$HOST_ROOT/program-result.log"
require_file "$HOST_ROOT/program.ok"
require_file "$HOST_ROOT/oneshot.log"
require_file "$HOST_ROOT/native-build-smoke"

grep -qx 'AROS_NATIVE_M68K_ONESHOT_PASS' "$HOST_ROOT/marker.log" ||
    die "guest did not report native compilation success"
grep -Eq '^build_rc=0[[:space:]]*$' "$HOST_ROOT/build-result.log" ||
    die "native fbc did not return status 0"
grep -Eq '^program_rc=73[[:space:]]*$' "$HOST_ROOT/program-result.log" ||
    die "native program did not return status 73"
grep -qx 'FREEBASIC_NATIVE_PROGRAM_EXECUTED' "$HOST_ROOT/program.ok" ||
    die "native program did not write its execution marker"

for phase in 'compiling C:' 'assembling:' 'linking:' 'making Hunk:'
do
    grep -Fq "$phase" "$HOST_ROOT/oneshot.log" ||
        die "compiler log does not contain phase: $phase"
done

if grep -Eiq \
    'Chunk allocator error|Memory header not located|Recoverable Alert|not enough memory|file truncated|cannot open output|cannot find -l|making Hunk failed|terminated with exit code' \
    "$HOST_ROOT/oneshot.log" "$EVIDENCE_ROOT/fs-uae.log"
then
    die "native smoke evidence contains an allocator, memory, or tool failure"
fi

require_loadable_hunk "$HOST_ROOT/native-build-smoke"

cp "$HOST_ROOT/marker.log" "$EVIDENCE_ROOT/"
cp "$HOST_ROOT/build-result.log" "$EVIDENCE_ROOT/"
cp "$HOST_ROOT/program-result.log" "$EVIDENCE_ROOT/"
cp "$HOST_ROOT/program.ok" "$EVIDENCE_ROOT/"
cp "$HOST_ROOT/oneshot.log" "$EVIDENCE_ROOT/"
cp "$HOST_ROOT/native-build-smoke" "$EVIDENCE_ROOT/"

EXECUTABLE_DESCRIPTION="$(file -b "$EVIDENCE_ROOT/native-build-smoke")"
EXECUTABLE_SHA256="$(sha256sum "$EVIDENCE_ROOT/native-build-smoke" |
    sed 's/[[:space:]].*$//')"
COMPILER_SHA256="$(sha256sum "$COMPILER" | sed 's/[[:space:]].*$//')"

cat > "$EVIDENCE_ROOT/summary.txt" <<EOF
FreeBASIC AROS m68k native compiler smoke test
================================================
result=PASS
generated_c_optimization=-O0
guest_stack_bytes=$STACK_BYTES
fs_uae_zorro_iii_kib=1048576
compiler_sha256=$COMPILER_SHA256
executable_sha256=$EXECUTABLE_SHA256
executable_format=$EXECUTABLE_DESCRIPTION
EOF

msg "AROS m68k native compiler smoke test passed"
echo "Evidence: $EVIDENCE_ROOT"
echo "Executable SHA-256: $EXECUTABLE_SHA256"

# end of aros-run-native-m68k-smoke.sh
