''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: if-missing-binary-operand.bas
''
'' Purpose:
''     Verify that a binary operator in #if requires its right operand.
''
'' Responsibilities:
''     Isolate conditional-expression recovery at end-of-line.
''
'' This file intentionally does not contain:
''     Other syntax errors that could hide the missing operand.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#if 1 +
#endif

'' end of if-missing-binary-operand.bas
