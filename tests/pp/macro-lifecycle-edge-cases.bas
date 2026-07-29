''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-lifecycle-edge-cases.bas
''
'' Purpose:
''     Exercise definition, undefinition, redefinition, and lexical scope
''     transitions for preprocessor symbols.
''
'' Responsibilities:
''     Verify that stale expansion state and local definitions do not survive
''     their intended lifetime.
''
'' This file intentionally does not contain:
''     Conflicting redefinitions, which require diagnostic-specific tests.
''
' TEST_MODE : COMPILE_ONLY_OK

'' A self-named body is harmless if it is replaced before it is expanded.
#define PP_LIFE_RESET PP_LIFE_RESET
#undef PP_LIFE_RESET
#define PP_LIFE_RESET 42

#assert PP_LIFE_RESET = 42

'' The replacement can change between object-like and function-like forms.
#undef PP_LIFE_RESET
#define PP_LIFE_RESET(value) ((value) + 2)

#assert PP_LIFE_RESET(40) = 42

#undef PP_LIFE_RESET
#define PP_LIFE_RESET 42

#assert PP_LIFE_RESET = 42

'' Repeating an identical object-like definition is accepted.
#define PP_LIFE_IDENTICAL (40 + 2)
#define PP_LIFE_IDENTICAL (40 + 2)

#assert PP_LIFE_IDENTICAL = 42

'' Empty definitions still have a well-defined lifetime.
#define PP_LIFE_EMPTY

#assert defined(PP_LIFE_EMPTY)

#undef PP_LIFE_EMPTY
#undef PP_LIFE_EMPTY

#assert not defined(PP_LIFE_EMPTY)

'' Defines declared in a lexical scope disappear with that scope.
scope
	#define PP_LIFE_LOCAL 42
	#assert defined(PP_LIFE_LOCAL)
	#assert PP_LIFE_LOCAL = 42
end scope

#assert not defined(PP_LIFE_LOCAL)

'' end of macro-lifecycle-edge-cases.bas
