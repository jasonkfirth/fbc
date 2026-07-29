''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-argument-edge-cases.bas
''
'' Purpose:
''     Exercise delimiters, empty positions, comments, strings, and continued
''     lines while the preprocessor collects macro arguments.
''
'' Responsibilities:
''     Verify that only top-level commas split arguments and that argument text
''     survives trimming and recursive expansion.
''
'' This file intentionally does not contain:
''     Invalid argument-count cases, which are isolated in failure tests.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_ARG_SECOND(first, second) second
#define PP_ARG_ADD(left, right) ((left) + (right))
#define PP_ARG_TEXT3(first, second, third) #first + "|" + #second + "|" + #third

'' Empty arguments retain their positions, and surrounding space is trimmed.
#assert PP_ARG_TEXT3(, middle, ) = "|middle|"
#assert PP_ARG_TEXT3( first, , third ) = "first||third"

'' Commas and closing parentheses inside strings are ordinary argument text.
#assert PP_ARG_SECOND("left,right", 42) = 42
#assert PP_ARG_SECOND("text, ) ""quoted""", 42) = 42

'' Parentheses protect nested commas even when the protected text is discarded.
#assert PP_ARG_SECOND((1, 2), 42) = 42
#assert PP_ARG_SECOND((((1, 2))), 42) = 42

'' Comments do not expose their commas or parentheses to the argument scanner.
#assert PP_ARG_SECOND(/' ignored, ) text '/ 1, 42) = 42
#assert PP_ARG_SECOND(1 /' ignored, ) text '/, 42) = 42

'' A nested macro may expand to parenthesized text containing a comma.
#define PP_ARG_PAIR(left, right) (left, right)
#define PP_ARG_DISCARD(value) 42

#assert PP_ARG_DISCARD(PP_ARG_PAIR(19, 23)) = 42

'' Nested calls have independent argument lists.
#assert PP_ARG_SECOND(PP_ARG_SECOND(1, 2), 42) = 42
#assert PP_ARG_ADD(PP_ARG_SECOND(1, 19), PP_ARG_SECOND(2, 23)) = 42

'' A physical line break is accepted when FreeBASIC continuation is explicit.
#assert PP_ARG_ADD( _
	19, _
	23 _
) = 42

'' Whitespace between a function-like macro name and '(' does not suppress it.
#assert PP_ARG_ADD  (19, 23) = 42

'' Stringizing uses the already-expanded FreeBASIC argument text.
#define PP_ARG_WORD expanded
#define PP_ARG_STRINGIZE(value) #value

#assert PP_ARG_STRINGIZE(PP_ARG_WORD) = "expanded"

'' end of macro-argument-edge-cases.bas
