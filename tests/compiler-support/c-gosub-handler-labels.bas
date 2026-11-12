/'
    Project: FreeBASIC C backend regression tests
    File: c-gosub-handler-labels.bas
    Purpose: Distinguish outlined handler labels from direct GOSUB targets.
    Responsibilities: Retain shared source entries and omit callback-only labels.
    This file contains no graphics, external assets or native assembly.
'/
#lang "fblite"
#cmdline "-ex"
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
    #cmdline "-Wc -Werror=unused-label"
#endif
option gosub

dim shared as integer visits, handled
on error goto callback_only
gosub work
on error goto shared_entry
gosub shared_entry
if visits <> 2 then end 1
error 5
if handled <> 1 then end 2
err = 0
on error goto callback_only
error 7
if handled <> 2 then end 3
on error goto 0
print "GOSUB_HANDLER_LABELS_PASS"
end 0

work:
visits += 1
return

shared_entry:
if err <> 0 then
    handled += 1
    resume next
end if
visits += 1
return

callback_only:
handled += 1
resume next

' end of c-gosub-handler-labels.bas
