#!/usr/bin/env bash
#
# Project: FreeBASIC Xbox package tests
# ------------------------------------
#
# File: msys2-test-freebasic-xbox-fbcunit-parallel-xemu.sh
#
# Purpose:
#
#   Run Xbox fbcunit suite-directory shards through multiple isolated xemu
#   instances.
#
# Responsibilities:
#
#   * split the packaged Xbox fbcunit object list by test directory
#   * create a separate xemu work directory for each shard
#   * give every shard its own writable HDD image and xemu.toml
#   * run several fbcunit-xemu shards concurrently
#   * read each shard's guest-written result file from its HDD image
#   * summarize pass, fail, inconclusive, and infrastructure-error shards
#
# This file intentionally does NOT contain:
#
#   * FreeBASIC compiler or runtime fixes
#   * test source modifications
#   * Xbox boot ROM, BIOS, EEPROM, or dashboard assets
#

set -euo pipefail

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/tests" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo "ERROR: could not locate the FreeBASIC project root." >&2
	exit 1
fi

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo "ERROR: this script must be run inside an MSYS2 environment." >&2
		exit 1
		;;
esac

BUILD_ROOT="${BUILD_ROOT:-/tmp/freebasic-xbox-build}"
DIST_DIR="${DIST_DIR:-${BUILD_ROOT}/dist/FreeBASIC-1.20.1-fbc-xbox}"
TEST_WORKDIR="${TEST_WORKDIR:-${ROOT}/.build-msys2/freebasic-xbox-tests/tests}"
WORKROOT="${WORKROOT:-${ROOT}/.build-msys2/freebasic-xbox-fbcunit-parallel-xemu}"
XEMU_DIR="${XEMU_DIR:-${ROOT}/xbox_emulator_xemu}"
XEMU_EXE="${XEMU_EXE:-${XEMU_DIR}/xemu.exe}"
BASE_XEMU_CONFIG="${BASE_XEMU_CONFIG:-${ROOT}/.build-msys2/freebasic-xbox-xemu-test/xemu.toml}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-120}"
JOBS="${JOBS:-4}"
KEEP_WORK="${KEEP_WORK:-0}"
SUITE_DIRS="${SUITE_DIRS:-}"
HDD_TEMPLATE="${HDD_TEMPLATE:-}"
EEPROM_TEMPLATE="${EEPROM_TEMPLATE:-}"

RUNNER_SCRIPT="${ROOT}/build_scripts/msys2-test-freebasic-xbox-fbcunit-xemu.sh"
FATX_EXTRACTOR="${ROOT}/build_scripts/extract-xbox-fatx-file.pl"
SUMMARY_LOG="${WORKROOT}/parallel-summary.log"

usage()
{
	cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --dist-dir PATH        packaged freebasic-xbox directory
  --test-workdir PATH    build directory from msys2-test-freebasic-xbox-tests.sh
  --xemu PATH            xemu.exe path
  --base-config PATH     base xemu.toml used to find ROM paths and HDD default
  --hdd-template PATH    base Xbox HDD image copied once per worker
  --eeprom-template PATH optional EEPROM image copied once per worker
  --suite-dirs LIST      comma-separated test directories to run
  --timeout SECONDS      seconds to let each guest run before reading result
  --jobs N              number of concurrent xemu workers
  --workroot PATH        parallel worker root
  --keep-work            leave generated worker directories in place
  -h, --help             show this help

Environment overrides use the same uppercase names as the option labels.
EOF
}

fail()
{
	echo "ERROR: $*" >&2
	exit 1
}

to_msys_path()
{
	cygpath -au "$1"
}

to_windows_path()
{
	cygpath -aw "$1"
}

safe_name()
{
	local text="$1"

	text="${text#./}"
	text="${text//\//__}"
	text="${text//\\/__}"
	text="${text// /_}"
	text="${text//:/_}"
	echo "$text"
}

read_toml_literal_path()
{
	local key="$1"
	local value

	value="$(
		sed -n "s/^[[:space:]]*${key}[[:space:]]*=[[:space:]]*'\\(.*\\)'[[:space:]]*$/\\1/p" \
			"$BASE_XEMU_CONFIG" | head -n 1
	)"

	[ -n "$value" ] || return 1
	printf '%s\n' "$value"
}

list_default_suites()
{
	sed -n 's#^\./\([^/][^/]*\)/.*#\1#p' "$UNIT_OBJECT_LIST" | sort -u
}

write_worker_config()
{
	local worker_dir="$1"
	local hdd_path="$2"
	local eeprom_path="$3"
	local config_path="$4"
	local bootrom_path
	local flashrom_path

	bootrom_path="$(read_toml_literal_path bootrom_path)" ||
		fail "base xemu config does not contain bootrom_path: $BASE_XEMU_CONFIG"
	flashrom_path="$(read_toml_literal_path flashrom_path)" ||
		fail "base xemu config does not contain flashrom_path: $BASE_XEMU_CONFIG"

	cat > "$config_path" <<EOF
[general]
show_welcome = false
skip_boot_anim = true

[general.updates]
check = false

[net]
enable = true
backend = 'nat'

[sys.files]
bootrom_path = '$bootrom_path'
flashrom_path = '$flashrom_path'
hdd_path = '$(to_windows_path "$hdd_path")'
EOF

	if [ -n "$eeprom_path" ]; then
		cat >> "$config_path" <<EOF
eeprom_path = '$(to_windows_path "$eeprom_path")'
EOF
	fi
}

detect_default_eeprom()
{
	local appdata_path
	local eeprom_path

	if [ -z "${APPDATA:-}" ]; then
		return 0
	fi

	appdata_path="$(to_msys_path "$APPDATA" 2>/dev/null || true)"
	[ -n "$appdata_path" ] || return 0

	eeprom_path="${appdata_path}/xemu/xemu/eeprom.bin"
	if [ -f "$eeprom_path" ]; then
		printf '%s\n' "$eeprom_path"
	fi
}

run_suite_shard()
{
	local suite_dir="$1"
	local suite_safe
	local suite_workroot
	local worker_hdd
	local worker_eeprom=""
	local worker_config
	local shard_stdout
	local shard_stderr
	local status_file
	local guest_result
	local guest_result_err
	local guest_log
	local guest_log_err
	local result

	suite_safe="$(safe_name "$suite_dir")"
	suite_workroot="${WORKROOT}/workers/${suite_safe}"
	worker_hdd="${suite_workroot}/xemu-hdd.qcow2"
	worker_config="${suite_workroot}/xemu.toml"
	shard_stdout="${WORKROOT}/logs/${suite_safe}.stdout.log"
	shard_stderr="${WORKROOT}/logs/${suite_safe}.stderr.log"
	status_file="${WORKROOT}/logs/${suite_safe}.status"
	guest_result="${WORKROOT}/logs/${suite_safe}.FBCUNIT.TXT"
	guest_result_err="${WORKROOT}/logs/${suite_safe}.fatx.stderr.log"
	guest_log="${WORKROOT}/logs/${suite_safe}.FBCUNIT.LOG"
	guest_log_err="${WORKROOT}/logs/${suite_safe}.fatx-log.stderr.log"

	rm -rf "$suite_workroot"
	mkdir -p "$suite_workroot" "$(dirname "$shard_stdout")"

	cp -f "$HDD_TEMPLATE" "$worker_hdd"

	if [ -n "$EEPROM_TEMPLATE" ]; then
		worker_eeprom="${suite_workroot}/eeprom.bin"
		cp -f "$EEPROM_TEMPLATE" "$worker_eeprom"
	fi

	write_worker_config "$suite_workroot" "$worker_hdd" "$worker_eeprom" "$worker_config"

	set +e
	WORKROOT="$suite_workroot" \
	"$RUNNER_SCRIPT" \
		--dist-dir "$DIST_DIR" \
		--test-workdir "$TEST_WORKDIR" \
		--xemu "$XEMU_EXE" \
		--config "$worker_config" \
		--suite-dirs "$suite_dir" \
		--timeout "$TIMEOUT_SECONDS" \
		--no-snapshot \
		--no-capture \
		--keep-work \
		> "$shard_stdout" 2> "$shard_stderr"
	local status=$?
	set -e

	if perl "$FATX_EXTRACTOR" "$worker_hdd" "E:/FBCUNIT.TXT" > "$guest_result" 2> "$guest_result_err"; then
		:
	else
		: > "$guest_result"
	fi

	if perl "$FATX_EXTRACTOR" "$worker_hdd" "E:/FBCUNIT.LOG" > "$guest_log" 2> "$guest_log_err"; then
		:
	else
		: > "$guest_log"
	fi

	if grep -q "FREEBASIC_XBOX_FBCUNIT_PASS" "$guest_result"; then
		result="PASS"
	elif grep -q "FREEBASIC_XBOX_FBCUNIT_FAIL" "$guest_result"; then
		result="FAIL"
	elif grep -q "FREEBASIC_XBOX_FBCUNIT_START" "$guest_result"; then
		result="INCONCLUSIVE"
	elif [ "$status" -eq 0 ]; then
		result="INCONCLUSIVE"
	else
		result="ERROR"
	fi

	printf '%s %s %s %s\n' "$result" "$status" "$suite_dir" "$suite_workroot" > "$status_file"

	if [ "$KEEP_WORK" = "0" ] && [ "$result" = "PASS" ]; then
		case "$suite_workroot" in
			"$WORKROOT"/workers/*)
				rm -rf "$suite_workroot" 2>/dev/null || true
				;;
		esac
	fi
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$2"
			shift 2
			;;
		--test-workdir)
			TEST_WORKDIR="$2"
			shift 2
			;;
		--xemu)
			XEMU_EXE="$2"
			shift 2
			;;
		--base-config)
			BASE_XEMU_CONFIG="$2"
			shift 2
			;;
		--hdd-template)
			HDD_TEMPLATE="$2"
			shift 2
			;;
		--eeprom-template)
			EEPROM_TEMPLATE="$2"
			shift 2
			;;
		--suite-dirs)
			SUITE_DIRS="$2"
			shift 2
			;;
		--timeout)
			TIMEOUT_SECONDS="$2"
			shift 2
			;;
		--jobs)
			JOBS="$2"
			shift 2
			;;
		--workroot)
			WORKROOT="$2"
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
			fail "unknown option: $1"
			;;
	esac
done

case "$TIMEOUT_SECONDS" in
	''|*[!0-9]*)
		fail "--timeout must be a positive integer"
		;;
esac

case "$JOBS" in
	''|*[!0-9]*)
		fail "--jobs must be a positive integer"
		;;
esac

[ "$JOBS" -gt 0 ] || fail "--jobs must be greater than zero"
[ "$TIMEOUT_SECONDS" -ge 15 ] || fail "--timeout must be at least 15 seconds"

DIST_DIR="$(to_msys_path "$DIST_DIR")"
TEST_WORKDIR="$(to_msys_path "$TEST_WORKDIR")"
WORKROOT="$(to_msys_path "$WORKROOT")"
XEMU_EXE="$(to_msys_path "$XEMU_EXE")"
BASE_XEMU_CONFIG="$(to_msys_path "$BASE_XEMU_CONFIG")"
UNIT_OBJECT_LIST="${TEST_WORKDIR}/unit-tests-obj.lst"
SUMMARY_LOG="${WORKROOT}/parallel-summary.log"

[ -x "$RUNNER_SCRIPT" ] || fail "missing fbcunit xemu runner: $RUNNER_SCRIPT"
[ -f "$FATX_EXTRACTOR" ] || fail "missing FATX extractor: $FATX_EXTRACTOR"
[ -x "${DIST_DIR}/fbc-xbox-package.sh" ] || fail "missing fbc-xbox wrapper in: $DIST_DIR"
[ -f "$UNIT_OBJECT_LIST" ] || fail "missing unit object list: $UNIT_OBJECT_LIST"
[ -f "$XEMU_EXE" ] || fail "missing xemu executable: $XEMU_EXE"
[ -f "$BASE_XEMU_CONFIG" ] || fail "missing base xemu config: $BASE_XEMU_CONFIG"

if [ -z "$HDD_TEMPLATE" ]; then
	HDD_TEMPLATE="$(read_toml_literal_path hdd_path || true)"
	[ -n "$HDD_TEMPLATE" ] || fail "base xemu config does not contain hdd_path: $BASE_XEMU_CONFIG"
fi

HDD_TEMPLATE="$(to_msys_path "$HDD_TEMPLATE")"
[ -f "$HDD_TEMPLATE" ] || fail "missing Xbox HDD template: $HDD_TEMPLATE"

if [ -z "$EEPROM_TEMPLATE" ]; then
	EEPROM_TEMPLATE="$(detect_default_eeprom || true)"
fi

if [ -n "$EEPROM_TEMPLATE" ]; then
	EEPROM_TEMPLATE="$(to_msys_path "$EEPROM_TEMPLATE")"
	[ -f "$EEPROM_TEMPLATE" ] || fail "missing EEPROM template: $EEPROM_TEMPLATE"
fi

if [ "$KEEP_WORK" = "0" ]; then
	rm -rf "$WORKROOT"
fi

mkdir -p "$WORKROOT/logs" "$WORKROOT/workers"
rm -f \
	"$WORKROOT"/logs/*.status \
	"$WORKROOT"/logs/*.stdout.log \
	"$WORKROOT"/logs/*.stderr.log \
	"$WORKROOT"/logs/*.FBCUNIT.TXT \
	"$WORKROOT"/logs/*.FBCUNIT.LOG \
	"$WORKROOT"/logs/*.fatx*.log
: > "$SUMMARY_LOG"

if [ -n "$SUITE_DIRS" ]; then
	IFS=',' read -r -a suites <<< "$SUITE_DIRS"
else
	mapfile -t suites < <(list_default_suites)
fi

[ "${#suites[@]}" -gt 0 ] || fail "no suite directories selected"

echo "Running ${#suites[@]} Xbox fbcunit shards with ${JOBS} workers"
echo "summary: $SUMMARY_LOG"

selected_suites=()
pids=()

for suite_dir in "${suites[@]}"; do
	suite_dir="${suite_dir#"${suite_dir%%[![:space:]]*}"}"
	suite_dir="${suite_dir%"${suite_dir##*[![:space:]]}"}"

	case "$suite_dir" in
		''|*[^A-Za-z0-9._-]*)
			fail "invalid suite directory name: $suite_dir"
			;;
	esac

	selected_suites+=( "$suite_dir" )

	while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do
		sleep 1
	done

	(
		run_suite_shard "$suite_dir"
	) &
	pids+=( "$!" )
done

wait_status=0
for pid in "${pids[@]}"; do
	if ! wait "$pid"; then
		wait_status=1
	fi
done

pass_count=0
fail_count=0
inconclusive_count=0
error_count=0

for suite_dir in "${selected_suites[@]}"; do
	suite_safe="$(safe_name "$suite_dir")"
	status_file="${WORKROOT}/logs/${suite_safe}.status"
	if [ ! -f "$status_file" ]; then
		printf 'ERROR 1 %s %s\n' "$suite_dir" "${WORKROOT}/workers/${suite_safe}" >> "$SUMMARY_LOG"
		error_count=$((error_count + 1))
		continue
	fi

	cat "$status_file" >> "$SUMMARY_LOG"
	result="$(awk '{ print $1 }' "$status_file")"
	case "$result" in
		PASS)
			pass_count=$((pass_count + 1))
			;;
		FAIL)
			fail_count=$((fail_count + 1))
			;;
		INCONCLUSIVE)
			inconclusive_count=$((inconclusive_count + 1))
			;;
		*)
			error_count=$((error_count + 1))
			;;
	esac
done

if [ "$wait_status" -ne 0 ] && [ "$error_count" -eq 0 ]; then
	printf 'ERROR 1 harness background-worker-failed %s\n' "$WORKROOT" >> "$SUMMARY_LOG"
	error_count=$((error_count + 1))
fi

echo
echo "Xbox fbcunit parallel xemu summary"
echo "passed:       $pass_count"
echo "failed:       $fail_count"
echo "inconclusive: $inconclusive_count"
echo "errors:       $error_count"
echo "log:          $SUMMARY_LOG"

if [ "$fail_count" -ne 0 ] || [ "$error_count" -ne 0 ]; then
	exit 1
fi

if [ "$inconclusive_count" -ne 0 ]; then
	exit 3
fi

exit 0

# end of msys2-test-freebasic-xbox-fbcunit-parallel-xemu.sh
