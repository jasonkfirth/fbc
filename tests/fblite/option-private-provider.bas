'' Project: FreeBASIC test suite
'' File: option-private-provider.bas
'' Purpose: Verify OPTION PRIVATE hides an otherwise public procedure.
'' Responsibilities: Define a same-named private procedure used locally.
'' This file intentionally does NOT contain: caller code.

#lang "fblite"

Option Private
Function fbcTestsOptionPrivateTarget() As Integer
	Return 2
End Function

Public Function fbcTestsOptionPrivateProviderValue() As Integer
	Return fbcTestsOptionPrivateTarget()
End Function

'' end of option-private-provider.bas
