#!/usr/bin/env bash
#
#   Project: FreeBASIC NuttX/RISC-V testing
#   ---------------------------------------
#
#   File: nuttx-riscv32-compile-generated-c.sh
#
#   Purpose:
#
#       Compile generated C from the fbctests log-test inventory with the
#       RISC-V bare-metal compiler used by the NuttX QEMU image.
#
#   Responsibilities:
#
#       * read compile-manifest.txt from nuttx-riscv32-full-fbctests.py
#       * compile each generated C file to a RISC-V 32 object
#       * treat selected rows as expected C-compiler failures
#       * write a small text report for the first failures
#
#   This file intentionally does NOT contain:
#
#       * FreeBASIC source parsing
#       * QEMU execution logic
#       * NuttX app staging
#

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)

generated_dir="$repo_root/generated/fbctests-nuttx-full-log"
manifest="$generated_dir/compile-manifest.txt"
report_dir="$repo_root/.build-nuttx-full-fbctests"
cc=${RISCV32_CC:-riscv64-unknown-elf-gcc}

usage() {
    cat <<'EOF'
Usage:
  nuttx-riscv32-compile-generated-c.sh [options]

Options:
  --generated-dir DIR    Directory containing compile-manifest.txt.
  --manifest FILE        Compile manifest produced by the Python driver.
  --report-dir DIR       Directory for logs and summary files.
  --cc FILE              RISC-V C compiler.

Environment:
  RISCV32_CC             Same as --cc.
  RISCV32_CFLAGS         Extra or replacement C flags.
EOF
}

die() {
    printf 'nuttx-compile-c: %s\n' "$*" >&2
    exit 1
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --generated-dir)
            [ "$#" -ge 2 ] || die "--generated-dir requires a directory"
            generated_dir=$2
            shift 2
            ;;
        --manifest)
            [ "$#" -ge 2 ] || die "--manifest requires a file"
            manifest=$2
            shift 2
            ;;
        --report-dir)
            [ "$#" -ge 2 ] || die "--report-dir requires a directory"
            report_dir=$2
            shift 2
            ;;
        --cc)
            [ "$#" -ge 2 ] || die "--cc requires a compiler path"
            cc=$2
            shift 2
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

[ -d "$generated_dir" ] || die "missing generated directory: $generated_dir"
[ -f "$manifest" ] || die "missing compile manifest: $manifest"
command -v "$cc" >/dev/null 2>&1 || die "compiler not found: $cc"

mkdir -p "$report_dir/compile-c-logs" "$report_dir/compile-c-objects"

if [ -z "${RISCV32_CFLAGS+x}" ]; then
    RISCV32_CFLAGS="-march=rv32imac_zicsr_zifencei -mabi=ilp32 -ffreestanding"
    RISCV32_CFLAGS="$RISCV32_CFLAGS -Wno-attributes -Wno-unused-label -Wno-unused-variable"
    RISCV32_CFLAGS="$RISCV32_CFLAGS -Wno-implicit-function-declaration -Wno-incompatible-pointer-types"
    RISCV32_CFLAGS="$RISCV32_CFLAGS -Wno-int-conversion -Wno-return-type -Wno-discarded-qualifiers"
fi

summary="$report_dir/compile-c-summary.txt"
: > "$summary"

total=0
passed=0
failed=0

while IFS=: read -r expected c_file source || [ -n "${expected:-}" ]; do
    [ -n "${expected:-}" ] || continue

    total=$((total + 1))

    c_path="$generated_dir/$c_file"
    safe=$(printf '%s' "$c_file" | tr '/\\:' '___')
    obj_path="$report_dir/compile-c-objects/$safe.o"
    log_path="$report_dir/compile-c-logs/$safe.log"

    if [ ! -f "$c_path" ]; then
        printf 'FAIL missing %s %s\n' "$source" "$c_path" >> "$summary"
        failed=$((failed + 1))
        continue
    fi

    set +e
    # shellcheck disable=SC2086
    "$cc" $RISCV32_CFLAGS -x c -c "$c_path" -o "$obj_path" > "$log_path" 2>&1
    rc=$?
    set -e

    case "$expected:$rc" in
        ok:0)
            printf 'PASS ok %s\n' "$source" >> "$summary"
            passed=$((passed + 1))
            ;;
        fail:0)
            printf 'FAIL expected-c-failure %s\n' "$source" >> "$summary"
            failed=$((failed + 1))
            ;;
        fail:*)
            printf 'PASS expected-c-failure %s\n' "$source" >> "$summary"
            passed=$((passed + 1))
            ;;
        ok:*)
            printf 'FAIL c-compile %s %s\n' "$source" "$log_path" >> "$summary"
            failed=$((failed + 1))
            ;;
        *)
            printf 'FAIL bad-manifest %s %s\n' "$expected" "$source" >> "$summary"
            failed=$((failed + 1))
            ;;
    esac
done < "$manifest"

printf 'nuttx-compile-c: total  %d\n' "$total"
printf 'nuttx-compile-c: passed %d\n' "$passed"
printf 'nuttx-compile-c: failed %d\n' "$failed"
printf 'nuttx-compile-c: summary %s\n' "$summary"

if [ "$failed" -ne 0 ]; then
    grep '^FAIL' "$summary" | head -40
    exit 1
fi

printf 'nuttx-compile-c: all generated C compile checks passed\n'

# end of nuttx-riscv32-compile-generated-c.sh
