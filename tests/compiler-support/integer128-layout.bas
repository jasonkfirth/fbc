'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-layout.bas
''
'' Purpose:
''
''     Verify that signed and unsigned 128-bit integers retain their width
''     and natural structure alignment.
''
'' Responsibilities:
''
''     - check primitive and aggregate storage sizes
''     - check field offsets under the target C ABI
''
'' This file intentionally does NOT contain:
''
''     - 128-bit arithmetic, which is covered by integer128-arithmetic.bas
''

#lang "fb"

Type CompilerSupportInt128Layout
	prefix As UByte
	signedValue As Integer<128>
	unsignedValue As UInteger<128>
	suffix As UByte
End Type

If SizeOf(Integer<128>) <> 16 Then End 1
If SizeOf(UInteger<128>) <> 16 Then End 2

Dim As CompilerSupportInt128Layout layoutValue
Dim As UByte Ptr baseAddress = @layoutValue
Dim As UInteger<128> scalarValue

If CULngInt(@layoutValue.signedValue) - CULngInt(baseAddress) <> 16 Then End 3
If CULngInt(@layoutValue.unsignedValue) - CULngInt(baseAddress) <> 32 Then End 4
If CULngInt(@layoutValue.suffix) - CULngInt(baseAddress) <> 48 Then End 5
If SizeOf(layoutValue) <> 64 Then End 6
If (CULngInt(baseAddress) And 15) <> 0 Then End 7
If (CULngInt(@scalarValue) And 15) <> 0 Then End 8

Print "128-bit integer layout passed"

'' end of integer128-layout.bas
