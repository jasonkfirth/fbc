''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-directive-edge-cases.bas
''
'' Purpose:
''     Exercise preprocessor directives produced by multiline macro expansion.
''
'' Responsibilities:
''     Check generated definitions, generated conditionals, state replacement,
''     and conditional terminators reached while source is being skipped.
''
'' This file intentionally does not contain:
''     Ordinary multiline statement generation unrelated to preprocessing.
''
' TEST_MODE : COMPILE_ONLY_OK

'' Parameters may become both the name and body of a generated definition.
#macro PP_DIRECTIVE_DEFINE(name, value)
	#define name value
#endmacro

PP_DIRECTIVE_DEFINE(PP_DIRECTIVE_CREATED, 42)

#assert defined(PP_DIRECTIVE_CREATED)
#assert PP_DIRECTIVE_CREATED = 42

'' Generated directives can replace an existing definition in one expansion.
#macro PP_DIRECTIVE_REPLACE(value)
	#undef PP_DIRECTIVE_CREATED
	#define PP_DIRECTIVE_CREATED value
#endmacro

PP_DIRECTIVE_REPLACE(40 + 2)

#assert PP_DIRECTIVE_CREATED = 42

'' A macro body may select which definition becomes visible after expansion.
#macro PP_DIRECTIVE_SELECT(flag, name)
	#if flag
		#define name 42
	#else
		#define name 0
	#endif
#endmacro

PP_DIRECTIVE_SELECT(1, PP_DIRECTIVE_SELECTED)

#assert defined(PP_DIRECTIVE_SELECTED)
#assert PP_DIRECTIVE_SELECTED = 42

'' Inactive-group scanning still expands macros because a macro can provide
'' the #else and #endif that return the lexer to active source.
#macro PP_DIRECTIVE_FALSE_TAIL()
	#else
		#define PP_DIRECTIVE_FROM_ELSE 42
	#endif
#endmacro

#if 0
	PP_DIRECTIVE_FALSE_TAIL()

#assert defined(PP_DIRECTIVE_FROM_ELSE)
#assert PP_DIRECTIVE_FROM_ELSE = 42

'' Directives emitted by a nested macro call retain the outer arguments.
#macro PP_DIRECTIVE_FORWARD(callable, name, value)
	callable(name, value)
#endmacro

PP_DIRECTIVE_FORWARD(PP_DIRECTIVE_DEFINE, PP_DIRECTIVE_FORWARDED, 42)

#assert defined(PP_DIRECTIVE_FORWARDED)
#assert PP_DIRECTIVE_FORWARDED = 42

'' end of macro-directive-edge-cases.bas
