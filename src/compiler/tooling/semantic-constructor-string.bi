'' Project: FreeBASIC compiler - constructor lookup observations
'' File: tooling/semantic-constructor-string.bi
'' Purpose: Observe string alternatives to selected numeric constructors.
'' Responsibilities: Read-only overload queries and complete query counts.
'' This file intentionally does NOT define decimal-library contracts or fixes.
#ifndef __FB_SEMANTIC_CONSTRUCTOR_STRING_BI__
#define __FB_SEMANTIC_CONSTRUCTOR_STRING_BI__

declare sub fbSemanticModelConstructorString(byval head as FBSYMBOL ptr, byval selected as FBSYMBOL ptr, _
	byval argument_count as integer, byval arguments as FB_CALL_ARG ptr, byval options as FB_SYMBFINDOPT)
declare sub fbSemanticModelConstructorStringCount( )

#endif
'' end of tooling/semantic-constructor-string.bi
