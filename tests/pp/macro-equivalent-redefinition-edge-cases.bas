''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-equivalent-redefinition-edge-cases.bas
''
'' Purpose:
''     Exercise harmless repetition of equivalent macro definitions.
''
'' Responsibilities:
''     Check object-like, function-like, and multiline definitions whose
''     replacement text has not changed.
''
'' This file intentionally does not contain:
''     Redefinitions with conflicting replacement text.
''
' TEST_MODE : COMPILE_ONLY_OK

'' Equivalent object-like definitions are already compared and accepted.
#define PP_REDEFINE_OBJECT (40 + 2)
#define PP_REDEFINE_OBJECT (40    +    2)

#assert PP_REDEFINE_OBJECT = 42

'' Parameterized definitions should receive the same treatment.
#define PP_REDEFINE_FUNCTION(value) ((value) + 1)
#define PP_REDEFINE_FUNCTION(value) ((value) + 1)

#assert PP_REDEFINE_FUNCTION(41) = 42

#define PP_REDEFINE_RENAMED(first) ((first) + 2)
#define PP_REDEFINE_RENAMED(second) ((second) + 2)

#assert PP_REDEFINE_RENAMED(40) = 42

#macro PP_REDEFINE_MULTILINE(value)
	#assert value = 42
#endmacro

#macro PP_REDEFINE_MULTILINE(value)
	#assert value = 42
#endmacro

PP_REDEFINE_MULTILINE(42)

'' end of macro-equivalent-redefinition-edge-cases.bas
