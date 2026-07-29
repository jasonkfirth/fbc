''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-call-separator-edge-cases.bas
''
'' Purpose:
''     Exercise lexical whitespace between a function-like macro name and its
''     argument list.
''
'' Responsibilities:
''     Check ordinary spaces, tabs, comments, and explicit line continuation
''     for macros with and without parameters.
''
'' This file intentionally does not contain:
''     Tests for whitespace in a macro declaration.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_CALL_SEPARATOR_ADD(left, right) ((left) + (right))
#define PP_CALL_SEPARATOR_ZERO() 42

'' A call remains a call when horizontal whitespace precedes its parentheses.
#assert PP_CALL_SEPARATOR_ADD (19, 23) = 42
#assert PP_CALL_SEPARATOR_ADD	(19, 23) = 42
#assert PP_CALL_SEPARATOR_ZERO () = 42

'' Block comments and explicit continuation are lexical whitespace too.
#assert PP_CALL_SEPARATOR_ADD /' comment '/ (19, 23) = 42
#assert PP_CALL_SEPARATOR_ADD _
	(19, 23) = 42

#assert PP_CALL_SEPARATOR_ZERO /' comment '/ () = 42
#assert PP_CALL_SEPARATOR_ZERO _
	() = 42

'' end of macro-call-separator-edge-cases.bas
