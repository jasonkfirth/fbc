#!/usr/bin/env bash
#
#   Project: FreeBASIC NuttX smoke testing
#   -------------------------------------
#
#   File: nuttx-riscv32-generate-examples.sh
#
#   Purpose:
#
#       Generate stable C sources for the NuttX/RISC-V example smoke tests
#       used by the QEMU device lab.
#
#   Responsibilities:
#
#       * invoke a host fbc that knows the NuttX RISC-V target
#       * compile selected examples/nuttx/*.bas files to generated C
#       * use a temporary source copy so examples/nuttx is not dirtied
#       * copy each generated C file into the shared device-lab staging
#         directory
#       * write a small manifest describing the staged example sources
#
#   This file intentionally does NOT contain:
#
#       * QEMU execution logic
#       * NuttX app-tree staging
#       * controller flashing or mounted-drive writes
#

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)

fbc=${FBC:-"$repo_root/bin/fbc.exe"}
target=${FB_NUTTX_TARGET:-nuttx-riscv32}
out_dir=${FB_NUTTX_GENERATED_C_DIR:-"$repo_root/.build-rp2350-pizero-generated"}
manifest_name="examples-manifest.txt"
tmp_root=${TMPDIR:-/tmp}

examples=(
    "fbhello:examples/nuttx/fbhello.bas"
    "fbhello_smoke:examples/nuttx/fbhello_smoke.bas"
    "fbgosub_smoke:examples/nuttx/fbgosub_smoke.bas"
    "fbfre_smoke:examples/nuttx/fbfre_smoke.bas"
    "fbfilecopy_smoke:examples/nuttx/fbfilecopy_smoke.bas"
    "fbmathcvn_smoke:examples/nuttx/fbmathcvn_smoke.bas"
    "fbfrac_smoke:examples/nuttx/fbfrac_smoke.bas"
    "fbsign_smoke:examples/nuttx/fbsign_smoke.bas"
    "fbgfx_smoke:examples/nuttx/fbgfx_smoke.bas"
    "fbgfx_hid_smoke:examples/nuttx/fbgfx_hid_smoke.bas"
    "fbgfx_sfx_smoke:examples/nuttx/fbgfx_sfx_smoke.bas"
    "fbsd_smoke:examples/nuttx/fbsd_smoke.bas"
    "fbsfx_smoke:examples/nuttx/fbsfx_smoke.bas"
)

usage() {
    cat <<'EOF'
Usage:
  nuttx-riscv32-generate-examples.sh [options]

Options:
  --fbc PATH              Host fbc executable.
                          Default: $FBC or <repo>/bin/fbc.exe

  --target TARGET         fbc target alias.
                          Default: $FB_NUTTX_TARGET or nuttx-riscv32

  --output-dir DIR        Directory for generated C files and manifest.
                          Default: $FB_NUTTX_GENERATED_C_DIR or
                          <repo>/.build-rp2350-pizero-generated

  --example SPEC          Add one example. The first --example replaces the
                          default slice. Format: output-name:path-to-bas

  -h, --help              Show this help.

Default slice:
  examples/nuttx/fbhello.bas
  examples/nuttx/fbhello_smoke.bas
  examples/nuttx/fbgosub_smoke.bas
  examples/nuttx/fbfre_smoke.bas
  examples/nuttx/fbfilecopy_smoke.bas
  examples/nuttx/fbmathcvn_smoke.bas
  examples/nuttx/fbfrac_smoke.bas
  examples/nuttx/fbsign_smoke.bas
  examples/nuttx/fbgfx_smoke.bas
  examples/nuttx/fbgfx_hid_smoke.bas
  examples/nuttx/fbgfx_sfx_smoke.bas
  examples/nuttx/fbsd_smoke.bas
  examples/nuttx/fbsfx_smoke.bas
EOF
}

die() {
    printf 'nuttx-examples-gen: %s\n' "$*" >&2
    exit 1
}

add_custom_example() {
    if [ "${have_custom_examples:-0}" -eq 0 ]; then
        examples=()
        have_custom_examples=1
    fi

    examples+=("$1")
}

generate_example() {
    local spec=$1
    local output_name
    local bas_path
    local abs_bas
    local bas_base
    local tmp_dir
    local tmp_bas
    local tmp_c
    local staged_c

    IFS=: read -r output_name bas_path <<<"$spec"

    [ -n "${output_name:-}" ] || die "empty output name in example spec: $spec"
    [ -n "${bas_path:-}" ] || die "empty BASIC path in example spec: $spec"

    case "$output_name" in
        *[!A-Za-z0-9_]*)
            die "output name must contain only letters, numbers, and underscores: $output_name"
            ;;
    esac

    case "$bas_path" in
        /*)
            abs_bas=$bas_path
            ;;
        *)
            abs_bas="$repo_root/$bas_path"
            ;;
    esac

    [ -f "$abs_bas" ] || die "missing BASIC source: $abs_bas"

    bas_base=$(basename -- "$abs_bas" .bas)
    staged_c="$out_dir/$output_name.c"
    tmp_dir=$(mktemp -d "$tmp_root/fb-nuttx-example-gen.XXXXXX")

    tmp_bas="$tmp_dir/$bas_base.bas"
    tmp_c="$tmp_dir/$bas_base.c"

    (
        trap 'rm -rf "$tmp_dir"' EXIT

        cp "$abs_bas" "$tmp_bas"

        printf 'nuttx-examples-gen: generate %-16s %s\n' "$output_name" "$bas_path"

        "$fbc" -target "$target" -gen gcc -r "$tmp_bas" -x "$tmp_dir/$output_name"

        [ -f "$tmp_c" ] || die "fbc did not produce C output: $tmp_c"

        cp "$tmp_c" "$staged_c"
        printf '%s:%s\n' "$output_name" "$output_name.c" >> "$out_dir/$manifest_name"
    )
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
        --example)
            [ "$#" -ge 2 ] || die "--example requires output-name:path-to-bas"
            add_custom_example "$2"
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

printf 'nuttx-examples-gen: fbc        %s\n' "$fbc"
printf 'nuttx-examples-gen: target     %s\n' "$target"
printf 'nuttx-examples-gen: output dir %s\n' "$out_dir"

for example_spec in "${examples[@]}"; do
    generate_example "$example_spec"
done

printf 'nuttx-examples-gen: manifest   %s\n' "$out_dir/$manifest_name"
printf 'nuttx-examples-gen: done\n'

# end of nuttx-riscv32-generate-examples.sh
