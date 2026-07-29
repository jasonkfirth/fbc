''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-delimiter-expansion-edge-cases.bas
''
'' Purpose:
''     Exercise argument delimiters assembled from more than one expansion
''     context.
''
'' Responsibilities:
''     Check parentheses and commas supplied by object-like replacement text.
''
'' This file intentionally does not contain:
''     Source that remains unbalanced after preprocessing.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_DELIMITER_ADD(left, right) ((left) + (right))
#define PP_DELIMITER_LEFT PP_DELIMITER_ADD(
#define PP_DELIMITER_RIGHT )
#define PP_DELIMITER_COMMA ,
#define PP_DELIMITER_OPEN PP_DELIMITER_ADD(19,
#define PP_DELIMITER_CLOSE 23)

'' The closing parenthesis may arrive while the argument list is being read.
#assert PP_DELIMITER_ADD(19, 23 PP_DELIMITER_RIGHT = 42

'' Both sides of a call can be supplied by independent replacements.
#assert PP_DELIMITER_LEFT 19, 23 PP_DELIMITER_RIGHT = 42
#assert PP_DELIMITER_OPEN PP_DELIMITER_CLOSE = 42

'' A replacement may become the top-level argument separator after rescanning.
#assert PP_DELIMITER_LEFT 19 PP_DELIMITER_COMMA 23 PP_DELIMITER_RIGHT = 42

'' end of macro-delimiter-expansion-edge-cases.bas
