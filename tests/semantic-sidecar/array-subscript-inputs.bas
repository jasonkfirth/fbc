' Project: FreeBASIC compiler semantic observations
' File: array-subscript-inputs.bas
' Purpose: Preserve array identities and indices before offset lowering.
' Responsibilities: Fixed/runtime storage, fields, rank, macros and query inputs.
' This file intentionally does NOT execute out-of-range accesses.
#lang "fb"
#define SELECT_ITEM(values, index) values(index)

Type ArrayContainer
    fixedValues(-2 To 2) As Integer
    dynamicValues(Any) As Integer
End Type

Sub ObserveSubscripts(argumentValues() As Integer)
    Dim As Integer fixedValues(-2 To 2)
    Dim As Integer matrixValues(0 To 2, 1 To 3)
    Dim As Integer dynamicValues()
    Dim As ArrayContainer container
    Dim As Integer index = 1
    Dim As Double floatingIndex = 1.5
    Print fixedValues(3)
    Print SELECT_ITEM(fixedValues, index + 1)
    Print matrixValues(index, 4)
    Print dynamicValues(index)
    Print argumentValues(index)
    Print container.fixedValues(-3)
    Print container.dynamicValues(index)
    Print fixedValues(floatingIndex)
    Print SizeOf(fixedValues(9))
    Print fixedValues(matrixValues(0, 1))
    fixedValues(3) = 1
    container.fixedValues(-3) = 2
    container.dynamicValues(index) = 3
End Sub
' end of array-subscript-inputs.bas
