'' Project: FreeBASIC C backend regression tests
'' File: c-vtable-abi-base.bas
'' Purpose: Supply the base side of a mixed-generator virtual-call fixture.
'' Responsibilities: Implement three signatures and count base destruction.
'' This file intentionally does NOT define derived methods or a program entry.
#include once "c-vtable-abi.bi"

dim shared vtable_destroyed as long

constructor TableBase()
	marker = 11
end constructor

function TableBase.number(byval value as long) as long
	return marker + value
end function

function TableBase.fraction(byval value as double) as double
	return marker + value
end function

function TableBase.reference() byref as long
	return marker
end function

destructor TableBase()
	vtable_destroyed += 1
end destructor

destructor TableAbstract()
end destructor

'' end of c-vtable-abi-base.bas
