#!/usr/bin/env bash
#
# Project: FreeBASIC OMA games for AROS
# --------------------------------------
#
# File: aros-build-oma-games.sh
#
# Purpose:
#
#     Cross-build and individually package the available OMA games for AROS.
#
# Responsibilities:
#
#     - keep the complete OMA and OpenSlicks source matrix in one place
#     - build every available game for x86_64, ARM hard-float, and m68k
#     - keep the AROS m68k 68000 and Hunk policy out of generic m68k support
#     - stage only the runtime assets used by each game
#     - create and verify one AROS PKG1 archive and download ZIP per game
#     - record absent source trees without hiding coverage gaps
#
# This file intentionally does NOT contain:
#
#     - AROS SDK or operating-system construction
#     - emulator launch qualification
#     - fbctests or Exampleageddon orchestration
#
# Architecture policy:
#
#     Generic m68k compiler behavior is shared with future operating systems.
#     This workflow selects the AROS-specific 68000 soft-float baseline through
#     the aros-m68k target and converts only that target's linked ELF to Hunk.

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
PACKAGE_OUTDIR="${AROS_PACKAGE_OUTDIR:-$AROS_ROOT/packages/oma}"
TARGETS="${AROS_TARGETS:-x86_64,m68k,arm}"
SELECTED_GAMES_TEXT=""
PACKAGE_REVISION="${AROS_OMA_PACKAGE_REVISION:-1}"

BUILD_LIBRARIES=1
PACKAGE_GAMES=1
STRICT_MATRIX=0
JOBS=""
TEMP_ROOT=""
PACKAGE_STAGE=""
EXTRACT_STAGE=""

##############################################################################
# Complete game matrix
##############################################################################

# Fields are tab-separated. The last field is the availability probe. The
# OpenSlicks row is distinguished from the fourteen-title OMA suite while
# remaining part of the playable corpus requested for this port.
GAME_ROWS=(
    $'arkanoid\tArkanoid\tOMA\tOMA-Arkanoid\tOMA/ArkanoidTest/ArkanoidTest.bas'
    $'behold\tBehold\tOMA\tOMA-Behold\tOMA/Behold/Behold.bas'
    $'demoderby\tDemolition Derby\tOMA\tOMA-DemolitionDerby\tOMA/DemolitionDerby/main.bas'
    $'duel999\tDuel 999\tOMA\tOMA-Duel999\tOMA/duel999/SD_Main.bas'
    $'kinematics\tKinematics\tOMA\tOMA-Kinematics\tOMA/kinematics/kinematic_man_two_bodies_self_collision_friction.bas'
    $'nietzsche\tNietzsche Special Edition\tOMA\tOMA-Nietzsche\tOMA/NietzscheSE-MSDOS-1.1/Nietzsche/src/win32/win11.bas'
    $'qfak\tQuest for a King\tOMA\tOMA-QuestForAKing\tOMA/QuestForAKing-Win32-1.5/src/win11.bas'
    $'rambo\tRambo vs Kitty Cat\tOMA\tOMA-RamboVsKittyCat\tOMA/RamboVsKittyCat-Win32-0.1/killquest.bas'
    $'starphalanx\tStar Phalanx\tOMA\tOMA-StarPhalanx\tOMA/StarPhalanx-win32-0.5/entryv2.bas'
    $'openmarket\tOpen Market\tOMA\tOMA-OpenMarket\tOMA/Tamper/tamper/src/openmarket_bootstrap.bas'
    $'openhostility\tOpenHostility\tOMA\tOMA-OpenHostility\tOMA/Scorched Earth/src/scorch_gfx.bas'
    $'turbotrek\tTurboTrek\tOMA\tOMA-TurboTrek\tOMA/TurboTrek/src/turbotrek/main.bas'
    $'vtrek\tvtrek\tOMA\tOMA-vtrek\tOMA/vtrek/src/vtrek.bas'
    $'openwallstreet\tOpenWallStreet\tOMA\tOMA-OpenWallStreet\tOMA/WSR5_3/src/raider.bas'
    $'openslicks\tOpen Slicks Racing\tOpenSlicks\tOpenSlicks\tOMA/Slicks n Slide/src/slicks.bas'
)

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

require_file() {
    local path="$1"

    [ -f "$path" ] || die "required file not found: $path"
}

cleanup() {
    if [ -n "$PACKAGE_STAGE" ] &&
       [[ "$PACKAGE_STAGE" == "$PACKAGE_OUTDIR"/.package-stage.* ]] &&
       [ -d "$PACKAGE_STAGE" ]; then
        rm -rf -- "$PACKAGE_STAGE"
    fi

    if [ -n "$EXTRACT_STAGE" ] &&
       [[ "$EXTRACT_STAGE" == "$PACKAGE_OUTDIR"/.extract-stage.* ]] &&
       [ -d "$EXTRACT_STAGE" ]; then
        rm -rf -- "$EXTRACT_STAGE"
    fi

    if [ -n "$TEMP_ROOT" ] &&
       [[ "$TEMP_ROOT" == "$OUTPUT_ROOT"/.build-stage.* ]] &&
       [ -d "$TEMP_ROOT" ]; then
        rm -rf -- "$TEMP_ROOT"
    fi
}

replace_directory() {
    local destination="$2"
    local source="$1"

    [ -d "$source" ] || die "staging source is not a directory: $source"
    [ "$destination" != "/" ] || die "refusing to replace the filesystem root"

    rm -rf -- "$destination"
    mkdir -p "$(dirname "$destination")"
    mv "$source" "$destination"
}

copy_runtime_file() {
    local destination="$2"
    local source="$1"

    require_file "$source"
    mkdir -p "$(dirname "$destination")"
    cp "$source" "$destination"
}

copy_runtime_tree() {
    local destination="$2"
    local source="$1"

    [ -d "$source" ] || die "required runtime directory not found: $source"
    mkdir -p "$destination"
    cp -a "$source/." "$destination/"

    # Repository metadata and host executables are not target runtime data.
    find "$destination" -type d -name .git -prune -exec rm -rf -- {} +
    find "$destination" -type f \
        \( -iname 'Thumbs.db' -o -iname 'Thumbs(1).db' \
           -o -iname '*.exe' -o -iname '*.dll' -o -iname '*.so' \
           -o -iname '*.apk' -o -iname '*.idsig' \) -delete
}

usage() {
    cat <<EOF
Usage: ./build_scripts/aros-build-oma-games.sh [options]

Options:
  --targets LIST       Comma-separated x86_64,m68k,arm list. Default: all
  --games LIST         Comma-separated game keys. Default: complete matrix
  --aros-root DIR      AROS SDK and build workspace. Default: out/aros
  --out-dir DIR        Build and staged-game root. Default: out/aros/oma
  --package-dir DIR    Package output. Default: out/aros/packages/oma
  --revision N         Package revision. Default: 1
  --skip-libs          Reuse existing rtlib, gfxlib2, and sfxlib archives.
  --no-package         Build and stage games without PKG and ZIP archives.
  --strict             Fail if a selected source tree is absent.
  --jobs N             Parallel library build jobs. Default: detected CPUs
  -h, --help           Show this help.

Game keys:
  arkanoid, behold, demoderby, duel999, kinematics, nietzsche, qfak,
  rambo, starphalanx, openmarket, openhostility, turbotrek, vtrek,
  openwallstreet, openslicks

The per-target manifest records built and absent games. Restoring an absent
source tree is sufficient for a later run to include it automatically.
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
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --package-dir)
            require_value "$1" "${2-}"
            PACKAGE_OUTDIR="$2"
            shift 2
            ;;
        --revision)
            require_value "$1" "${2-}"
            PACKAGE_REVISION="$2"
            shift 2
            ;;
        --skip-libs)
            BUILD_LIBRARIES=0
            shift
            ;;
        --no-package)
            PACKAGE_GAMES=0
            shift
            ;;
        --strict)
            STRICT_MATRIX=1
            shift
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
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

for positive_integer in "$JOBS" "$PACKAGE_REVISION"; do
    case "$positive_integer" in
        ''|*[!0-9]*|0) die "jobs and revision must be positive integers" ;;
    esac
done

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

declare -A VALID_GAMES=()
for row in "${GAME_ROWS[@]}"; do
    IFS=$'\t' read -r key _ <<< "$row"
    VALID_GAMES["$key"]=1
done

if [ -n "$SELECTED_GAMES_TEXT" ]; then
    IFS=',' read -r -a SELECTED_GAMES <<< "$SELECTED_GAMES_TEXT"
    for key in "${SELECTED_GAMES[@]}"; do
        [ -n "${VALID_GAMES[$key]:-}" ] || die "unknown game key: $key"
    done
else
    SELECTED_GAMES=()
fi

game_is_selected() {
    local selected
    local sought="$1"

    [ "${#SELECTED_GAMES[@]}" -gt 0 ] || return 0
    for selected in "${SELECTED_GAMES[@]}"; do
        [ "$selected" = "$sought" ] && return 0
    done
    return 1
}

##############################################################################
# Target mapping and validation
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
        *)
            die "internal unsupported target: $target"
            ;;
    esac
}

validate_target_elf() {
    local elf="$1"
    local header
    local target="$2"

    header="$("$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-readelf" -h "$elf")"
    grep -q 'OS/ABI:.*AROS' <<< "$header" || die "$elf does not have AROS OSABI"

    case "$target" in
        x86_64)
            grep -q 'Class:.*ELF64' <<< "$header" || die "$elf is not ELF64"
            grep -q 'Machine:.*X86-64' <<< "$header" || die "$elf is not x86_64"
            ;;
        arm)
            grep -q 'Class:.*ELF32' <<< "$header" || die "$elf is not ELF32"
            grep -q 'Data:.*little endian' <<< "$header" || die "$elf is not little-endian"
            grep -q 'Machine:.*ARM' <<< "$header" || die "$elf is not ARM"
            grep -q 'Flags:.*Version5 EABI' <<< "$header" || die "$elf is not ARM EABI5"
            ;;
        m68k)
            grep -q 'Class:.*ELF32' <<< "$header" || die "$elf is not ELF32"
            grep -q 'Data:.*big endian' <<< "$header" || die "$elf is not big-endian"
            grep -q 'Machine:.*MC68000' <<< "$header" || die "$elf is not m68k"
            grep -q 'Flags:.*m68000' <<< "$header" || die "$elf lacks the AROS 68000 baseline"
            ;;
    esac
}

for tool in base64 diff file find grep make python3 zip; do
    require_command "$tool"
done

FBC="$ROOT/bin/fbc"
PKG_TOOL="$SCRIPT_DIR/aros-pkg.py"
[ -x "$FBC" ] || die "host FreeBASIC compiler not found: $FBC"
[ -f "$PKG_TOOL" ] || die "AROS package tool not found: $PKG_TOOL"

mkdir -p "$OUTPUT_ROOT" "$PACKAGE_OUTDIR"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
[ "$OUTPUT_ROOT" != "/" ] || die "refusing to use / as the output root"
[ "$PACKAGE_OUTDIR" != "/" ] || die "refusing to use / as the package root"

##############################################################################
# Per-game compiler definitions
##############################################################################

compile_game() {
    local key="$1"
    local output="$2"
    local target="$3"
    local working_directory
    local -a sources=()
    local -a extra_arguments=()

    case "$key" in
        arkanoid)
            working_directory="$ROOT/OMA/ArkanoidTest"
            sources=("$working_directory/ArkanoidTest.bas")
            ;;
        behold)
            working_directory="$ROOT/OMA/Behold"
            sources=("$working_directory/Behold.bas")
            ;;
        demoderby)
            working_directory="$ROOT/OMA/DemolitionDerby"
            sources=("$working_directory/main.bas")
            ;;
        duel999)
            working_directory="$ROOT/OMA/duel999"
            sources=("$working_directory/SD_Main.bas")
            ;;
        kinematics)
            working_directory="$ROOT/OMA/kinematics"
            sources=("$working_directory/kinematic_man_two_bodies_self_collision_friction.bas")
            ;;
        nietzsche)
            working_directory="$ROOT/OMA/NietzscheSE-MSDOS-1.1/Nietzsche"
            sources=("$working_directory/src/win32/win11.bas")
            ;;
        qfak)
            working_directory="$ROOT/OMA/QuestForAKing-Win32-1.5"
            sources=("$working_directory/src/win11.bas")
            ;;
        rambo)
            working_directory="$ROOT/OMA/RamboVsKittyCat-Win32-0.1"
            sources=("$working_directory/killquest.bas")
            ;;
        starphalanx)
            working_directory="$ROOT/OMA/StarPhalanx-win32-0.5"
            sources=("$working_directory/entryv2.bas")
            ;;
        openmarket)
            working_directory="$ROOT/OMA/Tamper/tamper"
            sources=("$working_directory/src/openmarket_bootstrap.bas")
            ;;
        openhostility)
            working_directory="$ROOT/OMA/Scorched Earth"
            sources=(
                "$working_directory/src/scorch_gfx.bas"
                "$working_directory/src/scorch_platform.bas"
                "$working_directory/src/scorch_engine.bas"
                "$working_directory/src/scorch_io.bas"
                "$working_directory/src/scorch_mtn.bas"
            )
            extra_arguments=(
                -exx -w all
                -i "$working_directory/src"
                -i "$working_directory/src/omaGUI"
            )
            ;;
        turbotrek)
            working_directory="$ROOT/OMA/TurboTrek"
            sources=("$working_directory/src/turbotrek/main.bas")
            local module
            for module in \
                clone_scenario_file clone_scenario_import \
                clone_scenario_runtime clone_scenario clone_session_file \
                damage deflectors device_condition disruptor drone_projectile \
                residual_damage residual_damage_resolution original_random \
                photon plasma_projectile pulser_redirect critical_damage \
                energy fire_control game_state terrain_damage movement \
                operations operations_display repair robot_ai scenario_catalog \
                scenario_metadata scenario_objects scenario_world self_destruct \
                sensors ship_definition simulation weapon_targeting world; do
                sources+=("$working_directory/src/turbotrek/$module.bas")
            done
            extra_arguments=(
                -i "$working_directory/src/omaGUI"
                -i "$working_directory/src/turbotrek"
            )
            ;;
        vtrek)
            working_directory="$ROOT/OMA/vtrek"
            sources=("$working_directory/src/vtrek.bas")
            ;;
        openwallstreet)
            working_directory="$ROOT/OMA/WSR5_3"
            sources=("$working_directory/src/raider.bas")
            extra_arguments=(
                -lang fblite
                -i "$working_directory/src"
                -i "$working_directory/src/omaGUI"
            )
            ;;
        openslicks)
            working_directory="$ROOT/OMA/Slicks n Slide/src"
            sources=(
                "$working_directory/slicks.bas"
                "$working_directory/slicks_app.bas"
                "$working_directory/slicks_font.bas"
                "$working_directory/slicks_track.bas"
                "$working_directory/slicks_input.bas"
                "$working_directory/slicks_vehicle.bas"
                "$working_directory/slicks_game.bas"
                "$working_directory/slicks_render.bas"
                "$working_directory/slicks_menu.bas"
            )
            extra_arguments=(-i "$working_directory")
            ;;
        *)
            die "no compiler definition for game: $key"
            ;;
    esac

    local source
    for source in "${sources[@]}"; do
        require_file "$source"
    done

    #
    # The native m68k emulator qualification is intentionally a low-memory
    # smoke test.  Keeping these historical games at the compiler's baseline
    # optimization level avoids turning the launch test into a toolchain
    # stress test before their own behaviour is observed.
    #
    if [ "$target" = m68k ]; then
        extra_arguments+=(-O 0)
    fi

    (
        cd "$working_directory"
        env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
            "$FBC" \
                -target "$MAP_FBC_TARGET" \
                -mt \
                -i "$ROOT/inc" \
                "${extra_arguments[@]}" \
                "${sources[@]}" \
                -x "$output"
    )

    [ -s "$output" ] || die "FreeBASIC did not produce $output"
    validate_target_elf "$output" "$target"
}

##############################################################################
# Curated game assets
##############################################################################

stage_open_slicks_fixture() {
    local game_stage="$1"

    mkdir -p "$game_stage/TRACKS"

    # This version 2 track was generated by OpenSlicks' fixture builder. It
    # contains two ordinary road records and no original commercial game art.
    base64 -d > "$game_stage/TRACKS/ASFALTTI.SS" <<'EOF'
AABTUwACAY8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABQADIAAAAAAAAQXV0aG9yAFRpdGxlAAKAAtACMALQ
AAIAHigaBwAfKRsH
EOF
}

stage_game_assets() {
    local game_stage="$2"
    local key="$1"
    local root
    local source

    case "$key" in
        arkanoid|behold|demoderby|kinematics|turbotrek|vtrek)
            ;;
        duel999)
            root="$ROOT/OMA/duel999"
            copy_runtime_file "$root/defaults.cfg" "$game_stage/defaults.cfg"
            copy_runtime_file "$root/config.cfg" "$game_stage/config.cfg"
            copy_runtime_tree "$root/data" "$game_stage/data"
            ;;
        nietzsche)
            root="$ROOT/OMA/NietzscheSE-MSDOS-1.1/Nietzsche"
            for source in battles1.jss chars.spr jrpg.pal maps.txt script1; do
                copy_runtime_file "$root/$source" "$game_stage/$source"
            done
            copy_runtime_tree "$root/data" "$game_stage/data"
            copy_runtime_tree "$root/music" "$game_stage/music"
            copy_runtime_tree "$root/pics" "$game_stage/pics"
            mkdir -p "$game_stage/save"
            ;;
        qfak)
            root="$ROOT/OMA/QuestForAKing-Win32-1.5"
            for source in BATTLES1.JSS CHARS.SPR JRPG.PAL MAPS.TXT SCRIPT1 config.cfg; do
                copy_runtime_file "$root/$source" "$game_stage/$source"
            done
            copy_runtime_tree "$root/data" "$game_stage/data"
            copy_runtime_tree "$root/MUSIC" "$game_stage/MUSIC"
            copy_runtime_tree "$root/PICS" "$game_stage/PICS"
            mkdir -p "$game_stage/save"
            ;;
        rambo)
            root="$ROOT/OMA/RamboVsKittyCat-Win32-0.1"
            for source in config.cfg input.cfg map1.map; do
                copy_runtime_file "$root/$source" "$game_stage/$source"
            done
            copy_runtime_tree "$root/Images" "$game_stage/Images"
            ;;
        starphalanx)
            root="$ROOT/OMA/StarPhalanx-win32-0.5"
            copy_runtime_file "$root/config.cfg" "$game_stage/config.cfg"
            copy_runtime_tree "$root/data" "$game_stage/data"
            ;;
        openmarket)
            root="$ROOT/OMA/Tamper/tamper"
            copy_runtime_tree "$root/data/open_assets/bmp" \
                "$game_stage/data/open_assets/bmp"
            copy_runtime_tree "$root/data/open_assets/sng" \
                "$game_stage/data/open_assets/sng"
            copy_runtime_file "$root/data/tamper_port.cfg" \
                "$game_stage/data/tamper_port.cfg"
            mkdir -p "$game_stage/data/reports"
            ;;
        openhostility)
            root="$ROOT/OMA/Scorched Earth"
            while IFS= read -r -d '' source; do
                copy_runtime_file "$source" "$game_stage/$(basename "$source")"
            done < <(find "$root" -maxdepth 1 -type f \
                \( -iname '*.MTN' -o -iname '*.ppm' \
                   -o -iname 'OPENHOSTILITY*.CFG' \
                   -o -iname 'OPENHOSTILITY.MKT' \
                   -o -iname 'OPENHOSTILITY.ICO' \) -print0)
            ;;
        openwallstreet)
            root="$ROOT/OMA/WSR5_3/original_data"
            for source in \
                BADNEWS.DAT MISCNEWS.DAT SCENTXT1.DAT SCENTXT2.DAT \
                SCENTXT3.DAT GAMEHIGH.DAT NEWGAME1.DAT NEWGAME2.DAT \
                FILE_ID.DIZ VENDOR.DOC VENDOR.TXT REGISTER.DOC USERINFO.DOC; do
                copy_runtime_file "$root/$source" \
                    "$game_stage/original_data/$source"
            done
            copy_runtime_file \
                "$root/extracted/strings/strings_SUBSID1.EXE.txt" \
                "$game_stage/original_data/extracted/strings/strings_SUBSID1.EXE.txt"
            ;;
        openslicks)
            stage_open_slicks_fixture "$game_stage"
            ;;
        *)
            die "no asset definition for game: $key"
            ;;
    esac
}

##############################################################################
# AROS game staging and package construction
##############################################################################

write_game_files() {
    local display_name="$2"
    local game_directory="$3"
    local game_stage="$4"
    local key="$1"
    local package_base="$5"
    local slicks_note=""

    if [ "$key" = openslicks ]; then
        slicks_note="This package contains an original generated test track. Copy legally owned .SS or .SZT tracks into TRACKS to add courses. Original Slicks data is not distributed."
    fi

    cat > "$game_stage/Start" <<EOF
FailAt 21
CD SYS:Games/$game_directory
Stack 1048576
game
EOF

    cat > "$game_stage/README" <<EOF
$display_name for AROS

Install the package from an AROS shell with:

    Unpack $package_base.pkg TO SYS:

After package installation, start the game with:

    Execute SYS:Games/$game_directory/Start

The executable and runtime assets are specific to this package's architecture.
The game was linked with FreeBASIC's AROS rtlib, gfxlib2, and sfxlib. Graphics
use an Intuition window and CyberGraphX; audio uses ahi.device when available.

$slicks_note
EOF
}

package_game() {
    local display_name="$1"
    local game_directory="$2"
    local game_stage="$3"
    local package_base="$4"
    local package_path="$PACKAGE_OUTDIR/$package_base.pkg"
    local zip_path="$PACKAGE_OUTDIR/$package_base.zip"
    local zip_temporary="$PACKAGE_OUTDIR/$package_base.new.zip"

    PACKAGE_STAGE="$(mktemp -d "$PACKAGE_OUTDIR/.package-stage.XXXXXX")"
    EXTRACT_STAGE="$(mktemp -d "$PACKAGE_OUTDIR/.extract-stage.XXXXXX")"
    mkdir -p "$PACKAGE_STAGE/Games"
    cp -a "$game_stage" "$PACKAGE_STAGE/Games/$game_directory"
    find "$PACKAGE_STAGE" -depth -type d -empty -delete

    rm -f -- "$package_path" "$zip_path" "$zip_temporary"
    python3 "$PKG_TOOL" create "$PACKAGE_STAGE" "$package_path"
    python3 "$PKG_TOOL" extract "$package_path" "$EXTRACT_STAGE"
    diff -qr "$PACKAGE_STAGE" "$EXTRACT_STAGE"

    (
        cd "$PACKAGE_OUTDIR"
        zip -9 -j "$zip_temporary" \
            "$package_path" "$game_stage/README" >/dev/null
    )
    mv "$zip_temporary" "$zip_path"

    [ -s "$package_path" ] || die "package was not created for $display_name"
    [ -s "$zip_path" ] || die "download ZIP was not created for $display_name"
    rm -rf -- "$PACKAGE_STAGE" "$EXTRACT_STAGE"
    PACKAGE_STAGE=""
    EXTRACT_STAGE=""
}

##############################################################################
# Matrix execution
##############################################################################

TEMP_ROOT="$(mktemp -d "$OUTPUT_ROOT/.build-stage.XXXXXX")"
mkdir -p "$TEMP_ROOT/bin" "$TEMP_ROOT/games" "$TEMP_ROOT/manifests"

for target in "${SELECTED_TARGETS[@]}"; do
    map_target "$target"
    [ -x "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-gcc" ] ||
        die "AROS $target cross compiler not found in $MAP_TOOLCHAIN"
    [ -x "$MAP_TOOLCHAIN/$MAP_TOOL_PREFIX-readelf" ] ||
        die "AROS $target readelf not found in $MAP_TOOLCHAIN"

    if [ "$target" = m68k ]; then
        [ -x "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" ] ||
            die "AROS m68k elf2hunk tool is missing"
    fi

    if [ "$BUILD_LIBRARIES" -eq 1 ]; then
        msg "building AROS $target rtlib, gfxlib2, and sfxlib"
        env -u DEBUG PATH="$MAP_TOOLCHAIN:$PATH" \
            make -s -C "$ROOT" -j"$JOBS" \
                TARGET_TRIPLET="$MAP_TARGET_TRIPLET" \
                BUILD_FBC="$FBC" \
                libs
    fi

    target_bin="$TEMP_ROOT/bin/$target"
    target_games="$TEMP_ROOT/games/$target"
    target_manifest="$TEMP_ROOT/manifests/$target.tsv"
    mkdir -p "$target_bin" "$target_games"

    # A filtered rebuild must not remove previously built games from the
    # target staging tree or turn the complete matrix manifest into a partial
    # inventory. Preserve existing artifacts and rows, then replace only the
    # explicitly selected entries below.
    if [ "${#SELECTED_GAMES[@]}" -gt 0 ]; then
        if [ -d "$OUTPUT_ROOT/bin/$target" ]; then
            cp -a "$OUTPUT_ROOT/bin/$target/." "$target_bin/"
        fi
        if [ -d "$OUTPUT_ROOT/games/$target" ]; then
            cp -a "$OUTPUT_ROOT/games/$target/." "$target_games/"
        fi
    fi

    printf '%s\n' $'key\tdisplay\tkind\ttarget\tstatus\tsource\tgame_directory\tpackage\tnote' \
        > "$target_manifest"

    built_count=0
    missing_count=0

    for row in "${GAME_ROWS[@]}"; do
        IFS=$'\t' read -r key display_name kind game_directory source_path <<< "$row"
        if ! game_is_selected "$key"; then
            previous_row=""
            if [ -f "$OUTPUT_ROOT/manifest-$target.tsv" ]; then
                previous_row="$(awk -F '\t' -v key="$key" \
                    '$1 == key { print; exit }' \
                    "$OUTPUT_ROOT/manifest-$target.tsv")"
            fi
            if [ -n "$previous_row" ]; then
                printf '%s\n' "$previous_row" >> "$target_manifest"
            elif [ ! -f "$ROOT/$source_path" ]; then
                printf '%s\t%s\t%s\t%s\tmissing\t%s\t%s\t\tsource tree is not present\n' \
                    "$key" "$display_name" "$kind" "$target" "$source_path" \
                    "$game_directory" >> "$target_manifest"
            else
                printf '%s\t%s\t%s\t%s\tnot-built\t%s\t%s\t\tnot selected for this filtered build\n' \
                    "$key" "$display_name" "$kind" "$target" "$source_path" \
                    "$game_directory" >> "$target_manifest"
            fi
            continue
        fi

        if [ ! -f "$ROOT/$source_path" ]; then
            printf '%s\t%s\t%s\t%s\tmissing\t%s\t%s\t\tsource tree is not present\n' \
                "$key" "$display_name" "$kind" "$target" "$source_path" \
                "$game_directory" >> "$target_manifest"
            missing_count=$((missing_count + 1))
            continue
        fi

        msg "building $display_name for AROS $target"
        linked_elf="$target_bin/$key.elf"
        final_program="$target_bin/$key"
        game_stage="$target_games/$game_directory"
        package_base="${game_directory}-r${PACKAGE_REVISION}-aros-${target}"
        package_path="$PACKAGE_OUTDIR/$package_base.pkg"
        mkdir -p "$game_stage"

        compile_game "$key" "$linked_elf" "$target"
        if [ "$target" = m68k ]; then
            "$MAP_AROS_BUILD/bin/linux-x86_64/tools/elf2hunk" \
                "$linked_elf" "$final_program"
            [[ "$(file -b "$final_program")" == *"AmigaOS loadseg"* ]] ||
                die "$final_program is not an Amiga Hunk executable"
        else
            cp "$linked_elf" "$final_program"
        fi

        stage_game_assets "$key" "$game_stage"
        cp "$final_program" "$game_stage/game"
        write_game_files \
            "$key" "$display_name" "$game_directory" "$game_stage" \
            "$package_base"

        if [ "$PACKAGE_GAMES" -eq 1 ]; then
            package_game \
                "$display_name" "$game_directory" "$game_stage" "$package_base"
        else
            package_path=""
        fi

        printf '%s\t%s\t%s\t%s\tbuilt\t%s\t%s\t%s\t\n' \
            "$key" "$display_name" "$kind" "$target" "$source_path" \
            "$game_directory" "$package_path" >> "$target_manifest"
        built_count=$((built_count + 1))
    done

    replace_directory "$target_bin" "$OUTPUT_ROOT/bin/$target"
    replace_directory "$target_games" "$OUTPUT_ROOT/games/$target"
    mv "$target_manifest" "$OUTPUT_ROOT/manifest-$target.tsv"

    if [ "$STRICT_MATRIX" -eq 1 ] && [ "$missing_count" -ne 0 ]; then
        die "$missing_count selected source tree(s) are absent for $target"
    fi

    echo "==> AROS $target OMA games built: $built_count; missing: $missing_count"
done

rm -rf -- "$TEMP_ROOT"
TEMP_ROOT=""

echo
echo "==> AROS OMA build completed for: ${SELECTED_TARGETS[*]}"
echo "    Manifests: $OUTPUT_ROOT/manifest-{x86_64,m68k,arm}.tsv"
if [ "$PACKAGE_GAMES" -eq 1 ]; then
    echo "    Packages:  $PACKAGE_OUTDIR"
fi

# end of aros-build-oma-games.sh
