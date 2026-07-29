''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-empty-expansion-stress.bas
''
'' Purpose:
''     Exercise a broad macro expansion tree whose leaves are empty arguments.
''
'' Responsibilities:
''     Verify that empty expansions remain empty through repeated duplication.
''
'' This file intentionally does not contain:
''     Runtime code or unbounded expansion.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_EMPTY_STRESS_0(value) value
#define PP_EMPTY_STRESS_1(value) PP_EMPTY_STRESS_0(value) PP_EMPTY_STRESS_0(value)
#define PP_EMPTY_STRESS_2(value) PP_EMPTY_STRESS_1(value) PP_EMPTY_STRESS_1(value)
#define PP_EMPTY_STRESS_3(value) PP_EMPTY_STRESS_2(value) PP_EMPTY_STRESS_2(value)
#define PP_EMPTY_STRESS_4(value) PP_EMPTY_STRESS_3(value) PP_EMPTY_STRESS_3(value)
#define PP_EMPTY_STRESS_5(value) PP_EMPTY_STRESS_4(value) PP_EMPTY_STRESS_4(value)
#define PP_EMPTY_STRESS_6(value) PP_EMPTY_STRESS_5(value) PP_EMPTY_STRESS_5(value)
#define PP_EMPTY_STRESS_7(value) PP_EMPTY_STRESS_6(value) PP_EMPTY_STRESS_6(value)
#define PP_EMPTY_STRESS_8(value) PP_EMPTY_STRESS_7(value) PP_EMPTY_STRESS_7(value)
#define PP_EMPTY_STRESS_9(value) PP_EMPTY_STRESS_8(value) PP_EMPTY_STRESS_8(value)
#define PP_EMPTY_STRESS_10(value) PP_EMPTY_STRESS_9(value) PP_EMPTY_STRESS_9(value)
#define PP_EMPTY_STRESS_STRINGIZE(value) #value

'' This creates 1024 empty leaves before the outer argument is stringized.
#assert PP_EMPTY_STRESS_STRINGIZE(PP_EMPTY_STRESS_10()) = ""

'' end of macro-empty-expansion-stress.bas
