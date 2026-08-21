#!/usr/bin/env bash
#
# Project: FreeBASIC OMA games for Windows CE
# --------------------------------------------
#
# File: wince/run-oma-games.sh
#
# Purpose:
#
#     Launch-qualify every available OMA game in the Windows CE ARM emulator.
#
# Responsibilities:
#
#     - optionally invoke the authoritative OMA build and package matrix
#     - stage each game in CERF's shared folder without stale guest state
#     - require every game process to remain alive for a bounded startup period
#     - preserve per-game results, emulator logs, and live screenshots
#     - create a resumable aggregate execution report
#
# This file intentionally does NOT contain:
#
#     - OMA source, compiler, asset, or packaging definitions
#     - Windows CE ROM or emulator installation
#     - deterministic gameplay input automation
#     - Windows CE MIPS policy
#
# Test boundary:
#
#     These interactive games do not expose deterministic end-to-end test
#     APIs. A pass proves that the target executable loads on Windows CE,
#     resolves its staged assets, opens its gfxlib2 path, and remains alive
#     through startup. Each saved screenshot is reviewable visual evidence.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf}"
OMA_ROOT="${WINCE_OMA_OUTDIR:-$ROOT/out/wince/oma/arm}"
OUTPUT_ROOT="${WINCE_OMA_TEST_OUTDIR:-$OMA_ROOT/tests}"

BOOT_SECONDS=15
BUILD_GAMES=1
RESUME=0
SELECTED_GAMES_TEXT=""
STABILITY_SECONDS=8

SHARE_STAGE=""

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

game_is_selected() {
	local key="$1"

	[ "${#SELECTED_GAMES[@]}" -eq 0 ] && return 0
	[[ " ${SELECTED_GAMES[*]} " == *" $key "* ]]
}

cleanup() {
	if [ -n "$SHARE_STAGE" ] &&
	   [[ "$SHARE_STAGE" == "$CERF_ROOT/share"/.oma-stage.* ]] &&
	   [ -d "$SHARE_STAGE" ]; then
		rm -rf -- "$SHARE_STAGE"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/run-oma-games.sh [options]

Options:
  --games LIST       Comma-separated game keys. Default: all built games
  --stability SEC    Required live-process interval. Default: 8
  --boot-seconds N   Guest shell initialization wait. Default: 15
  --skip-build       Reuse the existing games and individual packages
  --resume           Reuse matching successful per-game evidence
  --out-dir DIR      Evidence root. Default: out/wince/oma/arm/tests
  -h, --help         Show this help

A passing launch saves a result, CERF log, and PNG screenshot below the
evidence root. Games absent from the source checkout remain recorded as
missing in the build manifest and are not launchable.
EOF
}

trap cleanup EXIT

##############################################################################
# Options and validation
##############################################################################

while [ "$#" -gt 0 ]; do
	case "$1" in
		--games)
			require_value "$1" "${2-}"
			SELECTED_GAMES_TEXT="$2"
			shift 2
			;;
		--stability)
			require_value "$1" "${2-}"
			STABILITY_SECONDS="$2"
			shift 2
			;;
		--boot-seconds)
			require_value "$1" "${2-}"
			BOOT_SECONDS="$2"
			shift 2
			;;
		--skip-build)
			BUILD_GAMES=0
			shift
			;;
		--resume)
			RESUME=1
			shift
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

for positive_integer in "$STABILITY_SECONDS" "$BOOT_SECONDS"; do
	case "$positive_integer" in
		''|*[!0-9]*|0) die "stability and boot durations must be positive integers" ;;
	esac
done

if [ -n "$SELECTED_GAMES_TEXT" ]; then
	IFS=',' read -r -a SELECTED_GAMES <<< "$SELECTED_GAMES_TEXT"
else
	SELECTED_GAMES=()
fi

for tool in awk cp docker find mkdir mktemp rm sha256sum sort tr xargs; do
	require_command "$tool"
done
[ -x "$SCRIPT_DIR/build-oma-games.sh" ] ||
	die "Windows CE OMA build script is not executable"
[ -x "$SCRIPT_DIR/run-arm-emulator.sh" ] ||
	die "Windows CE ARM emulator runner is not executable"
[ -f "$ROOT/tests/wince/oma-runner.c" ] ||
	die "Windows CE OMA guest runner source is missing"
[ -d "$CERF_ROOT/share" ] || die "CERF shared directory not found"

if [ "$BUILD_GAMES" -eq 1 ]; then
	"$SCRIPT_DIR/build-oma-games.sh"
fi

MANIFEST="$OMA_ROOT/manifest.tsv"
[ -s "$MANIFEST" ] || die "Windows CE OMA build manifest is missing"

mkdir -p "$OUTPUT_ROOT/results" "$OUTPUT_ROOT/logs" \
	"$OUTPUT_ROOT/screenshots" "$OUTPUT_ROOT/state"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
SHARE_ROOT="$CERF_ROOT/share"

##############################################################################
# Guest runner and staging
##############################################################################

msg "building Windows CE ARM OMA launch runner"
docker run --rm --user "$(id -u):$(id -g)" \
	-v "$ROOT:/src" \
	-v "$OUTPUT_ROOT:/output" \
	-w /src \
	"$TOOLCHAIN_IMAGE" \
	arm-mingw32ce-gcc \
		-O0 -march=armv4t -mfloat-abi=soft \
		-D_WIN32_WCE=0x0500 \
		-DOMA_STABILITY_MS="$((STABILITY_SECONDS * 1000))" \
		tests/wince/oma-runner.c \
		-o /output/oma-runner.exe

stage_game() {
	local game_directory="$2"
	local key="$1"
	local destination="$SHARE_ROOT/oma"

	[ -d "$OMA_ROOT/games/$game_directory" ] ||
		die "staged game directory not found: $game_directory"
	SHARE_STAGE="$(mktemp -d "$SHARE_ROOT/.oma-stage.XXXXXX")"
	mkdir -p "$SHARE_STAGE/$game_directory"
	cp -a "$OMA_ROOT/games/$game_directory/." \
		"$SHARE_STAGE/$game_directory/"
	if [ "$key" = duel999 ]; then
		printf '%s\r\n%s\r\n%s\r\n' \
			'127.0.0.1' 'WinCE' 'autostart' \
			> "$SHARE_STAGE/$game_directory/defaults.cfg"
	fi

	[ "$destination" != "/" ] || die "refusing to replace the filesystem root"
	rm -rf -- "$destination"
	mv "$SHARE_STAGE" "$destination"
	SHARE_STAGE=""

	printf '%s\r\n%s\r\n' "$key" "$game_directory" \
		> "$SHARE_ROOT/oma-launch.txt"
	cp "$OUTPUT_ROOT/oma-runner.exe" "$SHARE_ROOT/oma-runner.exe"
	rm -f -- "$SHARE_ROOT/oma.result" "$SHARE_ROOT/oma.done"
}

game_digest() {
	local game_directory="$1"

	(
		cd "$OMA_ROOT/games/$game_directory"
		find . -type f -print0 | sort -z | xargs -0 sha256sum
		sha256sum "$ROOT/tests/wince/oma-runner.c"
	) | sha256sum | awk '{ print $1 }'
}

saved_result_passes() {
	local digest="$4"
	local key="$1"
	local result_file="$2"
	local state_file="$3"

	[ -s "$result_file" ] && [ -s "$state_file" ] || return 1
	awk -F '\t' -v expected="$key" \
		'NR == 1 && $1 == expected && $2 == "alive" && ($3 + 0) == 259 &&
		 $4 == "gameplay" && ($6 + 0) > 0 && $7 ~ /^window\r?$/ {
			passed = 1
		}
		END { exit !passed }' "$result_file" || return 1
	awk -F '=' -v expected_digest="$digest" \
		-v expected_stability="$STABILITY_SECONDS" \
		'$1 == "digest" && $2 == expected_digest { digest_matches = 1 }
		 $1 == "stability_seconds" && $2 == expected_stability {
			stability_matches = 1
		 }
		 END { exit !(digest_matches && stability_matches) }' "$state_file"
}

save_evidence() {
	local key="$1"
	local digest="$2"

	cp "$SHARE_ROOT/oma.result" "$OUTPUT_ROOT/results/$key.result"
	printf 'digest=%s\nstability_seconds=%s\n' \
		"$digest" "$STABILITY_SECONDS" > "$OUTPUT_ROOT/state/$key.state"
	if [ -f "$CERF_ROOT/logs/oma-arm-$key.log" ]; then
		cp "$CERF_ROOT/logs/oma-arm-$key.log" "$OUTPUT_ROOT/logs/$key.log"
	fi
	if [ -f "$CERF_ROOT/logs/oma-arm-$key.png" ]; then
		cp "$CERF_ROOT/logs/oma-arm-$key.png" \
			"$OUTPUT_ROOT/screenshots/$key.png"
	fi
}

##############################################################################
# Matrix execution and report
##############################################################################

mapfile -t BUILT_ROWS < <(
	awk -F '\t' 'NR > 1 && $4 == "built" { print $1 "\t" $2 "\t" $6 }' \
		"$MANIFEST"
)
RUN_ROWS=()
for row in "${BUILT_ROWS[@]}"; do
	IFS=$'\t' read -r key display_name game_directory <<< "$row"
	if game_is_selected "$key"; then
		RUN_ROWS+=("$row")
	fi
done
[ "${#RUN_ROWS[@]}" -gt 0 ] || die "no built OMA games were selected"

for row in "${RUN_ROWS[@]}"; do
	IFS=$'\t' read -r key display_name game_directory <<< "$row"
	digest="$(game_digest "$game_directory")"
	saved_result="$OUTPUT_ROOT/results/$key.result"
	state_file="$OUTPUT_ROOT/state/$key.state"
	if [ "$RESUME" -eq 1 ] &&
	   saved_result_passes "$key" "$saved_result" "$state_file" "$digest"; then
		echo "==> PASS (saved): $display_name"
		continue
	fi

	msg "launching $display_name on Windows CE ARM"
	stage_game "$key" "$game_directory"
	#
	# QFAK deliberately streams input while its legacy typewriter scenes render.
	# The host allowance covers that bounded guest work; it does not relax any
	# process, window, input, gameplay, or stability requirement.
	#
	"$SCRIPT_DIR/run-arm-emulator.sh" \
		--program oma-runner.exe \
		--completion oma.done \
		--log-stem "oma-arm-$key" \
		--boot-seconds "$BOOT_SECONDS" \
		--run-seconds "$((STABILITY_SECONDS + 600))"

	[ -s "$SHARE_ROOT/oma.done" ] ||
		die "$display_name did not publish its completion marker"
	[ -s "$SHARE_ROOT/oma.result" ] ||
		die "$display_name did not publish its launch result"
	save_evidence "$key" "$digest"
	saved_result_passes "$key" "$saved_result" "$state_file" "$digest" ||
		die "$display_name exited or failed during startup"
	echo "==> PASS: $display_name"
done

RESULTS="$OUTPUT_ROOT/execution-results.tsv"
printf '%s\n' $'key\tstatus\tcode\tproof\tscript\tinputs\twindow' > "$RESULTS"
for row in "${RUN_ROWS[@]}"; do
	IFS=$'\t' read -r key display_name game_directory <<< "$row"
	digest="$(game_digest "$game_directory")"
	saved_result_passes "$key" "$OUTPUT_ROOT/results/$key.result" \
		"$OUTPUT_ROOT/state/$key.state" "$digest" ||
		die "saved OMA result is not a passing launch: $key"
	tr -d '\r' < "$OUTPUT_ROOT/results/$key.result" >> "$RESULTS"
done

REPORT="$OUTPUT_ROOT/execution-report.md"
cat > "$REPORT" <<EOF
# Windows CE ARM OMA gameplay execution

- Games available and run: ${#RUN_ROWS[@]}
- Passed: ${#RUN_ROWS[@]}
- Failed: 0
- Required post-input live-process interval: $STABILITY_SECONDS seconds
- Every pass delivered its complete title-specific input script to a game window.
- Per-game results: \`execution-results.tsv\`
- Emulator evidence: \`logs/\` and \`screenshots/\`
EOF

echo
echo "==> Windows CE ARM OMA gameplay qualification passed: ${#RUN_ROWS[@]}/${#RUN_ROWS[@]} games"
echo "    Execution report: $REPORT"

# end of build_scripts/wince/run-oma-games.sh
