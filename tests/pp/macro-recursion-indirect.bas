''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-recursion-indirect.bas
''
'' Purpose:
''     Verify that an indirect object-like macro cycle is rejected.
''
'' Responsibilities:
''     Isolate recursion detection across two replacement bodies.
''
'' This file intentionally does not contain:
''     Other parser errors that could hide the recursion diagnostic.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_RECURSE_LEFT PP_RECURSE_RIGHT
#define PP_RECURSE_RIGHT PP_RECURSE_LEFT

dim as integer result = PP_RECURSE_LEFT

'' end of macro-recursion-indirect.bas
