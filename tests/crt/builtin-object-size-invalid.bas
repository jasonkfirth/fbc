' TEST_MODE : COMPILE_ONLY_FAIL

'' FreeBASIC Compiler Test Suite
'' File: builtin-object-size-invalid.bas
'' Purpose: Reject an invalid object-size mode.
'' Responsibilities: Check LLVM builtin operand validation.
'' This file does not execute the invalid operation.

#cmdline "-gen llvm"
#cmdline "-restart"

#include once "../../inc/builtin.bi"

__builtin_object_size( 0, 4 )

'' end of builtin-object-size-invalid.bas
