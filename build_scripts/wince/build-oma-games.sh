#!/usr/bin/env bash
#
# Project: FreeBASIC OMA games for Windows CE
# --------------------------------------------
#
# File: wince/build-oma-games.sh
#
# Purpose:
#
#     Cross-build and individually package the available OMA games for
#     Windows CE ARM or MIPS.
#
# Responsibilities:
#
#     - keep the complete OMA and OpenSlicks source matrix in one place
#     - compile available games with the selected pinned WinCE toolchain
#     - stage only the runtime assets used by each game
#     - create and byte-verify one downloadable ZIP per game
#     - record absent source trees without hiding coverage gaps
#
# This file intentionally does NOT contain:
#
#     - Windows CE ROM or emulator construction
#     - launch qualification or gameplay automation
#     - fbctests or Exampleageddon orchestration
#     - compiler or runtime construction policy

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults and matrix
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
WORK_ROOT="${WINCE_WORK_ROOT:-$ROOT/out/wince/work}"
MIPS_COMPILER="${WINCE_MIPS_FBC:-$ROOT/src/tools/wince/fbc-wince-mips}"
MIPS_TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$ROOT/out/wince/mips-toolchain}"
OUTPUT_ROOT="${WINCE_OMA_OUTDIR:-}"
PACKAGE_OUTDIR="${WINCE_PACKAGE_OUTDIR:-}"
PACKAGE_REVISION="${WINCE_OMA_PACKAGE_REVISION:-1}"
TARGET_ARCH="${WINCE_ARCH:-arm}"

PACKAGE_GAMES=1
SELECTED_GAMES_TEXT=""
STRICT_MATRIX=0
TEMP_ROOT=""
PACKAGE_STAGE=""
EXTRACT_STAGE=""

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

require_command() {
	command -v "$1" >/dev/null 2>&1 ||
		die "required host tool not found: $1"
}

require_file() {
	[ -f "$1" ] || die "required file not found: $1"
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

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/build-oma-games.sh [options]

Options:
  --arch ARCH        Target architecture: arm or mips. Default: arm
  --games LIST       Comma-separated game keys. Default: complete matrix
  --out-dir DIR      Build and staged-game root. Default: out/wince/oma/ARCH
  --package-dir DIR  Package output. Default: out/packages/wince
  --revision N       Package revision. Default: 1
  --no-package       Build and stage games without ZIP archives
  --strict           Fail if a selected source tree is absent
  -h, --help         Show this help

Game keys:
  arkanoid, behold, demoderby, duel999, kinematics, nietzsche, qfak,
  rambo, starphalanx, openmarket, openhostility, turbotrek, vtrek,
  openwallstreet, openslicks
EOF
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
	find "$destination" -type d -name .git -prune -exec rm -rf -- {} +
	find "$destination" -type f \
		\( -iname 'Thumbs.db' -o -iname 'Thumbs(1).db' \
		   -o -iname '*.exe' -o -iname '*.dll' -o -iname '*.so' \
		   -o -iname '*.apk' -o -iname '*.idsig' \) -delete
}

game_is_selected() {
	local key="$1"

	[ "${#SELECTED_GAMES[@]}" -eq 0 ] && return 0
	[[ " ${SELECTED_GAMES[*]} " == *" $key "* ]]
}

trap cleanup EXIT

##############################################################################
# Options and validation
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--arch)
			require_value "$1" "${2-}"
			TARGET_ARCH="$2"
			shift 2
			;;
		--games)
			require_value "$1" "${2-}"
			SELECTED_GAMES_TEXT="$2"
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
		--no-package)
			PACKAGE_GAMES=0
			shift
			;;
		--strict)
			STRICT_MATRIX=1
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

case "$PACKAGE_REVISION" in
	''|*[!0-9]*|0) die "--revision must be a positive integer" ;;
esac
case "$TARGET_ARCH" in
	arm|mips) ;;
	*) die "--arch must be arm or mips" ;;
esac

if [ -z "$OUTPUT_ROOT" ]; then
	OUTPUT_ROOT="$ROOT/out/wince/oma/$TARGET_ARCH"
fi
if [ -z "$PACKAGE_OUTDIR" ]; then
	PACKAGE_OUTDIR="$ROOT/out/packages/wince"
fi

if [ -n "$SELECTED_GAMES_TEXT" ]; then
	IFS=',' read -r -a SELECTED_GAMES <<< "$SELECTED_GAMES_TEXT"
else
	SELECTED_GAMES=()
fi

for tool in base64 diff file find grep python3 unzip zip; do
	require_command "$tool"
done
if [ "$TARGET_ARCH" = arm ]; then
	require_command docker
	[ -x "$WORK_ROOT/bin/fbc" ] || die "prepared host compiler not found"
	RUNTIME_ROOT="$WORK_ROOT/lib/freebasic/wince-arm"
else
	[ -x "$MIPS_COMPILER" ] || die "Windows CE MIPS compiler wrapper not found"
	[ -x "$MIPS_TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" ] ||
		die "Windows CE MIPS object inspector not found"
	RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"
fi
for library in libfbmt.a libfbgfxmt.a libsfxmt.a; do
	[ -f "$RUNTIME_ROOT/$library" ] ||
		die "Windows CE $TARGET_ARCH runtime is missing $library"
done

mkdir -p "$OUTPUT_ROOT" "$PACKAGE_OUTDIR"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
PACKAGE_OUTDIR="$(cd "$PACKAGE_OUTDIR" && pwd)"
[ "$OUTPUT_ROOT" != "/" ] || die "refusing to use / as the output root"
[ "$PACKAGE_OUTDIR" != "/" ] || die "refusing to use / as the package root"

##############################################################################
# Compiler definitions
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

	if [ "$TARGET_ARCH" = arm ]; then
		docker run --rm --user "$(id -u):$(id -g)" \
			-v "$ROOT:/src" \
			-v "$WORK_ROOT:/work" \
			-v "$TEMP_ROOT:/build" \
			-w "${working_directory/$ROOT/\/src}" \
			"$TOOLCHAIN_IMAGE" \
			/work/bin/fbc \
				-target wince-arm -gen gcc -O 0 -mt \
				-i /src/inc/wince -i /src/inc \
				"${extra_arguments[@]/$ROOT/\/src}" \
				"${sources[@]/$ROOT/\/src}" \
				-x "/build/bin/$(basename "$output")"
	else
		(
			cd "$working_directory"
			"$MIPS_COMPILER" -O 0 -mt \
				"${extra_arguments[@]}" \
				"${sources[@]}" \
				-x "$output"
		)
	fi

	[ -s "$output" ] || die "FreeBASIC did not produce $output"
	if [ "$TARGET_ARCH" = arm ]; then
		[[ "$(file -b "$output")" == *"Windows CE"*"ARMv4"* ]] ||
			die "$output is not an ARMv4 Windows CE executable"
	else
		[[ "$(file -b "$output")" == *"Windows CE"*"MIPS R4000"* ]] ||
			die "$output is not a MIPS R4000 Windows CE executable"
		"$MIPS_TOOLCHAIN_ROOT/bin/mips-wince-pe-objdump" -f "$output" |
			grep -F 'file format pei-mips' >/dev/null ||
			die "$output does not use the MIPS PE format"
	fi
}

##############################################################################
# Assets and packages
##############################################################################

stage_open_slicks_fixture() {
	local game_stage="$1"

	mkdir -p "$game_stage/TRACKS"
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
	local key="$1"
	local game_stage="$2"
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
				FILE_ID.DIZ VENDOR.DOC VENDOR.TXT REGISTER.DOC \
				USERINFO.DOC; do
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

write_readme() {
	local display_name="$1"
	local game_stage="$2"

	cat > "$game_stage/README.txt" <<EOF
$display_name for Windows CE $TARGET_ARCH

Install:

1. Extract this package on a compatible Windows CE $TARGET_ARCH device.
2. Keep game.exe and all packaged data files in the same directory.
3. Start game.exe from File Explorer.

The executable is statically linked with the FreeBASIC Windows CE
runtime, gfxlib2, and sfxlib. Graphics use GDI and audio uses waveOut.
EOF
}

package_game() {
	local game_directory="$1"
	local game_stage="$2"
	local package_base="$3"
	local package_path="$PACKAGE_OUTDIR/$package_base.zip"
	local temporary_path="$PACKAGE_OUTDIR/$package_base.new.zip"

	PACKAGE_STAGE="$(mktemp -d "$PACKAGE_OUTDIR/.package-stage.XXXXXX")"
	EXTRACT_STAGE="$(mktemp -d "$PACKAGE_OUTDIR/.extract-stage.XXXXXX")"
	cp -a "$game_stage" "$PACKAGE_STAGE/$game_directory"

	rm -f -- "$package_path" "$temporary_path"
	(
		cd "$PACKAGE_STAGE"
		zip -9 -r "$temporary_path" "$game_directory" >/dev/null
	)
	unzip -q "$temporary_path" -d "$EXTRACT_STAGE"
	diff -qr "$PACKAGE_STAGE" "$EXTRACT_STAGE"
	mv "$temporary_path" "$package_path"
	[ -s "$package_path" ] || die "package was not created: $package_base"

	rm -rf -- "$PACKAGE_STAGE" "$EXTRACT_STAGE"
	PACKAGE_STAGE=""
	EXTRACT_STAGE=""
}

##############################################################################
# Matrix execution
##############################################################################

TEMP_ROOT="$(mktemp -d "$OUTPUT_ROOT/.build-stage.XXXXXX")"
mkdir -p "$TEMP_ROOT/bin" "$TEMP_ROOT/games"
MANIFEST_TEMP="$TEMP_ROOT/manifest.tsv"
printf '%s\n' $'key\tdisplay\tkind\tstatus\tsource\tgame_directory\tpackage\tnote' \
	> "$MANIFEST_TEMP"

built_count=0
missing_count=0

for row in "${GAME_ROWS[@]}"; do
	IFS=$'\t' read -r \
		key display_name kind game_directory source_path <<< "$row"
	if ! game_is_selected "$key"; then
		printf '%s\t%s\t%s\tnot-built\t%s\t%s\t\tnot selected\n' \
			"$key" "$display_name" "$kind" "$source_path" \
			"$game_directory" >> "$MANIFEST_TEMP"
		continue
	fi

	if [ ! -f "$ROOT/$source_path" ]; then
		printf '%s\t%s\t%s\tmissing\t%s\t%s\t\tsource tree is not present\n' \
			"$key" "$display_name" "$kind" "$source_path" \
			"$game_directory" >> "$MANIFEST_TEMP"
		missing_count=$((missing_count + 1))
		continue
	fi

	msg "building $display_name for Windows CE $TARGET_ARCH"
	program="$TEMP_ROOT/bin/$key.exe"
	game_stage="$TEMP_ROOT/games/$game_directory"
	package_base="${game_directory}-r${PACKAGE_REVISION}-wince-$TARGET_ARCH"
	package_path="$PACKAGE_OUTDIR/$package_base.zip"
	mkdir -p "$game_stage"

	compile_game "$key" "$program"
	stage_game_assets "$key" "$game_stage"
	cp "$program" "$game_stage/game.exe"
	write_readme "$display_name" "$game_stage"
	if [ "$PACKAGE_GAMES" -eq 1 ]; then
		package_game "$game_directory" "$game_stage" "$package_base"
	else
		package_path=""
	fi

	printf '%s\t%s\t%s\tbuilt\t%s\t%s\t%s\t\n' \
		"$key" "$display_name" "$kind" "$source_path" \
		"$game_directory" "$package_path" >> "$MANIFEST_TEMP"
	built_count=$((built_count + 1))
done

rm -rf -- "$OUTPUT_ROOT/bin" "$OUTPUT_ROOT/games"
mv "$TEMP_ROOT/bin" "$OUTPUT_ROOT/bin"
mv "$TEMP_ROOT/games" "$OUTPUT_ROOT/games"
mv "$MANIFEST_TEMP" "$OUTPUT_ROOT/manifest.tsv"
rm -rf -- "$TEMP_ROOT"
TEMP_ROOT=""

if [ "$STRICT_MATRIX" -eq 1 ] && [ "$missing_count" -ne 0 ]; then
	die "$missing_count selected source tree(s) are absent"
fi

echo
echo "==> Windows CE $TARGET_ARCH OMA games built: $built_count; missing: $missing_count"
echo "    Manifest: $OUTPUT_ROOT/manifest.tsv"
if [ "$PACKAGE_GAMES" -eq 1 ]; then
	echo "    Packages: $PACKAGE_OUTDIR"
fi

# end of build_scripts/wince/build-oma-games.sh
