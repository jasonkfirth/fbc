' Project: FreeBASIC optional source-fact controls
' File: requested-source-facts.bas
' Purpose: Exercise each optional group of original source observations.
' Responsibilities: Declarations, calls, include requests and literal joins.
' This file intentionally does NOT execute the observed procedures.
#lang "deprecated"
#include "requested-source-facts.bi"

#define ObserveInput(value) Print value

Type CapturedFields
    first_value As Long
    second_value As Long
End Type

Function CapturedResult%(ByVal input_value As Long)
    Dim stored_value% = input_value
    Dim dummy_value As Long = stored_value%
    Dim packed_value As String = MKL(dummy_value)
    ObserveInput(dummy_value)
    Print Len("captured" " text")
    Dim labels(0 To 0) As String = {"captured" " initializer"}
    CapturedResult% = dummy_value
End Function

Sub CaptureResize()
    Dim values() As Long
    ReDim values(0 To 1)
End Sub

' end of requested-source-facts.bas
