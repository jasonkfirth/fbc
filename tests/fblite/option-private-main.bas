'' Project: FreeBASIC test suite
'' File: option-private-main.bas
'' Purpose: Call the public module function and verify its result.
'' Responsibilities: Provide the public side of the linkage test.
'' This file intentionally does NOT contain: the private procedure definition.

#lang "fblite"

Function fbcTestsOptionPrivateTarget() As Integer
	Return 1
End Function

Declare Function fbcTestsOptionPrivateProviderValue() As Integer

If fbcTestsOptionPrivateTarget() <> 1 Then End 1
If fbcTestsOptionPrivateProviderValue() <> 2 Then End 2
End 0

'' end of option-private-main.bas
