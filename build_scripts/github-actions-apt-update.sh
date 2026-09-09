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

apt-get -o Acquire::Retries=5 update

##############################################################################
# end of github-actions-apt-update.sh
##############################################################################
