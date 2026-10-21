'' Project: FreeBASIC test suite
'' File: hash-lang.bas
'' Purpose: Verify #LANG switches a module from FB to FBLite mode.
'' Responsibilities: Use OPTION BASE after the source-level dialect change.
'' This file intentionally does NOT contain: a command-line language override.
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fblite"

Option Base 1
Dim values(2) As Integer
If LBound(values) <> 1 Then End 1
End 0

'' end of hash-lang.bas
