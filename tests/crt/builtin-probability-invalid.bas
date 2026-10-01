' TEST_MODE : COMPILE_ONLY_FAIL

'' FreeBASIC Compiler Test Suite
'' File: builtin-probability-invalid.bas
'' Purpose: Reject probabilities outside the unit interval.
'' Responsibilities: Check LLVM builtin operand validation.
'' This file does not execute the invalid operation.

#cmdline "-gen llvm"
#cmdline "-restart"

#include once "../../inc/builtin.bi"

__builtin_expect_with_probability( 1, 1, 1.5 )

'' end of builtin-probability-invalid.bas
