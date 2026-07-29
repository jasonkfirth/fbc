''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-string-argument-edge-cases.bas
''
'' Purpose:
''     Exercise string literal forms while macro arguments are collected.
''
'' Responsibilities:
''     Verify that quoted commas, parentheses, quotes, and trailing backslashes
''     cannot terminate or split an argument.
''
'' This file intentionally does not contain:
''     Runtime string operations.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_STRING_ARGUMENT_SECOND(first, second) second

'' Escaped strings can contain every delimiter used by the argument scanner.
#assert PP_STRING_ARGUMENT_SECOND(!"escaped comma \x2c and paren \x29", 42) = 42
#assert PP_STRING_ARGUMENT_SECOND(!"escaped quote \" comma, paren)", 42) = 42
#assert PP_STRING_ARGUMENT_SECOND(!"trailing slash \\", 42) = 42

'' No-escape and ordinary strings use doubled quotes instead.
#assert PP_STRING_ARGUMENT_SECOND($"noescape comma, paren) doubled ""quote""", 42) = 42
#assert PP_STRING_ARGUMENT_SECOND("ordinary comma, paren) doubled ""quote""", 42) = 42

'' end of macro-string-argument-edge-cases.bas
