' TEST_MODE : COMPILE_ONLY_FAIL

'' FreeBASIC Compiler Test Suite
'' File: builtin-intrinsic-address.bas
'' Purpose: Require direct calls to compiler-only builtins.
'' Responsibilities: Check LLVM builtin operand validation.
'' This file does not execute the invalid operation.

#cmdline "-gen llvm"
#cmdline "-restart"

#include once "../../inc/builtin.bi"

dim operation as function cdecl( byval as ulong ) as long = @__builtin_clz

'' end of builtin-intrinsic-address.bas
