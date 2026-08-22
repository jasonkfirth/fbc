#!/usr/bin/env bash
#
#   Project: FreeBASIC MSYS2 package builds
#   ---------------------------------------
#
#   File: msys2-pacman-retry.sh
#
#   Purpose:
#
#       Run one pacman operation with bounded retries for transient repository
#       and mirror failures.
#
#   Responsibilities:
#
#       * validate the configured retry limits
#       * repeat failed pacman operations after a short delay
#       * preserve pacman's final exit status for the calling build script
#
#   This file intentionally does NOT contain:
#
#       * package selection policy
#       * MSYS2 repository configuration
#       * build or packaging logic
#

set -u

##############################################################################
# Configuration
##############################################################################

PACMAN_RETRY_ATTEMPTS="${PACMAN_RETRY_ATTEMPTS:-3}"
PACMAN_RETRY_DELAY="${PACMAN_RETRY_DELAY:-10}"

##############################################################################
# Validation
##############################################################################

if [ "$#" -eq 0 ]; then
	echo "ERROR: no pacman arguments were supplied." >&2
	exit 2
fi

case "$PACMAN_RETRY_ATTEMPTS" in
	''|*[!0-9]*)
		echo "ERROR: PACMAN_RETRY_ATTEMPTS must be a positive integer." >&2
		exit 2
		;;
esac

if [ "$PACMAN_RETRY_ATTEMPTS" -lt 1 ]; then
	echo "ERROR: PACMAN_RETRY_ATTEMPTS must be at least 1." >&2
	exit 2
fi

case "$PACMAN_RETRY_DELAY" in
	''|*[!0-9]*)
		echo "ERROR: PACMAN_RETRY_DELAY must be a non-negative integer." >&2
		exit 2
		;;
esac

if ! command -v pacman >/dev/null 2>&1; then
	echo "ERROR: pacman was not found in PATH." >&2
	exit 127
fi

##############################################################################
# Retried package operation
##############################################################################

attempt=1

while [ "$attempt" -le "$PACMAN_RETRY_ATTEMPTS" ]; do
	echo "==> pacman $* (attempt $attempt of $PACMAN_RETRY_ATTEMPTS)"

	if pacman "$@"; then
		exit 0
	fi

	if [ "$attempt" -ge "$PACMAN_RETRY_ATTEMPTS" ]; then
		break
	fi

	# MSYS2 mirrors occasionally time out after a package has already reached
	# the local cache.  Repeating the same transaction safely reuses that data.
	echo "WARNING: pacman failed; retrying in $PACMAN_RETRY_DELAY seconds." >&2
	sleep "$PACMAN_RETRY_DELAY"
	attempt=$((attempt + 1))
done

echo "ERROR: pacman failed after $PACMAN_RETRY_ATTEMPTS attempts." >&2
exit 1

# end of msys2-pacman-retry.sh
