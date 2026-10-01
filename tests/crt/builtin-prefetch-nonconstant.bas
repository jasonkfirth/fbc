' TEST_MODE : COMPILE_ONLY_FAIL

'' FreeBASIC Compiler Test Suite
'' File: builtin-prefetch-nonconstant.bas
'' Purpose: Require constant prefetch hints.
'' Responsibilities: Check LLVM builtin operand validation.
'' This file does not execute the invalid operation.

#cmdline "-gen llvm"
#cmdline "-restart"

#include once "../../inc/builtin.bi"

dim hint as long = 1
__builtin_prefetch( 0, hint )

'' end of builtin-prefetch-nonconstant.bas
