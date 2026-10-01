'' Project: FreeBASIC semantic sidecar tests
'' File: procedures.bas
'' Purpose: Exercise overload selection, signatures, defaults, and pointer calls.
'' Responsibilities: Supply formal-to-body and canonical prototype relationships.
'' This file intentionally does NOT contain: external implementations.

declare function Choose overload(byval item as long) as long
declare function Choose overload(byval item as double) as double

function Choose overload(byval item as long) as long
	return item
end function

function Choose overload(byval item as double) as double
	return item
end function

sub Modes cdecl(byval value as long, byref reference as double, values() as long, byval optional_value as long = 7)
	reference += value + optional_value
end sub

sub Variadic cdecl(byval count as long, ...)
end sub

function RefResult(byref item as long) byref as long
	return item
end function

sub ProbeCalls()
	dim value as long = 1
	dim reference as double = 2
	dim values(0 to 2) as long
	dim callback as function(byval item as long) as long = @Choose
	print Choose(value), Choose(reference), callback(value)
	Modes(value, reference, values())
	Variadic(2, value, reference)
	print RefResult(value)
end sub

ProbeCalls()

'' end of procedures.bas
