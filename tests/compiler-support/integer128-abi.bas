'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-abi.bas
''
'' Purpose:
''
''     Verify by-value 128-bit integers match functions compiled by C.
''
'' Responsibilities:
''
''     - test signed and unsigned C parameters and return values
''     - exercise values with bits above the 64-bit boundary
''
'' This file intentionally does NOT contain:
''
''     - compiler-internal arithmetic implementation tests
''

#lang "fb"

Declare Function c2fb_test_add_signed128 CDecl Alias "c2fb_test_add_signed128" _
	(ByVal leftValue As Integer<128>, ByVal rightValue As Integer<128>) As Integer<128>
Declare Function c2fb_test_add_unsigned128 CDecl Alias "c2fb_test_add_unsigned128" _
	(ByVal leftValue As UInteger<128>, ByVal rightValue As UInteger<128>) As UInteger<128>

If c2fb_test_add_signed128(2147483647, 1) <> 2147483648 Then End 1

Dim As UInteger<128> oneValue = 1
Dim As UInteger<128> highValue = oneValue Shl 100
If c2fb_test_add_unsigned128(highValue, oneValue) - highValue <> oneValue Then End 2

Print "128-bit C ABI passed"

'' end of integer128-abi.bas
