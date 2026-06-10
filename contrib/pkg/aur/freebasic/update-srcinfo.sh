#!/usr/bin/env bash
#
# Project: FreeBASIC AUR package
# --------------------------------
#
# File: update-srcinfo.sh
#
# Purpose:
#
#     Regenerate .SRCINFO from the AUR PKGBUILD.
#
# Responsibilities:
#
#     * run makepkg's metadata generator from this package directory
#     * keep .SRCINFO synchronized with PKGBUILD metadata
#
# This file intentionally does NOT contain:
#
#     * package builds
#     * AUR git operations
#     * checksum generation
#

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${script_dir}"

makepkg --printsrcinfo > .SRCINFO

# end of update-srcinfo.sh
