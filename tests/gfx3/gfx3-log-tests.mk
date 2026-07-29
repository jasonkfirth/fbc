# gfx3-log-tests.mk
#
# Purpose:
#
#     Re-run the unchanged non-fbcunit gfx log cases with gfxlib3 selected.
#
# Responsibilities:
#
#     - limit log-tests.mk to tests/gfx
#     - pass the public -gfx3 option to compiler-only and run-time test paths
#     - link run-time fixtures against gfxlib3 rather than gfxlib2
#
# This file intentionally does NOT contain:
#
#     - copied test recipes
#     - edits to tests/gfx sources
#     - a replacement compiler-log harness

DIRLIST_INC := gfx3/gfx-only-dirlist.mk

include log-tests.mk

# The compiler pre-includes fbgfx3-option.bi for -gfx3. Its bare selection
# define matches existing source-level gfxlib3 selects, so the public compiler
# option now works for both ordinary and source-selected tests.
override FBC := $(abspath ../bin/fbc.exe)
override FBC_CFLAGS += -gfx3
override FBC_LFLAGS += -gfx3

# bmk-make.mk is recursive. Export the public option so it reaches per-module
# compilation before object metadata is emitted.
export FBC_EXTRA_CFLAGS := -gfx3

# The recursive bmk harness otherwise falls back to a bare gcc that is not on
# this Windows session's PATH. This is the active FreeBASIC MinGW toolchain.
override GCC := C:/freebasic-js/toolchain/ucrt64/bin/gcc.exe

# end of gfx3-log-tests.mk
