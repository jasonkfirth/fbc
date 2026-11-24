#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: archlinux-freebasic-matrix-build.sh
#
# Purpose:
#
#     Build the Arch Linux package matrix.
#
# Responsibilities:
#
#     * delegate matrix execution to the Arch-specific cross matrix script
#
# This file intentionally does NOT contain:
#
#     * build graph definition
#     * dependency installation
#     * package validation logic

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

exec "$SCRIPT_DIR/archlinux-cross-build-freebasic-matrix.sh" "$@"

##############################################################################
# end of archlinux-freebasic-matrix-build.sh
##############################################################################
