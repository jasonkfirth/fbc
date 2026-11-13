#!/usr/bin/env bash
# Project: FreeBASIC DOS qualification
# File: msdos-exampleageddon.sh
# Purpose: Run the shared example inventory with the packaged DOS compiler.
# Responsibilities: select MSYS2 build outputs and convert native process paths.
# This file intentionally does NOT build the compiler or classify examples.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
case "$(uname -s)" in
  MINGW*|MSYS*) ;;
  *) echo 'msdos-exampleageddon.sh currently requires MSYS2' >&2; exit 2 ;;
esac

OUT=${OUT:-$ROOT/out/msdos}
BUILDROOT=${BUILDROOT:-$ROOT/.build-msdos/msys2}
HOST_FBC=${HOST_FBC:-$BUILDROOT/host-worktree/bin/fbc.exe}
TOOLROOT=${TOOLROOT:-$BUILDROOT/toolchains}
TARGET_TRIPLET=${TARGET_TRIPLET:-i586-pc-msdosdjgpp}
export PATH="$TOOLROOT/djgpp-cross/djgpp/bin:/mingw64/bin:/usr/bin:$PATH"
PYTHON=${PYTHON:-/mingw64/bin/python}
if [ -z "${PKGDIR:-}" ]; then
  PKGDIR=$(find "$OUT" -maxdepth 1 -type d -name 'FreeBASIC-*-dos' | sort | tail -n 1)
fi
DOSBOX_BIN=${DOSBOX_BIN:-${DOSBOX_X_ROOT:-$ROOT/.build-msdos/dosbox-x}/portable/mingw-build/mingw/dosbox-x.exe}
test -n "$PKGDIR" && test -d "$PKGDIR/fb" || { echo 'DOS package missing' >&2; exit 2; }
test -x "$DOSBOX_BIN" && test -x "$PYTHON" || { echo 'native Python or DOSBox-X missing' >&2; exit 2; }
test -x "$HOST_FBC" || { echo 'host compiler missing for emulator preflight' >&2; exit 2; }

# The CI example lane still runs after a language-test failure. Check its
# emulator independently so a broken CPU core cannot start the full sweep.
PREFLIGHT_DIR="$OUT/qualification/exampleageddon-preflight"
mkdir -p "$PREFLIGHT_DIR"
rm -f "$PREFLIGHT_DIR/X87CHECK.EXE"
"$HOST_FBC" -prefix "$(cygpath -am "$PKGDIR/fb")" \
  -buildprefix "$TARGET_TRIPLET-" -target dos -arch 686 -fpu x87 -gen gcc \
  "$ROOT/tests/dos/emulator-x87.bas" -x "$PREFLIGHT_DIR/X87CHECK.EXE"
"$PYTHON" "$(cygpath -am "$ROOT/build_scripts/dos-run-program.py")" \
  --dosbox "$(cygpath -am "$DOSBOX_BIN")" --package "$(cygpath -am "$PKGDIR")" \
  --workdir "$(cygpath -am "$PREFLIGHT_DIR")" --program X87CHECK.EXE \
  --core "${DOS_TEST_CORE:-normal}" --timeout 30

# Native Python gives DOSBox-X Windows mount paths. MSYS Python has a different
# path model and must not pass /c/... mount commands to a native emulator.
arguments=(
  "$(cygpath -am "$ROOT/build_scripts/msdos-exampleageddon.py")"
  --root "$(cygpath -am "$ROOT")"
  --package "$(cygpath -am "$PKGDIR")"
  --dosbox "$(cygpath -am "$DOSBOX_BIN")"
  --core "${DOS_TEST_CORE:-normal}"
  --outdir "$(cygpath -am "$OUT/exampleageddon")"
)
if [ -n "${DOS_EXAMPLE_SDK:-}" ]; then
  arguments+=(--sdk "$(cygpath -am "$DOS_EXAMPLE_SDK")")
fi
"$PYTHON" "${arguments[@]}" "$@"
# end of msdos-exampleageddon.sh
