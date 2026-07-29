''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-recursion-through-wrapper.bas
''
'' Purpose:
''     Verify recursion detection when a wrapper expands to a self-reference.
''
'' Responsibilities:
''     Isolate a cycle whose recursive name is not the top-level source token.
''
'' This file intentionally does not contain:
''     Other parser errors that could hide the recursion diagnostic.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_RECURSE_SELF PP_RECURSE_SELF
#define PP_RECURSE_WRAPPER PP_RECURSE_SELF

dim as integer result = PP_RECURSE_WRAPPER

'' end of macro-recursion-through-wrapper.bas
