#!/bin/sh
# Project: FreeBASIC MIPS toolchain
# File: build_scripts/mips/fetch-source.sh
# Purpose: Fetch a pinned dependency despite an unavailable source mirror.
# Responsibilities: Bound transfers and verify every mirror against one digest.
# This file contains no dependency versions or build/test policy.

set -eu

if [ "$#" -lt 3 ]; then
    echo "usage: fetch-source.sh OUTPUT SHA256 URL [URL ...]" >&2
    exit 2
fi

output="$1"
digest="$2"
shift 2

# A mirror may return a partial transfer or an HTML error page. Neither may
# reach tar or replace a previously verified archive.
partial="${output}.part"
trap 'rm -f "$partial"' EXIT
trap 'exit 1' HUP INT TERM
for url in "$@"; do
    echo "Fetching dependency: $url" >&2
    # Both pinned source archives are below 4 MiB; cap even chunked responses.
    if curl -fsSL --connect-timeout 15 --max-time 90 --max-filesize 16777216 \
        --retry 2 --retry-delay 2 --retry-max-time 180 \
        "$url" -o "$partial"; then
        if printf '%s  %s\n' "$digest" "$partial" | sha256sum -c -; then
            mv -f "$partial" "$output"
            exit 0
        fi
    fi
    echo "Dependency mirror failed; trying the next source" >&2
    rm -f "$partial"
done

echo "ERROR: no source supplied the pinned dependency: $output" >&2
exit 1

# end of build_scripts/mips/fetch-source.sh
