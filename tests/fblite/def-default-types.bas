'' Project: FreeBASIC test suite
'' File: def-default-types.bas
'' Purpose: Verify FBLite default types selected by DEF statements.
'' Responsibilities: Cover all signed, unsigned, floating and string forms.
'' This file intentionally does NOT contain: implicit declarations without DIM.
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fblite"

DefByte a-a
DefUByte b-b
DefShort c-c
DefUShort d-d
DefInt e-e
DefUInt f-f
DefLng g-g
DefULng h-h
DefLongInt i-i
DefULongInt j-j
DefSng k-k
DefDbl l-l
DefStr m-m

Dim a
Dim b
Dim c
Dim d
Dim e
Dim f
Dim g
Dim h
Dim i
Dim j
Dim k
Dim l
Dim m

If SizeOf(a) <> SizeOf(Byte) Then End 1
If SizeOf(b) <> SizeOf(UByte) Then End 2
If SizeOf(c) <> SizeOf(Short) Then End 3
If SizeOf(d) <> SizeOf(UShort) Then End 4
If SizeOf(e) <> SizeOf(Integer) Then End 5
If SizeOf(f) <> SizeOf(UInteger) Then End 6
If SizeOf(g) <> SizeOf(Long) Then End 7
If SizeOf(h) <> SizeOf(ULong) Then End 8
If SizeOf(i) <> SizeOf(LongInt) Then End 9
If SizeOf(j) <> SizeOf(ULongInt) Then End 10
If SizeOf(k) <> SizeOf(Single) Then End 11
If SizeOf(l) <> SizeOf(Double) Then End 12
m = "default string"
If m <> "default string" Then End 13

End 0

'' end of def-default-types.bas
