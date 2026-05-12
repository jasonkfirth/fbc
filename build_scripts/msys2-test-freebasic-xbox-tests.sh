#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-test-freebasic-xbox-tests.sh
#
# Build the FreeBASIC tests/ tree with the packaged fbc-xbox compiler.
#
# Responsibilities:
#   - locate a packaged fbc-xbox distribution
#   - copy tests/ into an isolated work directory
#   - compile every log-test source in every dialect
#   - link the compile-and-run log tests as XBEs without host execution
#   - build the aggregate fbcunit test XBE
#   - record a concise pass/fail report
#
# This file intentionally does NOT contain:
#   - the fbc-xbox package build itself
#   - test source modifications
#   - per-test xemu execution
##############################################################################

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/tests" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo ""
	echo "ERROR: could not locate the FreeBASIC project root."
	exit 1
fi

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo ""
		echo "ERROR: this script must be run inside an MSYS2 environment."
		exit 1
		;;
esac

##############################################################################
# Options
##############################################################################

DIST_DIR=""
WORKROOT="${WORKROOT:-$ROOT/.build-msys2/freebasic-xbox-tests}"
LANGS="fb,fblite,qb,deprecated"
KEEP_WORKROOT=0

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic-xbox-tests.sh [options]

Options:
  --dist-dir DIR       FreeBASIC fbc-xbox distribution directory
  --workroot DIR       Test work directory
  --langs LIST         Comma-separated dialect list
                       (default: fb,fblite,qb,deprecated)
  --keep-workroot      Keep the generated test directory
  --help               Show this help text

The script performs Xbox compile/link coverage for the normal tests/ harness.
Runtime execution is intentionally left to the xemu smoke and focused runtime
scripts because the normal test harness expects host executables.
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$2"
			shift 2
			;;
		--workroot)
			WORKROOT="$2"
			shift 2
			;;
		--langs)
			LANGS="$2"
			shift 2
			;;
		--keep-workroot)
			KEEP_WORKROOT=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "ERROR: unknown option: $1" >&2
			usage >&2
			exit 1
			;;
	esac
done

##############################################################################
# Helpers
##############################################################################

msg() {
	echo ""
	echo "==> $1"
}

fail() {
	echo ""
	echo "ERROR: $1" >&2
	exit 1
}

have() {
	command -v "$1" >/dev/null 2>&1
}

find_mingw32_gxx() {
	local candidate

	for candidate in \
		"${MINGW32_CXX:-}" \
		"/mingw32/bin/g++" \
		"/mingw32/bin/i686-w64-mingw32-g++" \
		"i686-w64-mingw32-g++" \
		"g++"; do
		[ -n "$candidate" ] || continue
		if command -v "$candidate" >/dev/null 2>&1; then
			command -v "$candidate"
			return 0
		fi
	done

	return 1
}

first_existing_dir() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -d "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

safe_name() {
	local text="$1"

	text="${text#./}"
	text="${text//\//__}"
	text="${text//\\/__}"
	text="${text// /_}"
	text="${text//:/_}"
	echo "$text"
}

record_pass() {
	local mode="$1"
	local src="$2"

	PASS_COUNT=$((PASS_COUNT + 1))
	echo "PASS $mode $src" >> "$SUMMARY_LOG"
}

record_fail() {
	local mode="$1"
	local src="$2"
	local log="$3"

	FAIL_COUNT=$((FAIL_COUNT + 1))
	echo "FAIL $mode $src $log" >> "$SUMMARY_LOG"
}

run_logged() {
	local log="$1"
	shift

	"$@" > "$log" 2>&1
}

list_tests() {
	local inc="$1"
	local key="$2"

	awk -v key="$key" '$1 == key && $2 == "+=" { print $3 }' "$inc"
}

fbc_flags_for_lang() {
	local lang="$1"

	printf '%s\n' -w 3 -Wc -Wno-tautological-compare -lang "$lang"
}

compile_only_ok() {
	local lang="$1"
	local src="$2"
	local safe
	local log
	local obj

	safe="$(safe_name "$lang-COMPILE_ONLY_OK-$src")"
	log="$LOG_DIR/$safe.log"
	obj="$OBJ_DIR/${safe%.bas}.o"

	TEST_COUNT=$((TEST_COUNT + 1))
	mkdir -p "$(dirname "$obj")"

	if run_logged "$log" "$FBC_XBOX" $(fbc_flags_for_lang "$lang") -c "$src" -o "$obj"; then
		record_pass "COMPILE_ONLY_OK" "$src"
	else
		record_fail "COMPILE_ONLY_OK" "$src" "$log"
	fi
}

compile_only_fail() {
	local lang="$1"
	local src="$2"
	local safe
	local log
	local obj
	local status

	safe="$(safe_name "$lang-COMPILE_ONLY_FAIL-$src")"
	log="$LOG_DIR/$safe.log"
	obj="$OBJ_DIR/${safe%.bas}.o"

	TEST_COUNT=$((TEST_COUNT + 1))
	mkdir -p "$(dirname "$obj")"

	set +e
	"$FBC_XBOX" $(fbc_flags_for_lang "$lang") -c "$src" -o "$obj" > "$log" 2>&1
	status=$?
	set -e

	if [ "$status" -eq 1 ]; then
		record_pass "COMPILE_ONLY_FAIL" "$src"
	else
		echo "unexpected compiler status: $status" >> "$log"
		record_fail "COMPILE_ONLY_FAIL" "$src" "$log"
	fi
}

build_single_module() {
	local lang="$1"
	local mode="$2"
	local src="$3"
	local run_app="$4"
	local safe
	local log

	safe="$(safe_name "$lang-$mode-$src")"
	log="$LOG_DIR/$safe.log"

	TEST_COUNT=$((TEST_COUNT + 1))

	if run_logged "$log" \
		make -f bmk-make.mk \
			FILE="$src" \
			TEST_MODE="$mode" \
			FBC="$FBC_XBOX" \
			CC="$NXDK_CC" \
			CXX="$TEST_CXX" \
			FB_LANG="$lang" \
			TARGET=xbox \
			TARGET_EXEEXT=.xbe \
			RUN_APP="$run_app"; then
		record_pass "$mode" "$src"
	else
		record_fail "$mode" "$src" "$log"
	fi
}

build_multi_module() {
	local lang="$1"
	local mode="$2"
	local bmk="$3"
	local run_app="$4"
	local safe
	local log

	safe="$(safe_name "$lang-$mode-$bmk")"
	log="$LOG_DIR/$safe.log"

	TEST_COUNT=$((TEST_COUNT + 1))

	if run_logged "$log" \
		make -f bmk-make.mk \
			BMK="$bmk" \
			TEST_MODE="$mode" \
			FBC="$FBC_XBOX" \
			CC="$NXDK_CC" \
			CXX="$TEST_CXX" \
			FB_LANG="$lang" \
			TARGET=xbox \
			TARGET_EXEEXT=.xbe \
			RUN_APP="$run_app"; then
		record_pass "$mode" "$bmk"
	else
		record_fail "$mode" "$bmk" "$log"
	fi
}

run_log_tests_for_lang() {
	local lang="$1"
	local inc="log-tests-$lang.inc"
	local src

	[ -f "$inc" ] || fail "missing generated log-test list: $TEST_WORKDIR/$inc"

	msg "running Xbox compile/link log tests for -lang $lang"

	while IFS= read -r src; do
		[ -n "$src" ] || continue
		compile_only_ok "$lang" "$src"
	done < <(list_tests "$inc" SRCLIST_COMPILE_ONLY_OK)

	while IFS= read -r src; do
		[ -n "$src" ] || continue
		compile_only_fail "$lang" "$src"
	done < <(list_tests "$inc" SRCLIST_COMPILE_ONLY_FAIL)

	while IFS= read -r src; do
		[ -n "$src" ] || continue
		build_single_module "$lang" COMPILE_AND_RUN_OK "$src" true
	done < <(list_tests "$inc" SRCLIST_COMPILE_AND_RUN_OK)

	while IFS= read -r src; do
		[ -n "$src" ] || continue
		build_single_module "$lang" COMPILE_AND_RUN_FAIL "$src" false
	done < <(list_tests "$inc" SRCLIST_COMPILE_AND_RUN_FAIL)

	while IFS= read -r src; do
		[ -n "$src" ] || continue
		build_multi_module "$lang" MULTI_MODULE_OK "$src" true
	done < <(list_tests "$inc" SRCLIST_MULTI_MODULE_OK)

	while IFS= read -r src; do
		[ -n "$src" ] || continue
		build_multi_module "$lang" MULTI_MODULE_FAIL "$src" false
	done < <(list_tests "$inc" SRCLIST_MULTI_MODULE_FAIL)
}

build_unit_tests() {
	local log="$LOG_DIR/unit-tests-build.log"

	msg "building aggregate fbcunit Xbox test XBE"
	TEST_COUNT=$((TEST_COUNT + 1))

	if run_logged "$log" \
		make -f unit-tests.mk \
			make_fbcunit \
			build_tests \
			FBC="$FBC_XBOX" \
			TARGET=xbox \
			TARGET_OS=xbox \
			TARGET_EXEEXT=.xbe \
			EXEEXT=.xbe \
			FBCU_LIBS="$TEST_WORKDIR/fbcunit/lib/libfbcunit.a"; then
		record_pass "FBCUNIT_BUILD" "./fbc-tests.bas"
	else
		record_fail "FBCUNIT_BUILD" "./fbc-tests.bas" "$log"
	fi
}

##############################################################################
# Distribution and worktree setup
##############################################################################

if [ -z "$DIST_DIR" ]; then
	DIST_DIR="$(first_existing_dir \
		"/tmp/freebasic-xbox-build/dist/FreeBASIC-1.20.1-fbc-xbox" \
		"$ROOT/out/mingw32-xbox/FreeBASIC-1.20.1-fbc-xbox" \
		"/c/freebasic-xbox" \
		"/c/FreeBASIC-xbox" \
		|| true)"
fi

[ -n "$DIST_DIR" ] || fail "could not locate fbc-xbox distribution; pass --dist-dir"
DIST_DIR="$(CDPATH= cd -- "$DIST_DIR" && pwd)"
FBC_XBOX="$DIST_DIR/fbc-xbox-package.sh"
NXDK_CC="$DIST_DIR/nxdk/bin/nxdk-cc"
NXDK_CXX="$DIST_DIR/nxdk/bin/nxdk-cxx"
MINGW32_GXX="$(find_mingw32_gxx || true)"
TEST_CXX="$NXDK_CXX"

[ -f "$FBC_XBOX" ] || fail "missing fbc-xbox-package.sh in $DIST_DIR"
[ -f "$DIST_DIR/bin/fbc.exe" ] || fail "missing bin/fbc.exe in $DIST_DIR"
[ -f "$NXDK_CC" ] || fail "missing nxdk-cc in $DIST_DIR"
[ -f "$NXDK_CXX" ] || fail "missing nxdk-cxx in $DIST_DIR"
have make || fail "make was not found"
have awk || fail "awk was not found"

[ -n "$MINGW32_GXX" ] || fail "32-bit MinGW g++ was not found; set MINGW32_CXX or install mingw-w64-i686-gcc"

export PATH="$(dirname "$MINGW32_GXX"):$PATH"
case "$("$MINGW32_GXX" -dumpmachine 2>/dev/null || true)" in
	i?86-*-mingw*) ;;
	*)
		fail "$MINGW32_GXX is not a 32-bit MinGW g++"
		;;
esac

TEST_CXX="$MINGW32_GXX"

rm -rf "$WORKROOT"
mkdir -p "$WORKROOT"

TEST_WORKDIR="$WORKROOT/tests"
LOG_DIR="$WORKROOT/logs"
OBJ_DIR="$WORKROOT/obj"
SUMMARY_LOG="$WORKROOT/xbox-tests-summary.log"

mkdir -p "$TEST_WORKDIR" "$LOG_DIR" "$OBJ_DIR"

msg "copying tests/ into isolated work directory"
cp -a "$ROOT/tests/." "$TEST_WORKDIR/"

cd "$TEST_WORKDIR"

find . -type f \( \
	-name '*.o' -o \
	-name '*.exe' -o \
	-name '*.xbe' -o \
	-name '*.log' -o \
	-name '*.a' -o \
	-name 'failed-*.inc' -o \
	-name 'failed-*.log' -o \
	-name 'log-tests-results-*.log' -o \
	-name 'unit-tests-obj.lst' -o \
	-name 'unit-tests.inc' \
	\) -delete

: > "$SUMMARY_LOG"
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

##############################################################################
# Test execution
##############################################################################

IFS=',' read -r -a LANG_LIST <<< "$LANGS"
for lang in "${LANG_LIST[@]}"; do
	[ -n "$lang" ] || continue
	run_log_tests_for_lang "$lang"
done

build_unit_tests

##############################################################################
# Report
##############################################################################

msg "Xbox test-suite compile/link summary"
echo "tests:  $TEST_COUNT"
echo "passed: $PASS_COUNT"
echo "failed: $FAIL_COUNT"
echo "log:    $SUMMARY_LOG"

if [ "$FAIL_COUNT" -ne 0 ]; then
	echo ""
	echo "First failures:"
	grep '^FAIL ' "$SUMMARY_LOG" | head -40
	echo ""
	echo "Full logs are under: $LOG_DIR"
	exit 1
fi

if [ "$KEEP_WORKROOT" -eq 0 ]; then
	echo "workroot kept for review: $WORKROOT"
else
	echo "workroot kept: $WORKROOT"
fi

echo "freebasic-xbox tests/ compile-link pass completed"

# end of msys2-test-freebasic-xbox-tests.sh
