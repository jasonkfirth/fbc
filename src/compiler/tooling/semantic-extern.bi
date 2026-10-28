'' Project: FreeBASIC compiler - EXTERN policy observations
'' File: tooling/semantic-extern.bi
'' Purpose: Preserve native mangling scope selections and restorations.
'' Responsibilities: Copy committed EXTERN transitions and their source context.
'' This file intentionally does NOT contain: mangling selection or external name resolution.

#ifndef __FB_SEMANTIC_EXTERN_BI__
#define __FB_SEMANTIC_EXTERN_BI__
#include once "lexer/lex.bi"

declare function fbSemanticModelExternEnabled( ) as integer
declare sub fbSemanticModelExternTransition _
	( byref kind as const string, byval construct as longint, byval previous as integer, _
	  byval entry_context as longint, byref source as LEX_LOCATION, _
	  byref spelling as const string = "", byval present as integer = FALSE, byval recovered as integer = FALSE )
declare sub fbSemanticModelResetExtern( )
declare sub fbSemanticModelFinishExtern( byval commit as integer )

#endif
'' end of tooling/semantic-extern.bi
