' Project: FreeBASIC optional source-fact controls
' File: requested-source-facts.bas
' Purpose: Exercise identifier, declaration, keyword and macro-access captures.
' Responsibilities: Real source syntax and a function-result assignment.
' This file intentionally does NOT execute the observed procedures.
#lang "deprecated"

#define ObserveInput(value) Print value

Type CapturedFields
    first_value As Long
    second_value As Long
End Type

Function CapturedResult%(ByVal input_value As Long)
    Dim stored_value% = input_value
    Dim dummy_value As Long = stored_value%
    ObserveInput(dummy_value)
    CapturedResult% = dummy_value
End Function

Sub CaptureResize()
    Dim values() As Long
    ReDim values(0 To 1)
End Sub

' end of requested-source-facts.bas
