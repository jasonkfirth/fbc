'' Project: FreeBASIC C backend regressions
'' File: c-static-float-effects.bas
'' Purpose: Preserve floating evaluation when an unread static store is removed.
'' Responsibilities: Check runtime narrowing and the floating-point status flag.
'' This file intentionally does NOT read the unused static destination.
#lang "fb"

extern "C"
	declare function fbc_static_float_source cdecl() as double
	declare function fbc_static_float_clear cdecl() as long
	declare function fbc_static_float_inexact cdecl() as long
end extern

private sub narrowValue()
	static unreadValue as single
	unreadValue = fbc_static_float_source()
end sub

private sub narrowAutomatic()
	dim unreadValue as single
	unreadValue = fbc_static_float_source()
end sub

if fbc_static_float_clear() <> 0 then end 1
narrowValue()
if fbc_static_float_inexact() <> 1 then end 2
if fbc_static_float_clear() <> 0 then end 3
narrowAutomatic()
if fbc_static_float_inexact() <> 1 then end 4
print "STATIC_FLOAT_EFFECTS_PASS"

'' end of c-static-float-effects.bas
