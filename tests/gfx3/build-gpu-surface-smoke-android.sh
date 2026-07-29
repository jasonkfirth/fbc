#!/usr/bin/env bash

#
# Project: FreeBASIC gfxlib3 verification
# ----------------------------------------
#
# File: build-gpu-surface-smoke-android.sh
#
# Purpose:
#
#     Package the GPU-surface correctness smoke test for Android AArch64.
#
# Responsibilities:
#
#     - select the current checkout's threaded PIC gfxlib3 archive
#     - force the OpenGL ES path used by Android devices without Vulkan
#     - produce a self-terminating APK whose process result is logged
#
# This file intentionally does NOT contain:
#
#     - device installation or launch automation
#     - game assets
#

set -euo pipefail

root="/c/Nextcloud/games/FreeBASIC_packages/fbc"
output="${root}/.codex-tmp-gfx3-build/openslicks-android"
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

mkdir -p "${output}"
cd "${root}/tests/gfx3"

"${wrapper}" \
    --target android-aarch64 \
    --assets . \
    --package net.fbxl.gfx3surface.smoke \
    --label "gfx3 surface smoke" \
    --landscape \
    --env FBGFX3_LOG=info \
    -mt \
    -gfx3 \
    -d GFX3_OPENGL_TEST \
    -x "${output}/gpu-surface-smoke.apk" \
    gpu-surface-smoke.bas

# end of build-gpu-surface-smoke-android.sh
