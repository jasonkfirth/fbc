'' Project: FreeBASIC compiler - pragma history
'' File: tooling/semantic-pragmas.bi
'' Purpose: Observe native pragma parsing, stacks and callback actions.
'' Responsibilities: Copy actual selections, mutations, failures and source context.
'' This file intentionally does NOT contain: pragma policy or expression evaluation.

#ifndef __FB_SEMANTIC_PRAGMAS_BI__
#define __FB_SEMANTIC_PRAGMAS_BI__
#include once "symbols/symb.bi"

declare function fbSemanticModelPragmaBegin( byref requested as const string ) as longint
declare sub fbSemanticModelPragmaSelect( byval option_index as integer, byval group as integer, byval option_code as integer, byval flags as integer, byref mode as const string, byref spelling as const string )
declare sub fbSemanticModelPragmaUnknown( byref spelling as const string )
declare sub fbSemanticModelPragmaStack( byref kind as const string, byval option_index as integer, byval before as integer, byval after as integer, byval accepted as integer, byval value as longint )
declare sub fbSemanticModelPragmaApply( byval option_index as integer, byval requested as longint, byval supplied as longint, byval before as longint, byval after as longint, byval entry_context as longint )
declare sub fbSemanticModelPragmaCallback( byval option_index as integer, byref kind as const string )
declare sub fbSemanticModelPragmaReserve( byref role as const string, byval spelling as const zstring ptr, byval accepted as integer, byval sym as FBSYMBOL ptr = NULL )
declare sub fbSemanticModelPragmaEnd( byval identity as longint, byref reason as const string, byval errors_before as integer, byval errors_after as integer )
declare sub fbSemanticModelResetPragmas( )
declare sub fbSemanticModelFinishPragmas( byval commit as integer )

#endif
'' end of tooling/semantic-pragmas.bi
