#!/usr/bin/env bash
#
#   Project: FreeBASIC NuttX smoke testing
#   -------------------------------------
#
#   File: nuttx-riscv32-qemu-suite.sh
#
#   Purpose:
#
#       Run a small serial suite of FreeBASIC-generated C programs under the
#       NuttX RISC-V QEMU smoke-test harness.
#
#   Responsibilities:
#
#       * select generated C files to test
#       * run each test through nuttx-riscv32-qemu-smoke.sh
#       * keep QEMU memory and timeout settings consistent across the suite
#       * serialize runs so the shared NuttX app tree is not modified by two
#         smoke tests at the same time
#
#   This file intentionally does NOT contain:
#
#       * FreeBASIC-to-C generation
#       * NuttX configuration logic
#       * compiler feature probing
#       * target-specific runtime implementation
#

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)

smoke_script="$script_dir/nuttx-riscv32-qemu-smoke.sh"
nuttx_workdir=${NUTTX_WORKDIR:-/work}
generated_dir="$repo_root/generated"
memory=${QEMU_MEMORY:-2G}
timeout=${QEMU_TIMEOUT:-30}
skip_missing=0
skip_nuttx_config=0
have_custom_tests=0
manifest_files=()

tests=(
    "fbhello:fbhello.c"
    "fbbuiltin:crt_builtin.c"
    "fbpretest:pretest_compile_and_run_ok.c"
    "fbgfx:fbgfx_smoke.c:gfx-mock"
)

usage() {
    cat <<'EOF'
Usage:
  nuttx-riscv32-qemu-suite.sh [options]

Options:
  --nuttx-workdir DIR     NuttX workspace mounted in the container.
                          Default: $NUTTX_WORKDIR or /work

  --generated-dir DIR     Directory containing generated C files.
                          Default: <repo>/generated

  --memory SIZE           QEMU memory setting passed as QEMU_MEMORY.
                          Default: $QEMU_MEMORY or 2G

  --timeout SECONDS       QEMU timeout setting passed as QEMU_TIMEOUT.
                          Default: $QEMU_TIMEOUT or 30

  --test SPEC             Add one test. The first --test replaces the default
                          suite. Format: appname:path[+path...][:mode]

                          If path is relative, it is resolved under
                          --generated-dir. Mode may be gfx, mock, gfx-mock,
                          runfail, or gfx-runfail.

  --manifest FILE         Add tests from a text manifest. The first --test or
                          --manifest replaces the default suite.

                          Each non-empty, non-comment line uses the same
                          appname:path[:mode] format as --test.

  --skip-missing          Skip missing generated C files instead of failing.

  --skip-nuttx-config     Keep the existing NuttX board configuration and let
                          each case only update the app-specific settings.

  -h, --help              Show this help.

Default suite:
  fbhello   generated/fbhello.c
  fbbuiltin generated/crt_builtin.c
  fbpretest generated/pretest_compile_and_run_ok.c
  fbgfx     generated/fbgfx_smoke.c with gfxlib
EOF
}

die() {
    printf 'nuttx-suite: %s\n' "$*" >&2
    exit 1
}

split_test_spec() {
    local spec=$1
    local spec_body

    spec_app_name=${spec%%:*}
    spec_body=${spec#*:}
    spec_c_files=
    spec_mode=

    [ "$spec_body" != "$spec" ] || return 1

    case "$spec_body" in
        *:gfx-runfail)
            spec_c_files=${spec_body%:gfx-runfail}
            spec_mode=gfx-runfail
            ;;
        *:runfail-gfx)
            spec_c_files=${spec_body%:runfail-gfx}
            spec_mode=runfail-gfx
            ;;
        *:gfx-mock)
            spec_c_files=${spec_body%:gfx-mock}
            spec_mode=gfx-mock
            ;;
        *:mock-gfx)
            spec_c_files=${spec_body%:mock-gfx}
            spec_mode=mock-gfx
            ;;
        *:runfail)
            spec_c_files=${spec_body%:runfail}
            spec_mode=runfail
            ;;
        *:mock)
            spec_c_files=${spec_body%:mock}
            spec_mode=mock
            ;;
        *:gfx)
            spec_c_files=${spec_body%:gfx}
            spec_mode=gfx
            ;;
        *)
            spec_c_files=$spec_body
            ;;
    esac

    return 0
}

add_custom_test() {
    if [ "$have_custom_tests" -eq 0 ]; then
        tests=()
        have_custom_tests=1
    fi

    tests+=("$1")
}

manifest_spec_from_dir() {
    local spec=$1
    local manifest_dir=$2
    local generated_abs=$3
    local manifest_rel
    local app_name
    local c_files
    local mode
    local c_piece
    local c_prefix
    local new_files
    local sep
    local -a pieces

    case "$manifest_dir/" in
        "$generated_abs"/*)
            manifest_rel=${manifest_dir#"$generated_abs"/}
            ;;
        *)
            printf '%s\n' "$spec"
            return
            ;;
    esac

    [ -n "$manifest_rel" ] || {
        printf '%s\n' "$spec"
        return
    }

    split_test_spec "$spec" || {
        printf '%s\n' "$spec"
        return
    }

    app_name=$spec_app_name
    c_files=$spec_c_files
    mode=$spec_mode

    [ -n "${app_name:-}" ] || {
        printf '%s\n' "$spec"
        return
    }

    [ -n "${c_files:-}" ] || {
        printf '%s\n' "$spec"
        return
    }

    IFS=+ read -r -a pieces <<<"$c_files"
    new_files=
    sep=

    for c_piece in "${pieces[@]}"; do
        c_prefix=

        case "$c_piece" in
            gen:* | c:* | cxx:* | asm:*)
                c_prefix=${c_piece%%:*}:
                c_piece=${c_piece#*:}
                ;;
        esac

        case "$c_piece" in
            /* | "$manifest_rel"/*)
                ;;
            *)
                c_piece=$manifest_rel/$c_piece
                ;;
        esac

        new_files=$new_files$sep$c_prefix$c_piece
        sep=+
    done

    if [ -n "${mode:-}" ]; then
        printf '%s:%s:%s\n' "$app_name" "$new_files" "$mode"
    else
        printf '%s:%s\n' "$app_name" "$new_files"
    fi
}

load_manifest() {
    local manifest=$1
    local manifest_dir
    local generated_abs
    local line

    [ -f "$manifest" ] || die "missing manifest: $manifest"

    manifest_dir=$(cd "$(dirname "$manifest")" && pwd -P)
    generated_abs=$(cd "$generated_dir" && pwd -P)

    while IFS= read -r line || [ -n "$line" ]; do
        line=${line%$'\r'}

        case "$line" in
            "" | "#"*)
                continue
                ;;
        esac

        line=$(manifest_spec_from_dir "$line" "$manifest_dir" "$generated_abs")

        add_custom_test "$line"
    done < "$manifest"
}

resolve_c_file() {
    local c_file=$1

    case "$c_file" in
        /*)
            printf '%s\n' "$c_file"
            ;;
        *)
            printf '%s/%s\n' "$generated_dir" "$c_file"
            ;;
    esac
}

run_case() {
    local spec=$1
    local app_name
    local c_file
    local mode
    local with_gfx=0
    local qemu_mock_devices=0
    local expect_fail=0
    local resolved_c_file
    local c_piece
    local c_kind
    local -a c_files
    local -a command

    split_test_spec "$spec" || die "invalid test spec: $spec"

    app_name=$spec_app_name
    c_file=$spec_c_files
    mode=$spec_mode

    [ -n "${app_name:-}" ] || die "empty app name in test spec: $spec"
    [ -n "${c_file:-}" ] || die "empty generated C file in test spec: $spec"

    case "$mode" in
        "")
            ;;
        "gfx")
            with_gfx=1
            ;;
        "runfail")
            expect_fail=1
            ;;
        "gfx-runfail" | "runfail-gfx")
            with_gfx=1
            expect_fail=1
            ;;
        "mock")
            qemu_mock_devices=1
            ;;
        "gfx-mock" | "mock-gfx")
            with_gfx=1
            qemu_mock_devices=1
            ;;
        *)
            die "unknown test mode '$mode' in spec: $spec"
            ;;
    esac

    IFS=+ read -r -a c_files <<<"$c_file"
    [ "${#c_files[@]}" -gt 0 ] || die "empty generated C file in test spec: $spec"

    resolved_c_file=$(resolve_c_file "${c_files[0]}")

    if [ ! -f "$resolved_c_file" ]; then
        if [ "$skip_missing" -ne 0 ]; then
            printf 'nuttx-suite: skip %-12s missing %s\n' "$app_name" "$resolved_c_file"
            return 0
        fi

        die "missing generated C file for $app_name: $resolved_c_file"
    fi

    command=(
        bash "$smoke_script"
        --nuttx-workdir "$nuttx_workdir"
        --generated-c "$resolved_c_file"
        --app-name "$app_name"
    )

    if [ "$skip_nuttx_config" -ne 0 ]; then
        command+=(--skip-nuttx-config)
    fi

    if [ "${#c_files[@]}" -gt 1 ]; then
        for c_piece in "${c_files[@]:1}"; do
            c_kind=generated

            case "$c_piece" in
                gen:*)
                    c_piece=${c_piece#gen:}
                    ;;
                c:*)
                    c_kind=c
                    c_piece=${c_piece#c:}
                    ;;
                cxx:*)
                    c_kind=cxx
                    c_piece=${c_piece#cxx:}
                    ;;
                asm:*)
                    c_kind=asm
                    c_piece=${c_piece#asm:}
                    ;;
            esac

            c_piece=$(resolve_c_file "$c_piece")

            if [ ! -f "$c_piece" ]; then
                if [ "$skip_missing" -ne 0 ]; then
                    printf 'nuttx-suite: skip %-12s missing %s\n' "$app_name" "$c_piece"
                    return 0
                fi

                die "missing generated C file for $app_name: $c_piece"
            fi

            case "$c_kind" in
                generated)
                    command+=(--extra-generated-c "$c_piece")
                    ;;
                c)
                    command+=(--extra-c "$c_piece")
                    ;;
                cxx)
                    command+=(--extra-cxx "$c_piece")
                    ;;
                asm)
                    command+=(--extra-asm "$c_piece")
                    ;;
            esac
        done
    fi

    if [ "$with_gfx" -eq 1 ]; then
        command+=(--with-gfxlib)
    fi

    if [ "$qemu_mock_devices" -eq 1 ]; then
        command+=(--qemu-mock-devices)
    fi

    if [ "$expect_fail" -eq 1 ]; then
        command+=(--expect-fail)
    fi

    printf '\n'
    printf 'nuttx-suite: run %-12s %s\n' "$app_name" "$resolved_c_file"

    QEMU_MEMORY="$memory" QEMU_TIMEOUT="$timeout" "${command[@]}"

    printf 'nuttx-suite: ok  %-12s\n' "$app_name"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --nuttx-workdir)
            [ "$#" -ge 2 ] || die "--nuttx-workdir requires a directory"
            nuttx_workdir=$2
            shift 2
            ;;
        --generated-dir)
            [ "$#" -ge 2 ] || die "--generated-dir requires a directory"
            generated_dir=$2
            shift 2
            ;;
        --memory)
            [ "$#" -ge 2 ] || die "--memory requires a QEMU memory size"
            memory=$2
            shift 2
            ;;
        --timeout)
            [ "$#" -ge 2 ] || die "--timeout requires seconds"
            timeout=$2
            shift 2
            ;;
        --test)
            [ "$#" -ge 2 ] || die "--test requires appname:path[:gfx]"
            add_custom_test "$2"
            shift 2
            ;;
        --manifest)
            [ "$#" -ge 2 ] || die "--manifest requires a file"
            if [ "$have_custom_tests" -eq 0 ]; then
                tests=()
                have_custom_tests=1
            fi
            manifest_files+=("$2")
            shift 2
            ;;
        --skip-missing)
            skip_missing=1
            shift
            ;;
        --skip-nuttx-config)
            skip_nuttx_config=1
            shift
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

[ -f "$smoke_script" ] || die "missing smoke harness: $smoke_script"
[ -d "$nuttx_workdir" ] || die "missing NuttX workdir: $nuttx_workdir"
[ -d "$generated_dir" ] || die "missing generated C directory: $generated_dir"

for manifest_file in "${manifest_files[@]}"; do
    load_manifest "$manifest_file"
done

printf 'nuttx-suite: workdir       %s\n' "$nuttx_workdir"
printf 'nuttx-suite: generated dir %s\n' "$generated_dir"
printf 'nuttx-suite: qemu memory   %s\n' "$memory"
printf 'nuttx-suite: qemu timeout  %s\n' "$timeout"

for test_spec in "${tests[@]}"; do
    run_case "$test_spec"
done

printf '\n'
printf 'nuttx-suite: all tests passed\n'

# end of nuttx-riscv32-qemu-suite.sh
