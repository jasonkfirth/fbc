#!/bin/sh
# Project: FreeBASIC Linux graphics tests
# File: linux-wayland-webos-tests.sh
# Purpose: Run native webOS ABI/lifetime and existing Wayland input checks.
# Responsibilities: Build only focused C tests and remove their temporary files.
# This file does not connect to a compositor or build the FreeBASIC compiler.
set -eu
compiler=${CC:-cc}
test_root=$(mktemp -d /tmp/fbc-wayland-webos.XXXXXX)
trap 'rm -rf -- "$test_root"' EXIT HUP INT TERM
for test_name in linux-wayland-webos wayland-webos-occlusion wayland-text-region \
    wayland-text-commit wayland-window-title wayland-mobile-window-state wayland-touch-events
do
    if [ "$test_name" = linux-wayland-webos ]; then
        # This test includes the bridge implementation to check private ABI tables.
        "$compiler" -std=gnu11 -Wall -Wextra -Werror -pthread \
            "tests/gfx/$test_name.c" -o "$test_root/$test_name"
    else
        "$compiler" -std=gnu11 -Wall -Wextra -Werror -DDISABLE_X11 -DDISABLE_OPENGL \
            -ffunction-sections -fdata-sections -pthread "tests/gfx/$test_name.c" \
            src/gfxlib2/drivers/linux/fb_wayland_webos.c \
            -Wl,--gc-sections -ldl -lm -o "$test_root/$test_name"
    fi
    "$test_root/$test_name"
done
# end of linux-wayland-webos-tests.sh
