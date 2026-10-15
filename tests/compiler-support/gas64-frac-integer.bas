'' FreeBASIC GAS64 integer conversion regression
'' File: gas64-frac-integer.bas
'' Purpose: Check integer FRAC results returned through floating-point types.
'' Responsibilities: Scalar and overloaded UDT calls on both floating widths.
'' This file contains no emitter implementation or C ABI substitute.
#lang "fb"

Function fraction_single(ByRef value As LongInt) As Single
    Return Frac(value)
End Function

Function fraction_double(ByRef value As LongInt) As Double
    Return Frac(value)
End Function

Type WholeNumber
    value As LongInt
End Type

Operator Frac(ByRef value As WholeNumber) As Double
    Return Frac(value.value)
End Operator

Dim As LongInt values(0 To 4) = {0, 1, -1, 2147483648ll, -2147483648ll}
For index As Integer = 0 To UBound(values)
    If fraction_single(values(index)) <> 0.0! Then End 1
    If fraction_double(values(index)) <> 0.0 Then End 1
    Dim As WholeNumber whole = Type<WholeNumber>(values(index))
    If Frac(whole) <> 0.0 Then End 1
Next
Print "GAS64_FRAC_INTEGER=PASS cases=15"
End 0
'' end of gas64-frac-integer.bas
