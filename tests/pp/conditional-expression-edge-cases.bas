''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: conditional-expression-edge-cases.bas
''
'' Purpose:
''     Exercise precedence, macro expansion, symbol state, and skipped groups
''     in preprocessor constant expressions.
''
'' Responsibilities:
''     Check FreeBASIC operators and ensure inactive source cannot affect the
''     active compilation state.
''
'' This file intentionally does not contain:
''     C-only preprocessor operators or C conditional-expression syntax.
''
' TEST_MODE : COMPILE_ONLY_OK

'' Arithmetic and bitwise operators keep the language's normal precedence.
#assert 1 + 2 * 3 = 7
#assert (1 + 2) * 3 = 9
#assert (1 shl 5) = 32
#assert (132 shr 2) = 33
#assert (5 and 3) = 1
#assert (5 xor 3) = 6
#assert not 0
#assert not not 7

'' The ordinary operators promise not to evaluate these right operands.
#assert 1 orelse (1 \ 0)
#assert not (0 andalso (1 \ 0))

'' Replacement text is fully rescanned before the condition is evaluated.
#define PP_COND_LEFT 6
#define PP_COND_RIGHT 7
#define PP_COND_PRODUCT (PP_COND_LEFT * PP_COND_RIGHT)
#define PP_COND_IS_ANSWER (PP_COND_PRODUCT = 42)

#if PP_COND_IS_ANSWER
	#define PP_COND_SELECTED
#else
	#error PP_COND_IS_ANSWER should select the first branch
#endif

#assert defined(PP_COND_SELECTED)

'' The defined() result composes with the rest of a constant expression.
#define PP_COND_PRESENT

#assert defined(PP_COND_PRESENT) and not defined(PP_COND_MISSING)

#undef PP_COND_PRESENT

#assert not defined(PP_COND_PRESENT)

'' Inactive groups may contain invalid program text and inactive definitions.
#if 0
	this is intentionally not valid FreeBASIC
	#define PP_COND_LEAKED_FROM_FALSE_BRANCH
	#if 1 \ 0
		#error a nested inactive condition must not be evaluated
	#endif
#else
	#define PP_COND_ACTIVE_BRANCH
#endif

#assert defined(PP_COND_ACTIVE_BRANCH)
#assert not defined(PP_COND_LEAKED_FROM_FALSE_BRANCH)

'' Nested groups are balanced while their outer group is being skipped.
#if 0
	#if 1
		#define PP_COND_NESTED_LEAK
	#else
		#define PP_COND_NESTED_LEAK
	#endif
#endif

#assert not defined(PP_COND_NESTED_LEAK)

'' ELSEIFDEF and ELSEIFNDEF test symbol state without evaluating source text.
#define PP_COND_SWITCH

#if 0
	#error the first branch must remain inactive
#elseifdef PP_COND_SWITCH
	#define PP_COND_ELSEIFDEF_SELECTED
#else
	#error ELSEIFDEF should have selected its branch
#endif

#assert defined(PP_COND_ELSEIFDEF_SELECTED)

#undef PP_COND_SWITCH

#if 0
	#error the first branch must remain inactive
#elseifndef PP_COND_SWITCH
	#define PP_COND_ELSEIFNDEF_SELECTED
#else
	#error ELSEIFNDEF should have selected its branch
#endif

#assert defined(PP_COND_ELSEIFNDEF_SELECTED)

'' end of conditional-expression-edge-cases.bas
