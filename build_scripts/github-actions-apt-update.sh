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
readonly mirror_list="/etc/apt/apt-mirrors.txt"
readonly fallback_mirror="https://mirrors.edge.kernel.org/ubuntu/"

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

mirror_dns_failed=0
for mirror_host in azure.archive.ubuntu.com archive.ubuntu.com security.ubuntu.com; do
    if ! getent ahosts "$mirror_host" >/dev/null 2>&1; then
        mirror_dns_failed=1
        break
    fi
done

if [ "$mirror_dns_failed" -ne 0 ] && [ -f "$mirror_list" ]; then
    if getent ahosts mirrors.edge.kernel.org >/dev/null 2>&1; then
        printf '%s\n' "$fallback_mirror" > "$mirror_list"
        echo "Ubuntu archive DNS failed; using the independent fallback mirror" >&2
    else
        echo "WARNING: Ubuntu archive DNS and the fallback mirror are unavailable" >&2
    fi
fi

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
