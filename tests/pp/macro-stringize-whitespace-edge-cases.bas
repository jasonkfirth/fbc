''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-stringize-whitespace-edge-cases.bas
''
'' Purpose:
''     Exercise FreeBASIC whitespace preservation during stringizing.
''
'' Responsibilities:
''     Check trimming at argument boundaries and preservation of interior
''     spaces, comment gaps, and continued-line indentation.
''
'' This file intentionally does not contain:
''     C preprocessor whitespace-normalization assumptions.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_STRINGIZE_WHITESPACE(value) #value

'' Leading and trailing space is trimmed, but interior runs are retained.
#assert PP_STRINGIZE_WHITESPACE(     alpha     +     beta     ) = _
	"alpha     +     beta"

'' Removing a block comment leaves the whitespace on each side of it.
#assert PP_STRINGIZE_WHITESPACE(alpha /' comment '/ + beta) = "alpha  + beta"

'' Continued-line indentation remains part of the raw argument text.
#assert PP_STRINGIZE_WHITESPACE(alpha _
	+ beta) = "alpha " + chr(9) + "+ beta"

'' end of macro-stringize-whitespace-edge-cases.bas
