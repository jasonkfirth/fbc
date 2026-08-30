#!/usr/bin/env bash

##############################################################################
# FreeBASIC C and C++ whole-tree lint runner
##############################################################################
#
# Purpose:
#
#   Run Cppcheck over every tracked first-party C and C++ source file,
#   including platform and test sources that are inactive on the CI host.
#
# Responsibilities:
#
#   * derive the source manifest directly from Git
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
source_list="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/fbc-cppcheck-sources.txt"

cd "$repo_root"

command -v git >/dev/null 2>&1 || {
    echo "ERROR: git is required to build the C/C++ lint manifest" >&2
    exit 2
}
command -v cppcheck >/dev/null 2>&1 || {
    echo "ERROR: cppcheck is required for C/C++ linting" >&2
    exit 2
}

git ls-files \
    ':(glob)**/*.c' \
    ':(glob)**/*.cpp' \
    | grep -Fvx \
        -e 'examples/manual/libraries/bassmod.fbc-064CA364.c' \
        -e 'src/sfxlib/third_party/stb_vorbis.c' \
        -e 'tests/boolean/boolean_file.c' \
        -e 'tests/fblite/command-sweep.c' \
        -e 'tests/functions/array_param.c' \
        -e 'tests/functions/mangling-procptr-callconv-1.c' \
        -e 'tests/functions/static-local-access.c' \
        -e 'tests/functions/void_param.c' \
        -e 'tests/numbers/infnan.c' \
        -e 'tests/pretest/compile_and_run_ok.c' \
        -e 'tests/pretest/compile_only_ok.c' \
        -e 'tests/wince/basic_file.c' \
    > "$source_list"

source_count="$(wc -l < "$source_list")"
if [ "$source_count" -eq 0 ]; then
    echo "ERROR: the C/C++ lint manifest is empty" >&2
    exit 2
fi

echo "Cppcheck source manifest: $source_count first-party files"

cppcheck \
    --enable=warning,performance,portability \
    --error-exitcode=1 \
    --inline-suppr \
    --max-configs=1 \
    --quiet \
    --suppress=missingIncludeSystem \
    --suppress='*:src/sfxlib/third_party/*' \
    --suppress='uninitMemberVarNoCtor:src/gfxlib2/haiku/*' \
    --suppress='uninitMemberVarNoCtor:src/rtlib/fb_file.h' \
    --suppress='uninitMemberVarNoCtor:src/sfxlib/haiku/sfx_driver_haiku.cpp' \
    --suppress='CastAddressToIntegerAtReturn:src/gfxlib2/aros/gfx_unix.c' \
    --suppress='CastAddressToIntegerAtReturn:src/gfxlib2/haiku/haiku_window.cpp' \
    --suppress='ctunullpointer:src/rtlib/array_destructstr.c' \
    --suppress='ctuOneDefinitionRuleViolation:tests/cpp/*' \
    --suppress='duplInheritedMember:tests/cpp/derived-cpp.cpp' \
    --suppress='incorrectLogicOperator:src/rtlib/wince/mips32el/crt_compat.c' \
    --suppress='memleak:src/rtlib/wii/thread_ctx.c' \
    --suppress='preprocessorErrorDirective:src/rtlib/fb_config.h' \
    --suppress='preprocessorErrorDirective:src/sfxlib/unix/sfx_driver_oss_template.inc' \
    --suppress='resourceLeak:src/rtlib/dos/file_hreset.c' \
    --suppress='passedByValue:tests/cpp/mangle-struct-cpp.cpp' \
    --suppress='subtractPointers:src/gfxlib2/dos/gfx_dos.c' \
    --suppress='subtractPointers:src/rtlib/dos/sys_isr.c' \
    --suppress='syntaxError:src/gfxlib2/js/gfx_driver.c' \
    --suppress='syntaxError:src/rtlib/dos/profile_cycles.c' \
    --suppress='syntaxError:src/rtlib/js/hinit.c' \
    --suppress='syntaxError:src/rtlib/profile_cycles.c' \
    --suppress='syntaxError:src/rtlib/wince/profile_cycles.c' \
    --suppress='syntaxError:src/rtlib/xbox/cxxabi.c' \
    --suppress='syntaxError:src/sfxlib/js/sfx_driver_webaudio.c' \
    --suppress='syntaxError:build_scripts/wince/mips-toolchain/crt0.c' \
    --suppress='syntaxError:contrib/djgpp/libc/crt0/_main.c' \
    --suppress='unknownMacro:src/rtlib/aros/static/fbrt0.c' \
    --suppress='unknownMacro:src/rtlib/aros/static/fbrt1.c' \
    --suppress='unknownMacro:src/rtlib/aros/static/fbrt2.c' \
    --suppress='unknownMacro:src/rtlib/dos/symb_reg.txt' \
    --suppress='unknownMacro:src/rtlib/js/io_gety.c' \
    -Isrc \
    -Isrc/rtlib \
    -Isrc/gfxlib2 \
    -Isrc/gfxlib3 \
    -Isrc/sfxlib \
    --file-list="$source_list"

##############################################################################
# end of lint-c-sources.sh
##############################################################################
