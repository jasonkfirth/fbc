/'
	Project: FreeBASIC C backend regression tests
	File: c-readonly-longdouble-guard.bas
	Purpose: Retain native long-double conversion owners during const inference.
	Responsibilities: Exercise CDecl carrier arguments and return values together
	with otherwise read-only character inputs.
	This file intentionally does NOT contain: Calling convention changes or
	warning suppression.
'/
#lang "fb"
#include once "crt/longdouble.bi"

function ConsumeWide cdecl(byval text as zstring ptr, byval value as clongdouble) as long
	return len(*text) + cast(longint, value)
end function

function ProduceWide cdecl(byval text as zstring ptr) as clongdouble
	return CLongDoubleFromDoubleValue(len(*text) + 0.25#)
end function

dim as clongdouble value = CLongDoubleFromDoubleValue(2.5#)
if( ConsumeWide("abc", value) <> 5 ) then end 1
if( cast(double, ProduceWide("abcd")) <> 4.25# ) then end 2
print "C_READONLY_LONGDOUBLE_GUARD_PASS"

/' end of c-readonly-longdouble-guard.bas '/
