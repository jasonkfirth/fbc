'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-abi-input.bas
''
'' Purpose:
''
''     Verify that the GAS64 SysV backend passes 128-bit scalar arguments
''     using the C ABI's two integer registers.
''
'' Responsibilities:
''
''     - pass signed and unsigned values to a separately compiled C function
''     - pass a 128-bit value through a FreeBASIC by-value parameter
''
'' This file intentionally does NOT contain:
''
''     - 128-bit return value ABI checks
''

#lang "fb"

Declare Function c2fb_test_int128_args CDecl Alias "c2fb_test_int128_args" _
	(ByVal signedValue As Integer<128>, ByVal unsignedValue As UInteger<128>, _
	 ByVal thirdValue As UInteger<128>, ByVal fourthValue As Integer<128>) As LongInt

Private Function checkSigned128(ByVal value As Integer<128>) As LongInt
	If value <> -8 Then Return 0
	Return 42
End Function

Private Function identitySigned128(ByVal value As Integer<128>) As Integer<128>
	Return value
End Function

Private Function checkInt128Stack(ByVal firstValue As Integer<128>, _
								  ByVal secondValue As UInteger<128>, _
								  ByVal thirdValue As UInteger<128>, _
								  ByVal fourthValue As Integer<128>) As LongInt
	If firstValue <> -8 Then Return 0
	If secondValue <= thirdValue Then Return 0
	If thirdValue <> 41 Then Return 0
	If fourthValue <> -9 Then Return 0
	Return 42
End Function

Dim As Integer<128> signedValue = -8
Dim As UInteger<128> oneValue = 1
Dim As UInteger<128> unsignedValue = oneValue Shl 100
Dim As UInteger<128> thirdValue = 41
Dim As Integer<128> fourthValue = -9
If c2fb_test_int128_args(signedValue, unsignedValue, thirdValue, fourthValue) <> 42 Then End 1
If checkSigned128(signedValue) <> 42 Then End 2
If identitySigned128(signedValue) <> signedValue Then End 3
If checkInt128Stack(signedValue, unsignedValue, thirdValue, fourthValue) <> 42 Then End 4

Print "128-bit argument ABI passed"

'' end of integer128-abi-input.bas
