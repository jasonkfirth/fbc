#!/usr/bin/env bash
#
# Project: FreeBASIC Windows package tests
# ----------------------------------------
#
# File: msys2-test-freebasic.sh
#
# Purpose:
#
#   Run the normal FreeBASIC tests/ suite against a packaged MSYS2-built
#   Windows distribution.
#
# Responsibilities:
#
#   * locate the packaged Windows FreeBASIC distribution
#   * copy tests/, inc/, and test-facing sfxlib sources into isolated
#     per-architecture work directories
#   * run the fbc sanity checks
#   * build and run the fbcunit test executable
#   * run all four log-test dialect lanes
#   * report pass/fail summaries for each requested Windows architecture
#
# This file intentionally does NOT contain:
#
#   * FreeBASIC compiler or runtime fixes
#   * package building
#   * test source modifications
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

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

DIST_DIR="${DIST_DIR:-}"
WORKROOT="${WORKROOT:-$ROOT/.build-msys2/freebasic-msys2-tests}"
ARCHES="${ARCHES:-win32,win64}"
KEEP_WORK="${KEEP_WORK:-1}"

usage()
{
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic.sh [options]

Options:
  --dist-dir PATH      packaged FreeBASIC Windows distribution directory
  --workroot PATH      isolated test work root
  --arch LIST          comma-separated list: win32,win64,win32-aarch64
                       (default: win32,win64)
  --clean              remove the generated workroot after a successful run
  --keep-work          keep the generated workroot (default)
  -h, --help           show this help text

Environment overrides use DIST_DIR, WORKROOT, ARCHES, and KEEP_WORK.
EOF
}

fail()
{
	echo "ERROR: $*" >&2
	exit 1
}

msg()
{
	echo ""
	echo "==> $*"
}

to_msys_path()
{
	cygpath -au "$1"
}

default_dist_dir()
{
	local candidate

	for candidate in \
		"$ROOT/.build-msys2/dist/FreeBASIC-1.20.1-winlibs-gcc-16.1.0" \
		"$ROOT/.build-msys2/dist"/FreeBASIC-* \
		"$ROOT/out/mingw32"/FreeBASIC-*; do
		[ -d "$candidate" ] || continue
		if [ -f "$candidate/fbc32.exe" ] && [ -f "$candidate/fbc64.exe" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	return 1
}

assert_no_failed_log_tests()
{
	local lang="$1"
	local failed_log="failed-${lang}.log"

	[ -f "$failed_log" ] || fail "missing log-test failure summary: $failed_log"

	if grep -qi '^None Found$' "$failed_log"; then
		return 0
	fi

	echo "FAILED LOG - for log-tests -lang $lang"
	cat "$failed_log"
	return 1
}

stage_test_sources()
{
	local arch_work="$1"
	local tests_work="$arch_work/tests"

	rm -rf "$arch_work"
	mkdir -p "$tests_work" "$arch_work/logs"

	# A developer tree can contain ignored Windows .exe files beside tracked
	# extensionless Unix test fixtures.  MSYS treats those names as aliases
	# while copying, so a blind "cp -a" can collide and leave an incomplete
	# test tree.  Prefer Git's tracked plus non-ignored file list when it is
	# available.  Release source archives use the explicit artifact excludes.
	if command -v git >/dev/null 2>&1 &&
		git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
		git -C "$ROOT" ls-files -co --exclude-standard -z -- tests inc src/sfxlib |
			while IFS= read -r -d '' source_path; do
				[ -e "$ROOT/$source_path" ] || continue
				printf '%s\0' "$source_path"
			done |
			rsync -a --from0 --files-from=- "$ROOT/" "$arch_work/"
	else
		rsync -a --delete \
			--exclude='*.a' \
			--exclude='*.asm' \
			--exclude='*.dylib' \
			--exclude='*.exe' \
			--exclude='*.lib' \
			--exclude='*.log' \
			--exclude='*.o' \
			--exclude='*.obj' \
			--exclude='*.so' \
			--exclude='*.tmp' \
			--exclude='.fbwii-tmp/' \
			--exclude='.maketests-tmp/' \
			--exclude='fbc-tests' \
			--exclude='unit-tests.inc' \
			--exclude='unit-tests-obj.lst' \
			--exclude='log-tests-*.inc' \
			--exclude='failed-log-tests-*.inc' \
			--exclude='log-tests-*.lst' \
			--exclude='log-tests-results-*.log' \
			"$ROOT/tests/" "$tests_work/"

		rsync -a --delete "$ROOT/inc/" "$arch_work/inc/"
		rsync -a --delete "$ROOT/src/sfxlib/" "$arch_work/src/sfxlib/"
	fi
}

run_arch()
{
	local arch="$1"
	local fbc_name
	local mingw_root
	local arch_work
	local tests_work
	local logroot
	local fbc
	local cc
	local cxx
	local lang

	case "$arch" in
		win32)
			fbc_name="fbc32.exe"
			mingw_root="/mingw32"
			cc="$DIST_DIR/bin/$arch/gcc.exe"
			cxx="$DIST_DIR/bin/$arch/g++.exe"
			;;
		win64)
			fbc_name="fbc64.exe"
			mingw_root="/mingw64"
			cc="$DIST_DIR/bin/$arch/gcc.exe"
			cxx="$DIST_DIR/bin/$arch/g++.exe"
			;;
		win32-aarch64)
			fbc_name="fbcarm64.exe"
			mingw_root="/clangarm64"
			cc="$DIST_DIR/bin/$arch/clang.exe"
			cxx="$DIST_DIR/bin/$arch/clang++.exe"
			;;
		*)
			fail "unsupported architecture: $arch"
			;;
	esac

	arch_work="$WORKROOT/$arch"
	tests_work="$arch_work/tests"
	logroot="$arch_work/logs"
	fbc="$DIST_DIR/$fbc_name"

	[ -f "$fbc" ] || fail "missing packaged compiler: $fbc"
	[ -d "$DIST_DIR/bin/$arch" ] || fail "missing packaged tool directory: $DIST_DIR/bin/$arch"
	[ -x "$cc" ] || fail "missing packaged C compiler: $cc"
	[ -x "$cxx" ] || fail "missing packaged C++ compiler: $cxx"

	msg "$arch: preparing isolated tests tree"
	stage_test_sources "$arch_work"

	cd "$tests_work"

	export PATH="$DIST_DIR/bin/$arch:$DIST_DIR:$mingw_root/bin:/usr/bin:/c/Windows/System32:/c/Windows"
	export GCC="$cc"
	export CC="$cc"
	export CXX="$cxx"

	msg "$arch: compiler sanity checks"
	"$fbc" -version
	make clean FBC="$fbc" > "$logroot/clean.log" 2>&1
	make check FBC="$fbc" > "$logroot/check.log" 2>&1

	msg "$arch: building fbcunit tests"
	make unit-tests FBC="$fbc" > "$logroot/unit-tests-build.log" 2>&1

	msg "$arch: running fbcunit tests"
	./fbc-tests.exe > "$logroot/fbcunit-run.log" 2>&1

	msg "$arch: running log tests"
	make log-tests FBC="$fbc" > "$logroot/log-tests.log" 2>&1

	for lang in fb fblite qb deprecated; do
		assert_no_failed_log_tests "$lang"
	done

	echo "PASS $arch"
	cd "$ROOT"
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$(to_msys_path "$2")"
			shift 2
			;;
		--workroot)
			WORKROOT="$(to_msys_path "$2")"
			shift 2
			;;
		--arch|--arches)
			ARCHES="$2"
			shift 2
			;;
		--clean)
			KEEP_WORK=0
			shift
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
			echo "ERROR: unknown option: $1" >&2
			usage >&2
			exit 1
			;;
	esac
done

if [ -z "$DIST_DIR" ]; then
	DIST_DIR="$(default_dist_dir)" || fail "could not find a packaged FreeBASIC Windows distribution"
else
	DIST_DIR="$(to_msys_path "$DIST_DIR")"
fi

WORKROOT="$(to_msys_path "$WORKROOT")"

[ -d "$DIST_DIR" ] || fail "distribution directory does not exist: $DIST_DIR"

msg "FreeBASIC MSYS2 package test"
echo "dist:     $DIST_DIR"
echo "workroot: $WORKROOT"
echo "arches:   $ARCHES"

IFS=',' read -r -a ARCH_LIST <<< "$ARCHES"
for arch in "${ARCH_LIST[@]}"; do
	[ -n "$arch" ] || continue
	run_arch "$arch"
done

msg "FreeBASIC MSYS2 package tests passed"

if [ "$KEEP_WORK" -eq 0 ]; then
	rm -rf "$WORKROOT"
else
	echo "workroot kept for review: $WORKROOT"
fi

# end of msys2-test-freebasic.sh
