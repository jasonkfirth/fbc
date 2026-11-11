#!/usr/bin/env bash

##############################################################################
# GitHub Actions Ubuntu package installation
##############################################################################
#
# Project: FreeBASIC
# File: build_scripts/github-actions-apt-install.sh
#
# Purpose:
#
#     Install Ubuntu packages on GitHub-hosted runners with bounded retries
#     for transient package-mirror failures.
#
# Responsibilities:
#
#     * prepare package indexes through github-actions-apt-update.sh
#     * retry a failed package download or install a bounded number of times
#
# This file intentionally does NOT contain:
#
#     * package lists for individual jobs
#     * target sysroot or foreign-architecture source configuration
#     * changes to Ubuntu's distribution package sources
#
##############################################################################

set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: github-actions-apt-install.sh must run as root" >&2
    exit 2
fi

if [ "$#" -eq 0 ]; then
    echo "ERROR: at least one package name is required" >&2
    exit 2
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
bash "$script_dir/github-actions-apt-update.sh"

for attempt in 1 2 3; do
    if apt-get -o Acquire::Retries=2 install -y --no-install-recommends "$@"; then
        exit 0
    fi

    if [ "$attempt" -lt 3 ]; then
        delay=$((attempt * 5))
        echo "APT package install failed; retrying in ${delay}s (attempt $((attempt + 1))/3)" >&2
        sleep "$delay"
    fi
done

echo "ERROR: APT package install failed after 3 attempts" >&2
exit 1

##############################################################################
# end of github-actions-apt-install.sh
##############################################################################
