''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-rescan-edge-cases.bas
''
'' Purpose:
''     Exercise recursive macro rescanning and calls assembled from more than
''     one expansion context.
''
'' Responsibilities:
''     Check aliases, nested calls, callable arguments, generated names, and
''     expansion boundaries.
''
'' This file intentionally does not contain:
''     Tests for recursive definitions that must be rejected.
''
' TEST_MODE : COMPILE_ONLY_OK

'' Object-like replacement text is rescanned until it reaches ordinary source.
#define PP_RESCAN_VALUE 40
#define PP_RESCAN_ALIAS PP_RESCAN_VALUE
#define PP_RESCAN_SECOND_ALIAS PP_RESCAN_ALIAS

#assert PP_RESCAN_SECOND_ALIAS = 40

'' A replacement may refer to a define that is declared later.
#define PP_RESCAN_LATE_ALIAS PP_RESCAN_LATE_VALUE
#define PP_RESCAN_LATE_VALUE 42

#assert PP_RESCAN_LATE_ALIAS = 42

'' Arguments are expanded before the outer replacement is parsed again.
#define PP_RESCAN_ADD_ONE(value) ((value) + 1)
#define PP_RESCAN_IDENTITY(value) value
#define PP_RESCAN_APPLY(callable, value) callable(value)

#assert PP_RESCAN_ADD_ONE(PP_RESCAN_ADD_ONE(40)) = 42
#assert PP_RESCAN_IDENTITY(PP_RESCAN_IDENTITY(42)) = 42
#assert PP_RESCAN_APPLY(PP_RESCAN_ADD_ONE, 41) = 42

'' The alias becomes callable only after its replacement meets the source '('.
#define PP_RESCAN_CALLABLE PP_RESCAN_ADD_ONE

#assert PP_RESCAN_CALLABLE(41) = 42
#assert PP_RESCAN_IDENTITY(PP_RESCAN_ADD_ONE)(41) = 42

'' Expansion may supply the opening parenthesis while source supplies the end.
#define PP_RESCAN_OPEN_CALL PP_RESCAN_ADD_ONE(

#assert PP_RESCAN_OPEN_CALL 41) = 42

'' A define expanded while arguments are read may provide their separator.
#define PP_RESCAN_COMMA ,
#define PP_RESCAN_ADD(left, right) ((left) + (right))

#assert PP_RESCAN_ADD(19 PP_RESCAN_COMMA 23) = 42

'' FreeBASIC expands arguments before ## joins the resulting text.
#define PP_RESCAN_JOIN(left, right) left##right
#define PP_RESCAN_STEM PP_RESCAN_GENERATED_
#define PP_RESCAN_SUFFIX 2
#define PP_RESCAN_GENERATED_2 42

#assert PP_RESCAN_JOIN(PP_RESCAN_STEM, PP_RESCAN_SUFFIX) = 42

'' The generated identifier can itself name a function-like macro.
#define PP_RESCAN_OPERATION_2(value) ((value) + 2)

#assert PP_RESCAN_JOIN(PP_RESCAN_OPERATION_, 2)(40) = 42

'' end of macro-rescan-edge-cases.bas
