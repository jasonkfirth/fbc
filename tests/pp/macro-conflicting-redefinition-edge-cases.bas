''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-conflicting-redefinition-edge-cases.bas
''
'' Purpose:
''     Ensure that accepting equivalent function-like macro definitions does
''     not also accept definitions with different replacement text.
''
'' Responsibilities:
''     Require a duplicate-definition diagnostic for a conflicting macro.
''
'' This file intentionally does not contain:
''     Equivalent definitions, which are covered by the matching positive test.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_REDEFINE_CONFLICT(value) ((value) + 1)
#define PP_REDEFINE_CONFLICT(value) ((value) + 2)

'' end of macro-conflicting-redefinition-edge-cases.bas
