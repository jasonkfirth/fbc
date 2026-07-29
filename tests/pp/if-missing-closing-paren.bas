''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: if-missing-closing-paren.bas
''
'' Purpose:
''     Verify that #if diagnoses an unbalanced opening parenthesis.
''
'' Responsibilities:
''     Isolate parenthesis recovery in a preprocessor expression.
''
'' This file intentionally does not contain:
''     Other syntax errors that could hide the missing delimiter.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#if (1
#endif

'' end of if-missing-closing-paren.bas
