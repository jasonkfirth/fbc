'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-gas32-c-abi.bas
''
'' Purpose:
''
''     Check GAS32 calls against the 16-byte C aggregate ABI emitted by GCC.
''
'' Responsibilities:
''
''     - pass a 128-bit value to a C function by value
''     - receive a 128-bit C structure result through the hidden result slot
''     - fail through fb_End if the C and FreeBASIC layouts disagree
''
'' This file intentionally does NOT contain:
''
''     - compiler-generated arithmetic, which is tested separately
''     - assumptions about GCC support for native i386 __int128

#lang "fb"

Declare Function c2fbGas32Roundtrip CDecl Alias "c2fb_gas32_roundtrip" _
	(ByVal value As UInteger<128>) As UInteger<128>
Declare Function c2fbGas32Matches CDecl Alias "c2fb_gas32_matches" _
	(ByVal value As UInteger<128>, ByVal expectedLow As ULongInt, _
	 ByVal expectedHigh As ULongInt) As LongInt

Public Function c2fbGas32FbcRoundtrip CDecl _
	Alias "c2fb_gas32_fbc_roundtrip" (ByVal value As UInteger<128>) _
	As UInteger<128>
	If (CUInt(@value) And 15) <> 0 Then End 3
	Return value + 1
End Function

Dim As UInteger<128> oneValue = 1
Dim As UInteger<128> initialValue = oneValue Shl 64
Dim As UInteger<128> roundtripValue = c2fbGas32Roundtrip(initialValue)

If c2fbGas32Matches(roundtripValue, 1, 1) <> 1 Then End 1
If c2fbGas32Matches(initialValue, 0, 1) <> 1 Then End 2

'' end of integer128-gas32-c-abi.bas
