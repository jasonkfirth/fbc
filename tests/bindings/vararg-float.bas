'' FreeBASIC native variadic floating-point regression
'' File: vararg-float.bas
'' Purpose: Send immediate and stored floating values to native C va_arg readers.
'' Responsibilities: Register positions, stack spill and indirect calls.
'' This file contains no ABI emulation or audio-library implementation.
#lang "fb"
Extern "C"
    Declare Function vararg_float1(ByVal first As Long, ...) As Long
    Declare Function vararg_float2(ByVal first As Long, ByVal second As Long, ...) As Long
    Declare Function vararg_float3(ByVal first As Long, ByVal second As Long, ByVal third As Long, ...) As Long
    Declare Function vararg_float_single(ByVal first As Single, ByVal marker As Long, ...) As Long
    Declare Function vararg_float_double(ByVal first As Double, ByVal marker As Long, ...) As Long
End Extern

Dim As Long result = 0
result Or= vararg_float1(11, 1.25, 0.0, -3.75, 4.125, 5.5)
result Or= vararg_float2(11, 22, 1.25, 0.0, -3.75, 4.125, 5.5)
result Or= vararg_float3(11, 22, 33, 1.25, 0.0, -3.75, 4.125, 5.5)
Dim As Double a = 1.25, b = 0.0, c = -3.75, d = 4.125, e = 5.5
result Or= vararg_float1(11, a, b, c, d, e)
result Or= vararg_float2(11, 22, a, b, c, d, e)
result Or= vararg_float3(11, 22, 33, a, b, c, d, e)
Dim As Function Cdecl(ByVal As Long, ...) As Long indirect = @vararg_float1
result Or= indirect(11, 1.25, 0.0, -3.75, 4.125, 5.5)
result Or= indirect(11, a, b, c, d, e)
result Or= vararg_float_single(1.25!, 22, 1.25, 0.0, -3.75, 4.125, 5.5)
result Or= vararg_float_double(1.25, 22, 1.25, 0.0, -3.75, 4.125, 5.5)
Dim As Function Cdecl(ByVal As Single, ByVal As Long, ...) As Long single_indirect = @vararg_float_single
Dim As Function Cdecl(ByVal As Double, ByVal As Long, ...) As Long double_indirect = @vararg_float_double
result Or= single_indirect(1.25!, 22, a, b, c, d, e)
result Or= double_indirect(1.25, 22, a, b, c, d, e)
If result Then End 1
Print "VARARG_FLOAT=PASS direct=8 indirect=4"
End 0
'' end of vararg-float.bas
