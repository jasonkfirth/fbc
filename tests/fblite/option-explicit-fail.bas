'' Project: FreeBASIC test suite
'' File: option-explicit-fail.bas
'' Purpose: Verify OPTION EXPLICIT rejects implicit variable creation.
'' Responsibilities: Exercise one undeclared variable use in FBLite mode.
'' This file intentionally does NOT contain: runtime behavior.
' TEST_MODE : COMPILE_ONLY_FAIL

#lang "fblite"
Option Explicit
implicitValue = 1

'' end of option-explicit-fail.bas
