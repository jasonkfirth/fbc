' TEST_MODE : COMPILE_AND_RUN_OK

'' FreeBASIC Compiler Test Suite
'' File: builtin.bas
'' Purpose: Check the C ABI of builtin.bi declarations with GCC.
'' Responsibilities: Turn builtin declaration mismatches into errors and run
'' the shared builtin checks. LLVM uses builtin-backends.bas instead.
'' This file does not implement builtin operations.

#cmdline "-gen gcc"
#cmdline "-Wc -Wno-unknown-warning-option"
#cmdline "-Wc -Werror=builtin-declaration-mismatch"
#cmdline "-restart"

#include once "builtin-checks.bi"

'' end of builtin.bas
