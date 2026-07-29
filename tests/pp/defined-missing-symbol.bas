''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: defined-missing-symbol.bas
''
'' Purpose:
''     Verify that defined() requires a symbol operand.
''
'' Responsibilities:
''     Isolate recovery from an empty defined() invocation.
''
'' This file intentionally does not contain:
''     Other conditional syntax errors.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#if defined()
#endif

'' end of defined-missing-symbol.bas
