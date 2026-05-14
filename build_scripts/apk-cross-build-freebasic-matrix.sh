#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: apk-cross-build-freebasic-matrix.sh
#
# Purpose:
#
#     Build the APK-family package matrix for Alpine Linux and postmarketOS.
#
# Responsibilities:
#
#     * expose the APK package-family entry point used by the top-level Linux
#       package factory
#     * preserve distro/release/architecture filtering for one-shot builds
#     * delegate to the existing APK builder until the Alpine CHOST cross-build
#       path is promoted from package-manager plumbing to default production
#
# This file intentionally does NOT contain:
#
#     * the APK package implementation
#     * package validation
#     * Debian/RPM/Slackware package policy
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

ARGS=()

while [ $# -gt 0 ]; do
    case "$1" in
        --execute)
            shift
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done

exec "$SCRIPT_DIR/emulated-native-matrix/alpine-freebasic-matrix-build.sh" "${ARGS[@]}"

##############################################################################
# end of apk-cross-build-freebasic-matrix.sh
##############################################################################
