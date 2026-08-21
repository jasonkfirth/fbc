#!/usr/bin/env bash
#
# FreeBASIC Windows CE MIPS sfxlib test campaign
# ------------------------------------------------
#
# File: run-mips-sfxtests.sh
#
# Purpose:
#
#     Build and run the complete assertion-enabled sfxlib test suite in one
#     persistent Windows CE MIPS emulator session.
#
# Responsibilities:
#
#     - compile every runtime sfxlib test for Windows CE MIPS
#     - check the compile-pass and compile-fail language cases
#     - build the guest campaign runner with its CE memory preparation
#     - stage one bounded manifest and validate every process result
#     - preserve results, completion state, journal, and memory telemetry
#
# This file intentionally does NOT contain:
#
#     - ROM acquisition or redistribution
#     - CERF construction
#     - interactive audio-device testing
#     - host-native sfxlib tests
#

set -euo pipefail

##############################################################################
# Paths and defaults
##############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
MIPS_COMPILER="$ROOT/src/tools/wince/fbc-wince-mips"
TOOLCHAIN_ROOT="$ROOT/out/wince/mips-toolchain"
RUNTIME_LIB="$ROOT/out/wince/work-mips/lib/freebasic/wince-mips32el"
EMULATOR_RUNNER="$SCRIPT_DIR/run-mips-emulator.sh"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf-mips}"
OUTPUT_ROOT="$ROOT/out/wince/sfxtests/mips"
ROM_NAME="roms-rockhopper-mips4-ce6.bin"
RUN_SECONDS=7200
BUILD_TESTS=1

RUNTIME_TESTS=(
	capture-buffer
	command-latency
	convert-float-s16
	driver-dump
	driver-null
	lifecycle-stress
	midi-fm-controls
	midi-fm-fallback
	midi-fm-playback
	midi-fm-polyphony
	midi-fm-programs
	midi-fm-rates
	mixer-frame-clock
	noise-pitch
	play-64-voice-smoke
	raw-write
	render-polyphony
	render-tone
	sample-pitch
	sample-rate
	state
	voice-effects-stress
)

COMPILE_FAILURE_TESTS=(
	void-expression-capture-stop
	void-expression-device-list
	void-expression-sfx-stop
)

##############################################################################
# Diagnostics and argument handling
##############################################################################

msg() {
	printf 'INFO: %s\n' "$*"
}

die() {
	printf 'ERROR: %s\n' "$*" >&2
	exit 1
}

require_command() {
	command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

usage() {
	cat <<'USAGE'
Usage: build_scripts/wince/run-mips-sfxtests.sh [options]

Options:
  --skip-build       Reuse previously built test and runner executables.
  --out PATH         Build and evidence directory.
  --cerf PATH        Existing CERF tree containing the Rockhopper CE 6 ROM.
  --run-seconds N    Host campaign deadline. Default: 7200.
  -h, --help         Show this help.
USAGE
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--skip-build)
			BUILD_TESTS=0
			shift
			;;
		--out)
			[ "$#" -ge 2 ] || die "--out requires a path"
			OUTPUT_ROOT="$2"
			shift 2
			;;
		--cerf)
			[ "$#" -ge 2 ] || die "--cerf requires a path"
			CERF_ROOT="$2"
			shift 2
			;;
		--run-seconds)
			[ "$#" -ge 2 ] || die "--run-seconds requires a value"
			RUN_SECONDS="$2"
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

case "$RUN_SECONDS" in
	''|*[!0-9]*|0) die "run duration must be a positive integer" ;;
esac

for tool in awk clang cp mkdir; do
	require_command "$tool"
done

[ -x "$MIPS_COMPILER" ] || die "Windows CE MIPS compiler wrapper is missing"
[ -x "$EMULATOR_RUNNER" ] || die "MIPS emulator runner is missing"
[ -x "$TOOLCHAIN_ROOT/bin/mips-wince-pe-ar" ] ||
	die "MIPS PE archiver is missing"
[ -d "$RUNTIME_LIB" ] || die "Windows CE MIPS runtime libraries are missing"
[ -f "$CERF_ROOT/cerf.exe" ] || die "CERF executable is missing"
[ -f "$CERF_ROOT/cerf.json" ] || die "CERF configuration is missing"
[ -f "$CERF_ROOT/$ROM_NAME" ] || die "Rockhopper CE 6 ROM is missing"
[ -d "$CERF_ROOT/ce_apps/mips1" ] || die "CERF MIPS guest additions are missing"
[ -d "$CERF_ROOT/share" ] || die "CERF shared directory is missing"

mkdir -p "$OUTPUT_ROOT/bin" "$OUTPUT_ROOT/logs" "$OUTPUT_ROOT/objects"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
CERF_ROOT="$(cd "$CERF_ROOT" && pwd)"
SHARE_ROOT="$CERF_ROOT/share"
STAGE_ROOT="$SHARE_ROOT/sfxtests"

##############################################################################
# Windows CE MIPS compilation
##############################################################################

CLANG_FLAGS=(
	--target=mipsel-pc-windows-msvc
	-U__GNUC__
	-U__GNUC_MINOR__
	-U__GNUC_PATCHLEVEL__
	-D__GNUC__=4
	-D__GNUC_MINOR__=2
	-D__GNUC_PATCHLEVEL__=1
	-D__MINGW32CE__
	-D__MINGW32__
	-D__COREDLL__
	-DFFI_STATIC_BUILD
	-D_WIN32_WCE=0x0500
	-D_M_MRX000=4000
	-DMIPS
	-mcpu=mips2
	-mno-check-zero-division
	-mabi=32
	-msoft-float
	-ffreestanding
	-fno-builtin
	-I"$ROOT/src/rtlib/wince/mips32el"
	-I"$TOOLCHAIN_ROOT/include"
	-I"$ROOT/src/sfxlib"
	-I"$ROOT/src/rtlib"
)

FBC_FLAGS=(
	-O 2
	-eassert
	-exx
	-p "$RUNTIME_LIB"
	-l sfx
)

build_runtime_test() {
	local test="$1"
	local extra_objects=()

	if [ "$test" = "midi-fm-rates" ]; then
		extra_objects+=( "$OUTPUT_ROOT/objects/midi-fm-test-rate.o" )
	fi

	"$MIPS_COMPILER" "${FBC_FLAGS[@]}" \
		"$ROOT/tests/sfx/$test.bas" "${extra_objects[@]}" \
		-x "$OUTPUT_ROOT/bin/$test.exe" \
		>"$OUTPUT_ROOT/logs/$test.log" 2>&1
}

if [ "$BUILD_TESTS" -eq 1 ]; then
	msg "building Windows CE MIPS campaign runner"
	clang "${CLANG_FLAGS[@]}" \
		-c "$ROOT/tests/wince/exampleageddon-runner.c" \
		-o "$OUTPUT_ROOT/objects/exampleageddon-runner.o"
	"$MIPS_COMPILER" -O 2 \
		"$OUTPUT_ROOT/objects/exampleageddon-runner.o" \
		-x "$OUTPUT_ROOT/bin/wince-campaign-runner.exe" \
		>"$OUTPUT_ROOT/logs/wince-campaign-runner.log" 2>&1

	msg "building mixed-language MIDI sample-rate helper"
	clang "${CLANG_FLAGS[@]}" \
		-c "$ROOT/tests/sfx/midi-fm-test-rate.c" \
		-o "$OUTPUT_ROOT/objects/midi-fm-test-rate.o"

	for test in "${RUNTIME_TESTS[@]}"; do
		msg "building $test"
		build_runtime_test "$test"
	done

	msg "checking compile-pass syntax case"
	(
		cd "$OUTPUT_ROOT/objects"
		"$MIPS_COMPILER" -O 2 -eassert -exx -c \
			-i "$ROOT/tests/sfx" "$ROOT/tests/sfx/syntax.bas" \
			>"$OUTPUT_ROOT/logs/syntax.log" 2>&1
	)

	for test in "${COMPILE_FAILURE_TESTS[@]}"; do
		msg "checking expected compile failure: $test"
		if (
			cd "$OUTPUT_ROOT/objects"
			"$MIPS_COMPILER" -O 2 -eassert -exx -c \
				-i "$ROOT/tests/sfx" "$ROOT/tests/sfx/$test.bas" \
				>"$OUTPUT_ROOT/logs/$test.log" 2>&1
		); then
			die "compile-failure test unexpectedly succeeded: $test"
		fi
	done
fi

for test in "${RUNTIME_TESTS[@]}"; do
	[ -f "$OUTPUT_ROOT/bin/$test.exe" ] || die "test executable is missing: $test"
done
[ -f "$OUTPUT_ROOT/bin/wince-campaign-runner.exe" ] ||
	die "campaign runner executable is missing"

##############################################################################
# Persistent guest campaign
##############################################################################

mkdir -p "$STAGE_ROOT/sfx"
cp "$OUTPUT_ROOT/bin/wince-campaign-runner.exe" \
	"$SHARE_ROOT/wince-campaign-runner.exe"

: > "$SHARE_ROOT/exampleageddon-manifest.txt"
for test in "${RUNTIME_TESTS[@]}"; do
	cp "$OUTPUT_ROOT/bin/$test.exe" "$STAGE_ROOT/$test.exe"
	printf 'sfx-%s\tsfxtests\\%s.exe\t1800\t\r\n' "$test" "$test" \
		>> "$SHARE_ROOT/exampleageddon-manifest.txt"
done

rm -f -- \
	"$SHARE_ROOT/exampleageddon.result" \
	"$SHARE_ROOT/exampleageddon.journal" \
	"$SHARE_ROOT/exampleageddon.done" \
	"$SHARE_ROOT/exampleageddon-memory.txt"

msg "running ${#RUNTIME_TESTS[@]} tests in one Windows CE MIPS session"
WINCE_CERF_ROOT="$CERF_ROOT" "$EMULATOR_RUNNER" \
	--board-id nec_rockhopper \
	--rom "$CERF_ROOT/$ROM_NAME" \
	--program wince-campaign-runner.exe \
	--completion exampleageddon.done \
	--log-stem mips-sfxtests \
	--boot-seconds 30 \
	--run-seconds "$RUN_SECONDS"

##############################################################################
# Result validation and evidence
##############################################################################

[ -f "$SHARE_ROOT/exampleageddon.result" ] || die "campaign result is missing"
[ -f "$SHARE_ROOT/exampleageddon.done" ] || die "campaign completion is missing"
[ -f "$SHARE_ROOT/exampleageddon-memory.txt" ] || die "memory telemetry is missing"

awk -F '\t' -v expected="${#RUNTIME_TESTS[@]}" '
	{
		gsub( /\r/, "", $2 )
		if( NF < 2 || $2 != "0" )
			failed = 1
		count += 1
	}
	END {
		exit !(count == expected && failed == 0)
	}
' "$SHARE_ROOT/exampleageddon.result" || die "one or more runtime tests failed"

awk -F '\t' -v expected="${#RUNTIME_TESTS[@]}" '
	NR == 1 {
		gsub( /\r/, "", $2 )
		valid = ($1 == expected && $2 == "0")
	}
	END { exit !valid }
' "$SHARE_ROOT/exampleageddon.done" || die "campaign completion is invalid"

cp "$SHARE_ROOT/exampleageddon.result" "$OUTPUT_ROOT/results.tsv"
cp "$SHARE_ROOT/exampleageddon.journal" "$OUTPUT_ROOT/journal.tsv"
cp "$SHARE_ROOT/exampleageddon.done" "$OUTPUT_ROOT/completion.txt"
cp "$SHARE_ROOT/exampleageddon-memory.txt" "$OUTPUT_ROOT/memory.txt"

msg "Windows CE MIPS sfxlib tests passed: ${#RUNTIME_TESTS[@]}/${#RUNTIME_TESTS[@]}"

# end of build_scripts/wince/run-mips-sfxtests.sh
