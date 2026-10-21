'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-layout-gas32.bas
''
'' Purpose:
''
''     Verify 128-bit storage alignment in the 32-bit GAS backend.
''
'' Responsibilities:
''
''     - keep global 128-bit scalars aligned to their natural boundary
''     - verify aggregate size around a 128-bit member
''
'' This file intentionally does NOT contain:
''
''     - 128-bit arithmetic or function ABI checks
''

#lang "fb"

Type Gas32Integer128Layout
	marker As UByte
	value As Integer<128>
	suffix As UByte
End Type

Dim Shared integer128Value As Integer<128>
Dim Shared recordValue As Gas32Integer128Layout

If SizeOf(integer128Value) <> 16 Then End 1
If SizeOf(recordValue) <> 48 Then End 2

'' end of integer128-layout-gas32.bas
