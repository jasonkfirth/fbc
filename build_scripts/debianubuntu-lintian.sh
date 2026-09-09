#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: debianubuntu-lintian.sh
#
# Purpose:
#
#     Validate completed Debian-family package artifacts with Lintian.
#
# Responsibilities:
#
#     * find package change manifests in one output directory
#     * run the installed Lintian with strict warning handling
#     * bound validator runtime and retain a diagnostic log
#
# This file intentionally does NOT contain:
#
#     * package compilation or staging
#     * Docker or CPU-emulation policy
#     * Debian package metadata
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Helpers
##############################################################################

die() { echo "ERROR: $*" >&2; exit 1; }
need_cmd() { command -v "$1" >/dev/null 2>&1 || die "missing command: $1"; }

phase() {
    printf '==> [%s] %s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" "$*"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-lintian.sh OUTPUT_DIRECTORY

Validate every .changes file in OUTPUT_DIRECTORY with the installed Lintian.

Environment:
  FBC_PACKAGE_LINTIAN_TIMEOUT_SECONDS
                  Maximum time for each Lintian process (default: 1800)
EOF
}

##############################################################################
# Arguments and tooling
##############################################################################

if [ "$#" -eq 1 ] && { [ "$1" = "-h" ] || [ "$1" = "--help" ]; }; then
    usage
    exit 0
fi

if [ "$#" -ne 1 ]; then
    usage >&2
    exit 2
fi

OUTDIR="$1"
LINTIAN_TIMEOUT_SECONDS="${FBC_PACKAGE_LINTIAN_TIMEOUT_SECONDS:-1800}"
LINTIAN_KILL_GRACE_SECONDS=30

case "$LINTIAN_TIMEOUT_SECONDS" in
    ''|*[!0-9]*|0)
        die "FBC_PACKAGE_LINTIAN_TIMEOUT_SECONDS must be a positive integer"
        ;;
esac

[ -d "$OUTDIR" ] || die "package output directory does not exist: $OUTDIR"

need_cmd basename
need_cmd date
need_cmd grep
need_cmd lintian
need_cmd tee
need_cmd timeout

##############################################################################
# Strict package validation
##############################################################################

changes_files=()
lintian_fail_args=()
lintian_policy_rc=0

shopt -s nullglob
changes_files=("$OUTDIR"/*.changes)
shopt -u nullglob

if [ "${#changes_files[@]}" -eq 0 ]; then
    die "no .changes file was produced for Lintian in $OUTDIR"
fi

lintian_help="$(lintian --help 2>&1)" || die "could not query Lintian command-line options"

if grep -Eq -- '(^|[[:space:]])--fail-on([=[:space:]]|$)' <<< "$lintian_help"; then
    lintian_fail_args=(--fail-on "error,warning")
    lintian_policy_rc=2
elif grep -q -- '--fail-on-warnings' <<< "$lintian_help"; then
    #
    # Raspbian Buster provides Lintian 2.15. It predates --fail-on, but
    # its deprecated --fail-on-warnings option returns failure for either
    # warnings or errors. This keeps archived package rows as strict as
    # rows validated by current Lintian releases.
    #
    lintian_fail_args=(--fail-on-warnings)
    lintian_policy_rc=1
else
    die "installed Lintian cannot be configured to fail on warnings"
fi

for changes_file in "${changes_files[@]}"; do
    lintian_log="$OUTDIR/lintian-$(basename "$changes_file").log"
    lintian_rc=0

    phase "running Lintian for $(basename "$changes_file") with a ${LINTIAN_TIMEOUT_SECONDS}-second limit"

    # Give child collectors a short grace period after the main time limit.
    if timeout --foreground --kill-after="${LINTIAN_KILL_GRACE_SECONDS}s" "$LINTIAN_TIMEOUT_SECONDS" \
        lintian -IE --pedantic "${lintian_fail_args[@]}" "$changes_file" 2>&1 |
        tee "$lintian_log"
    then
        lintian_rc=0
    else
        lintian_rc=${PIPESTATUS[0]}
    fi

    if [ "$lintian_rc" -eq 124 ] || [ "$lintian_rc" -eq 137 ]; then
        die "Lintian exceeded the ${LINTIAN_TIMEOUT_SECONDS}-second limit for $(basename "$changes_file")"
    fi

    unexpected_lintian="$(
        grep -E '^[EW]:' "$lintian_log" |
            grep -Ev '^W: freebasic-dbgsym: elf-error In program headers: Unable to find program interpreter name \[usr/lib/debug/\.build-id/[[:xdigit:]]{2}/[[:xdigit:]]+\.debug\]$' ||
            true
    )"

    if [ -n "$unexpected_lintian" ]; then
        echo "ERROR: Lintian reported errors or warnings for $(basename "$changes_file")"
        printf '%s\n' "$unexpected_lintian"

        if [ "$lintian_rc" -eq 0 ]; then
            lintian_rc=1
        fi

        exit "$lintian_rc"
    fi

    if [ "$lintian_rc" -ne 0 ]; then
        #
        # Binutils debug files retain the executable's program headers while
        # objcopy removes the interpreter bytes. Jammy's Lintian reports that
        # normal dbgsym layout as an elf-error. Accept only that exact warning;
        # every other error or warning still fails.
        #
        if [ "$lintian_rc" -ne "$lintian_policy_rc" ]; then
            die "Lintian failed at runtime for $(basename "$changes_file") (exit=$lintian_rc)"
        elif grep -Eq '^W: freebasic-dbgsym: elf-error In program headers: Unable to find program interpreter name \[usr/lib/debug/\.build-id/[[:xdigit:]]{2}/[[:xdigit:]]+\.debug\]$' "$lintian_log"
        then
            echo "==> accepted Jammy Lintian's known dbgsym interpreter warning"
        else
            die "Lintian failed for $(basename "$changes_file") (exit=$lintian_rc)"
        fi
    fi

    phase "Lintian completed for $(basename "$changes_file")"
done

# end of debianubuntu-lintian.sh
