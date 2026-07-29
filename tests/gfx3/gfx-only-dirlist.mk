# gfx-only-dirlist.mk
#
# Purpose:
#
#     Limit unit-tests.mk to the unchanged fbcunit graphics suites so a
#     gfxlib3 verification run can force-rebuild their objects without also
#     rebuilding unrelated compiler tests.
#
# This file intentionally does NOT select a graphics archive. The caller must
# pass the desired FBC command and link flags, allowing the same suite to
# validate gfxlib2 or gfxlib3 explicitly.

DIRLIST_FB := gfx

# end of gfx-only-dirlist.mk
