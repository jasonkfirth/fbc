#!/usr/bin/env bash
# Project: FreeBASIC DOS qualification
# File: msdos-unit-tests.sh
# Purpose: Build the complete DOS unit suite and run it in DOSBox-X.
# Responsibilities: preserve target build settings and collect bounded receipts.
# This file intentionally does NOT build the DOS distribution or publish it.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
case "$(uname -s)" in
  MINGW*|MSYS*) ;;
  *) echo 'msdos-unit-tests.sh currently requires the MSYS2 cross toolchain' >&2; exit 2 ;;
esac

BUILDROOT=${BUILDROOT:-$ROOT/.build-msdos/msys2}
OUT=${OUT:-$ROOT/out/msdos}
DOS_WORKTREE=${DOS_WORKTREE:-$BUILDROOT/dos-worktree}
HOST_FBC=${HOST_FBC:-$BUILDROOT/host-worktree/bin/fbc.exe}
TARGET_TRIPLET=${TARGET_TRIPLET:-i586-pc-msdosdjgpp}
TOOLROOT=${TOOLROOT:-$BUILDROOT/toolchains}
CROSS_BIN="$TOOLROOT/djgpp-cross/djgpp/bin"
export PATH="$CROSS_BIN:/mingw64/bin:/usr/bin:$PATH"
MAKE_JOBS=${MAKE_JOBS:-4}
PYTHON=${PYTHON:-/mingw64/bin/python}
REPORT_DIR="$OUT/qualification"
mkdir -p "$REPORT_DIR"

if [ -z "${PKGDIR:-}" ]; then
  PKGDIR=$(find "$OUT" -maxdepth 1 -type d -name 'FreeBASIC-*-dos' | sort | tail -n 1)
fi
test -n "$PKGDIR" && test -d "$PKGDIR/fb" || { echo 'DOS package missing' >&2; exit 2; }
if [ -z "${DOSBOX_BIN:-}" ]; then
  DOSBOX_BIN="${DOSBOX_X_ROOT:-$ROOT/.build-msdos/dosbox-x}/portable/mingw-build/mingw/dosbox-x.exe"
fi
test -x "$DOSBOX_BIN" && test -x "$HOST_FBC" && test -x "$PYTHON" || { echo 'host compiler, native Python or DOSBox-X missing' >&2; exit 2; }
PREFIX_NATIVE=$(cygpath -am "$PKGDIR/fb")
fbc=("$HOST_FBC" -prefix "$PREFIX_NATIVE" -buildprefix "$TARGET_TRIPLET-" -gen gcc)
printf -v FBC_COMMAND '%q ' "${fbc[@]}"

# The distribution build owns this private source copy. Keep all target objects
# there so the normal Windows compiler/test objects remain independent.
cd "$DOS_WORKTREE/tests"
"${fbc[@]}" -target dos -arch 686 -fpu x87 "$ROOT/tests/dos/emulator-x87.bas" -x X87CHECK.EXE
run_dos() {
  "$PYTHON" "$(cygpath -am "$ROOT/build_scripts/dos-run-program.py")" \
    --dosbox "$(cygpath -am "$DOSBOX_BIN")" \
    --core "${DOS_TEST_CORE:-normal}" \
    --package "$(cygpath -am "$PKGDIR")" \
    --workdir "$(cygpath -am "$DOS_WORKTREE/tests")" "$@"
}
probe_status=0
run_dos --program X87CHECK.EXE --timeout 30 || probe_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/emulator-x87.json"
cp FBCIRUN.OUT "$REPORT_DIR/emulator-x87.txt"
test "$probe_status" -eq 0 || exit "$probe_status"

"$CROSS_BIN/$TARGET_TRIPLET-gcc.exe" -O2 -DDISABLE_WCHAR \
  -I"$DOS_WORKTREE/src/rtlib" "$ROOT/tests/dos/math-control.c" \
  -L"$PKGDIR/fb/lib/freebasic/dos" -lfb -lm -o MATHCTRL.EXE
math_status=0
run_dos --program MATHCTRL.EXE --timeout 30 || math_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/math-control.json"
cp FBCIRUN.OUT "$REPORT_DIR/math-control.txt"
test "$math_status" -eq 0 || exit "$math_status"

# Observe a parameter through its published C address while Intel assembly is
# still executing. This also exercises more static operands than registers.
"$CROSS_BIN/$TARGET_TRIPLET-gcc.exe" -O2 -masm=intel -c \
  "$ROOT/tests/compiler-support/c-asm-real-addresses.c" -o ASMOBS.O
"${fbc[@]}" -target dos -arch 686 -fpu x87 -asm intel -Wc -O2 \
  "$ROOT/tests/compiler-support/c-asm-real-addresses.bas" ASMOBS.O -x ASMADDR.EXE
asm_status=0
run_dos --program ASMADDR.EXE --timeout 30 || asm_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/asm-addresses.json"
cp FBCIRUN.OUT "$REPORT_DIR/asm-addresses.txt"
test "$asm_status" -eq 0 || exit "$asm_status"

# DJGPP has no native C TLS. Check that the compiler's runtime-backed
# variables preserve their initial values and remain private to each worker.
"${fbc[@]}" -target dos -arch 686 -fpu x87 -dos-threads pdmlwp \
  "$ROOT/tests/compiler-support/thread-local.bas" -x TLSCHECK.EXE
tls_status=0
run_dos --program TLSCHECK.EXE --timeout 30 || tls_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/thread-local.json"
cp FBCIRUN.OUT "$REPORT_DIR/thread-local.txt"
test "$tls_status" -eq 0 || exit "$tls_status"

"${fbc[@]}" -target dos -arch 686 -fpu x87 -dos-threads pdmlwp \
  -m tls-basic-consumer "$ROOT/tests/dos/tls-basic-consumer.bas" \
  "$ROOT/tests/dos/tls-basic-provider.bas" -x TLSXMOD.EXE
tls_status=0
run_dos --program TLSXMOD.EXE --timeout 30 || tls_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/thread-local-modules.json"
cp FBCIRUN.OUT "$REPORT_DIR/thread-local-modules.txt"
test "$tls_status" -eq 0 || exit "$tls_status"

"${fbc[@]}" -target dos -arch 686 -fpu x87 -gen gas -dos-threads pdmlwp \
  -m tls-basic-consumer "$ROOT/tests/dos/tls-basic-consumer.bas" \
  "$ROOT/tests/dos/tls-basic-provider.bas" -x TLSXGAS.EXE
tls_status=0
run_dos --program TLSXGAS.EXE --timeout 30 || tls_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/thread-local-gas.json"
cp FBCIRUN.OUT "$REPORT_DIR/thread-local-gas.txt"
test "$tls_status" -eq 0 || exit "$tls_status"

# Keep the storage API check independent of the compiler's lowering.
"$CROSS_BIN/$TARGET_TRIPLET-gcc.exe" -O2 -DDISABLE_WCHAR \
  -I"$DOS_WORKTREE/src/rtlib" "$ROOT/tests/dos/tls-var-key.c" \
  -L"$PKGDIR/fb/lib/freebasic/dos" -lfb -lm -o TLSKEY.EXE
tls_status=0
run_dos --program TLSKEY.EXE --timeout 30 || tls_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/thread-local-key.json"
cp FBCIRUN.OUT "$REPORT_DIR/thread-local-key.txt"
test "$tls_status" -eq 0 || exit "$tls_status"

"${fbc[@]}" -target dos -arch 686 -fpu x87 -dos-threads pdmlwp -exx \
  "$ROOT/tests/compiler-support/thread-local-arrays.bas" -x TLSARRAY.EXE
tls_status=0
run_dos --program TLSARRAY.EXE --timeout 30 || tls_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/thread-local-arrays.json"
cp FBCIRUN.OUT "$REPORT_DIR/thread-local-arrays.txt"
test "$tls_status" -eq 0 || exit "$tls_status"

# Managed TLS initialization and native C TLS are not supplied by this path.
# Invalid declarations must produce the intended diagnostics, not a crash.
tls_bad_status=0
"${fbc[@]}" -target dos -arch 686 -fpu x87 -dos-threads pdmlwp -r \
  "$ROOT/tests/dos/tls-unsupported.bas" -o "$REPORT_DIR/TLSBAD.C" \
  > "$REPORT_DIR/thread-local-unsupported.txt" 2>&1 || tls_bad_status=$?
"$PYTHON" - "$(cygpath -am "$REPORT_DIR")" "$tls_bad_status" <<'PY'
import json
from pathlib import Path
import re
import sys

directory = Path(sys.argv[1])
codes = re.findall(r'\berror (\d+):', (directory / 'thread-local-unsupported.txt').read_text())
passed = int(sys.argv[2]) == 1 and codes == ['338', '338']
(directory / 'thread-local-unsupported.json').write_text(json.dumps({
    'compiler_exit': int(sys.argv[2]), 'diagnostic_codes': codes,
    'passed': passed}, indent=2) + '\n')
if not passed:
    raise SystemExit('DOS unsupported TLS declarations did not produce the expected diagnostics')
PY

make -j"$MAKE_JOBS" -f unit-tests.mk build_tests \
  TARGET_TRIPLET="$TARGET_TRIPLET" HOST_TRIPLET=x86_64-w64-mingw32 CROSS_BUILD=yes \
  TARGET=dos TARGET_OS=dos ARCH=686 FPU=x87 GEN=gcc MAINEXE=FBCTEST.EXE \
  FBC="$FBC_COMMAND" CC="$CROSS_BIN/$TARGET_TRIPLET-gcc.exe" \
  AR="$CROSS_BIN/$TARGET_TRIPLET-ar.exe"
cp unit-tests.inc "$REPORT_DIR/unit-tests.inc"
cp unit-tests-obj.lst "$REPORT_DIR/unit-tests-obj.lst"

# XML is produced only after the full suite returns. A stale report must never
# turn an aborted emulator invocation into an accepted qualification.
rm -f REPORT.XML
suite_status=0
run_dos --program FBCTEST.EXE --arg=--brief-summary --arg=--hide-cases \
  --arg=--xml --arg=REPORT.XML --watch-file REPORT.XML \
  --timeout "${DOS_UNIT_TIMEOUT:-600}" || suite_status=$?
cp FBCIRUN.JSON "$REPORT_DIR/fbctests.json"
cp FBCIRUN.OUT "$REPORT_DIR/fbctests.txt"
if [ -s REPORT.XML ]; then
  cp REPORT.XML "$REPORT_DIR/fbctests.xml"
else
  echo 'DOS unit suite did not produce a complete XML report' >&2
  exit 1
fi
"$PYTHON" - "$(cygpath -am "$REPORT_DIR")" <<'PY'
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

directory = Path(sys.argv[1])
root = ET.parse(directory / 'fbctests.xml').getroot()
if root.tag != 'testsuites':
    raise SystemExit('unexpected FBCUnit XML root')
suites = root.findall('testsuite')
totals = {key: sum(int(suite.attrib[key]) for suite in suites)
          for key in ('tests', 'assertions', 'failures', 'errors')}
counts_match = all(len(suite.findall('testcase')) == int(suite.attrib['tests'])
                   for suite in suites)
receipt = json.loads((directory / 'fbctests.json').read_text())
passed = (receipt['passed'] and totals['tests'] > 0 and counts_match
          and totals['failures'] == 0 and totals['errors'] == 0
          and not root.findall('.//failure') and not root.findall('.//error'))
summary = dict(totals, suites=len(suites), counts_match=counts_match, passed=bool(passed))
(directory / 'fbctests-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary), flush=True)
if not passed:
    raise SystemExit('DOS FBCUnit qualification failed')
PY
exit "$suite_status"
# end of msdos-unit-tests.sh
