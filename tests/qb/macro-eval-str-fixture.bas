' TEST_MODE : COMPILE_ONLY_FAIL
' Project: FreeBASIC compiler tests
' File: macro-eval-str-fixture.bas
' Purpose: Guard QB-mode compilation of the byte-string macro evaluation fixture.
' Responsibilities: Require an ordinary compiler error instead of an abort.
' This wrapper intentionally does not duplicate the preprocessor test source.
' QB-specific diagnostics are expected; compiling must return a normal error.
#cmdline "-i ./fbcunit/inc"
#include "../pp/macro-eval-str.bas"
' end of macro-eval-str-fixture.bas
