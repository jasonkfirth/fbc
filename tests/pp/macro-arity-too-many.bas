''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-arity-too-many.bas
''
'' Purpose:
''     Verify that a function-like define rejects an extra argument.
''
'' Responsibilities:
''     Isolate the too-many-arguments diagnostic.
''
'' This file intentionally does not contain:
''     Other syntax errors that could hide the expected diagnostic.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_ARITY_PAIR(first, second) ((first) + (second))

dim as integer result = PP_ARITY_PAIR(1, 2, 3)

'' end of macro-arity-too-many.bas
