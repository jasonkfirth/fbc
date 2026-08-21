#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE test automation
# ------------------------------------------------
#
# File: build_scripts/wince/run-mips-test-session.sh
#
# Purpose:
#
#   Build fbctests shards and self-contained examples, then run every staged
#   executable sequentially during one persistent MIPS Windows CE session.
#
# Responsibilities:
#
#   - delegate compilation to the existing WinCE test workflows
#   - stage successful fbcunit shards and Exampleageddon cases
#   - create the extended guest campaign manifest
#   - boot CERF once and collect incremental child-process results
#
# This file intentionally does NOT contain:
#
#   - fbctests selection or linking rules
#   - Exampleageddon source classification
#   - Windows CE child-process implementation
#   - emulator or compiler installation
#

set -euo pipefail

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/../.." && pwd)"

FBCTESTS_SCRIPT="$ROOT/build_scripts/wince/run-mips-fbctests.sh"
EXAMPLEAGEDDON_SCRIPT="$ROOT/build_scripts/wince/run-exampleageddon.sh"
EMULATOR_SCRIPT="$ROOT/build_scripts/wince/run-mips-emulator.sh"
MIPS_COMPILER="$ROOT/src/tools/wince/fbc-wince-mips"
MIPS_TOOLCHAIN_ROOT="$ROOT/out/wince/mips-toolchain"

WORK_DIR="$ROOT/out/wince/work-mips"
FBCTESTS_OUT="$ROOT/out/wince/fbctests/mips-session"
EXAMPLEAGEDDON_OUT="$ROOT/out/wince/exampleageddon/mips-session"
SESSION_OUT="$ROOT/out/wince/test-session/mips"
CERF_DIR="$ROOT/out/wince/emulator/cerf-mips"
BOARD_ID="nec_rockhopper"
ROM="$CERF_DIR/roms-rockhopper-mips4-ce6.bin"

DIRS=""
JOBS=2
BOOT_SECONDS=30
RUN_SECONDS=""
FBCTEST_TIMEOUT=180
FBCTEST_BATCH_SIZE=12
EXAMPLE_TIMEOUT=15
EXAMPLE_LIMIT=0
BUILD_TESTS=1
RUN_TESTS=1
INCLUDE_FBCTESTS=1
INCLUDE_EXAMPLES=1
RESUME=0

usage()
{
	cat <<EOF
Usage: ./build_scripts/wince/run-mips-test-session.sh [options]

Options:
  --dirs LIST            Comma-separated fbctests directories. Default: all
  --example-limit N      Stage at most N examples. Default: all
  --jobs N               Parallel cross-build jobs. Default: 2
  --boot-seconds N       Windows CE shell initialization wait. Default: 30
  --run-seconds N        Whole-campaign timeout. Default: calculated maximum
  --fbctest-timeout N    Per-fbctests-shard timeout. Default: 180
  --fbctest-batch-size N Maximum source modules per shard. Default: 12
  --example-timeout N    Per-example timeout. Default: 15
  --skip-build           Reuse existing compile inventories and executables
  --stage-only           Build and stage without launching CERF
  --skip-fbctests        Build and run only Exampleageddon
  --skip-examples        Build and run only fbctests
  --resume               Forward resume mode to the delegated build workflows
  --work-dir DIR         Prepared source tree. Default: out/wince/work-mips
  --fbctests-out DIR     fbctests build directory
  --exampleageddon-out DIR
                         Exampleageddon build directory
  --session-out DIR      Persistent-session result directory
  --cerf-dir DIR         Prepared CERF tree
  --board-id ID          CERF board id. Default: nec_rockhopper
  --rom PATH             Windows CE ROM. Default: Rockhopper CE6 image
  -h, --help             Show this help

Every child process is closed before the next one starts. Results are flushed
after each child, so a later emulator failure does not discard earlier status.
EOF
}

die()
{
	echo "ERROR: $*" >&2
	exit 1
}

positive_integer()
{
	case "$1" in
		''|*[!0-9]*|0)
			return 1
			;;
	esac

	return 0
}

safe_id()
{
	case "$1" in
		''|*[^A-Za-z0-9._-]*)
			return 1
			;;
	esac

	return 0
}

stage_test_data_directory()
{
	local directory="$1"
	local destination="$SHARE_DIR/$directory"
	local source="$WORK_DIR/tests/$directory"

	[ -d "$source" ] || return 0

	rm -rf "$destination"
	mkdir -p "$destination"
	cp -a "$source/." "$destination/"
	find "$destination" -type f ! \( \
		-name '*.bas' -o \
		-name '*.bi' -o \
		-name '*.bin' -o \
		-name '*.bmp' -o \
		-name '*.csv' -o \
		-name '*.dat' -o \
		-name '*.json' -o \
		-name '*.png' -o \
		-name '*.raw' -o \
		-name '*.txt' -o \
		-name '*.wav' \
	\) -delete
	find "$destination" -depth -type d -empty -delete
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dirs)
			DIRS="$2"
			shift 2
			;;
		--example-limit)
			EXAMPLE_LIMIT="$2"
			shift 2
			;;
		--jobs)
			JOBS="$2"
			shift 2
			;;
		--boot-seconds)
			BOOT_SECONDS="$2"
			shift 2
			;;
		--run-seconds)
			RUN_SECONDS="$2"
			shift 2
			;;
		--fbctest-timeout)
			FBCTEST_TIMEOUT="$2"
			shift 2
			;;
		--fbctest-batch-size)
			FBCTEST_BATCH_SIZE="$2"
			shift 2
			;;
		--example-timeout)
			EXAMPLE_TIMEOUT="$2"
			shift 2
			;;
		--skip-build)
			BUILD_TESTS=0
			shift
			;;
		--stage-only)
			RUN_TESTS=0
			shift
			;;
		--skip-fbctests)
			INCLUDE_FBCTESTS=0
			shift
			;;
		--skip-examples)
			INCLUDE_EXAMPLES=0
			shift
			;;
		--resume)
			RESUME=1
			shift
			;;
		--work-dir)
			WORK_DIR="$2"
			shift 2
			;;
		--fbctests-out)
			FBCTESTS_OUT="$2"
			shift 2
			;;
		--exampleageddon-out)
			EXAMPLEAGEDDON_OUT="$2"
			shift 2
			;;
		--session-out)
			SESSION_OUT="$2"
			shift 2
			;;
		--cerf-dir)
			CERF_DIR="$2"
			shift 2
			;;
		--board-id)
			BOARD_ID="$2"
			shift 2
			;;
		--rom)
			ROM="$2"
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

positive_integer "$JOBS" || die "--jobs must be a positive integer"
positive_integer "$BOOT_SECONDS" ||
	die "--boot-seconds must be a positive integer"
positive_integer "$FBCTEST_TIMEOUT" ||
	die "--fbctest-timeout must be a positive integer"
positive_integer "$FBCTEST_BATCH_SIZE" ||
	die "--fbctest-batch-size must be a positive integer"
positive_integer "$EXAMPLE_TIMEOUT" ||
	die "--example-timeout must be a positive integer"

case "$EXAMPLE_LIMIT" in
	''|*[!0-9]*)
		die "--example-limit must be zero or a positive integer"
		;;
esac

if [ -n "$RUN_SECONDS" ]; then
	positive_integer "$RUN_SECONDS" ||
		die "--run-seconds must be a positive integer"
fi

[ "$FBCTEST_TIMEOUT" -le 3600 ] ||
	die "--fbctest-timeout exceeds the guest runner maximum of 3600"
[ "$EXAMPLE_TIMEOUT" -le 3600 ] ||
	die "--example-timeout exceeds the guest runner maximum of 3600"
[ "$INCLUDE_FBCTESTS" -ne 0 ] || [ "$INCLUDE_EXAMPLES" -ne 0 ] ||
	die "nothing selected after --skip-fbctests and --skip-examples"

[ -x "$FBCTESTS_SCRIPT" ] || die "missing fbctests workflow: $FBCTESTS_SCRIPT"
[ -x "$EXAMPLEAGEDDON_SCRIPT" ] ||
	die "missing Exampleageddon workflow: $EXAMPLEAGEDDON_SCRIPT"
[ -x "$EMULATOR_SCRIPT" ] || die "missing emulator runner: $EMULATOR_SCRIPT"
[ -x "$MIPS_COMPILER" ] || die "missing MIPS compiler wrapper: $MIPS_COMPILER"
[ -d "$MIPS_TOOLCHAIN_ROOT/include" ] ||
	die "missing MIPS toolchain includes: $MIPS_TOOLCHAIN_ROOT/include"
[ -d "$CERF_DIR" ] || die "missing CERF directory: $CERF_DIR"
[ -f "$ROM" ] || die "missing Windows CE ROM: $ROM"

mkdir -p "$SESSION_OUT"

if [ "$BUILD_TESTS" -ne 0 ]; then
	if [ "$INCLUDE_FBCTESTS" -ne 0 ]; then
		fbctests_args=(
			--build-only
			--jobs "$JOBS"
			--batch-size "$FBCTEST_BATCH_SIZE"
			--work-dir "$WORK_DIR"
			--out-dir "$FBCTESTS_OUT"
			--cerf-dir "$CERF_DIR"
		)

		if [ -n "$DIRS" ]; then
			fbctests_args+=( --dirs "$DIRS" )
		fi
		if [ "$RESUME" -ne 0 ]; then
			fbctests_args+=( --resume )
		fi

		"$FBCTESTS_SCRIPT" "${fbctests_args[@]}"
	fi

	if [ "$INCLUDE_EXAMPLES" -ne 0 ]; then
		exampleageddon_args=(
			--arch mips
			--compile-only
			--jobs "$JOBS"
			--out-dir "$EXAMPLEAGEDDON_OUT"
		)

		if [ "$RESUME" -ne 0 ]; then
			exampleageddon_args+=( --resume )
		fi

		"$EXAMPLEAGEDDON_SCRIPT" "${exampleageddon_args[@]}"
	fi
fi

mkdir -p "$EXAMPLEAGEDDON_OUT"
clang --target=mipsel-pc-windows-msvc \
	-mcpu=mips2 -mno-check-zero-division -mabi=32 -msoft-float \
	-ffreestanding -fno-builtin -fno-exceptions \
	-fno-unwind-tables -fno-asynchronous-unwind-tables \
	-D__MINGW32CE__ -D__MINGW32__ -D__COREDLL__ \
	-D__GNUC__=4 -D__GNUC_MINOR__=2 -D_M_MRX000=4000 -DMIPS \
	-D_WIN32_WCE=0x0500 \
	-isystem "$MIPS_TOOLCHAIN_ROOT/include" -O0 \
	-c "$ROOT/tests/wince/exampleageddon-runner.c" \
	-o "$EXAMPLEAGEDDON_OUT/exampleageddon-runner.o"
"$MIPS_COMPILER" -O 0 \
	"$EXAMPLEAGEDDON_OUT/exampleageddon-runner.o" \
	-x "$EXAMPLEAGEDDON_OUT/exampleageddon-runner.exe"

SESSION_CERF="$SESSION_OUT/cerf"
case "$SESSION_CERF" in
	"$SESSION_OUT"/cerf)
		rm -rf "$SESSION_CERF"
		;;
	*)
		die "refusing to clear unexpected session CERF tree: $SESSION_CERF"
		;;
esac

mkdir -p "$SESSION_CERF"
(
	cd "$CERF_DIR"
	tar --exclude='./share' --exclude='./logs' -cf - .
) | tar -C "$SESSION_CERF" -xf -
mkdir -p "$SESSION_CERF/share" "$SESSION_CERF/logs"

ROM_RELATIVE="$(realpath --relative-to="$CERF_DIR" "$ROM")"
case "$ROM_RELATIVE" in
	..|../*)
		die "the ROM must be located below the selected CERF directory"
		;;
esac
SESSION_ROM="$SESSION_CERF/$ROM_RELATIVE"
[ -f "$SESSION_ROM" ] || die "session ROM was not staged: $SESSION_ROM"

SHARE_DIR="$SESSION_CERF/share"
STAGE_ROOT="$SHARE_DIR/wince-campaign"
MANIFEST="$SHARE_DIR/exampleageddon-manifest.txt"
GUEST_RESULTS="$SHARE_DIR/exampleageddon.result"
GUEST_COMPLETION="$SHARE_DIR/exampleageddon.done"
RUNNER_SOURCE="$EXAMPLEAGEDDON_OUT/exampleageddon-runner.exe"
RUNNER_STAGED="$SHARE_DIR/wince-campaign-runner.exe"

case "$STAGE_ROOT" in
	"$SESSION_CERF"/share/wince-campaign)
		rm -rf "$STAGE_ROOT"
		;;
	*)
		die "refusing to clear unexpected campaign stage: $STAGE_ROOT"
		;;
esac

mkdir -p "$STAGE_ROOT/fbctests" "$STAGE_ROOT/examples"
rm -f "$MANIFEST" "$GUEST_RESULTS" "$GUEST_COMPLETION"
: > "$MANIFEST"

fbctest_count=0
example_count=0

if [ "$INCLUDE_FBCTESTS" -ne 0 ]; then
	FBCTESTS_SUMMARY="$FBCTESTS_OUT/build-summary.tsv"
	[ -f "$FBCTESTS_SUMMARY" ] ||
		die "missing fbctests build summary: $FBCTESTS_SUMMARY"

	while IFS=$'\t' read -r test_id build_status data_directory; do
		[ "$test_id" != "id" ] || continue
		[ "$test_id" != "directory" ] || continue
		[ "$build_status" = "pass" ] || continue
		safe_id "$test_id" || die "unsafe fbctests id in summary: $test_id"
		[ -n "$data_directory" ] || data_directory="$test_id"
		safe_id "$data_directory" ||
			die "unsafe fbctests data directory in summary: $data_directory"

		test_program="$FBCTESTS_OUT/bin/$test_id.exe"
		[ -f "$test_program" ] ||
			die "missing successful fbctests executable: $test_program"

		cp "$test_program" "$STAGE_ROOT/fbctests/$test_id.exe"
		printf 'fbctests-%s\twince-campaign\\fbctests\\%s.exe\t%s\t--xml "\\Storage Card\\fbctests-%s.xml" --brief-summary --hide-cases\r\n' \
			"$test_id" "$test_id" "$FBCTEST_TIMEOUT" "$test_id" >> "$MANIFEST"
		stage_test_data_directory "$data_directory"
		fbctest_count=$((fbctest_count + 1))
	done < "$FBCTESTS_SUMMARY"

	[ "$fbctest_count" -gt 0 ] || die "no successful fbctests shards to stage"

	stage_test_data_directory data
	stage_test_data_directory console
	stage_test_data_directory pp
	stage_test_data_directory string
fi

if [ "$INCLUDE_EXAMPLES" -ne 0 ]; then
	EXAMPLE_MANIFEST="$EXAMPLEAGEDDON_OUT/manifest.tsv"
	[ -f "$EXAMPLE_MANIFEST" ] ||
		die "missing Exampleageddon manifest: $EXAMPLE_MANIFEST"

	while IFS=$'\t' read -r example_index case_id source_path \
	                               example_program compile_directory; do
		[ "$example_index" != "index" ] || continue
		if [ "$EXAMPLE_LIMIT" -ne 0 ] &&
		   [ "$example_count" -ge "$EXAMPLE_LIMIT" ]; then
			break
		fi

		safe_id "$case_id" || die "unsafe Exampleageddon case id: $case_id"
		[ -f "$example_program" ] ||
			die "missing Exampleageddon executable: $example_program"
		[ -d "$compile_directory" ] ||
			die "missing Exampleageddon resource directory: $compile_directory"

		case_stage="$STAGE_ROOT/examples/$case_id"
		mkdir -p "$case_stage"
		(
			cd "$compile_directory"
			tar --exclude='*.o' --exclude='*.exe' -cf - .
		) | tar -C "$case_stage" -xf -
		cp "$example_program" "$case_stage/runner.exe"

		printf '%s\twince-campaign\\examples\\%s\\runner.exe\t%s\t\r\n' \
			"$case_id" "$case_id" "$EXAMPLE_TIMEOUT" >> "$MANIFEST"
		example_count=$((example_count + 1))
	done < "$EXAMPLE_MANIFEST"

	[ "$example_count" -gt 0 ] || die "no Exampleageddon cases to stage"
fi

[ -f "$RUNNER_SOURCE" ] || die "missing guest campaign runner: $RUNNER_SOURCE"
cp "$RUNNER_SOURCE" "$RUNNER_STAGED"

total_count=$((fbctest_count + example_count))
[ "$total_count" -gt 0 ] || die "campaign manifest is empty"

if [ -z "$RUN_SECONDS" ]; then
	RUN_SECONDS=$((
		fbctest_count * FBCTEST_TIMEOUT +
		example_count * EXAMPLE_TIMEOUT +
		60
	))
fi

printf 'fbctests\t%s\nexamples\t%s\ntotal\t%s\nrun_seconds\t%s\n' \
	"$fbctest_count" "$example_count" "$total_count" "$RUN_SECONDS" \
	> "$SESSION_OUT/campaign-summary.tsv"

echo "Staged persistent Windows CE campaign:"
echo "  fbctests shards: $fbctest_count"
echo "  examples:        $example_count"
echo "  total children:  $total_count"
echo "  timeout:         $RUN_SECONDS seconds"

if [ "$RUN_TESTS" -eq 0 ]; then
	echo "Campaign staged without launching CERF"
	exit 0
fi

WINCE_CERF_ROOT="$SESSION_CERF" \
"$EMULATOR_SCRIPT" \
	--board-id "$BOARD_ID" \
	--rom "$SESSION_ROM" \
	--program "$(basename "$RUNNER_STAGED")" \
	--completion "$(basename "$GUEST_COMPLETION")" \
	--log-stem wince-mips-test-session \
	--boot-seconds "$BOOT_SECONDS" \
	--run-seconds "$RUN_SECONDS"

[ -f "$GUEST_RESULTS" ] || die "guest campaign did not produce results"
[ -f "$GUEST_COMPLETION" ] || die "guest campaign did not complete"

cp "$GUEST_RESULTS" "$SESSION_OUT/results.tsv"
cp "$GUEST_COMPLETION" "$SESSION_OUT/completion.tsv"

result_count="$(wc -l < "$GUEST_RESULTS")"
failure_count="$(awk -F '\t' '$2 != 0 { failures += 1 } END { print failures + 0 }' \
	"$GUEST_RESULTS")"

echo "Persistent Windows CE campaign completed:"
echo "  recorded: $result_count/$total_count"
echo "  failures: $failure_count"
echo "  results:  $SESSION_OUT/results.tsv"

[ "$result_count" -eq "$total_count" ] ||
	die "guest result count does not match the staged campaign"
[ "$failure_count" -eq 0 ] || exit 1

exit 0

# end of build_scripts/wince/run-mips-test-session.sh
