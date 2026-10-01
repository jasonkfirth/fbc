' TEST_MODE : COMPILE_ONLY_FAIL

'' FreeBASIC Compiler Test Suite
'' File: builtin-invalid-declaration.bas
'' Purpose: Reject an invalid LLVM builtin prototype before invoking llc.
'' Responsibilities: Check that a C alias cannot bypass operand validation.
'' This file does not execute the invalid operation.

#cmdline "-gen llvm"
#cmdline "-restart"

declare function invalidBuiltin cdecl alias "__builtin_popcount"( byval as any ptr ) as long
dim value as long
print invalidBuiltin( @value )

'' end of builtin-invalid-declaration.bas
