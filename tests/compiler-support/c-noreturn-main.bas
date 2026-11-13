'' Project: FreeBASIC C backend regression tests
'' File: c-noreturn-main.bas
'' Purpose: Exercise a generated main body which terminates through the runtime.
'' Responsibilities: Keep a synthetic return from contradicting its noreturn contract.
'' This file intentionally does NOT create threads or require a target SDK.

#lang "fb"

Function finish cdecl Alias "fb_noreturn_finish"() As Long
	End 0
End Function

Print 42
finish()

'' end of c-noreturn-main.bas
