#!/usr/bin/env bash
#
#   Project: FreeBASIC Wii testing
#   --------------------------------
#
#   File: msys2-test-freebasic-wii-exampleageddon.sh
#
#   Purpose:
#
#       Run exampleageddon against the packaged fbc-wii driver.
#
#   Responsibilities:
#
#       * locate the packaged FreeBASIC Wii install tree
#       * configure the devkitPro environment used by fbc-wii
#       * compile the example tree for Wii in multithreaded no-run mode
#       * fail when self-contained examples stop compiling
#
#   This file intentionally does NOT contain:
#
#       * Dolphin execution for every generated DOL
#       * installer construction
#       * test-case filtering hidden inside the shell script
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

FBVERSION="$({ sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "${ROOT_DIR}/mk/version.mk" 2>/dev/null || true; } | head -n 1)"
if [ -z "${FBVERSION}" ]; then
    FBVERSION="unknown"
fi

PACKAGE_ROOT="${PACKAGE_ROOT:-/tmp/freebasic-wii-build/dist/FreeBASIC-${FBVERSION}-fbc-wii}"
OUTDIR="${OUTDIR:-/tmp/freebasic-wii-tests/exampleageddon}"
JOBS="${JOBS:-}"
COMPILE_TIMEOUT="${COMPILE_TIMEOUT:-120}"
FAIL_ON_SELF_CONTAINED=1

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

msg()
{
    printf '\n==> %s\n' "$*"
}

fail()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

have()
{
    command -v "$1" >/dev/null 2>&1
}

run()
{
    printf '+'
    printf ' %q' "$@"
    printf '\n'
    "$@"
}

usage()
{
    cat <<EOF
Usage: $(basename "$0") [options]

Compile the FreeBASIC example tree for Wii with exampleageddon.

Options:
  --package-root DIR          FreeBASIC Wii package tree [${PACKAGE_ROOT}]
  --outdir DIR                exampleageddon output directory [${OUTDIR}]
  --jobs N                    Parallel compile jobs
  --compile-timeout SECONDS   Timeout per example compile [${COMPILE_TIMEOUT}]
  --no-fail-on-self-contained Report self-contained failures but exit 0
  -h, --help                  Show this help
EOF
}

normalize_path()
{
    local path="$1"

    if [ -n "${path}" ] && have cygpath; then
        cygpath -u "${path}" 2>/dev/null || printf '%s\n' "${path}"
    else
        printf '%s\n' "${path}"
    fi
}

max_jobs()
{
    local count

    if [ -n "${JOBS}" ]; then
        printf '%s\n' "${JOBS}"
        return 0
    fi

    count="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '2')"
    if [ -z "${count}" ] || [ "${count}" -lt 1 ] 2>/dev/null; then
        count=2
    fi

    printf '%s\n' "${count}"
}

# ---------------------------------------------------------------------------
# Argument handling
# ---------------------------------------------------------------------------

while [ "$#" -gt 0 ]; do
    case "$1" in
        --package-root)
            PACKAGE_ROOT="$(normalize_path "$2")"
            shift 2
            ;;
        --outdir)
            OUTDIR="$(normalize_path "$2")"
            shift 2
            ;;
        --jobs|-j)
            JOBS="$2"
            shift 2
            ;;
        --compile-timeout)
            COMPILE_TIMEOUT="$2"
            shift 2
            ;;
        --no-fail-on-self-contained)
            FAIL_ON_SELF_CONTAINED=0
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

if [ -z "${JOBS}" ]; then
    JOBS="$(max_jobs)"
fi

PACKAGE_ROOT="$(normalize_path "${PACKAGE_ROOT}")"
OUTDIR="$(normalize_path "${OUTDIR}")"

if [ ! -f "${PACKAGE_ROOT}/freebasic-wii-env.sh" ]; then
    fail "missing FreeBASIC Wii package tree: ${PACKAGE_ROOT}"
fi

if ! have python; then
    fail "python is required for exampleageddon"
fi

# The environment file is supplied by the package selected at runtime.
# shellcheck disable=SC1091
. "${PACKAGE_ROOT}/freebasic-wii-env.sh"

FBC_WII="${PACKAGE_ROOT}/fbc-wii-package.sh"
if [ ! -x "${FBC_WII}" ]; then
    fail "missing fbc-wii package launcher: ${FBC_WII}"
fi

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        ;;
    *)
        fail "this script is intended to run under MSYS2 on Windows"
        ;;
esac

msg "FreeBASIC Wii exampleageddon"
printf 'package : %s\n' "${PACKAGE_ROOT}"
printf 'outdir  : %s\n' "${OUTDIR}"
printf 'jobs    : %s\n' "${JOBS}"

args=(
    "${ROOT_DIR}/build_scripts/exampleageddon-freebasic.py"
    --root "${ROOT_DIR}"
    --outdir "${OUTDIR}"
    --fbc "${FBC_WII}"
    --prefix "${PACKAGE_ROOT}"
    --include-dir "${PACKAGE_ROOT}/include/freebasic-wii"
    --target-os wii
    --fbc-arg=-mt
    --jobs "${JOBS}"
    --compile-timeout "${COMPILE_TIMEOUT}"
    --no-run
)

if [ "${FAIL_ON_SELF_CONTAINED}" -ne 0 ]; then
    args+=(--fail-on-self-contained)
fi

run python "${args[@]}"

msg "Wii exampleageddon compile sweep complete"

# end of msys2-test-freebasic-wii-exampleageddon.sh
