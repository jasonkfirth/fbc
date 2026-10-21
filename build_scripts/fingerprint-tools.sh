#!/bin/sh
# Project: FreeBASIC build system
# File: fingerprint-tools.sh
# Purpose: Detect replacement of tools even when their command paths are stable.
# Responsibilities: Hash selected executables and command-wrapper files as data.
# This file intentionally does NOT execute a tool or write a fingerprint cache.

set -eu
for entry do
    name=${entry%%=*}
    command=${entry#*=}
    # Try the complete path first, including spaces. For a wrapper command,
    # inspect its words without evaluating shell syntax or expanding globs.
    signature=$(
        printf '%s\n' "$command"
        set -f
        if [ -f "$command" ]; then
            cksum "$command"
        else
            for word in $command; do
                # Older /bin/sh parsers need the opening pattern parenthesis
                # inside a command substitution to pair the closing one.
                case "$word" in
                    (-*|*=*) continue ;;
                esac
                path=$(command -v "$word" 2>/dev/null || true)
                if [ -n "$path" ] && [ -f "$path" ]; then
                    cksum "$path"
                elif [ -f "$word" ]; then
                    cksum "$word"
                fi
            done
        fi
    )
    # Keep the output Make-word safe, even if an executable path has spaces.
    checksum=$(printf '%s\n' "$signature" | cksum)
    set -- $checksum
    printf '%s:%s:%s\n' "$name" "$1" "$2"
done

# end of fingerprint-tools.sh
