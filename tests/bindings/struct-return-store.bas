'' FreeBASIC native structure-return regression
'' File: struct-return-store.bas
'' Purpose: Receive C aggregates through array, member and pointer destinations.
'' Responsibilities: Exact fields and neighboring canaries for all register classes.
'' This file contains no ABI emulation or emitter implementation.
#lang "fb"
Type Float3
    As Single x, y, z
End Type
Type Integer2
    As ULongInt x, y
End Type
Type MixedRX
    As ULongInt x
    As Double y
End Type
Type MixedXR
    As Double x
    As ULongInt y
End Type
Extern "C"
    Declare Function make_float3() As Float3
    Declare Function make_integer2() As Integer2
    Declare Function make_mixed_rx() As MixedRX
    Declare Function make_mixed_xr() As MixedXR
End Extern

ReDim As Float3 floats(0 To 2)
floats(0) = Type<Float3>(11, 22, 33)
floats(2) = Type<Float3>(44, 55, 66)
floats(1) = make_float3()
If floats(1).x <> 1.25 OrElse floats(1).y <> -2.5 OrElse floats(1).z <> 3.75 Then End 1
If floats(0).x <> 11 OrElse floats(0).y <> 22 OrElse floats(0).z <> 33 Then End 1
If floats(2).x <> 44 OrElse floats(2).y <> 55 OrElse floats(2).z <> 66 Then End 1
Dim As Float3 Ptr destination = @floats(1)
*destination = make_float3()
If destination->x <> 1.25 OrElse destination->y <> -2.5 OrElse destination->z <> 3.75 Then End 1

ReDim As Integer2 integers(0 To 2)
integers(0) = Type<Integer2>(11, 22)
integers(2) = Type<Integer2>(33, 44)
integers(1) = make_integer2()
If integers(1).x <> &h11223344aabbccddull OrElse integers(1).y <> &h8877665544332211ull Then End 1
If integers(0).x <> 11 OrElse integers(0).y <> 22 OrElse integers(2).x <> 33 OrElse integers(2).y <> 44 Then End 1

Type ContainerRX
    As ULongInt before_guard
    As MixedRX value
    As ULongInt after_guard
End Type
Type ContainerXR
    As ULongInt before_guard
    As MixedXR value
    As ULongInt after_guard
End Type
Dim As ContainerRX rx
Dim As ContainerXR xr
rx.before_guard = 11 : rx.after_guard = 22
xr.before_guard = 33 : xr.after_guard = 44
rx.value = make_mixed_rx()
xr.value = make_mixed_xr()
If rx.value.x <> &h11223344aabbccddull OrElse rx.value.y <> -2.5 Then End 1
If xr.value.x <> 1.25 OrElse xr.value.y <> &h8877665544332211ull Then End 1
If rx.before_guard <> 11 OrElse rx.after_guard <> 22 OrElse xr.before_guard <> 33 OrElse xr.after_guard <> 44 Then End 1
Print "STRUCT_RETURN_STORE=PASS classes=4 canaries=pass"
End 0
'' end of struct-return-store.bas
