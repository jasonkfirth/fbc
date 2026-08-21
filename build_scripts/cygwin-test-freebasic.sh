#!/usr/bin/env bash
#
# Project: FreeBASIC Cygwin package validation
# --------------------------------------------
#
# File: cygwin-test-freebasic.sh
#
# Purpose:
#
#   Install and validate a completed native Cygwin package.
#
# Responsibilities:
#
#   * verify and install the selected package archive
#   * stage fbctests and examples outside synchronized source trees
#   * run compiler checks, fbcunit, log-tests, and Exampleageddon
#   * preserve logs and Exampleageddon reports with the release artifacts
#
# This file intentionally does NOT contain:
#
#   * compiler or runtime source fixes
#   * package construction or publication
#   * Cygwin dependency installation
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

SELF_DIR="$(CDPATH='' cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH='' cd -- "$SELF_DIR/.." && pwd)"

PACKAGE=""
JOBS="${JOBS:-}"
WORKROOT="${WORKROOT:-/tmp/freebasic-cygwin-package-tests}"
OUTROOT="${OUTROOT:-$ROOT/out/cygwin-tests}"

usage()
{
	cat <<EOF
Usage: ./build_scripts/cygwin-test-freebasic.sh [options]

Options:
  --package FILE       Cygwin freebasic-*.tar.xz package to install and test.
  --jobs N             Parallel jobs for fbctests and Exampleageddon.
  --workroot DIR       Disposable test workspace. Default: $WORKROOT
  --outroot DIR        Preserved logs and reports. Default: $OUTROOT
  -h, --help           Show this help text.

Environment overrides use JOBS, WORKROOT, and OUTROOT.
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

to_cygwin_path()
{
	cygpath -au "$1"
}

default_jobs()
{
	local count

	count="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
	case "$count" in
		''|*[!0-9]*|0) count=1 ;;
	esac
	printf '%s\n' "$count"
}

default_package()
{
	find "$ROOT/out/cygwin" -maxdepth 1 -type f -name 'freebasic-*.tar.xz' \
		-print 2>/dev/null | sort -V | tail -n 1
}

run_logged()
{
	local log="$1"
	shift

	printf '+ '
	printf '%q ' "$@"
	printf '\n'
	"$@" 2>&1 | tee "$log"
}

stage_source_tree()
{
	local source_root="$WORKROOT/source"

	rm -rf "$WORKROOT"
	mkdir -p "$source_root" "$OUTROOT"

	if command -v git >/dev/null 2>&1 &&
		git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
		git -C "$ROOT" ls-files -co --exclude-standard -z -- \
			tests inc src/sfxlib examples build_scripts/exampleageddon-freebasic.py |
			while IFS= read -r -d '' source_path; do
				[ -e "$ROOT/$source_path" ] || continue
				printf '%s\0' "$source_path"
			done |
			rsync -a --from0 --files-from=- "$ROOT/" "$source_root/"
	else
		rsync -a --delete \
			--exclude='*.a' \
			--exclude='*.exe' \
			--exclude='*.log' \
			--exclude='*.o' \
			--exclude='*.obj' \
			--exclude='*.tmp' \
			"$ROOT/tests/" "$source_root/tests/"
		rsync -a --delete "$ROOT/inc/" "$source_root/inc/"
		rsync -a --delete \
			--exclude='obj' \
			--exclude='*.a' \
			--exclude='*.o' \
			"$ROOT/src/sfxlib/" "$source_root/src/sfxlib/"
		rsync -a --delete "$ROOT/examples/" "$source_root/examples/"
		mkdir -p "$source_root/build_scripts"
		cp "$ROOT/build_scripts/exampleageddon-freebasic.py" "$source_root/build_scripts/"
	fi
}

install_package()
{
	[ -w /usr/bin ] || fail "package installation requires write access to /usr"
	[ -s "$PACKAGE" ] || fail "package archive is missing or empty: $PACKAGE"

	msg "Installing Cygwin package"
	tar -C / -xJf "$PACKAGE"
	[ -x /usr/bin/fbc.exe ] || fail "package did not install /usr/bin/fbc.exe"
	/usr/bin/fbc.exe -version | tee "$OUTROOT/fbc-version.log"
}

run_compiler_smoke()
{
	local source="$WORKROOT/package-smoke.bas"
	local program="$WORKROOT/package-smoke.exe"
	local output

	cat > "$source" <<'EOF'
print "FreeBASIC Cygwin package test OK"
EOF

	msg "Running installed compiler smoke test"
	/usr/bin/fbc.exe "$source" -x "$program"
	output="$("$program")"
	output="${output%$'\r'}"
	[ "$output" = "FreeBASIC Cygwin package test OK" ] || fail "installed compiler produced unexpected output"
}

assert_log_tests_passed()
{
	local failed_log

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "$failed_log" ] || fail "missing log-tests summary: $failed_log"
		if ! grep -qi '^None Found$' "$failed_log"; then
			cat "$failed_log"
			fail "log-tests reported failures in $failed_log"
		fi
	done
}

run_fbctests()
{
	local tests="$WORKROOT/source/tests"

	msg "Running fbctests with $JOBS job(s)"
	cd "$tests"
	run_logged "$OUTROOT/fbctests-clean.log" make clean FBC=/usr/bin/fbc.exe
	run_logged "$OUTROOT/fbctests-check.log" make check FBC=/usr/bin/fbc.exe
	run_logged "$OUTROOT/fbctests-unit.log" make -j "$JOBS" unit-tests \
		FBC=/usr/bin/fbc.exe UNITTEST_RUN_ARGS="${FBCTESTS_UNIT_ARGS:-}"
	run_logged "$OUTROOT/fbctests-log.log" make -j "$JOBS" log-tests FBC=/usr/bin/fbc.exe
	assert_log_tests_passed
	cd "$ROOT"
}

run_exampleageddon()
{
	local python
	local source_root="$WORKROOT/source"
	local report_root="$OUTROOT/exampleageddon"

	python="$(command -v python3 || true)"
	[ -n "$python" ] || fail "Cygwin python3 is required for Exampleageddon"
	[ "$("$python" -c 'import sys; print(sys.platform)')" = "cygwin" ] ||
		fail "python3 must be the native Cygwin interpreter"

	rm -rf "$report_root"
	mkdir -p "$report_root"

	msg "Running Exampleageddon with $JOBS job(s)"
	run_logged "$OUTROOT/exampleageddon.log" "$python" \
		"$source_root/build_scripts/exampleageddon-freebasic.py" \
		--root "$source_root" \
		--outdir "$report_root" \
		--prefix /usr \
		--include-dir "$source_root/inc" \
		--fbc /usr/bin/fbc.exe \
		--jobs "$JOBS" \
		--compile-timeout 180 \
		--run-timeout 10 \
		--fail-on-self-contained

	[ -f "$report_root/report.md" ] || fail "Exampleageddon report was not created"
	[ -f "$report_root/results.csv" ] || fail "Exampleageddon results CSV was not created"
	grep -qx -- '- Self-contained problems: 0' "$report_root/report.md" ||
		fail "Exampleageddon reported self-contained example problems"
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package) PACKAGE="$2"; shift 2 ;;
		--jobs) JOBS="$2"; shift 2 ;;
		--workroot) WORKROOT="$2"; shift 2 ;;
		--outroot) OUTROOT="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*) fail "unknown option: $1" ;;
	esac
done

case "$(uname -s)" in
	CYGWIN*) ;;
	*) fail "this script must be run inside Cygwin" ;;
esac

if [ -z "$PACKAGE" ]; then
	PACKAGE="$(default_package)"
fi
[ -n "$PACKAGE" ] || fail "could not find a Cygwin FreeBASIC package"

PACKAGE="$(to_cygwin_path "$PACKAGE")"
WORKROOT="$(to_cygwin_path "$WORKROOT")"
OUTROOT="$(to_cygwin_path "$OUTROOT")"

case "$JOBS" in
	''|*[!0-9]*|0) JOBS="$(default_jobs)" ;;
esac

export PATH=/usr/bin:/bin

stage_source_tree
install_package
run_compiler_smoke
run_fbctests
run_exampleageddon

msg "Cygwin package validation passed"
echo "Logs and reports: $OUTROOT"

# end of cygwin-test-freebasic.sh
