''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-call-unterminated.bas
''
'' Purpose:
''     Verify that end-of-file cannot silently terminate a macro invocation.
''
'' Responsibilities:
''     Isolate argument-list recovery at the physical end of the source file.
''
'' This file intentionally does not contain:
''     Additional parser errors after the incomplete invocation.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_UNTERMINATED_IDENTITY(value) value

dim as integer result = PP_UNTERMINATED_IDENTITY(42

'' end of macro-call-unterminated.bas
