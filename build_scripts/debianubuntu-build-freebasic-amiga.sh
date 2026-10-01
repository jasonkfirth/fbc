#!/usr/bin/env bash
#
# FreeBASIC classic AmigaOS build workflow
# ---------------------------------------
#
# File: debianubuntu-build-freebasic-amiga.sh
# Purpose: Build the compiler and Amiga runtime, graphics, and sound archives.
# Responsibilities:
#   - provision the immutable SDK used by FreeBASIC-NG's Amiga preview
#   - relocate SDK links and repair its inspected newlib startup objects
#   - use one 68020 soft-float contract for every target library variant
# This file intentionally does NOT contain emulator boot or test selection.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
AMIGA_ROOT="${AMIGA_ROOT:-$ROOT/out/amiga}"
TOOLCHAIN_ROOT="${AMIGA_GCC_ROOT:-$AMIGA_ROOT/toolchain}"
IMAGE="amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed"
JOBS="${AMIGA_JOBS:-8}"
SKIP_TOOLCHAIN=0
SKIP_COMPILER=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --skip-toolchain) SKIP_TOOLCHAIN=1; shift ;;
        --skip-compiler) SKIP_COMPILER=1; shift ;;
        --jobs) JOBS="${2:?--jobs needs a value}"; shift 2 ;;
        --toolchain-root) TOOLCHAIN_ROOT="${2:?--toolchain-root needs a value}"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 [--skip-toolchain] [--skip-compiler] [--jobs N] [--toolchain-root PATH]"
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done
[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || { echo "Invalid job count: $JOBS" >&2; exit 2; }

mkdir -p "$AMIGA_ROOT" "$TOOLCHAIN_ROOT"
TOOLCHAIN_ROOT="$(cd "$TOOLCHAIN_ROOT" && pwd)"

if [ ! -x "$TOOLCHAIN_ROOT/bin/m68k-amigaos-gcc" ]; then
    [ "$SKIP_TOOLCHAIN" = 0 ] || { echo "Amiga SDK is missing" >&2; exit 1; }
    command -v docker >/dev/null || { echo "SDK provisioning requires Docker" >&2; exit 1; }
    docker pull "$IMAGE"
    docker run --rm --entrypoint sh \
        -v "$TOOLCHAIN_ROOT:/extract" "$IMAGE" \
        -c 'cp -a /opt/amiga/. /extract/'
    docker run --rm --entrypoint chown \
        -v "$TOOLCHAIN_ROOT:/extract" "$IMAGE" \
        -R "$(id -u):$(id -g)" /extract
    printf '%s\n' "$IMAGE" >"$TOOLCHAIN_ROOT/source-image.txt"
fi

# The container SDK has absolute links rooted at /opt/amiga. Resolve only
# that known prefix; unrelated SDK links and host paths retain their meaning.
python3 - "$TOOLCHAIN_ROOT" <<'PY'
from pathlib import Path
import os
import sys
root = Path(sys.argv[1])
for path in root.rglob('*'):
    if not path.is_symlink():
        continue
    target = str(path.readlink())
    if target.startswith('/opt/amiga/'):
        replacement = root / target[len('/opt/amiga/'):]
        path.unlink()
        path.symlink_to(os.path.relpath(replacement, path.parent))
PY

python3 "$SCRIPT_DIR/amiga-patch-toolchain.py" "$TOOLCHAIN_ROOT"
export PATH="$TOOLCHAIN_ROOT/bin:$PATH"
unset DEBUG

python3 "$SCRIPT_DIR/amiga-build-softfloat.py" \
    --toolchain-root "$TOOLCHAIN_ROOT" --work "$AMIGA_ROOT/softfloat" \
    --output "$ROOT/lib/freebasic/amiga-m68k/libfbsoftfloat.a"

if [ "$SKIP_COMPILER" = 0 ]; then
    make -C "$ROOT" -j"$JOBS" compiler
fi

make -C "$ROOT" -j"$JOBS" rtlib gfxlib2 sfxlib \
    TARGET_TRIPLET=m68k-amigaos \
    CC="$TOOLCHAIN_ROOT/bin/m68k-amigaos-gcc" \
    AR="$TOOLCHAIN_ROOT/bin/m68k-amigaos-ar" \
    DISABLE_FFI=YesPlease DISABLE_NCURSES=YesPlease

echo "AmigaOS libraries: $ROOT/lib/freebasic/amiga-m68k"

# end of debianubuntu-build-freebasic-amiga.sh
