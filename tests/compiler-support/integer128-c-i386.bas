'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-c-i386.bas
''
'' Purpose:
''
''     Verify that the GCC C backend lowers 128-bit values for 32-bit x86.
''
'' Responsibilities:
''
''     - exercise helper-lowered arithmetic and conversions
''     - compile FreeBASIC by-value 128-bit arguments and results as C structs
''     - test wide StdCall arguments followed by narrower parameters
''     - test indirect C calls with two wide arguments and a wide result
''     - check the 16-byte type layout used by generated C
''
'' This file intentionally does NOT contain:
''
''     - C ABI declarations using __int128, which i386 GCC does not support
''     - runtime checks requiring a 32-bit C library installation
''

#lang "fb"

Type Integer128AddCallback As Function _
	(ByVal leftValue As UInteger<128>, ByVal rightValue As UInteger<128>) As UInteger<128>

Type Integer128I386Layout
	prefix As UByte
	value As UInteger<128>
	suffix As UByte
End Type

Private Function signedIdentity(ByVal value As Integer<128>) As Integer<128>
	Return value
End Function

Private Function addUnsigned(ByVal leftValue As UInteger<128>, _
							 ByVal rightValue As UInteger<128>) As UInteger<128>
	Return leftValue + rightValue
End Function

Private Function addStdCall StdCall (ByVal value As UInteger<128>, _
									 ByVal addition As UInteger) As UInteger<128>
	Return value + addition
End Function

Dim Shared As Integer<128> initialSignedValue = -19
Dim Shared As UInteger<128> initialUnsignedValue = 17
Dim As Integer128AddCallback addCallback = @addUnsigned
Dim As Integer<128> signedResult = signedIdentity(initialSignedValue)
Dim As UInteger<128> unsignedResult = addUnsigned(initialUnsignedValue, 25)
Dim As UInteger<128> highInput = 1
highInput = highInput Shl 100
highInput += 17
Dim As Integer128I386Layout layoutValue
Dim As UInteger<128> localValue

If SizeOf(Integer<128>) <> 16 Then End 1
If SizeOf(layoutValue) <> 48 Then End 2
If signedResult <> -19 Then End 3
If unsignedResult <> 42 Then End 4
If addStdCall(highInput, 5) <> highInput + 5 Then End 5
If addCallback(highInput, 9) <> highInput + 9 Then End 6
If (CUInt(@layoutValue) And 15) <> 0 Then End 7
If (CUInt(@localValue) And 15) <> 0 Then End 8

'' end of integer128-c-i386.bas
