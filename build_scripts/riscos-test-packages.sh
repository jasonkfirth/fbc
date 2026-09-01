#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS package acceptance workflow
# -------------------------------------------------------
#
# File: riscos-test-packages.sh
#
# Purpose:
#
#     Prove that the locally published RISC OS packages run together on a
#     fresh RISC OS Open filesystem without HostFS build staging.
#
# Responsibilities:
#
#     - create a clean RPCEmu HostFS from the pinned HardDisc4 archive
#     - install only the RiscPkg archive payloads in out/riscos/packages
#     - run GCC4, the UnixLib and audio modules, and native FreeBASIC
#     - compile and execute the packaged console and media examples
#     - launch every packaged OMA application and record liveness evidence
#
# This file intentionally does NOT contain:
#
#     - FreeBASIC, GCCSDK, or OMA compilation
#     - RiscPkg manager implementation or network package installation
#     - subjective graphics or audio-quality assessment
#
# Test-environment ownership:
#
#     The selected output directory contains an isolated copy of an existing
#     configured RPCEmu runtime.  Its HostFS is replaced with the stock
#     HardDisc4 tree, local package archives, their installed payloads, and
#     generated test control files.  The source RPCEmu runtime is read only.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BASE_WORKDIR="${RISCOS_RPCEMU_WORKDIR:-$ROOT/out/riscos/rpcemu}"
PACKAGE_DIR="${RISCOS_PACKAGE_OUTDIR:-$ROOT/out/riscos/packages}"
OUTPUT_ROOT="${RISCOS_PACKAGE_TEST_OUTDIR:-$ROOT/out/riscos/package-test}"

# A native compile starts the complete GCCSDK toolchain inside an emulator.
# Give that intentionally slow path enough time on modest development hosts.
TIMEOUT_SECONDS=900
STABILITY_SECONDS=8
RPCEMU_MEMORY=256
SCREENSHOT_INTERVAL_SECONDS=30
GAME_MIN_CHANGED_PIXELS=1024
GAME_MAX_SOLID_WHITE_PIXELS=8192

TEST_RUNTIME=""
TEST_SOURCE=""
TEST_HOSTFS=""
TEST_BINARY=""
TEST_TASK=""
TEST_LOG=""
RPCEMU_PID=""
TEMP_EXTRACT=""
TEMP_HARDDISC=""
SCREENSHOT_DIRECTORY=""
SCREENSHOT_TOOL=""
SCREENSHOT_NUMBER=0
CURRENT_SCREENSHOT_LABEL="startup"
LAST_SCREENSHOT_PATH=""

declare -a PACKAGE_ARCHIVES=()
declare -a GAME_PACKAGES=()

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

stop_emulator() {
    local attempt

    [ -n "$RPCEMU_PID" ] || return 0
    if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
        RPCEMU_PID=""
        return 0
    fi

    kill "$RPCEMU_PID" 2>/dev/null || true
    for attempt in 1 2 3 4 5; do
        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            break
        fi
        sleep 1
    done

    if kill -0 "$RPCEMU_PID" 2>/dev/null; then
        kill -KILL "$RPCEMU_PID" 2>/dev/null || true
    fi
    wait "$RPCEMU_PID" 2>/dev/null || true
    RPCEMU_PID=""
}

cleanup() {
    stop_emulator

    if [ -n "$TEMP_EXTRACT" ] &&
       [[ "$TEMP_EXTRACT" == "$OUTPUT_ROOT"/.extract.* ]] &&
       [ -d "$TEMP_EXTRACT" ]; then
        rm -rf -- "$TEMP_EXTRACT"
    fi

    if [ -n "$TEMP_HARDDISC" ] &&
       [[ "$TEMP_HARDDISC" == "$OUTPUT_ROOT"/.harddisc.* ]] &&
       [ -d "$TEMP_HARDDISC" ]; then
        rm -rf -- "$TEMP_HARDDISC"
    fi
}

control_field() {
    local archive="$1"
    local field="$2"

    unzip -p "$archive" RiscPkg/Control |
        awk -F ': ' -v wanted="$field" '$1 == wanted { print substr($0, length(wanted) + 3); exit }'
}

find_runtime_log() {
    local base="$1"

    if [ -f "$base,ffd" ]; then
        printf '%s\n' "$base,ffd"
    elif [ -f "$base" ]; then
        printf '%s\n' "$base"
    fi
}

capture_emulator_screenshot() {
    local label="$1"
    local screenshot_path
    local captured=0
    local window_id=""

    [ -n "$SCREENSHOT_TOOL" ] || return 0
    [ -n "$SCREENSHOT_DIRECTORY" ] || return 0

    LAST_SCREENSHOT_PATH=""

    SCREENSHOT_NUMBER=$((SCREENSHOT_NUMBER + 1))
    screenshot_path="$(printf '%s/%03d-%s.png' \
        "$SCREENSHOT_DIRECTORY" "$SCREENSHOT_NUMBER" "$label")"

    # RPCEmu has no command-line frame capture interface.  Prefer its X11
    # window so an unrelated workspace or foreground application cannot hide
    # the emulator.  Full-desktop capture remains a fallback for hosts that
    # do not provide xdotool.  Screenshot failures must not hide a compiler
    # or package failure on hosts without a desktop session.
    case "$SCREENSHOT_TOOL" in
        window-import)
            window_id="$(xdotool search --name '^RPCEmu' 2>/dev/null | tail -n 1 || true)"
            if [ -n "$window_id" ] &&
               import -window "$window_id" "$screenshot_path"; then
                captured=1
            fi
            ;;
        gnome-screenshot)
            if "$SCREENSHOT_TOOL" -f "$screenshot_path"; then
                captured=1
            fi
            ;;
        import)
            if "$SCREENSHOT_TOOL" -window root "$screenshot_path"; then
                captured=1
            fi
            ;;
        *)
            return 0
            ;;
    esac

    if [ "$captured" -ne 1 ] || [ ! -s "$screenshot_path" ]; then
        rm -f -- "$screenshot_path"
        echo "WARNING: unable to capture RISC OS screenshot: $label" >&2
        return 0
    fi

    LAST_SCREENSHOT_PATH="$screenshot_path"
}

game_frame_delta() {
    local initial_screenshot="$1"
    local final_screenshot="$2"
    local compare_output
    local changed_pixels

    [ -s "$initial_screenshot" ] || return 1
    [ -s "$final_screenshot" ] || return 1
    command -v compare >/dev/null 2>&1 || return 1

    compare_output="$(compare -metric AE "$initial_screenshot" \
        "$final_screenshot" null: 2>&1 || true)"
    changed_pixels="${compare_output%%[[:space:]]*}"

    case "$changed_pixels" in
        ''|*[!0-9]*) return 1 ;;
    esac

    printf '%s\n' "$changed_pixels"
}

game_blank_work_area_pixels() {
    local screenshot_path="$1"

    [ -s "$screenshot_path" ] || return 1
    command -v convert >/dev/null 2>&1 || return 1

    # A healthy title screen may include white artwork, but it must not be a
    # large, untouched white Wimp work area.  Convert finds connected exact
    # white regions without requiring screen-position assumptions for every
    # OMA title.  The 100x100 guard ignores ordinary window decorations and
    # text glyphs.
    convert "$screenshot_path" -fill black +opaque '#FFFFFF' \
        -define connected-components:verbose=true \
        -connected-components 4 null: 2>&1 | awk '
            /^(  )*[0-9]+: [0-9]+x[0-9]+\+/ &&
            /(srgb\(255,255,255\)|gray\(255\))/ {
                split($2, geometry, /[x+]/)
                if (geometry[1] >= 100 && geometry[2] >= 100 &&
                    $4 >= 10000 && $4 > largest)
                {
                    largest = $4
                }
            }
            END {
                if (largest > 0)
                    print largest
            }
        '
}

wait_for_marker() {
    local log_base="$1"
    local marker="$2"
    local elapsed=0
    local runtime_log=""
    local next_screenshot="$SCREENSHOT_INTERVAL_SECONDS"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        runtime_log="$(find_runtime_log "$log_base")"
        if [ -n "$runtime_log" ] && grep -a -q "$marker" "$runtime_log"; then
            printf '%s\n' "$runtime_log"
            return 0
        fi

        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            die "RPCEmu exited before marker $marker was written"
        fi

        if [ "$SCREENSHOT_INTERVAL_SECONDS" -gt 0 ] &&
           [ "$elapsed" -ge "$next_screenshot" ]; then
            capture_emulator_screenshot "$CURRENT_SCREENSHOT_LABEL"
            next_screenshot=$((next_screenshot + SCREENSHOT_INTERVAL_SECONDS))
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    die "timed out after $TIMEOUT_SECONDS seconds waiting for $marker"
}

launch_emulator() {
    (
        cd "$TEST_SOURCE"
        exec "$TEST_BINARY"
    ) &
    RPCEMU_PID="$!"

    # The initial capture documents a launch failure that occurs before the
    # first periodic screenshot interval.  Give Qt a moment to map its frame.
    sleep 2
    capture_emulator_screenshot "$CURRENT_SCREENSHOT_LABEL"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-test-packages.sh [options]

Options:
  --rpcemu-workdir DIR  Prepared source RPCEmu runtime used as a read-only base.
  --package-dir DIR     Directory containing the local RiscPkg archives.
  --output-dir DIR      Isolated test runtime and reports. Default: out/riscos/package-test
  --timeout SEC         Per-launch timeout. Default: 900
  --stability SEC       Seconds an interactive game must remain active. Default: 8
  --memory MIB          RPCEmu RAM. Default: 256; maximum: 256
  --screenshot-interval SEC
                       Seconds between host-display captures. Default: 30;
                       use 0 to disable screenshots.
  -h, --help            Show this help.

This installs archives through their documented RiscPkg payload layout.  The
guest receives the stock RISC OS Open HardDisc4 tree, the copied local ZIPs,
and their installed payloads; it receives no out/riscos/hostfs build staging.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing and prerequisite validation
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --rpcemu-workdir)
            require_value "$1" "${2-}"
            BASE_WORKDIR="$2"
            shift 2
            ;;
        --package-dir)
            require_value "$1" "${2-}"
            PACKAGE_DIR="$2"
            shift 2
            ;;
        --output-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --timeout)
            require_value "$1" "${2-}"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --stability)
            require_value "$1" "${2-}"
            STABILITY_SECONDS="$2"
            shift 2
            ;;
        --memory)
            require_value "$1" "${2-}"
            RPCEMU_MEMORY="$2"
            shift 2
            ;;
        --screenshot-interval)
            require_value "$1" "${2-}"
            SCREENSHOT_INTERVAL_SECONDS="$2"
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

for numeric_value in \
    "$TIMEOUT_SECONDS" "$STABILITY_SECONDS" "$SCREENSHOT_INTERVAL_SECONDS"; do
    case "$numeric_value" in
        ''|*[!0-9]*) die "timeout, stability, and screenshot interval must be numeric" ;;
    esac
done

case "$RPCEMU_MEMORY" in
    4|8|16|32|64|128|256) ;;
    *) die "--memory must be one of: 4, 8, 16, 32, 64, 128, 256" ;;
esac

for tool in awk cp find grep pgrep python3 sort strings unzip; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

BASE_WORKDIR="$(cd "$BASE_WORKDIR" && pwd)"
PACKAGE_DIR="$(cd "$PACKAGE_DIR" && pwd)"
mkdir -p "$OUTPUT_ROOT"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
SCREENSHOT_DIRECTORY="$OUTPUT_ROOT/screenshots"
mkdir -p "$SCREENSHOT_DIRECTORY"

# Each verifier run owns this directory.  Removing prior captures prevents a
# failed or partial run from being mistaken for current package evidence.
find "$SCREENSHOT_DIRECTORY" -maxdepth 1 -type f -name '*.png' -delete

if command -v xdotool >/dev/null 2>&1 &&
   command -v import >/dev/null 2>&1; then
    SCREENSHOT_TOOL="window-import"
elif command -v gnome-screenshot >/dev/null 2>&1; then
    SCREENSHOT_TOOL="gnome-screenshot"
elif command -v import >/dev/null 2>&1; then
    SCREENSHOT_TOOL="import"
else
    echo 'WARNING: neither gnome-screenshot nor ImageMagick import is available; screenshots are disabled' >&2
fi

# The command-line contract says that an interval of zero disables screenshots.
# Keep the initial and final game captures under the same policy so a headless
# run does not accidentally require ImageMagick frame comparison.
if [ "$SCREENSHOT_INTERVAL_SECONDS" -eq 0 ]; then
    SCREENSHOT_TOOL=""
fi

BASE_SOURCE="$BASE_WORKDIR/source"
BASE_BINARY="$BASE_SOURCE/rpcemu-recompiler"
HARDDISC_ARCHIVE="$BASE_WORKDIR/riscos-open-harddisc4-5.30.zip"

[ -x "$BASE_BINARY" ] || die "RPCEmu recompiler not found: $BASE_BINARY"
[ -f "$BASE_SOURCE/cmos.ram" ] ||
    die "configured RPCEmu CMOS state not found: $BASE_SOURCE/cmos.ram"
[ -d "$BASE_SOURCE/hostfs/!Boot/Choices" ] ||
    die "configured RISC OS Choices tree not found in the base runtime"
[ -f "$HARDDISC_ARCHIVE" ] ||
    die "pinned RISC OS Open HardDisc4 archive not found: $HARDDISC_ARCHIVE"

if pgrep -f '(^|/)rpcemu-(recompiler|interpreter)( |$)' >/dev/null 2>&1; then
    die "an RPCEmu process is already running; close it before package tests"
fi

##############################################################################
# Archive inventory and clean RISC OS preparation
##############################################################################

shopt -s nullglob
archives=("$PACKAGE_DIR"/*.zip)
shopt -u nullglob
[ "${#archives[@]}" -gt 0 ] || die "no RiscPkg archives found in $PACKAGE_DIR"

for archive in "${archives[@]}"; do
    package_name="$(control_field "$archive" Package)"
    [ -n "$package_name" ] ||
        die "archive has no RiscPkg Package field: $archive"

    case "$package_name" in
        GCC4|SharedUnixLibrary|DRenderer|FreeBASIC)
            ;;
        OMABehold|OMADuel999|OMAKinematics|OMANietzsche|OMAOpenMarket|\
        OMAQuestKing|OMARambo|OMAStarPhalanx|OpenSlicks)
            GAME_PACKAGES+=("$package_name")
            ;;
        *)
            die "unsupported archive in the local package set: $package_name"
            ;;
    esac

    PACKAGE_ARCHIVES+=("$archive")
done

for required_package in GCC4 SharedUnixLibrary DRenderer FreeBASIC; do
    match_count=0
    for archive in "${PACKAGE_ARCHIVES[@]}"; do
        if [ "$(control_field "$archive" Package)" = "$required_package" ]; then
            match_count=$((match_count + 1))
        fi
    done
    [ "$match_count" -eq 1 ] ||
        die "exactly one $required_package package is required; found $match_count"
done

[ "${#GAME_PACKAGES[@]}" -gt 0 ] ||
    die "the local package directory contains no packaged OMA applications"

TEST_RUNTIME="$OUTPUT_ROOT/runtime"
TEST_SOURCE="$TEST_RUNTIME/source"
TEST_HOSTFS="$TEST_SOURCE/hostfs"

[ "$TEST_RUNTIME" = "$OUTPUT_ROOT/runtime" ] ||
    die "refusing to replace an unexpected test runtime path"
if [ -e "$TEST_RUNTIME" ]; then
    rm -rf -- "$TEST_RUNTIME"
fi

msg "creating an isolated RPCEmu runtime"
mkdir -p "$TEST_RUNTIME"
cp -a "$BASE_SOURCE" "$TEST_SOURCE"
rm -rf -- "$TEST_HOSTFS"

TEMP_HARDDISC="$(mktemp -d "$OUTPUT_ROOT/.harddisc.XXXXXX")"
unzip -q -F "$HARDDISC_ARCHIVE" -d "$TEMP_HARDDISC"
[ -d "$TEMP_HARDDISC/HardDisc4/!Boot" ] ||
    die "HardDisc4 archive did not contain the expected boot tree"
cp -a "$TEMP_HARDDISC/HardDisc4" "$TEST_HOSTFS"
rm -rf -- "$TEMP_HARDDISC"
TEMP_HARDDISC=""

# RPCEmu needs the one-time desktop choices selected by the user.  They are
# RISC OS configuration data, not a compiler or runtime dependency.  Refuse
# a contaminated choice tree so the clean-install claim remains checkable.
if grep -a -r -q -E 'FreeBASIC|OMAGames|Apps\.Development\.\!GCC' \
    "$BASE_SOURCE/hostfs/!Boot/Choices"; then
    die "the base RISC OS Choices tree refers to a port payload"
fi
cp -a "$BASE_SOURCE/hostfs/!Boot/Choices" \
    "$TEST_HOSTFS/!Boot/"

TEST_SYSTEM="$TEST_HOSTFS/!Boot/Resources/!System"
[ ! -d "$TEST_HOSTFS/FreeBASIC" ] ||
    die "clean HostFS unexpectedly contains FreeBASIC staging"
[ ! -d "$TEST_HOSTFS/!GCC" ] ||
    die "clean HostFS unexpectedly contains GCC staging"
[ ! -f "$TEST_SYSTEM/310/Modules/SharedULib,ffa" ] ||
    die "clean HostFS unexpectedly contains the SharedUnixLibrary module"
[ ! -f "$TEST_SYSTEM/310/Modules/DRenderer,ffa" ] ||
    die "clean HostFS unexpectedly contains the DigitalRenderer module"

##############################################################################
# Local archive installation
##############################################################################

install_archive() {
    local archive="$1"
    local package_name
    local component
    local app_name

    package_name="$(control_field "$archive" Package)"
    TEMP_EXTRACT="$(mktemp -d "$OUTPUT_ROOT/.extract.XXXXXX")"
    unzip -q -F "$archive" -d "$TEMP_EXTRACT"

    case "$package_name" in
        GCC4|FreeBASIC)
            [ -d "$TEMP_EXTRACT/Apps/Development" ] ||
                die "$package_name archive has no development application"
            mkdir -p "$TEST_HOSTFS/Apps"
            cp -a "$TEMP_EXTRACT/Apps/." "$TEST_HOSTFS/Apps/"
            ;;
        SharedUnixLibrary)
            [ -s "$TEMP_EXTRACT/System/310/Modules/SharedULib,ffa" ] ||
                die "SharedUnixLibrary archive has no SharedULib module"
            mkdir -p "$TEST_SYSTEM/310/Modules"
            cp "$TEMP_EXTRACT/System/310/Modules/SharedULib,ffa" \
                "$TEST_SYSTEM/310/Modules/SharedULib,ffa"
            ;;
        DRenderer)
            [ -s "$TEMP_EXTRACT/System/310/Modules/DRenderer,ffa" ] ||
                die "DRenderer archive has no DigitalRenderer module"
            mkdir -p "$TEST_SYSTEM/310/Modules"
            cp "$TEMP_EXTRACT/System/310/Modules/DRenderer,ffa" \
                "$TEST_SYSTEM/310/Modules/DRenderer,ffa"
            ;;
        OMABehold|OMADuel999|OMAKinematics|OMANietzsche|OMAOpenMarket|\
        OMAQuestKing|OMARambo|OMAStarPhalanx|OpenSlicks)
            component="$(control_field "$archive" Components)"
            case "$component" in
                Apps.Games.*' (Movable LookAt)')
                    app_name="${component#Apps.Games.}"
                    app_name="${app_name% (Movable LookAt)}"
                    ;;
                *)
                    die "$package_name has an unsupported Components field"
                    ;;
            esac
            [ -s "$TEMP_EXTRACT/Apps/Games/$app_name/game,ff8" ] ||
                die "$package_name archive has no game AIF"
            mkdir -p "$TEST_HOSTFS/Apps"
            cp -a "$TEMP_EXTRACT/Apps/." "$TEST_HOSTFS/Apps/"
            printf '%s\t%s\t%s\n' "$package_name" "$app_name" "$archive" \
                >> "$OUTPUT_ROOT/games.tsv"
            ;;
        *)
            die "internal installer has no rule for $package_name"
            ;;
    esac

    rm -rf -- "$TEMP_EXTRACT"
    TEMP_EXTRACT=""
}

msg "installing only the local RiscPkg payloads"
printf '%s\n' $'package\tapp\tarchive' > "$OUTPUT_ROOT/games.tsv"
mkdir -p "$TEST_HOSTFS/Packages" "$TEST_HOSTFS/PackageTest"
for archive in "${PACKAGE_ARCHIVES[@]}"; do
    cp "$archive" "$TEST_HOSTFS/Packages/$(basename "$archive")"
    install_archive "$archive"
done

[ -s "$TEST_HOSTFS/Apps/Development/!GCC/bin/gcc,ff8" ] ||
    die "GCC4 did not install its native compiler"
[ -s "$TEST_HOSTFS/Apps/Development/!FreeBASIC/fbc,ff8" ] ||
    die "FreeBASIC did not install its native compiler"
[ -s "$TEST_SYSTEM/310/Modules/SharedULib,ffa" ] ||
    die "SharedUnixLibrary was not installed"
[ -s "$TEST_SYSTEM/310/Modules/DRenderer,ffa" ] ||
    die "DRenderer was not installed"

TEST_BINARY="$TEST_SOURCE/rpcemu-recompiler"
[ -x "$TEST_BINARY" ] || die "isolated RPCEmu binary is unavailable"

##############################################################################
# Compiler and media package execution
##############################################################################

CORE_LOG_BASE="$TEST_HOSTFS/PackageTest/core-log"
CORE_LOG_SAVED="$OUTPUT_ROOT/core.log"
TEST_TASK="$TEST_HOSTFS/!Boot/Choices/Boot/Tasks/RunPackageCore,feb"
mkdir -p "$(dirname "$TEST_TASK")"
mkdir -p "$TEST_HOSTFS/PackageTest/bas"

cat > "$TEST_HOSTFS/PackageTest/bas/package-hello,fff" <<'EOF'
' FreeBASIC RISC OS package acceptance
' -------------------------------------
'
' File: package-hello.bas
'
' Purpose:
'
'     Provide a non-interactive program for proving the compiler package can
'     build and execute a new FreeBASIC application after a clean install.
'
' Responsibilities:
'
'     - verify decimal conversion without UnixLib strtod()
'     - emit deterministic compiler-success markers
'
' This file intentionally does NOT contain:
'
'     - input handling
'     - graphics or sound setup
'     - dependencies on build-tree files
'
print "PKG_FBC_RGB_RED=" & hex( rgb( 255, 0, 0 ) )
print "PKG_FBC_RGB_GREEN=" & hex( rgb( 0, 255, 0 ) )
print "PKG_FBC_RGB_BLUE=" & hex( rgb( 0, 0, 255 ) )

dim as double decimal_value = val( "23.5" )
dim as uinteger decimal_word0 = *cptr( uinteger ptr, @decimal_value )
dim as uinteger decimal_word1 = *cptr( uinteger ptr, cptr( ubyte ptr, @decimal_value ) + 4 )

print "PKG_FBC_DECIMAL_WORD0=" & hex( decimal_word0 )
print "PKG_FBC_DECIMAL_WORD1=" & hex( decimal_word1 )

if decimal_word0 <> &h40378000 or decimal_word1 <> 0 then
    print "PKG_FBC_DECIMAL_ERROR"
    end 3
end if

print "PKG_FBC_DECIMAL_OK"
print "PKG_FBC_HELLO_OK"

' end of package-hello.bas
EOF

cat > "$TEST_HOSTFS/PackageTest/bas/package-math,fff" <<'EOF'
' FreeBASIC RISC OS package math acceptance
' ------------------------------------------
'
' File: package-math.bas
'
' Purpose:
'
'     Verify software-floating point math used by the threaded OMA games.
'
' Responsibilities:
'
'     - exercise SIN and COS with RISC OS ABI double values
'     - prove the multithreaded runtime can complete a math-only program
'
' This file intentionally does NOT contain:
'
'     - graphics setup
'     - game data or OMA source dependencies
'     - interactive input
'
dim as double circular_value = sin( 0.5 ) + cos( 0.5 )

if circular_value < 1.35 or circular_value > 1.36 then
    print "PKG_FBC_MATH_ERROR=" & str( circular_value )
    end 4
end if

print "PKG_FBC_MATH_OK"

' end of package-math.bas
EOF

cat > "$TEST_HOSTFS/PackageTest/RunCore,feb" <<EOF
| FreeBASIC RISC OS package core acceptance task
| ------------------------------------------------
|
| File: RunCore
|
| Purpose:
|
|     Exercise the installed system modules, GCC4, and FreeBASIC package.
|
| Responsibilities:
|
|     - load the module payloads installed from local archives
|     - start GCC4 and record its return code
|     - compile and run a new console program and both media link variants
|
| This generated task intentionally does NOT depend on HostFS build staging.

Set Sys\$RCLimit 2147483647
Spool HostFS:\$.PackageTest.core-log
Echo PKG_CORE_BEGIN
Echo PKG_CORE_PAD_1_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_CORE_PAD_2_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_CORE_PAD_3_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_CORE_PAD_4_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_CORE_PAD_5_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
| Static UnixLib applications load SharedULib from System:Modules themselves.
| The console executable and sfxlib media smoke below prove both module
| payloads by using their documented automatic load paths.
Echo PKG_MODULES_OK
Echo PKG_MODULES_PAD_1_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_MODULES_PAD_2_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_MODULES_PAD_3_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_MODULES_PAD_4_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_MODULES_PAD_5_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
| !GCC places gcc on Run\$Path in !Run rather than !Boot.  Activate it exactly
| as an installed user does before invoking the compiler by its command name.
Obey HostFS:\$.Apps.Development.!GCC.!Run
gcc -v
Set PackageTest\$GCCStatus <Sys\$ReturnCode>
Echo PKG_GCC_STATUS <PackageTest\$GCCStatus>
Echo PKG_GCC_PAD_1_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_GCC_PAD_2_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_GCC_PAD_3_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_GCC_PAD_4_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Echo PKG_GCC_PAD_5_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Obey HostFS:\$.Apps.Development.!FreeBASIC.!Boot
| The source file uses UnixLib's bas suffix directory.  Pass its canonical
| UnixLib path and the movable package prefix, as the installed Compile task
| does, rather than asking the RISC OS CLI to interpret a host filename.
fbc -v -prefix /HostFS:\$/Apps/Development/!FreeBASIC /HostFS:\$/PackageTest/package-hello.bas -x /HostFS:\$/PackageTest/package-hello
Set PackageTest\$HelloCompileStatus <Sys\$ReturnCode>
Echo PKG_FBC_HELLO_COMPILE_STATUS <PackageTest\$HelloCompileStatus>
Run HostFS:\$.PackageTest.package-hello
Set PackageTest\$HelloStatus <Sys\$ReturnCode>
Echo PKG_FBC_HELLO_STATUS <PackageTest\$HelloStatus>
fbc -v -R -d FB_RISCOS_PACKAGE_CAPTURE -prefix /HostFS:\$/Apps/Development/!FreeBASIC /HostFS:\$/Apps/Development/!FreeBASIC/examples/media-smoke.bas -x /HostFS:\$/PackageTest/package-media
Set PackageTest\$MediaCompileStatus <Sys\$ReturnCode>
Echo PKG_FBC_MEDIA_COMPILE_STATUS <PackageTest\$MediaCompileStatus>
Set UnixEnv\$package-media\$sfix "BMP:bmp"
Dir HostFS:\$.Apps.Development.!FreeBASIC.examples
Run HostFS:\$.PackageTest.package-media
Set PackageTest\$MediaStatus <Sys\$ReturnCode>
Echo PKG_FBC_MEDIA_STATUS <PackageTest\$MediaStatus>
| OMA applications link gfxlib2's multithreaded archive.  Exercise that
| exact installed library variant before qualifying the game packages.
fbc -v -mt -d FB_RISCOS_PACKAGE_CAPTURE -prefix /HostFS:\$/Apps/Development/!FreeBASIC /HostFS:\$/Apps/Development/!FreeBASIC/examples/media-smoke.bas -x /HostFS:\$/PackageTest/package-media-mt
Set PackageTest\$MediaMtCompileStatus <Sys\$ReturnCode>
Echo PKG_FBC_MEDIA_MT_COMPILE_STATUS <PackageTest\$MediaMtCompileStatus>
Set UnixEnv\$package-media-mt\$sfix "BMP:bmp"
Dir HostFS:\$.Apps.Development.!FreeBASIC.examples
Run HostFS:\$.PackageTest.package-media-mt
Set PackageTest\$MediaMtStatus <Sys\$ReturnCode>
Echo PKG_FBC_MEDIA_MT_STATUS <PackageTest\$MediaMtStatus>
fbc -v -mt -prefix /HostFS:\$/Apps/Development/!FreeBASIC /HostFS:\$/PackageTest/package-math.bas -x /HostFS:\$/PackageTest/package-math
Set PackageTest\$MathCompileStatus <Sys\$ReturnCode>
Echo PKG_FBC_MATH_COMPILE_STATUS <PackageTest\$MathCompileStatus>
Run HostFS:\$.PackageTest.package-math
Set PackageTest\$MathStatus <Sys\$ReturnCode>
Echo PKG_FBC_MATH_STATUS <PackageTest\$MathStatus>
Echo PKG_CORE_DONE
Spool

| end of RunCore
EOF

CORE_SLOT_KIB=$((RPCEMU_MEMORY * 3 * 1024 / 4))
cat > "$TEST_TASK" <<EOF
| FreeBASIC RISC OS package core launch task
| -------------------------------------------
|
| File: RunPackageCore
|
| Purpose:
|
|     Allocate a large application slot and start the package core test.
|
| This generated task intentionally does NOT run any build-staging program.

WimpSlot -min ${CORE_SLOT_KIB}K
Obey HostFS:\$.PackageTest.RunCore

| end of RunPackageCore
EOF

msg "running GCC4, module, and FreeBASIC package acceptance"
rm -f -- "$CORE_LOG_BASE,ffd" "$CORE_LOG_BASE" "$CORE_LOG_SAVED"
CURRENT_SCREENSHOT_LABEL="core"
launch_emulator
TEST_LOG="$(wait_for_marker "$CORE_LOG_BASE" 'PKG_CORE_DONE')"
stop_emulator
strings -a "$TEST_LOG" > "$CORE_LOG_SAVED"
cat "$CORE_LOG_SAVED"

for marker in \
    '^PKG_MODULES_OK$' \
    '^PKG_GCC_STATUS 0$' \
    '^PKG_FBC_HELLO_COMPILE_STATUS 0$' \
    '^PKG_FBC_HELLO_STATUS 0$' \
    '^PKG_FBC_HELLO_OK$' \
    '^PKG_FBC_DECIMAL_OK$' \
    '^PKG_FBC_MEDIA_COMPILE_STATUS 0$' \
    '^PKG_FBC_MEDIA_STATUS 0$' \
    '^PKG_FBC_MEDIA_MT_COMPILE_STATUS 0$' \
    '^PKG_FBC_MEDIA_MT_STATUS 0$' \
    '^PKG_FBC_MATH_COMPILE_STATUS 0$' \
    '^PKG_FBC_MATH_STATUS 0$' \
    '^PKG_FBC_MATH_OK$' \
    '^FB_RISCOS_GFX_WINDOWED_OK$' \
    '^FB_RISCOS_GFX_FULLSCREEN_OK$' \
    '^FB_RISCOS_SFX_DRIVER=RISC OS DigitalRenderer$' \
    '^FB_RISCOS_MEDIA_SMOKE_OK$'; do
    grep -q "$marker" "$CORE_LOG_SAVED" ||
        die "package core marker is missing: $marker; see $CORE_LOG_SAVED"
done

##############################################################################
# Individually packaged OMA application execution
##############################################################################

GAME_RESULTS="$OUTPUT_ROOT/game-results.tsv"
printf '%s\n' $'package\tapp\tstatus\tdetail\tlog' > "$GAME_RESULTS"
GAME_SLOT_KIB=$((RPCEMU_MEMORY * 3 * 1024 / 4))

run_game_package() {
    local package_name="$1"
    local app_name="$2"
    local game_log_base="$TEST_HOSTFS/PackageTest/games/$package_name"
    local game_log=""
    local saved_log="$OUTPUT_ROOT/games-$package_name.log"
    local elapsed=0
    local final_screenshot=""
    local initial_screenshot=""
    local next_screenshot="$SCREENSHOT_INTERVAL_SECONDS"
    local changed_pixels=""
    local blank_work_area_pixels=""
    local status="fail"
    local detail=""

    mkdir -p "$TEST_HOSTFS/PackageTest/games"
    rm -f -- "$game_log_base,ffd" "$game_log_base" "$saved_log"

    cat > "$TEST_HOSTFS/PackageTest/RunGame,feb" <<EOF
| $package_name RISC OS package launch task
| ------------------------------------------
|
| File: RunGame
|
| Purpose:
|
|     Start the installed OMA application and capture launch evidence.
|
| This generated task intentionally does NOT provide game runtime files.

Set Sys\$RCLimit 2147483647
Spool HostFS:\$.PackageTest.games.$package_name
Echo PKG_GAME_BEGIN_$package_name
Run HostFS:\$.Apps.Games.$app_name.!Run
Set PackageTest\$GameStatus <Sys\$ReturnCode>
Echo PKG_GAME_RETURN_$package_name <PackageTest\$GameStatus>
Spool

| end of RunGame
EOF

    cat > "$TEST_TASK" <<EOF
| $package_name RISC OS package boot task
| ----------------------------------------
|
| File: RunPackageCore
|
| Purpose:
|
|     Allocate an application slot and start one installed game package.
|
| This generated task intentionally does NOT start another package.

WimpSlot -min ${GAME_SLOT_KIB}K
Obey HostFS:\$.PackageTest.RunGame

| end of RunPackageCore
EOF

    msg "running $package_name from its installed package payload"
    CURRENT_SCREENSHOT_LABEL="game-$package_name"
    launch_emulator
    initial_screenshot="$LAST_SCREENSHOT_PATH"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        game_log="$(find_runtime_log "$game_log_base")"
        if [ -n "$game_log" ] &&
           grep -a -q "PKG_GAME_RETURN_$package_name" "$game_log"; then
            detail="application returned before the stability interval"
            break
        fi

        if ! kill -0 "$RPCEMU_PID" 2>/dev/null; then
            detail="RPCEmu exited before package launch qualified"
            break
        fi

        # RISC OS flushes the Spool file when the launched program exits.
        # A graphical game therefore cannot expose its begin marker while it
        # remains active.  Treat a non-returning launcher and a live emulator
        # as the launch condition, then retain host screenshots for visual
        # confirmation that it is a game screen rather than an alert or
        # TaskWindow prompt.
        if [ "$SCREENSHOT_INTERVAL_SECONDS" -gt 0 ] &&
           [ "$elapsed" -ge "$next_screenshot" ]; then
            capture_emulator_screenshot "$CURRENT_SCREENSHOT_LABEL"
            next_screenshot=$((next_screenshot + SCREENSHOT_INTERVAL_SECONDS))
        fi

        if [ "$elapsed" -ge "$STABILITY_SECONDS" ]; then
            capture_emulator_screenshot "${CURRENT_SCREENSHOT_LABEL}-final"
            final_screenshot="$LAST_SCREENSHOT_PATH"

            # A persistent foreground process alone is not enough evidence:
            # a failed graphics initialization can leave an all-black screen
            # while the game waits for input.  On this 640x512 emulator the
            # smallest known title screen changes 2,579 pixels from launch,
            # so 1,024 leaves room for static text screens while rejecting a
            # cursor-only black frame.
            if [ "$SCREENSHOT_TOOL" = "window-import" ]; then
                blank_work_area_pixels="$(game_blank_work_area_pixels \
                    "$final_screenshot" || true)"
                if [ -n "$blank_work_area_pixels" ] &&
                   [ "$blank_work_area_pixels" -gt \
                     "$GAME_MAX_SOLID_WHITE_PIXELS" ]; then
                    detail="visual frame contains ${blank_work_area_pixels} untouched white work-area pixels"
                    break
                fi

                changed_pixels="$(game_frame_delta "$initial_screenshot" \
                    "$final_screenshot" || true)"
                if [ -z "$changed_pixels" ]; then
                    detail="unable to compare package game frames"
                    break
                fi
                if [ "$changed_pixels" -lt "$GAME_MIN_CHANGED_PIXELS" ]; then
                    detail="visual frame changed only ${changed_pixels} pixels"
                    break
                fi
            fi

            status="pass"
            detail="launcher active for ${STABILITY_SECONDS}s; visual delta ${changed_pixels:-unchecked} pixels"
            break
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    if [ "$status" != "pass" ] && [ -z "$detail" ]; then
        detail="timed out after ${TIMEOUT_SECONDS}s without a persistent launcher"
    fi

    stop_emulator
    if [ -n "$game_log" ] && [ -f "$game_log" ]; then
        strings -a "$game_log" > "$saved_log"
    else
        : > "$saved_log"
    fi

    printf '%s\t%s\t%s\t%s\t%s\n' \
        "$package_name" "$app_name" "$status" "$detail" \
        "$(basename "$saved_log")" >> "$GAME_RESULTS"

    [ "$status" = "pass" ] ||
        die "$package_name package failed: $detail; see $saved_log"
}

while IFS=$'\t' read -r package_name app_name archive; do
    [ "$package_name" != "package" ] || continue
    run_game_package "$package_name" "$app_name"
done < "$OUTPUT_ROOT/games.tsv"

rm -f -- "$TEST_TASK"
TEST_TASK=""

echo
echo '==> RISC OS local package acceptance passed'
echo "    Core log:      $CORE_LOG_SAVED"
echo "    Game results:  $GAME_RESULTS"
echo "    Test runtime:  $TEST_RUNTIME"

# end of riscos-test-packages.sh
