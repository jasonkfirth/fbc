#!/usr/bin/env bash

##############################################################################
# GitHub Actions Ubuntu package-index preparation
##############################################################################
#
# Purpose:
#
#   Prepare the hosted Ubuntu runner's APT sources for FreeBASIC CI jobs.
#
# Responsibilities:
#
#   * disable the runner image's unrelated Google Chrome package source
#   * update the remaining package indexes with bounded network retries
#
# This script intentionally does NOT contain:
#
#   * package installation commands
#   * target sysroot or foreign-architecture source configuration
#   * changes to Ubuntu's distribution package sources
#
##############################################################################

set -euo pipefail

readonly sources_dir="/etc/apt/sources.list.d"
readonly disabled_dir="${sources_dir}/fbc-ci-disabled"

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: github-actions-apt-update.sh must run as root" >&2
    exit 2
fi

for source_file in "${sources_dir}"/*.list "${sources_dir}"/*.sources; do
    [ -f "$source_file" ] || continue
    if grep -Fq 'dl.google.com/linux/chrome' "$source_file"; then
        mkdir -p "$disabled_dir"
        mv -- "$source_file" "$disabled_dir/$(basename "$source_file")"
    fi
done

for attempt in 1 2 3; do
    if apt-get \
        -o Acquire::Retries=2 \
        -o APT::Update::Error-Mode=any \
        update; then
        exit 0
    fi

    if [ "$attempt" -lt 3 ]; then
        delay=$((attempt * 5))
        echo "APT index update failed; retrying in ${delay}s (attempt $((attempt + 1))/3)" >&2
        sleep "$delay"
    fi
done

echo "ERROR: APT index update failed after 3 attempts" >&2
exit 1

##############################################################################
# end of github-actions-apt-update.sh
##############################################################################
