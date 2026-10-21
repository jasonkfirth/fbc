'' Project: FreeBASIC test suite
'' File: option-nogosub-fail.bas
'' Purpose: Verify OPTION NOGOSUB disables GOSUB statements.
'' Responsibilities: Reject GOSUB after the option is turned off.
'' This file intentionally does NOT contain: runtime behavior.
' TEST_MODE : COMPILE_ONLY_FAIL

#lang "fblite"
Option GoSub
Option NoGoSub
GoSub missingTarget

'' end of option-nogosub-fail.bas
