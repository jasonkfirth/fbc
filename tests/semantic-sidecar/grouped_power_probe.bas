' Project: FreeBASIC semantic expression observations
' File: grouped_power_probe.bas
' Purpose: Distinguish grouped built-in power from arbitrary calls in storage.
' Responsibilities: Comparison, logical and assignment input forests.
' This file intentionally does NOT execute external call lookalikes.
#lang "fb"

Declare Function PowerLookalike CDecl Alias "pow" (ByVal BaseValue As Double, ByVal ExponentValue As Double) As Double

Private Function UserPower(ByVal BaseValue As Double, ByVal ExponentValue As Double) As Double
    Return BaseValue + ExponentValue
End Function

Sub ObservePower(ByVal Subject As Double, ByVal ExponentValue As Double)
    If Subject = 2 ^ ExponentValue Then Print Subject
    If Subject = (3 ^ ExponentValue) Then Print Subject
    If Subject = (((4 ^ ExponentValue))) Then Print Subject
    If Subject = (5 ^ ExponentValue) OrElse Subject = (6 ^ ExponentValue) Then Print Subject
    If Subject = (UserPower(7, ExponentValue)) Then Print Subject
    If Subject = (PowerLookalike(8, ExponentValue)) Then Print Subject
    Dim As Double BareValue, GroupedValue, NestedValue
    BareValue = 9 ^ ExponentValue
    GroupedValue = (10 ^ ExponentValue)
    NestedValue = (((11 ^ ExponentValue)))
    Print BareValue, GroupedValue, NestedValue
End Sub

' end of grouped_power_probe.bas
