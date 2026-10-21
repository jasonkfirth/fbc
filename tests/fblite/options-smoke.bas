'' Project: FreeBASIC test suite
'' File: options-smoke.bas
'' Purpose: Verify observable FBLite compiler option behavior.
'' Responsibilities: Array bases and storage, argument defaults, escapes,
''                   keyword removal, explicit declarations and return mode.
'' This file intentionally does NOT contain: expected compile failures.
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fblite"

Option Explicit
Option Base 1
Dim oneBased(3) As Integer
Option Base 0
Dim zeroBased(3) As Integer

Option Dynamic
Dim dynamicValues(0 To 1) As Integer
Option Static
Dim fixedValues(0 To 1) As Integer

'$DYNAMIC
Dim metaDynamicValues(0 To 1) As Integer
'$STATIC
Dim metaFixedValues(0 To 1) As Integer

Option Escape
Dim escapedNewline As String = "\n"
Dim literalBackslashN As String = $"\n"

Sub fbcTestsDefaultByref(value As Integer)
	value += 1
End Sub

Option Byval
Sub fbcTestsDefaultByval(value As Integer)
	value += 1
End Sub

Dim Shared privateProbeValue As Integer
Option Private
Sub fbcTestsOptionPrivateProbe()
	privateProbeValue = 1
End Sub

Option GoSub
Option NoGoSub
Function fbcTestsNoGosubReturn() As Integer
	Return 23
End Function

Dim byrefValue As Integer = 3
Dim byvalValue As Integer = 3
Dim sleepValue As Integer

Option NoKeyword Integer
Dim integer As Long

dynamicValues(0) = 12
Redim Preserve dynamicValues(0 To 2)
fixedValues(0) = 14
Erase fixedValues
metaDynamicValues(0) = 18
Redim Preserve metaDynamicValues(0 To 2)
metaFixedValues(0) = 22
Erase metaFixedValues

fbcTestsDefaultByref(byrefValue)
fbcTestsDefaultByval(byvalValue)
fbcTestsOptionPrivateProbe()
integer = 42
sleepValue = integer

If LBound(oneBased) <> 1 Then End 1
If LBound(zeroBased) <> 0 Then End 2
If UBound(dynamicValues) <> 2 Then End 3
If dynamicValues(0) <> 12 Then End 4
If fixedValues(0) <> 0 Then End 5
If UBound(metaDynamicValues) <> 2 Then End 6
If metaDynamicValues(0) <> 18 Then End 7
If metaFixedValues(0) <> 0 Then End 8
If escapedNewline <> Chr(10) Then End 9
If Len(literalBackslashN) <> 2 Then End 10
If Asc(literalBackslashN, 1) <> 92 Then End 11
If byrefValue <> 4 Then End 12
If byvalValue <> 3 Then End 13
If privateProbeValue <> 1 Then End 14
If sleepValue <> 42 Then End 15
If fbcTestsNoGosubReturn() <> 23 Then End 16

End 0

'' end of options-smoke.bas
