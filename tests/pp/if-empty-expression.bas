''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: if-empty-expression.bas
''
'' Purpose:
''     Verify that #if requires a constant expression.
''
'' Responsibilities:
''     Isolate recovery from an empty conditional directive.
''
'' This file intentionally does not contain:
''     Source inside the conditional group that could add another error.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#if
#endif

'' end of if-empty-expression.bas
