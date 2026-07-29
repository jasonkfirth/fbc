#!/usr/bin/env bash

#
# Project: FreeBASIC gfxlib3 verification
# ----------------------------------------
#
# File: build-openslicks-android.sh
#
# Purpose:
#
#     Package Open Slicks Racing for Android AArch64 against the current
#     checkout's threaded PIC gfxlib3 archive.
#
# Responsibilities:
#
#     - stage the original Slix151 tracks and DAT artwork
#     - compile the same separately built game modules used on desktop
#     - produce a landscape APK with renderer diagnostics enabled
#
# This file intentionally does NOT contain:
#
#     - device installation or launch automation
#     - OpenSlicks source changes
#     - a forced renderer selection
#

set -euo pipefail

root="/c/Nextcloud/games/FreeBASIC_packages/fbc"
openslicks="/e/openSlicks"
output="${root}/.codex-tmp-gfx3-build/openslicks-android"
assets="${output}/assets"
wrapper="${root}/src/tools/android/fbc-android"

export ANDROID_HOME="/c/freebasic-android/toolchain/android-sdk"
export ANDROID_SDK_ROOT="${ANDROID_HOME}"
export ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/27.2.12479018"
export JAVA_HOME="/c/freebasic-android/toolchain/java"
export FBANDROID_PREFIX="/c/freebasic-android"
export FBANDROID_LIBROOT="/c/freebasic-android/lib/freebasic-android"
export FBANDROID_COMPILER="${root}/bin/fbc-android-compiler.exe"
export FBANDROID_INCDIR="/c/freebasic-android/include/freebasic-android"
export FBANDROID_SHARE="/c/freebasic-android/share/freebasic-android"
export FBANDROID_LIBDIR="${root}/lib/freebasic/android-aarch64"
export PATH="${JAVA_HOME}/bin:${PATH}"

mkdir -p "${assets}" "${output}"
cp -R "${openslicks}/Slix151" "${assets}/"

sources=(
    slicks.bas
    slicks_app.bas
    slicks_font.bas
    slicks_track.bas
    slicks_input.bas
    oma_native_input.bas
    slicks_vehicle.bas
    slicks_ai_track.bas
    slicks_ai_driver.bas
    slicks_game.bas
    slicks_render.bas
    slicks_menu.bas
)

source_paths=()

for source_name in "${sources[@]}"; do
    source_paths+=("${openslicks}/src/${source_name}")
done

"${wrapper}" \
    --emit-apk \
    --package net.fbxl.gfx3openslicks \
    --label "Open Slicks gfxlib3" \
    --target android-aarch64 \
    --landscape \
    --hideKeyboardButton \
    --assets "${assets}" \
    --env FBGFX3_LOG=info \
    --env FBGFX3_PROFILE=1 \
    -mt \
    -gfx3 \
    -exx \
    -w all \
    -i "${openslicks}/src" \
    -x "${output}/openslicks-gfx3.apk" \
    "${source_paths[@]}"

# end of build-openslicks-android.sh
