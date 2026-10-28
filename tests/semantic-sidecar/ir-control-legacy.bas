'' Project: FreeBASIC semantic sidecar tests
'' File: ir-control-legacy.bas
'' Purpose: Exercise native GOSUB, computed dispatch and error/resume lowering.
'' Responsibilities: Local calls/returns, address-taken labels and indirect transfers.
'' This file intentionally does NOT claim a runtime value for indirect targets.
#lang "fblite"
option gosub

sub IRFlowLegacy(byval choice as integer)
    on local error goto Trouble
    gosub BodyLabel
    on choice goto BodyLabel, OtherLabel
    on choice gosub BodyLabel, OtherLabel
    goto Finish
BodyLabel:
    return
OtherLabel:
    return Finish
Trouble:
    resume next
Finish:
end sub

IRFlowLegacy(0)
print "legacy-ok"

'' end of ir-control-legacy.bas
