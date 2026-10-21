'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-gas64-windows.bas
''
'' Purpose:
''
''     Verify the GAS64 non-SysV 128-bit procedure-call convention.
''
'' Responsibilities:
''
''     - compile 16-byte by-value arguments through Windows register slots
''     - compile hidden-pointer 128-bit function results
''     - compile stack-slot and nested 128-bit calls
''
'' This file intentionally does NOT contain:
''
''     - runtime execution, because the Linux host cannot run Windows output
''     - external C ABI declarations for a native Windows __int128 type

#lang "fb"

Private Function windowsAdjustSigned(ByVal value As Integer<128>) As Integer<128>
	value += 1
	Return value
End Function

Private Function windowsAddUnsigned(ByVal leftValue As UInteger<128>, _
										ByVal rightValue As UInteger<128>) As UInteger<128>
	Return leftValue + rightValue
End Function

Private Function windowsAddMany(ByVal firstValue As UInteger<128>, _
								ByVal secondValue As UInteger<128>, _
								ByVal thirdValue As UInteger<128>, _
								ByVal fourthValue As UInteger<128>, _
								ByVal fifthValue As UInteger<128>) As UInteger<128>
	Return firstValue + secondValue + thirdValue + fourthValue + fifthValue
End Function

Dim As Integer<128> signedInput = -41
Dim As Integer<128> signedResult = windowsAdjustSigned(signedInput)
Dim As UInteger<128> unsignedResult = windowsAddUnsigned(1, 2)
Dim As UInteger<128> stackResult = windowsAddMany(1, 2, 3, 4, 5)
Dim As UInteger<128> nestedResult = windowsAddUnsigned(windowsAddUnsigned(1, 2), 4)

If signedResult <> -40 Then End 1
If unsignedResult <> 3 Then End 2
If stackResult <> 15 Then End 3
If nestedResult <> 7 Then End 4

'' end of integer128-gas64-windows.bas
