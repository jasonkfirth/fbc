# gfx3-unit-tests.mk
#
# Purpose:
#
#     Force-rebuild the unchanged fbcunit graphics suites with gfxlib3 selected.
#     This wrapper is run from tests/ with:
#
#         make -B -f gfx3/gfx3-unit-tests.mk all
#
# Responsibilities:
#
#     - limit unit-tests.mk to the fbcunit gfx directory
#     - add -gfx3 to every test compilation after stock flags are assembled
#     - link a separate executable against gfxlib3 rather than gfxlib2
#
# This file intentionally does NOT contain:
#
#     - a replacement fbcunit build system
#     - edits to unchanged gfx fbcunit sources
#     - runtime test-selection policy

DIRLIST_INC := gfx3/gfx-only-dirlist.mk

include unit-tests.mk

# unit-tests.mk passes FBC to its nested fbcunit make. An absolute path keeps
# that submake correct while its current working directory is tests/fbcunit.
override FBC := $(abspath ../bin/fbc.exe)

# Do not put -gfx3 inside FBC. GNU make treats a whitespace-containing command
# variable as a shell fragment in some recursive paths. An ordinary compiler
# flag reaches every source compile and preserves object metadata reliably.
override FBC_CFLAGS += -gfx3

# Replace the stock -fbgfx link choice after unit-tests.mk has defined its
# helper paths. MAINEXE is also replaced so this verification never overwrites
# the regular fbc-tests.exe artifact.
override MAINEXE := fbc-tests-gfx3fresh.exe
override FBC_LFLAGS := $(FBCU_LIBS) -p $(FBCU_LIB) -gfx3 -x $(MAINEXE) -v -mt -g

# end of gfx3-unit-tests.mk
