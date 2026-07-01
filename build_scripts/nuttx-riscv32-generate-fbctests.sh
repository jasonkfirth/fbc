#!/usr/bin/env bash
#
#   Project: FreeBASIC NuttX smoke testing
#   -------------------------------------
#
#   File: nuttx-riscv32-generate-fbctests.sh
#
#   Purpose:
#
#       Generate C sources for an initial slice of real fbctests using the
#       NuttX RISC-V target.
#
#   Responsibilities:
#
#       * invoke a host fbc that knows the NuttX target
#       * generate C output for selected TEST_MODE : COMPILE_AND_RUN_OK tests
#       * preserve per-test language modes such as -lang fblite
#       * add the shared include paths used by ordinary fbctests sources
#       * stage fbcunit support sources when selected tests need them
#       * copy the generated C files into a stable staging directory
#       * write a manifest consumable by nuttx-riscv32-qemu-suite.sh
#
#   This file intentionally does NOT contain:
#
#       * QEMU execution logic
#       * NuttX app-tree staging
#       * target runtime implementations
#

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)

fbc=${FBC:-"$repo_root/bin/fbc.exe"}
target=${FB_NUTTX_TARGET:-nuttx-riscv32}
out_dir="$repo_root/generated/fbctests-nuttx"
include_dir="$repo_root/inc"
fbcunit_include_dir="$repo_root/tests/fbcunit/inc"
fbcunit_src_dir="$repo_root/tests/fbcunit/src"
manifest_name="manifest.txt"
need_fbcunit=0

tests=(
    "fbpretest:tests/pretest/compile_and_run_ok.bas"
    "fbcrtbuiltin:tests/crt/builtin.bas"
    "fbcrtclongproto:tests/crt/clong-prototype.bas"
    "fbfblitelen:tests/fblite/len.bas:fblite"
    "fbfblitecmd:tests/fblite/command-sweep.bas:fblite"
    "fbrtcmdsweep:tests/command-sweep/rtlib-command-sweep.bas"
)

usage() {
    cat <<'EOF'
Usage:
  nuttx-riscv32-generate-fbctests.sh [options]

Options:
  --fbc PATH              Host fbc executable.
                          Default: $FBC or <repo>/bin/fbc.exe

  --target TARGET         fbc target alias.
                          Default: $FB_NUTTX_TARGET or nuttx-riscv32

  --output-dir DIR        Directory for generated C files and manifest.
                          Default: <repo>/generated/fbctests-nuttx

  --test SPEC             Add one test. The first --test replaces the default
                          slice. Format: appname:path-to-bas[:lang]

  -h, --help              Show this help.

Default slice:
  tests/pretest/compile_and_run_ok.bas
  tests/crt/builtin.bas
  tests/crt/clong-prototype.bas
  tests/fblite/len.bas
  tests/fblite/command-sweep.bas
  tests/command-sweep/rtlib-command-sweep.bas
EOF
}

die() {
    printf 'nuttx-fbctests-gen: %s\n' "$*" >&2
    exit 1
}

add_custom_test() {
    if [ "${have_custom_tests:-0}" -eq 0 ]; then
        tests=()
        have_custom_tests=1
    fi

    tests+=("$1")
}

restore_generated_source() {
    local generated_c=$1
    local backup_c=$2
    local had_backup=$3

    if [ "$had_backup" -ne 0 ]; then
        cp "$backup_c" "$generated_c"
    else
        rm -f "$generated_c"
    fi
}

generate_fbcunit_support_source() {
    local bas_name=$1
    local abs_bas="$fbcunit_src_dir/$bas_name.bas"
    local generated_c="$fbcunit_src_dir/$bas_name.c"
    local staged_c="$out_dir/$bas_name.c"
    local backup_c="$out_dir/.backup-$bas_name.c"
    local had_backup=0

    [ -f "$abs_bas" ] || die "missing fbcunit source: $abs_bas"

    if [ -f "$generated_c" ]; then
        cp "$generated_c" "$backup_c"
        had_backup=1
    else
        rm -f "$backup_c"
    fi

    printf 'nuttx-fbctests-gen: generate %-18s %s\n' "$bas_name" "tests/fbcunit/src/$bas_name.bas"

    "$fbc" -target "$target" -d FBCU_NO_INCLIB -i "$include_dir" -i "$fbcunit_include_dir" -i "$fbcunit_src_dir" -gen gcc -r "$abs_bas"

    [ -f "$generated_c" ] || die "fbc did not produce C output: $generated_c"
    cp "$generated_c" "$staged_c"
    restore_generated_source "$generated_c" "$backup_c" "$had_backup"
    rm -f "$backup_c"
}

generate_fbcunit_support() {
    generate_fbcunit_support_source fbcunit
    generate_fbcunit_support_source fbcunit_qb
    generate_fbcunit_support_source fbcunit_console
    generate_fbcunit_support_source fbcunit_report
}

generate_case() {
    local spec=$1
    local app_name
    local bas_path
    local lang
    local abs_bas
    local bas_dir
    local bas_base
    local generated_c
    local staged_c
    local backup_c
    local had_backup=0

    IFS=: read -r app_name bas_path lang <<<"$spec"

    [ -n "${app_name:-}" ] || die "empty app name in test spec: $spec"
    [ -n "${bas_path:-}" ] || die "empty test path in test spec: $spec"

    case "$bas_path" in
        /*)
            abs_bas=$bas_path
            ;;
        *)
            abs_bas="$repo_root/$bas_path"
            ;;
    esac

    [ -f "$abs_bas" ] || die "missing test source: $abs_bas"

    bas_dir=$(dirname -- "$abs_bas")
    bas_base=$(basename -- "$abs_bas" .bas)
    generated_c="$bas_dir/$bas_base.c"
    staged_c="$out_dir/$app_name.c"
    backup_c="$out_dir/.backup-$app_name.c"

    if [ -f "$generated_c" ]; then
        cp "$generated_c" "$backup_c"
        had_backup=1
    else
        rm -f "$backup_c"
    fi

    printf 'nuttx-fbctests-gen: generate %-18s %s\n' "$app_name" "$bas_path"

    if [ -n "${lang:-}" ]; then
        "$fbc" -target "$target" -lang "$lang" -i "$include_dir" -i "$fbcunit_include_dir" -gen gcc -r "$abs_bas"
    else
        "$fbc" -target "$target" -i "$include_dir" -i "$fbcunit_include_dir" -gen gcc -r "$abs_bas"
    fi

    [ -f "$generated_c" ] || die "fbc did not produce C output: $generated_c"
    if grep -q '_ZN4FBCU' "$generated_c"; then
        need_fbcunit=1
    fi

    cp "$generated_c" "$staged_c"
    restore_generated_source "$generated_c" "$backup_c" "$had_backup"
    rm -f "$backup_c"

    printf '%s:%s\n' "$app_name" "$app_name.c" >> "$out_dir/$manifest_name"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --fbc)
            [ "$#" -ge 2 ] || die "--fbc requires a path"
            fbc=$2
            shift 2
            ;;
        --target)
            [ "$#" -ge 2 ] || die "--target requires a target alias"
            target=$2
            shift 2
            ;;
        --output-dir)
            [ "$#" -ge 2 ] || die "--output-dir requires a directory"
            out_dir=$2
            shift 2
            ;;
        --test)
            [ "$#" -ge 2 ] || die "--test requires appname:path-to-bas"
            add_custom_test "$2"
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

[ -x "$fbc" ] || [ -f "$fbc" ] || die "missing fbc executable: $fbc"

mkdir -p "$out_dir"
: > "$out_dir/$manifest_name"

printf 'nuttx-fbctests-gen: fbc        %s\n' "$fbc"
printf 'nuttx-fbctests-gen: target     %s\n' "$target"
printf 'nuttx-fbctests-gen: output dir %s\n' "$out_dir"

for test_spec in "${tests[@]}"; do
    generate_case "$test_spec"
done

if [ "$need_fbcunit" -ne 0 ]; then
    generate_fbcunit_support
fi

printf 'nuttx-fbctests-gen: manifest   %s\n' "$out_dir/$manifest_name"
printf 'nuttx-fbctests-gen: done\n'

# end of nuttx-riscv32-generate-fbctests.sh
