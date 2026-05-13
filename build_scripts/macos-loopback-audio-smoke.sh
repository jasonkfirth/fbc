#!/usr/bin/env bash
#
# FreeBASIC macOS loopback audio smoke-test runner.
#
# This script builds and runs the Darwin CoreAudio loopback FFT smoke test.
# It expects macOS to expose a virtual loopback device such as BlackHole as
# the default input and output device.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FBC="${FBC:-$ROOT_DIR/bin/fbc}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/out/macos-smoke}"
REPEATS="${LOOPBACK_REPEATS:-3}"
SKIP_STATUS=77

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "macos-loopback-audio-smoke: skipped, this runner requires Darwin"
    exit "$SKIP_STATUS"
fi

if [[ ! -x "$FBC" ]]; then
    echo "macos-loopback-audio-smoke: fbc not found: $FBC" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

"$FBC" \
    -i "$ROOT_DIR/inc" \
    -i "$ROOT_DIR/tests/sfx" \
    "$ROOT_DIR/tests/macos/sfx-loopback-fft-smoke.bas" \
    -x "$OUT_DIR/sfx-loopback-fft-smoke"

for ((run = 1; run <= REPEATS; run++)); do
    echo "macos-loopback-audio-smoke: run $run/$REPEATS"
    set +e
    env SFXLIB_DRIVER=CoreAudio "$OUT_DIR/sfx-loopback-fft-smoke"
    status=$?
    set -e

    if [[ "$status" -eq "$SKIP_STATUS" ]]; then
        echo "macos-loopback-audio-smoke: skipped, CoreAudio loopback is not available"
        exit "$SKIP_STATUS"
    fi

    if [[ "$status" -ne 0 ]]; then
        echo "macos-loopback-audio-smoke: failed run $run/$REPEATS (status=$status)" >&2
        exit "$status"
    fi
done

echo "macos-loopback-audio-smoke: passed $REPEATS run(s)"

# end of macos-loopback-audio-smoke.sh
