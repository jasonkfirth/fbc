'' Project: FreeBASIC compiler C backend tests
'' File: c-label-flag-layout.bas
'' Purpose: Check the compiler-owned label flag and its containing layouts.
'' Responsibilities: Verify native field offsets, copies and normalized truth.
'' This file intentionally does NOT exercise external handler ownership.

#lang "fb"
#include once "core/fbint.bi"

'' Labels already reserve four native slots. The flag must use the last slot
'' without changing the containing symbol union or its alignment.
#assert sizeof(FBS_LABEL) = 4 * sizeof(any ptr)
#assert offsetof(FBS_LABEL, parent) = 0
#assert offsetof(FBS_LABEL, declared) = sizeof(any ptr)
#assert offsetof(FBS_LABEL, stmtnum) = 2 * sizeof(any ptr)
#assert offsetof(FBS_LABEL, gosub) = 3 * sizeof(any ptr)

dim first as FBS_LABEL
dim owner as FBSYMBOL
first.parent = @owner
if first.gosub <> FALSE then end 1
first.gosub = TRUE
if first.gosub = FALSE then end 2

dim copied as FBS_LABEL = first
owner.lbl = copied
if owner.lbl.gosub = FALSE then end 8
if copied.parent <> @owner then end 9
if copied.gosub = FALSE then end 3
copied.gosub = FALSE
if copied.gosub <> FALSE then end 4
if first.gosub = FALSE then end 5
if abs(first.gosub <> FALSE) <> 1 then end 6
if abs(copied.gosub <> FALSE) <> 0 then end 7

print "C_LABEL_FLAG_LAYOUT_PASS"; sizeof(FBS_LABEL); sizeof(FBSYMBOL); _
    offsetof(FBS_LABEL, gosub); offsetof(FBSYMBOL, lbl)

'' end of c-label-flag-layout.bas
