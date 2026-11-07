'' Project: FreeBASIC compiler backend tests
'' File: label-flag-gosub.bas
'' Purpose: Exercise direct and computed labels through the actual GOSUB emitter.
'' Responsibilities: Preserve local values and return to the correct call sites.
'' This file intentionally does NOT install or invoke error handlers.

#lang "fblite"
option gosub

dim shared visits as long

sub Work()
    dim value as long = 17
    gosub AddTwo
    if value <> 19 then end 1
    dim selected as long = 2
    on selected gosub AddTwo, AddThree
    if value <> 22 then end 2
    exit sub

AddTwo:
    value += 2
    visits += 1
    return

AddThree:
    value += 3
    visits += 1
    return
end sub

Work()
if visits <> 2 then end 3
print "LABEL_FLAG_GOSUB_PASS"

'' end of label-flag-gosub.bas
