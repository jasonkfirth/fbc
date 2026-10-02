'' Project: FreeBASIC semantic sidecar tests
'' File: sufficiency.bas
'' Purpose: Distinguish source typing from lowered calls and formal parameters.
'' Responsibilities: Exercise omitted defaults, explicit values, and folded calls.
'' This file intentionally does NOT contain: assumptions about future effect records.

declare sub OptionalValue(byval value as long = 7)
declare function UnevaluatedValue() as long
declare sub ChangeValue(byref value as long)

sub DefaultCallSite()
	OptionalValue()
end sub

sub ExplicitCallSite()
	OptionalValue(7)
end sub

sub FoldedSourceCallSite()
	if 1 orelse UnevaluatedValue() then print 1
	if 0 andalso UnevaluatedValue() then print 2
end sub

sub AccessRoleSite()
	dim value as long
	value = 1
	value += 2
	print value
	ChangeValue(value)
end sub

'' end of sufficiency.bas
