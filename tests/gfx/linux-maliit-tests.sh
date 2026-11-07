#!/bin/sh
#
# Project: FreeBASIC Linux graphics tests
# --------------------------------------
#
# File: linux-maliit-tests.sh
# Purpose: Run the isolated Maliit bridge and graphics input regressions.
# Responsibilities: Build in a temporary directory and remove it afterwards.
# This file intentionally does NOT connect to a real desktop or input method.
#
# Run from any directory with a C compiler and GIO development files:
#     sh tests/gfx/linux-maliit-tests.sh
#
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
compiler=${CC:-cc}
if ! command -v "$compiler" >/dev/null 2>&1 || ! pkg-config --exists gio-2.0; then
    echo 'A C compiler, pkg-config and GIO development files are required.' >&2
    exit 1
fi
build=$(mktemp -d "${TMPDIR:-/tmp}/fbc-maliit-tests.XXXXXXXX")
trap 'rm -rf -- "$build"' EXIT
trap 'exit 1' HUP INT TERM
cd "$root"

# The private peer needs GIO; the runtime bridge itself loads it dynamically.
gio_flags=$(pkg-config --cflags --libs gio-2.0)
"$compiler" -std=c11 -Wall -Wextra -Werror -pthread \
    tests/gfx/linux-maliit-peer.c src/gfxlib2/drivers/linux/fb_linux_maliit.c \
    -o "$build/peer" $gio_flags -ldl
"$compiler" -std=c11 -Wall -Wextra -Werror -pthread \
    tests/gfx/linux-maliit-region.c -o "$build/region" $gio_flags -ldl
"$compiler" -std=c11 -Wall -Wextra -Werror -pthread \
    tests/gfx/linux-maliit-queue-fallback.c -o "$build/queue-fallback" $gio_flags -ldl
"$compiler" -std=c11 -Wall -Wextra -Werror -pthread \
    tests/gfx/linux-osk-phosh-peer.c -o "$build/phosh-peer" $gio_flags -ldl
"$compiler" -std=c11 -Wall -Wextra -Werror -pthread \
    -ffunction-sections -fdata-sections tests/gfx/linux-maliit-lifecycle.c \
    -Wl,--gc-sections $gio_flags -ldl -o "$build/lifecycle"
for backend in gfx gfx3; do
    "$compiler" -std=gnu11 -Wall -Wextra -Werror -pthread \
        -ffunction-sections -fdata-sections "tests/$backend/linux-maliit-events.c" \
        -Wl,--gc-sections -ldl -lm -o "$build/events-$backend"
done
for variant in nomt mt; do
    mt_flag=
    if [ "$variant" = mt ]; then mt_flag=-DENABLE_MT; fi
    "$compiler" -std=gnu11 -Wall -Wextra -Werror -pthread $mt_flag \
        -ffunction-sections -fdata-sections tests/gfx3/linux-mobile-lifetime.c \
        src/gfxlib3/api/gfx3_query_api.c \
        src/gfxlib3/drivers/linux/gfx3_mobile_lifetime.c \
        -Wl,--gc-sections -ldl -lm -o "$build/mode-$variant"
done
for test in linux-osk-fallback linux-osk-async; do
    "$compiler" -std=c11 -Wall -Wextra -Werror -pthread \
        "tests/gfx/$test.c" -o "$build/$test"
done

for test in peer region queue-fallback phosh-peer lifecycle events-gfx events-gfx3 \
    mode-nomt mode-mt linux-osk-fallback linux-osk-async; do
    G_DEBUG=fatal-warnings "$build/$test"
done
echo LINUX_MALIIT_TESTS_PASS
# end of linux-maliit-tests.sh
