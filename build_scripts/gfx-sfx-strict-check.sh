#!/usr/bin/env bash

##############################################################################
# FreeBASIC rtlib/gfxlib2/sfxlib strict diagnostics runner
##############################################################################
#
# Purpose:
#
#   Build the core rtlib, gfxlib2, and sfxlib runtime libraries with a
#   warning-heavy diagnostics profile.
#
# Responsibilities:
#
#   * build only the core, graphics, and sound runtime libraries
#   * isolate strict-check objects from normal build objects
#   * prefer clang when it is available, but allow gcc or explicit CC/CXX
#   * make unsupported warning options harmless across compiler families
#   * keep long-standing FreeBASIC GNU C extension usage out of the default
#     failure set while still allowing a heavier --paranoid audit
#   * optionally run source linters when they are installed
#
# This script intentionally does NOT contain:
#
#   * package building
#   * VM or device smoke testing
#   * broad static-analysis replacement for third-party bundled decoders
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
TARGET_TRIPLET=""
STRICT_KEY=""
USE_WERROR=1
USE_PARANOID=0
RUN_LINTERS=1
RUN_LINTER_STYLE=0
RUN_CPPCHECK_RECOMMENDED_ADDONS=0
RUN_CPPCHECK_EXHAUSTIVE=0
RUN_CPPCHECK_INCONCLUSIVE=0
CPPCHECK_BUILD_DIR=""
CPPCHECK_PLATFORM=""
CPPCHECK_ADDONS=()
KEEP_OBJECTS=0
CC_NAME="${CC:-}"
CXX_NAME="${CXX:-}"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/gfx-sfx-strict-check.sh [options]

Options:
  --target TRIPLET        Target triplet to pass to make.
  --jobs N                Parallel make jobs. Default: host CPU count.
  --cc PATH               C compiler. Default: clang, then gcc, then cc.
  --cxx PATH              C++ compiler. Default: clang++, then g++, then c++.
  --strict-key NAME       Object/library key. Default: strict-<target>.
  --no-werror             Do not promote warnings to errors.
  --paranoid              Add conversion/sign-conversion style warnings.
  --lint-style            Include style-only cppcheck findings.
  --cppcheck-addon NAME   Run a cppcheck addon. Can be repeated.
  --cppcheck-recommended-addons
                           Run defect-oriented cppcheck addons: misc,
                           threadsafety, and y2038.
  --cppcheck-exhaustive   Use cppcheck --check-level=exhaustive.
  --cppcheck-inconclusive Include cppcheck inconclusive findings.
  --cppcheck-build-dir DIR
                           Use a cppcheck analysis/cache directory. Addon
                           runs default to out/cppcheck/rt-gfx-sfx-<key>.
  --cppcheck-platform NAME
                           Pass cppcheck --platform=NAME.
  --no-linters            Skip optional cppcheck/clang-tidy probes.
  --keep-objects          Leave strict-check object directories in place.
  -h, --help              Show this help.

The default profile is intended for day-to-day cleanup.  --paranoid is useful
when auditing a smaller patch or when you are ready to chase noisier findings.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--target) TARGET_TRIPLET="$2"; shift 2 ;;
		--jobs) JOBS="$2"; shift 2 ;;
		--cc) CC_NAME="$2"; shift 2 ;;
		--cxx) CXX_NAME="$2"; shift 2 ;;
		--strict-key) STRICT_KEY="$2"; shift 2 ;;
		--no-werror) USE_WERROR=0; shift ;;
		--paranoid) USE_PARANOID=1; shift ;;
		--lint-style) RUN_LINTER_STYLE=1; shift ;;
		--cppcheck-addon) CPPCHECK_ADDONS+=("$2"); shift 2 ;;
		--cppcheck-recommended-addons) RUN_CPPCHECK_RECOMMENDED_ADDONS=1; shift ;;
		--cppcheck-exhaustive) RUN_CPPCHECK_EXHAUSTIVE=1; shift ;;
		--cppcheck-inconclusive) RUN_CPPCHECK_INCONCLUSIVE=1; shift ;;
		--cppcheck-build-dir) CPPCHECK_BUILD_DIR="$2"; shift 2 ;;
		--cppcheck-platform) CPPCHECK_PLATFORM="$2"; shift 2 ;;
		--no-linters) RUN_LINTERS=0; shift ;;
		--keep-objects) KEEP_OBJECTS=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

case "$JOBS" in ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;; esac

find_tool() {
	local tool

	for tool in "$@"; do
		if command -v "$tool" >/dev/null 2>&1; then
			command -v "$tool"
			return 0
		fi
	done

	return 1
}

if [ -z "$CC_NAME" ]; then
	CC_NAME="$(find_tool clang gcc cc)" || die "no C compiler found"
fi

if [ -z "$CXX_NAME" ]; then
	CXX_NAME="$(find_tool clang++ g++ c++)" || die "no C++ compiler found"
fi

compiler_accepts_flag() {
	local compiler="$1"
	local language="$2"
	local flag="$3"
	local suffix

	case "$language" in
		c) suffix=c ;;
		c++) suffix=cpp ;;
		*) die "internal error: unsupported language $language" ;;
	esac

	printf 'int main(void) { return 0; }\n' |
		"$compiler" -x "$language" "$flag" -c -o "/tmp/fbc-strict-flag-test.$suffix.o" - \
		>/dev/null 2>&1
}

append_supported_flags() {
	local compiler="$1"
	local language="$2"
	shift 2

	local flag

	for flag in "$@"; do
		if compiler_accepts_flag "$compiler" "$language" "$flag"; then
			printf '%s\n' "$flag"
		fi
	done
}

target_key() {
	if [ -n "$STRICT_KEY" ]; then
		printf '%s\n' "$STRICT_KEY"
		return 0
	fi

	if [ -n "$TARGET_TRIPLET" ]; then
		printf 'strict-%s\n' "$TARGET_TRIPLET" | tr -c 'A-Za-z0-9_.-' '_'
	else
		printf 'strict-host\n'
	fi
}

target_platform() {
	case "$TARGET_TRIPLET" in
		*android*) printf 'android\n' ;;
		*dragonfly*) printf 'dragonfly\n' ;;
		*freebsd*) printf 'freebsd\n' ;;
		*haiku*) printf 'haiku\n' ;;
		*netbsd*) printf 'netbsd\n' ;;
		*openbsd*) printf 'openbsd\n' ;;
		*darwin*) printf 'darwin\n' ;;
		*mingw*|*windows*) printf 'win32\n' ;;
		*xbox*) printf 'xbox\n' ;;
		*js*|*emscripten*) printf 'js\n' ;;
		*linux*|'')
			case "$(uname -s)" in
				DragonFly) printf 'dragonfly\n' ;;
				FreeBSD) printf 'freebsd\n' ;;
				Haiku) printf 'haiku\n' ;;
				NetBSD) printf 'netbsd\n' ;;
				OpenBSD) printf 'openbsd\n' ;;
				Darwin) printf 'darwin\n' ;;
				*) printf 'linux\n' ;;
			esac
			;;
		*) printf 'linux\n' ;;
	esac
}

collect_lint_paths() {
	local platform="$1"

	find \
		"$ROOT/src/rtlib" \
		"$ROOT/src/rtlib/unix" \
		"$ROOT/src/rtlib/$platform" \
		"$ROOT/src/gfxlib2" \
		"$ROOT/src/gfxlib2/unix" \
		"$ROOT/src/gfxlib2/$platform" \
		"$ROOT/src/sfxlib" \
		"$ROOT/src/sfxlib/unix" \
		"$ROOT/src/sfxlib/$platform" \
		-maxdepth 1 -type f \( -name '*.c' -o -name '*.cpp' \) 2>/dev/null |
	sort -u
}

build_flags() {
	local compiler="$1"
	local language="$2"
	local std="$3"
	local flags
	local extra

	flags="$(append_supported_flags "$compiler" "$language" \
		-O2 \
		"$std" \
		-Wall \
		-Wextra \
		-Wformat=2 \
		-Wformat-security \
		-Wundef \
		-Wwrite-strings \
		-Wredundant-decls \
		-Wnull-dereference \
		-Wno-unused-parameter)"

	if [ "$language" = "c" ]; then
		extra="$(append_supported_flags "$compiler" "$language" \
			-Wstrict-prototypes \
			-Wnested-externs \
			-Wold-style-definition)"
		flags="$flags
$extra"
	else
		extra="$(append_supported_flags "$compiler" "$language" \
			-Wold-style-cast \
			-Woverloaded-virtual \
			-Wnon-virtual-dtor)"
		flags="$flags
$extra"
	fi

	if [ "$USE_PARANOID" -ne 0 ]; then
		extra="$(append_supported_flags "$compiler" "$language" \
			-Wpedantic \
			-Wpointer-arith \
			-Wcast-align \
			-Wshadow \
			-Wmissing-declarations \
			-Wconversion \
			-Wsign-conversion \
			-Wdouble-promotion \
			-Wfloat-equal)"
		if [ "$language" = "c" ]; then
			extra="$extra $(append_supported_flags "$compiler" "$language" \
				-Wbad-function-cast \
				-Wmissing-prototypes)"
		fi
		flags="$flags
$extra"
	fi

	if [ "$USE_WERROR" -ne 0 ]; then
		extra="$(append_supported_flags "$compiler" "$language" -Werror)"
		flags="$flags
$extra"
	fi

	printf '%s\n' "$flags" | awk 'NF { printf "%s%s", sep, $0; sep = " " }'
}

run_linters() {
	if [ "$RUN_LINTERS" -eq 0 ]; then
		return 0
	fi

	local cppcheck_enable
	local -a cppcheck_args
	local cppcheck_build_dir
	local -a lint_paths
	local addon
	local platform
	local file
	local output

	platform="$(target_platform)"
	mapfile -t lint_paths < <(collect_lint_paths "$platform")

	cppcheck_enable="warning,performance,portability"
	if [ "$RUN_LINTER_STYLE" -ne 0 ]; then
		cppcheck_enable="$cppcheck_enable,style"
	fi

	if command -v cppcheck >/dev/null 2>&1; then
		msg "Running cppcheck over rtlib, gfxlib2, and sfxlib"
		cppcheck_args=(
			--enable="$cppcheck_enable"
			--error-exitcode=1
			--inline-suppr
			--quiet
			-j "$JOBS"
			--suppress=normalCheckLevelMaxBranches
			--suppress=toomanyconfigs
			--suppress=allocaCalled
			--suppress='nullPointer:*/src/rtlib/fb_unicode.h'
			--suppress='syntaxError:*/src/rtlib/profile_cycles.c'
			--suppress='subtractPointers:*/src/gfxlib2/dos/gfx_dos.c'
			--suppress='syntaxError:*/src/gfxlib2/gfx_xpad.c'
			--suppress='*:*/src/sfxlib/third_party/*'
			-I "$ROOT/src"
			-I "$ROOT/src/gfxlib2"
			-I "$ROOT/src/sfxlib"
			-I "$ROOT/src/rtlib"
			-i "$ROOT/src/rtlib/obj"
			-i "$ROOT/src/gfxlib2/obj"
			-i "$ROOT/src/gfxlib2/js"
			-i "$ROOT/src/sfxlib/obj"
			-i "$ROOT/src/sfxlib/js"
			-i "$ROOT/src/sfxlib/third_party"
		)

		if [ "$RUN_CPPCHECK_RECOMMENDED_ADDONS" -ne 0 ]; then
			CPPCHECK_ADDONS+=(misc threadsafety y2038)
		fi

		cppcheck_build_dir="$CPPCHECK_BUILD_DIR"
		if [ "${#CPPCHECK_ADDONS[@]}" -gt 0 ] && [ -z "$cppcheck_build_dir" ]; then
			cppcheck_build_dir="$ROOT/out/cppcheck/rt-gfx-sfx-$(target_key)"
		fi

		if [ "$RUN_CPPCHECK_EXHAUSTIVE" -ne 0 ]; then
			cppcheck_args+=(--check-level=exhaustive)
		fi

		if [ "$RUN_CPPCHECK_INCONCLUSIVE" -ne 0 ]; then
			cppcheck_args+=(--inconclusive)
		fi

		if [ -n "$cppcheck_build_dir" ]; then
			mkdir -p "$cppcheck_build_dir"
			cppcheck_args+=(--cppcheck-build-dir="$cppcheck_build_dir")
		fi

		if [ -n "$CPPCHECK_PLATFORM" ]; then
			cppcheck_args+=(--platform="$CPPCHECK_PLATFORM")
		fi

		for addon in "${CPPCHECK_ADDONS[@]}"; do
			cppcheck_args+=(--addon="$addon")
		done

		cppcheck \
			"${cppcheck_args[@]}" \
			"${lint_paths[@]}"
	else
		warn "cppcheck not found; skipping optional source linter"
	fi

	if command -v clang-tidy >/dev/null 2>&1; then
		msg "Running clang-tidy over rtlib, gfxlib2, and sfxlib source set for $platform"

		while IFS= read -r file; do
			case "$file" in
				"$ROOT/src/sfxlib/sfx_decode.c")
					continue
					;;
			esac

			case "$file" in
				*.cpp)
					output="$(clang-tidy --quiet "$file" \
						--checks='clang-diagnostic-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,-clang-analyzer-security.FloatLoopCounter,bugprone-*,-bugprone-assignment-in-if-condition,-bugprone-branch-clone,-bugprone-easily-swappable-parameters,-bugprone-reserved-identifier,-bugprone-switch-missing-default-case,-bugprone-implicit-widening-of-multiplication-result,-bugprone-narrowing-conversions,-bugprone-macro-parentheses,performance-*,-performance-no-int-to-ptr,portability-*' \
						--warnings-as-errors='clang-diagnostic-*,clang-analyzer-*,bugprone-*,performance-*,portability-*' \
						-- -std=gnu++17 -I "$ROOT/src" -I "$ROOT/src/gfxlib2" -I "$ROOT/src/sfxlib" -I "$ROOT/src/rtlib" -I "$ROOT/inc" 2>&1)" ||
					{
						printf '%s\n' "$output"
						return 1
					}
					;;
				*.c)
					output="$(clang-tidy --quiet "$file" \
						--checks='clang-diagnostic-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,-clang-analyzer-security.FloatLoopCounter,bugprone-*,-bugprone-assignment-in-if-condition,-bugprone-branch-clone,-bugprone-easily-swappable-parameters,-bugprone-reserved-identifier,-bugprone-switch-missing-default-case,-bugprone-implicit-widening-of-multiplication-result,-bugprone-narrowing-conversions,-bugprone-macro-parentheses,performance-*,-performance-no-int-to-ptr,portability-*' \
						--warnings-as-errors='clang-diagnostic-*,clang-analyzer-*,bugprone-*,performance-*,portability-*' \
						-- -std=gnu11 -I "$ROOT/src" -I "$ROOT/src/gfxlib2" -I "$ROOT/src/sfxlib" -I "$ROOT/src/rtlib" -I "$ROOT/inc" 2>&1)" ||
					{
						printf '%s\n' "$output"
						return 1
					}
					;;
			esac

			if [ -n "$output" ] &&
				printf '%s\n' "$output" | grep -vE '^[0-9]+ warnings? generated\.$' | grep -q .; then
				printf '%s\n' "$output"
			fi
		done < <(printf '%s\n' "${lint_paths[@]}")
	else
		warn "clang-tidy not found; skipping optional clang-tidy pass"
	fi
}

cleanup_objects() {
	local key="$1"

	rm -rf \
		"$ROOT/src/rtlib/obj/$key" \
		"$ROOT/src/gfxlib2/obj/$key" \
		"$ROOT/src/sfxlib/obj/$key" \
		"$ROOT/lib/freebasic/$key" \
		"$ROOT/lib/$key"
}

main() {
	local key
	local cflags
	local cxxflags
	local make_args

	key="$(target_key)"
	cflags="$(build_flags "$CC_NAME" c -std=gnu11)"
	cxxflags="$(build_flags "$CXX_NAME" c++ -std=gnu++17)"

	msg "Strict diagnostics key: $key"
	msg "C compiler: $CC_NAME"
	msg "C++ compiler: $CXX_NAME"

	cleanup_objects "$key"
	run_linters

	make_args=(
		-j "$JOBS"
		"FBTARGET_DIR_OVERRIDE=$key"
		"CC=$CC_NAME"
		"CXX=$CXX_NAME"
		"CFLAGS=$cflags"
		"CXXFLAGS=$cxxflags"
	)

	if [ -n "$TARGET_TRIPLET" ]; then
		make_args+=("TARGET_TRIPLET=$TARGET_TRIPLET")
	fi

	msg "Building rtlib, gfxlib2, and sfxlib with strict diagnostics"
	(
		cd "$ROOT"
		make "${make_args[@]}" rtlib gfxlib2 sfxlib
	)

	if [ "$KEEP_OBJECTS" -eq 0 ]; then
		cleanup_objects "$key"
	fi

	msg "rtlib/gfxlib2/sfxlib strict diagnostics completed"
}

main "$@"

##############################################################################
# end of gfx-sfx-strict-check.sh
##############################################################################
