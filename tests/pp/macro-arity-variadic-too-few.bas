''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-arity-variadic-too-few.bas
''
'' Purpose:
''     Verify that a variadic define still requires all fixed arguments.
''
'' Responsibilities:
''     Distinguish an optional variadic tail from a missing fixed position.
''
'' This file intentionally does not contain:
''     A missing variadic tail, because that form is valid FreeBASIC.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_ARITY_VARIADIC(first, second, rest...) ((first) + (second))

dim as integer result = PP_ARITY_VARIADIC(1)

'' end of macro-arity-variadic-too-few.bas
