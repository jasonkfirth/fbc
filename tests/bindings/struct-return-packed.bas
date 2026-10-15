'' FreeBASIC packed structure-return regression
'' File: struct-return-packed.bas
'' Purpose: Receive 9..16-byte C aggregates into guarded array elements.
'' Responsibilities: Exact tail bytes, integer payloads and adjacent canaries.
'' This file contains no ABI emulation or compiler implementation.
#lang "fb"

'' Field = 1 mirrors the packed C layout: eight payload bytes followed by
'' one to eight tail bytes. Separate types exercise every partial store.
Type Packed9 Field = 1
    head As ULongInt
    tail(0 To 0) As UByte
End Type
Extern "C"
    Declare Function make9() As Packed9
End Extern
Type Packed10 Field = 1
    head As ULongInt
    tail(0 To 1) As UByte
End Type
Extern "C"
    Declare Function make10() As Packed10
End Extern
Type Packed11 Field = 1
    head As ULongInt
    tail(0 To 2) As UByte
End Type
Extern "C"
    Declare Function make11() As Packed11
End Extern
Type Packed12 Field = 1
    head As ULongInt
    tail(0 To 3) As UByte
End Type
Extern "C"
    Declare Function make12() As Packed12
End Extern
Type Packed13 Field = 1
    head As ULongInt
    tail(0 To 4) As UByte
End Type
Extern "C"
    Declare Function make13() As Packed13
End Extern
Type Packed14 Field = 1
    head As ULongInt
    tail(0 To 5) As UByte
End Type
Extern "C"
    Declare Function make14() As Packed14
End Extern
Type Packed15 Field = 1
    head As ULongInt
    tail(0 To 6) As UByte
End Type
Extern "C"
    Declare Function make15() As Packed15
End Extern
Type Packed16 Field = 1
    head As ULongInt
    tail(0 To 7) As UByte
End Type
Extern "C"
    Declare Function make16() As Packed16
End Extern

Sub PackedFailure(ByVal byte_count As Long, ByVal byte_index As Long, ByVal actual_value As UByte)
    Print "PACKED_FAIL size="; byte_count; " byte="; byte_index; " value="; actual_value
    End 1
End Sub

ReDim a9(0 To 2) As Packed9
a9(0).head = 11
a9(2).head = 22
a9(1) = make9()
If a9(1).head <> &h11223344aabbccddull Then End 1
If a9(0).head <> 11 OrElse a9(2).head <> 22 Then End 1
If a9(1).tail(0) <> 128 Then PackedFailure(9, 0, a9(1).tail(0))

ReDim a10(0 To 2) As Packed10
a10(0).head = 11
a10(2).head = 22
a10(1) = make10()
If a10(1).head <> &h11223344aabbccddull Then End 1
If a10(0).head <> 11 OrElse a10(2).head <> 22 Then End 1
If a10(1).tail(0) <> 128 Then PackedFailure(10, 0, a10(1).tail(0))
If a10(1).tail(1) <> 129 Then PackedFailure(10, 1, a10(1).tail(1))

ReDim a11(0 To 2) As Packed11
a11(0).head = 11
a11(2).head = 22
a11(1) = make11()
If a11(1).head <> &h11223344aabbccddull Then End 1
If a11(0).head <> 11 OrElse a11(2).head <> 22 Then End 1
If a11(1).tail(0) <> 128 Then PackedFailure(11, 0, a11(1).tail(0))
If a11(1).tail(1) <> 129 Then PackedFailure(11, 1, a11(1).tail(1))
If a11(1).tail(2) <> 130 Then PackedFailure(11, 2, a11(1).tail(2))

ReDim a12(0 To 2) As Packed12
a12(0).head = 11
a12(2).head = 22
a12(1) = make12()
If a12(1).head <> &h11223344aabbccddull Then End 1
If a12(0).head <> 11 OrElse a12(2).head <> 22 Then End 1
If a12(1).tail(0) <> 128 Then PackedFailure(12, 0, a12(1).tail(0))
If a12(1).tail(1) <> 129 Then PackedFailure(12, 1, a12(1).tail(1))
If a12(1).tail(2) <> 130 Then PackedFailure(12, 2, a12(1).tail(2))
If a12(1).tail(3) <> 131 Then PackedFailure(12, 3, a12(1).tail(3))

ReDim a13(0 To 2) As Packed13
a13(0).head = 11
a13(2).head = 22
a13(1) = make13()
If a13(1).head <> &h11223344aabbccddull Then End 1
If a13(0).head <> 11 OrElse a13(2).head <> 22 Then End 1
If a13(1).tail(0) <> 128 Then PackedFailure(13, 0, a13(1).tail(0))
If a13(1).tail(1) <> 129 Then PackedFailure(13, 1, a13(1).tail(1))
If a13(1).tail(2) <> 130 Then PackedFailure(13, 2, a13(1).tail(2))
If a13(1).tail(3) <> 131 Then PackedFailure(13, 3, a13(1).tail(3))
If a13(1).tail(4) <> 132 Then PackedFailure(13, 4, a13(1).tail(4))

ReDim a14(0 To 2) As Packed14
a14(0).head = 11
a14(2).head = 22
a14(1) = make14()
If a14(1).head <> &h11223344aabbccddull Then End 1
If a14(0).head <> 11 OrElse a14(2).head <> 22 Then End 1
If a14(1).tail(0) <> 128 Then PackedFailure(14, 0, a14(1).tail(0))
If a14(1).tail(1) <> 129 Then PackedFailure(14, 1, a14(1).tail(1))
If a14(1).tail(2) <> 130 Then PackedFailure(14, 2, a14(1).tail(2))
If a14(1).tail(3) <> 131 Then PackedFailure(14, 3, a14(1).tail(3))
If a14(1).tail(4) <> 132 Then PackedFailure(14, 4, a14(1).tail(4))
If a14(1).tail(5) <> 133 Then PackedFailure(14, 5, a14(1).tail(5))

ReDim a15(0 To 2) As Packed15
a15(0).head = 11
a15(2).head = 22
a15(1) = make15()
If a15(1).head <> &h11223344aabbccddull Then End 1
If a15(0).head <> 11 OrElse a15(2).head <> 22 Then End 1
If a15(1).tail(0) <> 128 Then PackedFailure(15, 0, a15(1).tail(0))
If a15(1).tail(1) <> 129 Then PackedFailure(15, 1, a15(1).tail(1))
If a15(1).tail(2) <> 130 Then PackedFailure(15, 2, a15(1).tail(2))
If a15(1).tail(3) <> 131 Then PackedFailure(15, 3, a15(1).tail(3))
If a15(1).tail(4) <> 132 Then PackedFailure(15, 4, a15(1).tail(4))
If a15(1).tail(5) <> 133 Then PackedFailure(15, 5, a15(1).tail(5))
If a15(1).tail(6) <> 134 Then PackedFailure(15, 6, a15(1).tail(6))

ReDim a16(0 To 2) As Packed16
a16(0).head = 11
a16(2).head = 22
a16(1) = make16()
If a16(1).head <> &h11223344aabbccddull Then End 1
If a16(0).head <> 11 OrElse a16(2).head <> 22 Then End 1
If a16(1).tail(0) <> 128 Then PackedFailure(16, 0, a16(1).tail(0))
If a16(1).tail(1) <> 129 Then PackedFailure(16, 1, a16(1).tail(1))
If a16(1).tail(2) <> 130 Then PackedFailure(16, 2, a16(1).tail(2))
If a16(1).tail(3) <> 131 Then PackedFailure(16, 3, a16(1).tail(3))
If a16(1).tail(4) <> 132 Then PackedFailure(16, 4, a16(1).tail(4))
If a16(1).tail(5) <> 133 Then PackedFailure(16, 5, a16(1).tail(5))
If a16(1).tail(6) <> 134 Then PackedFailure(16, 6, a16(1).tail(6))
If a16(1).tail(7) <> 135 Then PackedFailure(16, 7, a16(1).tail(7))
Print "PACKED_RETURN=PASS sizes=9..16"
'' end of struct-return-packed.bas
