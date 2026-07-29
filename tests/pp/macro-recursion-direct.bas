''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-recursion-direct.bas
''
'' Purpose:
''     Verify that a directly recursive function-like macro is rejected.
''
'' Responsibilities:
''     Isolate recursion detection after argument substitution.
''
'' This file intentionally does not contain:
''     Other parser errors that could hide the recursion diagnostic.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_RECURSE_DIRECT(value) PP_RECURSE_DIRECT(value)

dim as integer result = PP_RECURSE_DIRECT(42)

'' end of macro-recursion-direct.bas
