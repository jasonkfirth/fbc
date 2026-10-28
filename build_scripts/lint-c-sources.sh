#!/usr/bin/env bash

##############################################################################
# FreeBASIC C and C++ whole-tree lint runner
##############################################################################
#
# Purpose:
#
#   Run Cppcheck over every present first-party C and C++ source file,
#   including working-tree additions and platform sources inactive on the CI
#   host.
#
# Responsibilities:
#
#   * derive the source manifest from the current Git worktree
#   * exclude only an explicitly vendored implementation and generated output
#   * keep known platform-model limitations narrow and visible
#   * fail when Cppcheck reports a new warning, error, or portability issue
#
# This script intentionally does NOT contain:
#
#   * compiler or linker invocations for a target platform
#   * linting of vendored decoder implementations and headers
#   * automatic source mutation or diagnostic suppression generation
#
##############################################################################

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
temp_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}"

mkdir -p "$temp_dir"
source_list="$(mktemp "$temp_dir/fbc-cppcheck-sources.XXXXXX")"
manifest_list="$(mktemp "$temp_dir/fbc-cppcheck-manifest.XXXXXX")"
lint_log="$(mktemp "$temp_dir/fbc-cppcheck-log.XXXXXX")"
trap 'rm -f -- "$source_list" "$manifest_list" "$lint_log"' EXIT

cd "$repo_root"

command -v git >/dev/null 2>&1 || {
    echo "ERROR: git is required to build the C/C++ lint manifest" >&2
    exit 2
}
command -v cppcheck >/dev/null 2>&1 || {
    echo "ERROR: cppcheck is required for C/C++ linting" >&2
    exit 2
}

git ls-files --cached -z -- \
    ':(glob)**/*.c' \
    ':(glob)**/*.cpp' \
    > "$manifest_list"

# Include working-tree files added within the runtime libraries under review.
# Avoid a repository-wide untracked walk: developer worktrees may contain
# junctions or mounted paths that lead outside the checkout.
git ls-files --others --exclude-standard -z -- \
    ':(glob)src/rtlib/**/*.c' \
    ':(glob)src/rtlib/**/*.cpp' \
    ':(glob)src/gfxlib2/**/*.c' \
    ':(glob)src/gfxlib2/**/*.cpp' \
    ':(glob)src/gfxlib3/**/*.c' \
    ':(glob)src/gfxlib3/**/*.cpp' \
    ':(glob)src/sfxlib/**/*.c' \
    ':(glob)src/sfxlib/**/*.cpp' \
    >> "$manifest_list"

# The source tree may contain moves that are still unstaged.  Git's index can
# then name deleted paths while the replacement files are untracked.  Read
# both sets, keep only files that exist now, and preserve arbitrary spaces in
# names while building Cppcheck's line-based file list.
while IFS= read -r -d '' source_path; do
    if [ ! -f "$source_path" ]; then
        continue
    fi

    case "$source_path" in
        examples/manual/libraries/bassmod.fbc-064CA364.c \
        |src/sfxlib/third_party/stb_vorbis.c \
        |tests/boolean/boolean_file.c \
        |tests/fblite/command-sweep.c \
        |tests/functions/array_param.c \
        |tests/functions/mangling-procptr-callconv-1.c \
        |tests/functions/static-local-access.c \
        |tests/functions/void_param.c \
        |tests/numbers/infnan.c \
        |tests/pretest/compile_and_run_ok.c \
        |tests/pretest/compile_only_ok.c \
        |tests/wince/basic_file.c)
            continue
            ;;
    esac

    case "$source_path" in
        *$'\n'*|*$'\r'*)
            echo "ERROR: Cppcheck file lists cannot represent this source path" >&2
            exit 2
            ;;
    esac

    printf '%s\n' "$source_path" >> "$source_list"
done < "$manifest_list"

LC_ALL=C sort -u "$source_list" -o "$source_list"

source_count="$(wc -l < "$source_list")"
if [ "$source_count" -eq 0 ]; then
    echo "ERROR: the C/C++ lint manifest is empty" >&2
    exit 2
fi

echo "Cppcheck source manifest: $source_count first-party files"
for library in rtlib gfxlib2 gfxlib3 sfxlib; do
    library_count="$(grep -c "^src/$library/" "$source_list" || true)"
    if [ "$library_count" -eq 0 ]; then
        echo "ERROR: source manifest has no C/C++ files for $library" >&2
        exit 2
    fi
    echo "  $library: $library_count C/C++ files"
done

# Ubuntu 24.04 ships Cppcheck 2.13, while developer machines may use newer
# releases.  Some parser limitations have different diagnostic identifiers
# between those versions, so both narrow names are retained where necessary.
if ! cppcheck \
    --enable=warning,performance,portability \
    --error-exitcode=1 \
    --inline-suppr \
    --max-configs=1 \
    --quiet \
    --suppress=missingIncludeSystem \
    --suppress='*:src/sfxlib/third_party/*' \
    --suppress='uninitMemberVarNoCtor:src/gfxlib2/drivers/haiku/*' \
    --suppress='uninitMemberVarNoCtor:src/rtlib/fb_file.h' \
    --suppress='uninitMemberVarNoCtor:src/sfxlib/haiku/sfx_driver_haiku.cpp' \
    --suppress='CastAddressToIntegerAtReturn:src/gfxlib2/drivers/aros/gfx_unix.c' \
    --suppress='CastAddressToIntegerAtReturn:src/gfxlib2/drivers/haiku/haiku_window.cpp' \
    --suppress='comparePointers:contrib/djgpp/libc/crt0/_main.c' \
    --suppress='comparePointers:src/gfxlib2/drivers/dos/gfx_dos.c' \
    --suppress='comparePointers:src/rtlib/drivers/dos/sys_isr.c' \
    --suppress='ctunullpointer:src/rtlib/arrays/array_destructstr.c' \
    --suppress='ctunullpointer:src/rtlib/fb_string.h:147' \
    --suppress='ctuOneDefinitionRuleViolation:tests/cpp/*' \
    --suppress='duplInheritedMember:tests/cpp/derived-cpp.cpp' \
    --suppress='incorrectLogicOperator:src/rtlib/drivers/wince/mips32el/crt_compat.c' \
    --suppress='internalAstError:src/gfxlib2/drivers/js/gfx_driver.c' \
    --suppress='internalAstError:src/rtlib/drivers/js/hinit.c' \
    --suppress='memleak:src/rtlib/drivers/wii/thread_ctx.c' \
    --suppress='nullPointer:src/rtlib/fb_string.h:147' \
    --suppress='nullPointer:src/rtlib/strings/str_format.c' \
    --suppress='nullPointer:src/rtlib/drivers/xbox/str_format.c' \
    --suppress='preprocessorErrorDirective:src/rtlib/fb_config.h' \
    --suppress='preprocessorErrorDirective:src/sfxlib/unix/sfx_driver_oss_template.inc' \
    --suppress='resourceLeak:src/rtlib/drivers/dos/file_hreset.c' \
    --suppress='passedByValue:tests/cpp/mangle-struct-cpp.cpp' \
    --suppress='subtractPointers:src/gfxlib2/drivers/dos/gfx_dos.c' \
    --suppress='subtractPointers:src/rtlib/drivers/dos/sys_isr.c' \
    --suppress='syntaxError:src/gfxlib2/drivers/js/gfx_driver.c' \
    --suppress='syntaxError:src/rtlib/drivers/dos/profile_cycles.c' \
    --suppress='syntaxError:src/rtlib/drivers/js/hinit.c' \
    --suppress='syntaxError:src/rtlib/profiling/profile_cycles.c' \
    --suppress='syntaxError:src/rtlib/drivers/wince/profile_cycles.c' \
    --suppress='syntaxError:src/rtlib/drivers/xbox/cxxabi.c' \
    --suppress='syntaxError:src/sfxlib/js/sfx_driver_webaudio.c' \
    --suppress='syntaxError:build_scripts/wince/mips-toolchain/crt0.c' \
    --suppress='syntaxError:contrib/djgpp/libc/crt0/_main.c' \
    --suppress='unknownMacro:src/rtlib/drivers/aros/static/fbrt0.c' \
    --suppress='unknownMacro:src/rtlib/drivers/aros/static/fbrt1.c' \
    --suppress='unknownMacro:src/rtlib/drivers/aros/static/fbrt2.c' \
    --suppress='unknownMacro:src/rtlib/drivers/dos/symb_reg.txt' \
    --suppress='unknownMacro:src/rtlib/drivers/js/io_gety.c' \
    -Isrc \
    -Isrc/rtlib \
    -Isrc/gfxlib2 \
    -Isrc/gfxlib3 \
    -Isrc/sfxlib \
    --file-list="$source_list" \
    2> "$lint_log"; then
    cat "$lint_log" >&2
    # Keep the same diagnostics for the Actions failure artifact. The
    # temporary working log is removed by the exit trap.
    if [ -n "${RUNNER_TEMP:-}" ]; then
        cp "$lint_log" "$RUNNER_TEMP/fbc-cppcheck.log"
    fi
    exit 1
fi

##############################################################################
# end of lint-c-sources.sh
##############################################################################
