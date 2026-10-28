'' Project: FreeBASIC compiler - native lookup explanations
'' File: tooling/semantic-lookup.bi
'' Purpose: Observe actual name searches without repeating them.
'' Responsibilities: Scoped probes, precedence decisions and returned chain copies.
'' This file intentionally does NOT contain: hash lookup or candidate selection policy.

#ifndef __FB_SEMANTIC_LOOKUP_BI__
#define __FB_SEMANTIC_LOOKUP_BI__

#include once "core/fbint.bi"
#include once "symbols/symb.bi"

declare function fbSemanticModelLookupBegin( byref kind as const string, byval spelling as const zstring ptr, _
	byval namespc as FBSYMBOL ptr, byval preserve_case as integer, byval search_imports as integer ) as longint
declare function fbSemanticModelLookupProbe( byref phase as const string, byval namespc as FBSYMBOL ptr, _
	byval found as FBSYMBOL ptr ) as FBSYMBOL ptr
declare sub fbSemanticModelLookupDecision( byref reason as const string, byval namespc as FBSYMBOL ptr, _
	byval found as FBSYMBOL ptr )
declare function fbSemanticModelLookupEnd( byval identity as longint, byval result as FBSYMCHAIN ptr, _
	byval token as integer, byval token_class as integer, byref route as const string ) as FBSYMCHAIN ptr
declare sub fbSemanticModelResetLookup( )
declare sub fbSemanticModelFinishLookup( byval commit as integer )

#endif
'' end of tooling/semantic-lookup.bi
