/'
	Project: FreeBASIC C backend regression controls
	File: c-integer-abs-qb.bas
	Purpose: Check the unpromoted integer widths used by the QB dialect.
	Responsibilities: Exhaustive 8/16-bit values and 32/64-bit boundary patterns.
	This file intentionally does NOT contain: Legacy I/O or platform interfaces.
'/
#lang "qb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_INTEGER_ABS_QB_FAILED", __LINE__
		end 1
	end if
#endmacro

dim i as long, expected as long
dim b as __byte, ub as __ubyte
dim s as __short, us as __ushort
for i = -128 to 127
	b = i
	if i = -128 then
		expected = -128
	elseif i < 0 then
		expected = -i
	else
		expected = i
	end if
	VERIFY(abs(b) = expected)
next
for i = 0 to 255
	ub = i
	VERIFY(abs(ub) = i)
next
for i = -32768 to 32767
	s = i
	if i = -32768 then
		expected = -32768
	elseif i < 0 then
		expected = -i
	else
		expected = i
	end if
	VERIFY(abs(s) = expected)
next
for i = 0 to 65535
	us = i
	VERIFY(abs(us) = i)
next

dim sl as long, ul as __ulong
sl = (-2147483647& - 1&)
ul = &hffffffff
VERIFY(abs(sl) = sl)
VERIFY(abs(ul) = 1)
ul = &h80000000
VERIFY(abs(ul) = ul)

dim ll as __longint, ull as __ulongint
ll = &h8000000000000000
ull = &hffffffffffffffff
VERIFY(abs(ll) = ll)
VERIFY(abs(ull) = 1)
ull = &h8000000000000000
VERIFY(abs(ull) = ull)
print "C_INTEGER_ABS_QB_PASS"

/' end of c-integer-abs-qb.bas '/
