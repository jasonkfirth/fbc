'' Project: FreeBASIC test suite
'' File: legacy-printer.bas
'' Purpose: Verify FBLite printer statement and position function syntax.
'' Responsibilities: Compile LPRINT and LPOS without touching a printer.
'' This file intentionally does NOT contain: printer device access.
' TEST_MODE : COMPILE_ONLY_OK

#lang "fblite"

LPrint "FreeBASIC compile test"
Dim As Long printerPosition = LPos(1)

'' end of legacy-printer.bas
