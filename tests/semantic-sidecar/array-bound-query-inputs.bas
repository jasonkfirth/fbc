' Project: FreeBASIC compiler semantic observations
' File: array-bound-query-inputs.bas
' Purpose: Retain array bound queries through conversion and constant folding.
' Responsibilities: Fixed/runtime arrays, dimensions, fields, macros and queries.
' This file intentionally does NOT execute array element accesses.
#lang "fb"
#define ARRAY_UPPER(values) UBound(values)

Type BoundContainer
    fixedValues(-2 To 2) As Integer
    dynamicValues(Any) As Integer
End Type

Sub ObserveBounds(argumentValues() As Integer)
    Dim fixedValues(-2 To 2) As Integer
    Dim matrixValues(1 To 3, -1 To 4) As Integer
    Dim dynamicValues() As Integer
    Dim container As BoundContainer
    Dim dimensionIndex As Integer = 2
    Print LBound(fixedValues)
    Print UBound(fixedValues)
    Print UBound(matrixValues, 2)
    Print UBound(matrixValues, 1.6)
    Print LBound(dynamicValues)
    Print UBound(dynamicValues, dimensionIndex)
    Print UBound(argumentValues, 0)
    Print LBound(container.fixedValues)
    Print UBound(container.dynamicValues)
    Print ARRAY_UPPER(fixedValues)
    Print SizeOf(UBound(fixedValues))
#if 0
    Print UBound(fixedValues, 8)
#endif
End Sub
' end of array-bound-query-inputs.bas
