#!/usr/bin/env bash
#
# FreeBASIC AROS m68k graphics smoke test
# ----------------------------------------
#
# File: aros-run-m68k-gfx-colour-smoke.sh
#
# Purpose:
#
#     Build and visually qualify the 32-bit primary-colour gfxlib2 path on
#     m68k AROS under FS-UAE.
#
# Responsibilities:
#
#     - cross-compile the AROS primary-colour smoke fixture
#     - convert its ELF output to the executable Hunk format used by AROS
#     - stage the program on a host directory mounted as GfxSmoke:
#     - boot a graphics-capable AROS m68k image under FS-UAE
#     - save a capture after the guest reports that the colours were drawn
#
# This file intentionally does NOT contain:
#
#     - AROS SDK or FreeBASIC compiler construction
#     - general OMA game launch logic
#     - an automated judgement of the screenshot's visual contents
#
# Test boundary:
#
#     The image is the acceptance artifact.  The left, middle, and right
#     regions must be red, green, and blue respectively.  The fixture writes
#     its marker after the framebuffer fills, preventing a startup screen from
#     being mistaken for test output.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Paths and process state
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

AROS_ROOT="${AROS_ROOT:-$ROOT/out/aros}"
OUTPUT_ROOT="${AROS_GFX_COLOUR_OUTDIR:-$AROS_ROOT/gfx-colour-smoke/m68k}"
BASE_ISO="${AROS_M68K_ISO:-}"
SOURCE_FILE="${AROS_GFX_FIXTURE:-$ROOT/examples/aros/gfx-colour-smoke.bas}"
ASSET_FILE="${AROS_GFX_ASSET:-}"
FBC="$ROOT/bin/fbc"
TOOLCHAIN="$AROS_ROOT/toolchain-amiga-m68k"
AROS_BUILD="$AROS_ROOT/build-amiga-m68k"
TIMEOUT_SECONDS=120
HEADLESS=0
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

require_value() {
    local option="$1"
    local value="${2-}"

    [ -n "$value" ] || die "$option requires a value"
}

stop_emulator() {
    local elapsed=0

    [ -n "$EMULATOR_PID" ] || return 0
    if kill -0 "$EMULATOR_PID" 2>/dev/null; then
        kill "$EMULATOR_PID" 2>/dev/null || true
    fi
    while kill -0 "$EMULATOR_PID" 2>/dev/null && [ "$elapsed" -lt 5 ]; do
        sleep 1
        elapsed=$((elapsed + 1))
    done
    if kill -0 "$EMULATOR_PID" 2>/dev/null; then
        kill -KILL "$EMULATOR_PID" 2>/dev/null || true
    fi
    wait "$EMULATOR_PID" 2>/dev/null || true
    EMULATOR_PID=""
}

capture_emulator_window() {
    local elapsed=0
    local screenshot="$1"
    local window_id=""

    [ "$HEADLESS" -eq 0 ] || return 0

    while [ "$elapsed" -lt 15 ]; do
        window_id="$(xdotool search --onlyvisible --pid "$EMULATOR_PID" \
            2>/dev/null | tail -1 || true)"
        if [ -z "$window_id" ]; then
            window_id="$(xdotool search --onlyvisible --name 'FS-UAE' \
                2>/dev/null | tail -1 || true)"
        fi
        if [ -n "$window_id" ]; then
            import -window "$window_id" "$screenshot"
            [ -s "$screenshot" ] || die "captured an empty FS-UAE window"
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done

    die "could not find the FS-UAE display window"
}

wait_for_draw_marker() {
    local elapsed=0
    local marker="$1"

    while [ "$elapsed" -lt "$TIMEOUT_SECONDS" ]; do
        if grep -F -q 'AROS_GFX_COLOUR_SMOKE: DRAWN' "$marker" 2>/dev/null; then
            return 0
        fi
        kill -0 "$EMULATOR_PID" 2>/dev/null ||
            die "FS-UAE exited before the colour fixture drew its window"
        sleep 1
        elapsed=$((elapsed + 1))
    done

    die "timed out waiting for the AROS colour fixture"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-run-m68k-gfx-colour-smoke.sh [options]

Options:
  --aros-root DIR     AROS workspace. Default: out/aros
  --aros-iso FILE     AROS m68k ISO to customise. Default: build output
  --fixture FILE      FreeBASIC smoke fixture. Default: primary-colour test
  --asset FILE        Optional file staged on GfxSmoke: for the fixture
  --out-dir DIR       Evidence directory. Default: out/aros/gfx-colour-smoke/m68k
  --timeout SEC       Guest startup timeout. Default: 120
  --headless          Run without capturing the FS-UAE window.
  -h, --help          Show this help.
EOF
}

trap stop_emulator EXIT

##############################################################################
# Command-line parsing and prerequisites
##############################################################################

while [ "$#" -gt 0 ]; do
    case "$1" in
        --aros-root)
            require_value "$1" "${2-}"
            AROS_ROOT="$2"
            TOOLCHAIN="$AROS_ROOT/toolchain-amiga-m68k"
            AROS_BUILD="$AROS_ROOT/build-amiga-m68k"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --aros-iso)
            require_value "$1" "${2-}"
            BASE_ISO="$2"
            shift 2
            ;;
        --fixture)
            require_value "$1" "${2-}"
            SOURCE_FILE="$2"
            shift 2
            ;;
        --asset)
            require_value "$1" "${2-}"
            ASSET_FILE="$2"
            shift 2
            ;;
        --timeout)
            require_value "$1" "${2-}"
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --headless)
            HEADLESS=1
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

#
# An existing, known-bootable AROS image may be supplied while the SDK image
# is being built separately.  The runner writes its temporary startup files
# into a copy, so the selected base image is never modified.
#
if [ -z "$BASE_ISO" ]; then
    BASE_ISO="$AROS_BUILD/distfiles/aros-amiga-m68k.iso"
fi

for tool in cp file grep mkdir sleep timeout xorriso; do
    require_command "$tool"
done
require_command fs-uae
if [ "$HEADLESS" -eq 0 ]; then
    require_command import
    require_command xdotool
    [ -n "${DISPLAY:-}" ] ||
        die "DISPLAY is unset; use --headless or provide an X display"
fi

[ -x "$FBC" ] || die "FreeBASIC compiler not found: $FBC"
[ -x "$TOOLCHAIN/m68k-aros-gcc" ] ||
    die "m68k AROS cross compiler not found: $TOOLCHAIN/m68k-aros-gcc"
[ -x "$AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" ] ||
    die "AROS elf2hunk tool not found under: $AROS_BUILD"
[ -f "$BASE_ISO" ] ||
    die "AROS m68k ISO not found: $BASE_ISO"
[ -f "$AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-rom.bin" ] ||
    die "AROS m68k ROM not found under: $AROS_BUILD"
[ -f "$AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-ext.bin" ] ||
    die "AROS m68k extension ROM not found under: $AROS_BUILD"
[ -f "$SOURCE_FILE" ] || die "graphics fixture not found: $SOURCE_FILE"
if [ -n "$ASSET_FILE" ]; then
    [ -f "$ASSET_FILE" ] || die "fixture asset not found: $ASSET_FILE"
fi

##############################################################################
# Fixture build and AROS media construction
##############################################################################

mkdir -p "$OUTPUT_ROOT/control" "$OUTPUT_ROOT/host" "$OUTPUT_ROOT/logs"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"

PROGRAM_ELF="$OUTPUT_ROOT/gfx-colour-smoke.elf"
PROGRAM_HUNK="$OUTPUT_ROOT/host/gfx-colour-smoke"
MARKER="$OUTPUT_ROOT/host/gfx-colour-smoke.drawn"
STARTUP_SEQUENCE="$OUTPUT_ROOT/control/Startup-Sequence"
USER_STARTUP="$OUTPUT_ROOT/control/User-Startup"
CURRENT_ISO="$OUTPUT_ROOT/current.iso"
CONFIG_FILE="$OUTPUT_ROOT/control/gfx-colour-smoke.fs-uae"
EMULATOR_LOG="$OUTPUT_ROOT/logs/fs-uae.log"
SCREENSHOT="$OUTPUT_ROOT/gfx-colour-smoke.png"

msg "building the m68k AROS colour fixture"
PATH="$TOOLCHAIN:$PATH" "$FBC" -target aros-m68k -mt -O 0 \
    -i "$ROOT/inc/aros" -i "$ROOT/inc" "$SOURCE_FILE" -x "$PROGRAM_ELF"

file "$PROGRAM_ELF" | grep -F -q 'ELF 32-bit MSB'
"$AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" "$PROGRAM_ELF" "$PROGRAM_HUNK"
chmod u+x "$PROGRAM_HUNK"
rm -f -- "$MARKER" "$SCREENSHOT"

if [ -n "$ASSET_FILE" ]; then
    #
    # Fixture assets are copied by basename because the mounted GfxSmoke:
    # volume is the fixture's complete test namespace.
    #
    cp "$ASSET_FILE" "$OUTPUT_ROOT/host/$(basename "$ASSET_FILE")"
fi

cat > "$STARTUP_SEQUENCE" <<'EOF'
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

cat > "$USER_STARTUP" <<'EOF'
FailAt 21
Stack 1048576
GfxSmoke:gfx-colour-smoke
Wait 120
Shutdown
EOF

cp --reflink=auto --sparse=always \
    "$BASE_ISO" "$CURRENT_ISO"
if ! xorriso -dev "$CURRENT_ISO" \
    -boot_image any replay \
    -map "$STARTUP_SEQUENCE" /S/Startup-Sequence \
    -map "$USER_STARTUP" /S/User-Startup \
    -commit -end > "$OUTPUT_ROOT/logs/xorriso.log" 2>&1; then
    tail -80 "$OUTPUT_ROOT/logs/xorriso.log" >&2
    die "could not construct the m68k AROS smoke-test ISO"
fi

cat > "$CONFIG_FILE" <<EOF
[fs-uae]
amiga_model = A4000/040
kickstart_file = $AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-rom.bin
kickstart_ext_file = $AROS_BUILD/bin/amiga-m68k/AROS/boot/amiga/aros-ext.bin
cdrom_drive_0 = $CURRENT_ISO
hard_drive_0 = $OUTPUT_ROOT/host
hard_drive_0_label = GfxSmoke
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

##############################################################################
# Guest execution and evidence capture
##############################################################################

msg "booting the m68k AROS colour fixture"
: > "$EMULATOR_LOG"
timeout "$TIMEOUT_SECONDS" fs-uae "$CONFIG_FILE" --no-gui \
    > "$EMULATOR_LOG" 2>&1 &
EMULATOR_PID="$!"

wait_for_draw_marker "$MARKER"

#
# The fixture writes its marker after the fills.  A short interval lets the
# guest's Intuition presentation path reach the FS-UAE host window.
#
sleep 1
capture_emulator_window "$SCREENSHOT"

if [ "$HEADLESS" -eq 0 ]; then
    echo "==> saved colour evidence: $SCREENSHOT"
    echo "    Expected order: red, green, blue from left to right."
fi

stop_emulator

# end of aros-run-m68k-gfx-colour-smoke.sh
