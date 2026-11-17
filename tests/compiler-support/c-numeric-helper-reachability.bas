'' Project: FreeBASIC C backend regressions
'' File: c-numeric-helper-reachability.bas
'' Purpose: Retain numeric helpers only when their emitted callers survive.
'' Responsibilities: Cover dead Single helpers and live Double/ABS evaluation.
'' This file intentionally does NOT disable native or BASIC diagnostics.
#lang "fb"

extern "C"
	declare function fbc_numeric_helper_unavailable cdecl() as single
end extern

private function deadSingle(byval value as single) as long
	dim discarded as single
	discarded = value + 1.0f
	if value = 1.5f then return abs(clng(value))
	return clng(value)
end function

private function deadOwner() as long
	return deadSingle(fbc_numeric_helper_unavailable())
end function

'' These bindings belong to this regression's live runtime contract.
dim shared liveInput as double = 1.5
dim shared evaluationCount as long

private function nextValue() as double
	evaluationCount += 1
	return liveInput
end function

private sub keepEffects()
	dim discarded as double
	discarded = nextValue() / 2.0
end sub

'' Retain source bodies through the frontend's conservative ASM boundary.
'' The C dependency pass must then separate dead and live numeric helpers.
asm nop

dim roundedValue as long = clng(liveInput)
if roundedValue <> 2 then end 1
if abs(-roundedValue) <> 2 then end 2
if liveInput <> 1.5 then end 3
keepEffects()
if evaluationCount <> 1 then end 4
print "NUMERIC_HELPER_REACHABILITY_PASS"

'' end of c-numeric-helper-reachability.bas
