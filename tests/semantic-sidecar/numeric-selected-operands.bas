' Project: FreeBASIC semantic-sidecar tests
' File: numeric-selected-operands.bas
' Purpose: Exercise compiler-selected numeric coercions before folding.
' Responsibilities: Mixed widths, floating shifts, macros and quiet domains.
' This file intentionally does NOT execute unsafe or target-sensitive shifts.
#lang "fb"
#define CompareValues(lhs, rhs) ((lhs) < (rhs))
Dim As Long signed_value
Dim As ULong unsigned_value
Dim As Byte small_signed
Dim As UByte small_unsigned
Dim As Single single_value
Dim As Double double_value
Dim As Boolean boolean_value
Dim As Any Ptr pointer_value
Dim As Integer result_value
result_value = signed_value < unsigned_value
result_value = small_signed < small_unsigned
result_value = single_value < double_value
result_value = signed_value Shl 64.1
result_value = signed_value Shl -1
result_value = CompareValues(signed_value, unsigned_value)
result_value = (1 + 2) < 4
result_value = boolean_value = False
result_value = pointer_value = 0
' end of numeric-selected-operands.bas
