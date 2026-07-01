' TEST_MODE : COMPILE_ONLY_OK

#define _DSL_KEYWORDS A, B, C, D, E, F, G, H

#macro _DSL_WALK0(P, C, R...)
	P(C)
	#if __FB_ARG_COUNT__(R) <= 0
		#undef W1
		#undef W0
	#else
		W1(P, R)
	#endif
#endmacro

#macro _DSL_WALK1(P, C, R...)
	P(C)
	#if __FB_ARG_COUNT__(R) <= 0
		#undef W1
		#undef W0
	#else
		W0(P, R)
	#endif
#endmacro

#macro _DSL_PROCESS(A)
	#print A
#endmacro

#macro _DSL_KEYWORD?(KW, VA...)
	#define W0 _DSL_WALK0
	#define W1 _DSL_WALK1
	W0(_DSL_PROCESS, _DSL_KEYWORDS)
#endmacro

_DSL_KEYWORD
