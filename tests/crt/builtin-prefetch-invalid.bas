' TEST_MODE : COMPILE_ONLY_FAIL

'' FreeBASIC Compiler Test Suite
'' File: builtin-prefetch-invalid.bas
'' Purpose: Reject an invalid prefetch locality.
'' Responsibilities: Check LLVM builtin operand validation.
'' This file does not execute the invalid operation.

#cmdline "-gen llvm"
#cmdline "-restart"

#include once "../../inc/builtin.bi"

__builtin_prefetch( 0, 0, 4 )

'' end of builtin-prefetch-invalid.bas
