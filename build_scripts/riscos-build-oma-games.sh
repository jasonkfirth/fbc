#!/usr/bin/env bash
#
# Project: FreeBASIC OMA games for RISC OS
# -----------------------------------------
#
# File: riscos-build-oma-games.sh
#
# Purpose:
#
#     Cross-compile the available OMA games for RISC OS and create one
#     installable RiscPkg archive for each game.
#
# Responsibilities:
#
#     - keep the complete OMA build matrix in one place
#     - build static ARM programs against the RISC OS gfxlib2 and sfxlib
#     - stage only the runtime assets used by each game
#     - translate Unix filename suffixes into UnixLib suffix directories
#     - convert each linked ELF program into a native RISC OS AIF image
#     - create and validate one Acorn-metadata RiscPkg ZIP per game
#     - preserve a machine-readable manifest for emulator tests
#
# This file intentionally does NOT contain:
#
#     - GCCSDK, FreeBASIC, or RPCEmu construction
#     - guest-side launch automation
#     - copies of original Slicks 'n' Slide data
#     - game implementation or platform compatibility code
#
# Output ownership:
#
#     The oma build tree, OMA HostFS test tree, and OMA package archives below
#     the selected output directories are replaced. Other output is preserved.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

TOOLCHAIN_BIN="${GCCSDK_INSTALL_CROSSBIN:-$ROOT/out/riscos/gccsdk/cross/bin}"
TARGET_ENV="${GCCSDK_TARGET_ENV:-$ROOT/out/riscos/gccsdk/cross/arm-unknown-riscos}"
OUTPUT_ROOT="${RISCOS_OMA_OUTDIR:-$ROOT/out/riscos/oma}"
HOSTFS_ROOT="${RISCOS_HOSTFS_ROOT:-$ROOT/out/riscos/hostfs}"
PACKAGE_OUTDIR="${RISCOS_PACKAGE_OUTDIR:-$ROOT/out/riscos/packages}"
PACKAGE_REVISION="${RISCOS_OMA_PACKAGE_REVISION:-1}"
PACKAGE_MAINTAINER="${RISCOS_PACKAGE_MAINTAINER:-SJ_Zero <sj@fbxl.net>}"

BUILD_RUNTIME=1
PACKAGE_GAMES=1
STRICT_MATRIX=0
JOBS=""

TEMP_ROOT=""
PACKAGE_STAGE=""

##############################################################################
# Complete game matrix
##############################################################################

# Fields are separated by tabs. The source path is also the availability
# probe. OpenSlicks is kept in this matrix because it is a playable game tree
# under OMA, but its kind distinguishes it from the fourteen-title OMA suite.
GAME_ROWS=(
    $'arkanoid\tArkanoid\tOMA\t!Arkanoid\tOMAArkanoid\tOMA/ArkanoidTest/ArkanoidTest.bas'
    $'behold\tBehold\tOMA\t!Behold\tOMABehold\tOMA/Behold/Behold.bas'
    $'demoderby\tDemolition Derby\tOMA\t!DemoDerby\tOMADemoDerby\tOMA/DemolitionDerby/main.bas'
    $'duel999\tDuel 999\tOMA\t!Duel999\tOMADuel999\tOMA/duel999/SD_Main.bas'
    $'kinematics\tKinematics\tOMA\t!Kinematic\tOMAKinematics\tOMA/kinematics/kinematic_man_two_bodies_self_collision_friction.bas'
    $'nietzsche\tNietzsche Special Edition\tOMA\t!Nietzsche\tOMANietzsche\tOMA/NietzscheSE-MSDOS-1.1/Nietzsche/src/win32/win11.bas'
    $'qfak\tQuest for a King\tOMA\t!QFAK\tOMAQuestKing\tOMA/QuestForAKing-Win32-1.5/src/win11.bas'
    $'rambo\tRambo vs Kitty Cat\tOMA\t!Rambo\tOMARambo\tOMA/RamboVsKittyCat-Win32-0.1/killquest.bas'
    $'starphalanx\tStar Phalanx\tOMA\t!StarPhal\tOMAStarPhalanx\tOMA/StarPhalanx-win32-0.5/entryv2.bas'
    $'openmarket\tOpen Market\tOMA\t!OpenMarket\tOMAOpenMarket\tOMA/Tamper/tamper/src/openmarket_bootstrap.bas'
    $'openhostility\tOpenHostility\tOMA\t!Hostility\tOMAHostility\tOMA/Scorched Earth/src/scorch_gfx.bas'
    $'turbotrek\tTurboTrek\tOMA\t!TurboTrek\tOMATurboTrek\tOMA/TurboTrek/src/turbotrek/main.bas'
    $'vtrek\tvtrek\tOMA\t!VTrek\tOMAVTrek\tOMA/vtrek/src/vtrek.bas'
    $'openwallstreet\tOpenWallStreet\tOMA\t!WallStreet\tOMAWallStreet\tOMA/WSR5_3/src/raider.bas'
    $'openslicks\tOpen Slicks Racing\tOpenSlicks\t!OpenSlicks\tOpenSlicks\tOMA/Slicks n Slide/src/slicks.bas'
)

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

cleanup() {
    if [ -n "$PACKAGE_STAGE" ] &&
       [[ "$PACKAGE_STAGE" == "$PACKAGE_OUTDIR"/.oma-package.* ]] &&
       [ -d "$PACKAGE_STAGE" ]; then
        rm -rf -- "$PACKAGE_STAGE"
    fi

    if [ -n "$TEMP_ROOT" ] &&
       [[ "$TEMP_ROOT" == "$OUTPUT_ROOT"/.stage.* ]] &&
       [ -d "$TEMP_ROOT" ]; then
        rm -rf -- "$TEMP_ROOT"
    fi
}

replace_directory() {
    local source="$1"
    local destination="$2"

    [ -d "$source" ] || die "staging source is not a directory: $source"
    [ "$destination" != "/" ] || die "refusing to replace the filesystem root"

    rm -rf -- "$destination"
    mkdir -p "$(dirname "$destination")"
    mv "$source" "$destination"
}

require_file() {
    local path="$1"

    [ -f "$path" ] || die "required file not found: $path"
}

copy_runtime_file() {
    local source="$1"
    local destination="$2"

    require_file "$source"
    mkdir -p "$(dirname "$destination")"
    cp "$source" "$destination"
}

copy_runtime_tree() {
    local source="$1"
    local destination="$2"

    [ -d "$source" ] || die "required runtime directory not found: $source"
    mkdir -p "$destination"
    cp -a "$source/." "$destination/"

    # Explorer caches and native host binaries are never runtime game data.
    find "$destination" -type f \
        \( -iname 'Thumbs.db' -o -iname 'Thumbs(1).db' \
           -o -iname '*.exe' -o -iname '*.dll' -o -iname '*.so' \
           -o -iname '*.apk' -o -iname '*.idsig' \) -delete
}

suffix_swap_tree() {
    local tree="$1"
    local changed=1
    local directory
    local destination
    local file
    local leaf
    local stem
    local suffix

    declare -gA GAME_SUFFIXES=()

    while [ "$changed" -eq 1 ]; do
        changed=0
        while IFS= read -r -d '' file; do
            directory="$(dirname "$file")"
            leaf="$(basename "$file")"
            stem="${leaf%.*}"
            suffix="${leaf##*.}"

            [ -n "$stem" ] || continue
            suffix="${suffix,,}"
            case "$suffix" in
                ''|*[!a-z0-9_+-]*)
                    die "unsupported resource suffix in $file"
                    ;;
            esac

            destination="$directory/$suffix/$stem"
            [ ! -e "$destination" ] ||
                die "resource suffix conversion collision: $destination"
            mkdir -p "$directory/$suffix"
            mv "$file" "$destination"
            GAME_SUFFIXES["$suffix"]=1
            changed=1
        done < <(find "$tree" -type f -name '*.*' -print0)
    done
}

sorted_suffixes() {
    local suffix
    local uppercase_suffix

    # UnixLib matches suffix-list entries case-sensitively even though RISC OS
    # filesystems normally match the resulting directory names without regard
    # to case. OMA sources use both `.bmp` and `.BMP` spellings, so expose both
    # forms while retaining one physical suffix directory.

    for suffix in "${!GAME_SUFFIXES[@]}"; do
        printf '%s\n' "$suffix"
        uppercase_suffix="${suffix^^}"
        if [ "$uppercase_suffix" != "$suffix" ]; then
            printf '%s\n' "$uppercase_suffix"
        fi
    done | sort -u | paste -sd: -
}

validate_elf() {
    local elf="$1"
    local header

    header="$("$READELF" -h "$elf")"
    grep -q 'Class:.*ELF32' <<<"$header" || die "$elf is not ELF32"
    grep -q 'Data:.*little endian' <<<"$header" ||
        die "$elf is not little-endian"
    grep -q 'Machine:.*ARM' <<<"$header" || die "$elf is not ARM"
    grep -q 'Flags:.*software FP' <<<"$header" ||
        die "$elf does not use the GCCSDK software floating-point ABI"
    [[ "$(file -b "$elf")" == *"statically linked"* ]] ||
        die "$elf is not statically linked"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-build-oma-games.sh [options]

Options:
  --toolchain-bin DIR  Directory containing arm-unknown-riscos-gcc.
  --target-env DIR     GCCSDK target environment directory.
  --out-dir DIR        Build and application staging root.
  --hostfs-root DIR    HostFS root receiving OMAGames test applications.
  --package-dir DIR    RiscPkg archive output directory.
  --package-revision N Package revision. Default: 1
  --skip-runtime       Reuse existing RISC OS runtime libraries.
  --no-package         Build and stage applications without ZIP archives.
  --strict             Fail if any of the fourteen OMA sources are absent.
  --jobs N             Parallel runtime build jobs.
  -h, --help           Show this help.

The manifest is written to out/riscos/oma/manifest.tsv by default. Missing
source trees are recorded there and are built automatically when restored.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --toolchain-bin)
            require_value "$1" "${2-}"
            TOOLCHAIN_BIN="$2"
            shift 2
            ;;
        --target-env)
            require_value "$1" "${2-}"
            TARGET_ENV="$2"
            shift 2
            ;;
        --out-dir)
            require_value "$1" "${2-}"
            OUTPUT_ROOT="$2"
            shift 2
            ;;
        --hostfs-root)
            require_value "$1" "${2-}"
            HOSTFS_ROOT="$2"
            shift 2
            ;;
        --package-dir)
            require_value "$1" "${2-}"
            PACKAGE_OUTDIR="$2"
            shift 2
            ;;
        --package-revision)
            require_value "$1" "${2-}"
            PACKAGE_REVISION="$2"
            shift 2
            ;;
        --skip-runtime)
            BUILD_RUNTIME=0
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

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

case "$PACKAGE_REVISION" in
    ''|*[!0-9A-Za-z.+~-]*) die "unsafe package revision: $PACKAGE_REVISION" ;;
esac

##############################################################################
# Toolchain and output validation
##############################################################################

for tool in base64 file find grep make paste python3 sort; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

FBC="$ROOT/bin/fbc"
TARGET_GCC="$TOOLCHAIN_BIN/arm-unknown-riscos-gcc"
READELF="$TOOLCHAIN_BIN/arm-unknown-riscos-readelf"
ELF2AIF="$TOOLCHAIN_BIN/elf2aif"
if [ ! -x "$ELF2AIF" ]; then
    ELF2AIF="$TOOLCHAIN_BIN/arm-unknown-riscos-elf2aif"
fi

[ -x "$FBC" ] || die "bootstrap compiler not found: $FBC"
[ -x "$TARGET_GCC" ] || die "GCCSDK compiler not found: $TARGET_GCC"
[ -x "$READELF" ] || die "GCCSDK readelf not found: $READELF"
[ -x "$ELF2AIF" ] || die "GCCSDK elf2aif not found below $TOOLCHAIN_BIN"
[ -d "$TARGET_ENV/include" ] ||
    die "GCCSDK target environment not found: $TARGET_ENV"

OUTPUT_ROOT="$(mkdir -p "$OUTPUT_ROOT" && cd "$OUTPUT_ROOT" && pwd)"
HOSTFS_ROOT="$(mkdir -p "$HOSTFS_ROOT" && cd "$HOSTFS_ROOT" && pwd)"
PACKAGE_OUTDIR="$(mkdir -p "$PACKAGE_OUTDIR" && cd "$PACKAGE_OUTDIR" && pwd)"

[ "$OUTPUT_ROOT" != "/" ] || die "refusing to use / as the output root"
[ "$HOSTFS_ROOT" != "/" ] || die "refusing to use / as the HostFS root"
[ "$PACKAGE_OUTDIR" != "/" ] || die "refusing to use / as the package root"

export GCCSDK_INSTALL_CROSSBIN="$TOOLCHAIN_BIN"
export GCCSDK_TARGET_ENV="$TARGET_ENV"
export PATH="$TOOLCHAIN_BIN:$PATH"

# The target makefiles use these names for compiler overrides. Host values
# would silently bypass TARGET_TRIPLET selection.
unset GCC CLANG

##############################################################################
# Runtime construction
##############################################################################

if [ "$BUILD_RUNTIME" -eq 1 ]; then
    msg "building RISC OS runtime, gfxlib2, and sfxlib"
    make -C "$ROOT" -j"$JOBS" \
        TARGET_TRIPLET=arm-unknown-riscos \
        FBC="$FBC" \
        rtlib fbrt gfxlib2 sfxlib
fi

##############################################################################
# Per-game compiler definitions
##############################################################################

compile_game() {
    local key="$1"
    local output="$2"
    local working_directory
    local -a sources=()
    local -a extra_arguments=()

    case "$key" in
        arkanoid)
            working_directory="$ROOT/OMA/ArkanoidTest"
            sources=("$ROOT/OMA/ArkanoidTest/ArkanoidTest.bas")
            ;;
        behold)
            working_directory="$ROOT/OMA/Behold"
            sources=("$ROOT/OMA/Behold/Behold.bas")
            ;;
        demoderby)
            working_directory="$ROOT/OMA/DemolitionDerby"
            sources=("$ROOT/OMA/DemolitionDerby/main.bas")
            ;;
        duel999)
            working_directory="$ROOT/OMA/duel999"
            sources=("$ROOT/OMA/duel999/SD_Main.bas")
            ;;
        kinematics)
            working_directory="$ROOT/OMA/kinematics"
            sources=("$ROOT/OMA/kinematics/kinematic_man_two_bodies_self_collision_friction.bas")
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

    [ -d "$working_directory" ] ||
        die "working directory not found for $key: $working_directory"

    local source
    for source in "${sources[@]}"; do
        require_file "$source"
    done

    (
        cd "$working_directory"
        "$FBC" \
            -target arm-unknown-riscos \
            -static \
            -mt \
            -i "$ROOT/inc" \
            "${extra_arguments[@]}" \
            "${sources[@]}" \
            -x "$output"
    )

    [ -s "$output" ] || die "FreeBASIC did not produce $output"
    validate_elf "$output"
}

##############################################################################
# Curated game assets
##############################################################################

stage_open_slicks_fixture() {
    local app_stage="$1"

    mkdir -p "$app_stage/TRACKS"

    # This 411-byte version 2 track was generated by OpenSlicks' own fixture
    # builder. It contains two ordinary road records and no original game art.
    base64 -d > "$app_stage/TRACKS/ASFALTTI.SS" <<'EOF'
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
    local key="$1"
    local app_stage="$2"
    local root
    local source

    case "$key" in
        arkanoid|behold|demoderby|kinematics|turbotrek|vtrek)
            ;;
        duel999)
            root="$ROOT/OMA/duel999"
            copy_runtime_file "$root/defaults.cfg" "$app_stage/defaults.cfg"
            copy_runtime_file "$root/config.cfg" "$app_stage/config.cfg"
            copy_runtime_tree "$root/data" "$app_stage/data"
            ;;
        nietzsche)
            root="$ROOT/OMA/NietzscheSE-MSDOS-1.1/Nietzsche"
            for source in battles1.jss chars.spr jrpg.pal maps.txt script1; do
                copy_runtime_file "$root/$source" "$app_stage/$source"
            done
            copy_runtime_tree "$root/data" "$app_stage/data"
            copy_runtime_tree "$root/music" "$app_stage/music"
            copy_runtime_tree "$root/pics" "$app_stage/pics"
            mkdir -p "$app_stage/save"
            ;;
        qfak)
            root="$ROOT/OMA/QuestForAKing-Win32-1.5"
            for source in BATTLES1.JSS CHARS.SPR JRPG.PAL MAPS.TXT SCRIPT1 config.cfg; do
                copy_runtime_file "$root/$source" "$app_stage/$source"
            done
            copy_runtime_tree "$root/data" "$app_stage/data"
            copy_runtime_tree "$root/MUSIC" "$app_stage/MUSIC"
            copy_runtime_tree "$root/PICS" "$app_stage/PICS"
            mkdir -p "$app_stage/save"
            ;;
        rambo)
            root="$ROOT/OMA/RamboVsKittyCat-Win32-0.1"
            for source in config.cfg input.cfg map1.map; do
                copy_runtime_file "$root/$source" "$app_stage/$source"
            done
            copy_runtime_tree "$root/Images" "$app_stage/Images"
            ;;
        starphalanx)
            root="$ROOT/OMA/StarPhalanx-win32-0.5"
            copy_runtime_file "$root/config.cfg" "$app_stage/config.cfg"
            copy_runtime_tree "$root/data" "$app_stage/data"
            ;;
        openmarket)
            root="$ROOT/OMA/Tamper/tamper"
            copy_runtime_tree "$root/data/open_assets/bmp" \
                "$app_stage/data/open_assets/bmp"
            copy_runtime_tree "$root/data/open_assets/sng" \
                "$app_stage/data/open_assets/sng"
            copy_runtime_file "$root/data/tamper_port.cfg" \
                "$app_stage/data/tamper_port.cfg"
            mkdir -p "$app_stage/data/reports"
            ;;
        openhostility)
            root="$ROOT/OMA/Scorched Earth"
            while IFS= read -r -d '' source; do
                copy_runtime_file "$source" "$app_stage/$(basename "$source")"
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
                    "$app_stage/original_data/$source"
            done
            copy_runtime_file \
                "$root/extracted/strings/strings_SUBSID1.EXE.txt" \
                "$app_stage/original_data/extracted/strings/strings_SUBSID1.EXE.txt"
            ;;
        openslicks)
            stage_open_slicks_fixture "$app_stage"
            ;;
        *)
            die "no asset definition for game: $key"
            ;;
    esac
}

##############################################################################
# RISC OS application and package construction
##############################################################################

write_application_files() {
    local key="$1"
    local display_name="$2"
    local app_stage="$3"
    local suffixes="$4"
    local slicks_note=""

    if [ "$key" = "openslicks" ]; then
        slicks_note="This package contains an original generated test track. Copy legally owned .SS or .SZT tracks into TRACKS to add more courses. Original Slicks data is not distributed."
    fi

    cat > "$app_stage/!Boot,feb" <<EOF
| $display_name RISC OS application boot file
| --------------------------------------------
|
| File: !Boot
|
| Purpose:
|
|     Configure UnixLib filename suffixes for the packaged game.
|
| Responsibilities:
|
|     - publish the application directory
|     - expose staged suffix directories to the game executable
|
| This generated file intentionally does NOT start the game.

Set OMAGame\$Dir <Obey\$Dir>
Set UnixEnv\$game\$sfix "$suffixes"

| end of !Boot
EOF

    cat > "$app_stage/!Run,feb" <<EOF
| $display_name RISC OS application run file
| -------------------------------------------
|
| File: !Run
|
| Purpose:
|
|     Start the packaged game from its application directory.
|
| Responsibilities:
|
|     - apply the UnixLib environment from !Boot
|     - reserve a 192 MiB Wimp slot
|     - make relative game asset paths resolve inside the application
|
| This generated file intentionally does NOT install runtime modules.

Obey <Obey\$Dir>.!Boot
WimpSlot -min 196608K
Dir <Obey\$Dir>
Run game

| end of !Run
EOF

    cat > "$app_stage/!Help,fff" <<EOF
$display_name for RISC OS
$(printf '%*s' "${#display_name}" '' | tr ' ' '=')============

File: !Help

Purpose:

  Explain how to start this RISC OS package and identify its dependencies.

Responsibilities:

  - identify the packaged OMA game
  - state the SharedUnixLibrary and DigitalRenderer requirements
  - describe the package's runtime data policy

This file intentionally does not contain emulator or cross-build instructions.

Double-click this application to start $display_name.

The SharedUnixLibrary and DigitalRenderer RiscPkg packages must be installed.
The game executable is a static ARM AIF built by FreeBASIC for RISC OS.

$slicks_note

The source build is maintained with the FreeBASIC RISC OS port.

end of !Help
EOF
}

write_package_metadata() {
    local display_name="$1"
    local package_name="$2"
    local app_name="$3"
    local package_stage="$4"
    local package_version="$5"

    mkdir -p "$package_stage/RiscPkg"

    printf '%s\n' \
        "Package: $package_name" \
        "Version: $package_version" \
        'Priority: Optional' \
        'Section: Games' \
        "Maintainer: $PACKAGE_MAINTAINER" \
        'Standards-Version: 0.4.0' \
        'Licence: Free' \
        'Environment: arm' \
        'Depends: SharedUnixLibrary (>= 1.12), DRenderer (>= 0.56)' \
        "Components: Apps.Games.$app_name (Movable LookAt)" \
        "Description: $display_name for RISC OS" \
        'Homepage: https://github.com/jasonkfirth/fbc' \
        > "$package_stage/RiscPkg/Control,fff"

    printf '%s\n' \
        "$display_name for RISC OS" \
        '========================' \
        '' \
        'Copyright (C) the game authors and contributors.' \
        '' \
        'This archive was built from the OMA source and runtime data present' \
        'in the FreeBASIC development workspace. Consult the corresponding' \
        'source tree for title-specific copyright and licence information.' \
        '' \
        'OpenSlicks packages do not contain original Slicks n Slide data.' \
        > "$package_stage/RiscPkg/Copyright,fff"
}

package_game() {
    local display_name="$1"
    local app_name="$2"
    local package_name="$3"
    local app_stage="$4"
    local package_path="$5"
    local package_version="$6"

    PACKAGE_STAGE="$(mktemp -d "$PACKAGE_OUTDIR/.oma-package.XXXXXX")"
    mkdir -p "$PACKAGE_STAGE/Apps/Games"
    cp -a "$app_stage" "$PACKAGE_STAGE/Apps/Games/$app_name"
    write_package_metadata \
        "$display_name" "$package_name" "$app_name" \
        "$PACKAGE_STAGE" "$package_version"

    if find "$PACKAGE_STAGE" -type f -printf '%f\n' | grep -q '\.'; then
        die "package staging contains a leaf with a RISC OS path separator"
    fi

    python3 "$SCRIPT_DIR/riscos-zip.py" create \
        "$PACKAGE_STAGE" "$package_path"
    python3 "$SCRIPT_DIR/riscos-zip.py" check "$package_path" \
        --require 'RiscPkg/Control=fff' \
        --require 'RiscPkg/Copyright=fff' \
        --require "Apps/Games/$app_name/!Boot=feb" \
        --require "Apps/Games/$app_name/!Run=feb" \
        --require "Apps/Games/$app_name/!Help=fff" \
        --require "Apps/Games/$app_name/game=ff8"

    rm -rf -- "$PACKAGE_STAGE"
    PACKAGE_STAGE=""
}

##############################################################################
# Matrix execution
##############################################################################

ELF_ROOT="$OUTPUT_ROOT/elf"
APP_ROOT="$OUTPUT_ROOT/apps"
MANIFEST="$OUTPUT_ROOT/manifest.tsv"
HOSTFS_GAMES="$HOSTFS_ROOT/OMAGames"
PACKAGE_VERSION="1.0-$PACKAGE_REVISION"

TEMP_ROOT="$(mktemp -d "$OUTPUT_ROOT/.stage.XXXXXX")"
mkdir -p "$TEMP_ROOT/elf" "$TEMP_ROOT/apps" "$TEMP_ROOT/hostfs"

printf '%s\n' $'key\tdisplay\tkind\tstatus\tsource\tapp\tsuffixes\tpackage\tnote' \
    > "$TEMP_ROOT/manifest.tsv"

built_count=0
missing_count=0
missing_oma_count=0

for row in "${GAME_ROWS[@]}"; do
    IFS=$'\t' read -r key display_name kind app_name package_name source_path \
        <<<"$row"

    if [ ! -f "$ROOT/$source_path" ]; then
        printf '%s\t%s\t%s\tmissing\t%s\t%s\t\t\t%s\n' \
            "$key" "$display_name" "$kind" "$source_path" "$app_name" \
            "source tree is not present" >> "$TEMP_ROOT/manifest.tsv"
        missing_count=$((missing_count + 1))
        if [ "$kind" = "OMA" ]; then
            missing_oma_count=$((missing_oma_count + 1))
        fi
        continue
    fi

    msg "building $display_name"

    elf="$TEMP_ROOT/elf/$key"
    app_stage="$TEMP_ROOT/apps/$app_name"
    package_path="$PACKAGE_OUTDIR/${package_name}_${PACKAGE_VERSION}.zip"

    mkdir -p "$app_stage"
    compile_game "$key" "$elf"
    stage_game_assets "$key" "$app_stage"
    suffix_swap_tree "$app_stage"

    # Games can create these common output file types even when no matching
    # input asset was staged. UnixLib needs the destination directories first.
    for output_suffix in cfg dat log sav tmp tsv txt; do
        mkdir -p "$app_stage/$output_suffix"
        GAME_SUFFIXES["$output_suffix"]=1
    done

    suffixes="$(sorted_suffixes)"
    cp "$elf" "$app_stage/game,ff8"
    "$ELF2AIF" "$app_stage/game,ff8"
    [ -s "$app_stage/game,ff8" ] ||
        die "elf2aif did not produce an AIF for $display_name"

    write_application_files "$key" "$display_name" "$app_stage" "$suffixes"

    mkdir -p "$TEMP_ROOT/hostfs/$app_name"
    cp -a "$app_stage/." "$TEMP_ROOT/hostfs/$app_name/"

    if [ "$PACKAGE_GAMES" -eq 1 ]; then
        package_game \
            "$display_name" "$app_name" "$package_name" \
            "$app_stage" "$package_path" "$PACKAGE_VERSION"
    else
        package_path=""
    fi

    printf '%s\t%s\t%s\tbuilt\t%s\t%s\t%s\t%s\t\n' \
        "$key" "$display_name" "$kind" "$source_path" "$app_name" \
        "$suffixes" "$package_path" >> "$TEMP_ROOT/manifest.tsv"
    built_count=$((built_count + 1))
done

replace_directory "$TEMP_ROOT/elf" "$ELF_ROOT"
replace_directory "$TEMP_ROOT/apps" "$APP_ROOT"
replace_directory "$TEMP_ROOT/hostfs" "$HOSTFS_GAMES"
mv "$TEMP_ROOT/manifest.tsv" "$MANIFEST"
rm -rf -- "$TEMP_ROOT"
TEMP_ROOT=""

if [ "$STRICT_MATRIX" -eq 1 ] && [ "$missing_oma_count" -ne 0 ]; then
    die "$missing_oma_count OMA source tree(s) are absent; see $MANIFEST"
fi

echo
echo "==> RISC OS OMA game build completed"
echo "    Built:    $built_count"
echo "    Missing:  $missing_count"
echo "    Manifest: $MANIFEST"
if [ "$PACKAGE_GAMES" -eq 1 ]; then
    echo "    Packages: $PACKAGE_OUTDIR"
fi

# end of riscos-build-oma-games.sh
