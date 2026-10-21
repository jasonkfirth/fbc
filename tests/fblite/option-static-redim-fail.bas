'' Project: FreeBASIC test suite
'' File: option-static-redim-fail.bas
'' Purpose: Verify OPTION STATIC declares fixed-size arrays.
'' Responsibilities: Reject REDIM of an array declared under OPTION STATIC.
'' This file intentionally does NOT contain: runtime behavior.
' TEST_MODE : COMPILE_ONLY_FAIL

#lang "fblite"
Option Static
Dim fixedValues(0 To 1) As Integer
Redim fixedValues(0 To 2)

'' end of option-static-redim-fail.bas
