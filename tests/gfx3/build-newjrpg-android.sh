#!/usr/bin/env bash

#
# Project: FreeBASIC gfxlib3 verification
# ----------------------------------------
#
# File: build-newjrpg-android.sh
#
# Purpose:
#
#     Package the JRPG renderer smoke test for Android AArch64.
#
# Responsibilities:
#
#     - compile the asset-independent JRPG renderer assertions
#     - use the current checkout's threaded PIC gfxlib3 archive
#     - produce a self-terminating APK for connected-device verification
#
# This file intentionally does NOT contain:
#
#     - device installation or launch automation
#     - JRPG production assets
#

set -euo pipefail

root="/c/Nextcloud/games/FreeBASIC_packages/fbc"
jrpg="/c/Nextcloud/games/newjrpg"
output="${root}/.codex-tmp-gfx3-build/newjrpg-android"
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
export PATH="/usr/bin:${JAVA_HOME}/bin:${PATH}"

mkdir -p "${output}"

"${wrapper}" \
    --emit-apk \
    --package net.fbxl.gfx3newjrpg.smoke \
    --label "JRPG gfxlib3 smoke" \
    --target android-aarch64 \
    --landscape \
    --hideKeyboardButton \
    --env FBGFX3_LOG=info \
    -mt \
    -gfx3 \
    -exx \
    -w all \
    -i "${jrpg}" \
    -i "${jrpg}/omaGui" \
    -x "${output}/newjrpg-renderer-smoke.apk" \
    "${jrpg}/jrpg_RuntimeRenderer_Smoke.bas"

# end of build-newjrpg-android.sh
