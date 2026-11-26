#!/usr/bin/env bash
#
# Project: FreeBASIC AmigaOS package
# ----------------------------------
#
# File: amiga-package-freebasic.sh
#
# Purpose:
#
#     Assemble a relocatable AmigaOS m68k compiler distribution from a
#     native FreeBASIC compiler and the matching target headers and libraries.
#
# Responsibilities:
#
#     - stage the compiler, include files, runtime libraries, and license
#     - create LHA and ZIP archives for AmigaOS and host-side transfer
#     - verify both archives and record their checksums
#
# This file intentionally does NOT contain:
#
#     - compiler or target-library builds
#     - AmigaOS emulator startup or test selection
#     - AmigaOS SDK provisioning
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
AMIGA_ROOT="${AMIGA_ROOT:-$ROOT/out/amiga}"
FBC_PREFIX="${AMIGA_FBC_PREFIX:-$ROOT}"
OUTPUT_DIR="${AMIGA_PACKAGE_DIR:-$AMIGA_ROOT/packages}"
NATIVE_FBC="${AMIGA_NATIVE_FBC:-$AMIGA_ROOT/native-fbc}"
PACKAGE_NAME="FreeBASIC-1.20.6-1-AmigaOS-m68k"

die() {
    echo "ERROR: $*" >&2
    exit 1
}

command -v lha >/dev/null 2>&1 || die "lha is required to create the AmigaOS archive"
command -v zip >/dev/null 2>&1 || die "zip is required to create the transfer archive"
command -v unzip >/dev/null 2>&1 || die "unzip is required to verify the transfer archive"

[ -f "$NATIVE_FBC" ] || die "native AmigaOS compiler not found: $NATIVE_FBC"
[ -d "$FBC_PREFIX/inc" ] || die "FreeBASIC include tree not found below: $FBC_PREFIX"
[ -d "$FBC_PREFIX/lib/freebasic/amiga-m68k" ] || \
    die "AmigaOS runtime libraries not found below: $FBC_PREFIX"

mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(cd "$OUTPUT_DIR" && pwd)"
STAGE_ROOT="$(mktemp -d "$OUTPUT_DIR/.amiga-package-stage.XXXXXX")"
VERIFY_ROOT="$(mktemp -d "$OUTPUT_DIR/.amiga-package-verify.XXXXXX")"
PACKAGE_ROOT="$STAGE_ROOT/$PACKAGE_NAME"
ARCHIVE_LHA="$OUTPUT_DIR/$PACKAGE_NAME.lha"
ARCHIVE_ZIP="$OUTPUT_DIR/$PACKAGE_NAME.zip"
TMP_LHA="$OUTPUT_DIR/.$PACKAGE_NAME.lha.tmp"
TMP_ZIP="$OUTPUT_DIR/.$PACKAGE_NAME.zip.tmp"

cleanup() {
    rm -rf "$STAGE_ROOT" "$VERIFY_ROOT"
    rm -f "$TMP_LHA" "$TMP_ZIP"
}
trap cleanup EXIT

mkdir -p "$PACKAGE_ROOT/C" "$PACKAGE_ROOT/inc" \
    "$PACKAGE_ROOT/lib/freebasic" "$PACKAGE_ROOT/doc"
install -m 755 "$NATIVE_FBC" "$PACKAGE_ROOT/C/fbc"
cp -a "$FBC_PREFIX/inc/." "$PACKAGE_ROOT/inc/"
cp -a "$FBC_PREFIX/lib/freebasic/amiga-m68k" "$PACKAGE_ROOT/lib/freebasic/"
install -m 644 "$ROOT/doc/gpl.txt" "$PACKAGE_ROOT/LICENSE.txt"
install -m 644 "$ROOT/doc/fbc.1" "$PACKAGE_ROOT/doc/fbc.1"
install -m 644 "$ROOT/changelog.txt" "$PACKAGE_ROOT/doc/changelog.txt"

cat > "$PACKAGE_ROOT/README-AmigaOS.txt" <<'EOF'
FreeBASIC 1.20.6-1 for AmigaOS m68k
==================================

This distribution contains the native AmigaOS compiler, FreeBASIC include
files, and the matching amiga-m68k runtime libraries.

Install the archive under SYS:FreeBASIC and copy C/fbc to C:fbc. Keep the
include and lib directories together. From an AmigaDOS shell, compile with:

    fbc -prefix SYS:FreeBASIC -target amiga-m68k hello.bas

The bundled runtime is built for the AmigaOS m68k target with the 68020
instruction set and software floating point.
EOF

(
    cd "$STAGE_ROOT"
    lha a "$TMP_LHA" "$PACKAGE_NAME"
    zip -q -r -9 "$TMP_ZIP" "$PACKAGE_NAME"
)

lha t "$TMP_LHA" >/dev/null
grep -q "$PACKAGE_NAME/C/fbc" < <(lha l "$TMP_LHA") || \
    die "LHA archive is missing the native compiler"
unzip -tq "$TMP_ZIP"

(
    cd "$VERIFY_ROOT"
    lha xq "$TMP_LHA"
)
diff -qr "$PACKAGE_ROOT" "$VERIFY_ROOT/$PACKAGE_NAME" >/dev/null || \
    die "LHA archive contents differ from the staged package"

mv -f "$TMP_LHA" "$ARCHIVE_LHA"
mv -f "$TMP_ZIP" "$ARCHIVE_ZIP"
(cd "$OUTPUT_DIR" && sha256sum "$PACKAGE_NAME.lha" "$PACKAGE_NAME.zip" \
    > "$PACKAGE_NAME.SHA256SUMS")

echo "AmigaOS package ready:"
ls -lh "$ARCHIVE_LHA" "$ARCHIVE_ZIP" "$OUTPUT_DIR/$PACKAGE_NAME.SHA256SUMS"

# end of amiga-package-freebasic.sh
