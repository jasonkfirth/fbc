' TEST_MODE : COMPILE_AND_RUN_OK

'' FreeBASIC Compiler Test Suite
'' File: builtin-backends.bas
'' Purpose: Check builtin.bi using the backend selected by the test suite.
'' Responsibilities: Run the same checks for LLVM and the C backends.
'' This file does not require builtins on the assembly backends.

#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang") or (__FB_BACKEND__ = "llvm")
	#include once "builtin-checks.bi"
#endif

'' end of builtin-backends.bas
