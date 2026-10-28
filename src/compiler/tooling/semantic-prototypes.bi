'' Project: FreeBASIC compiler - native prototype comparisons
'' File: tooling/semantic-prototypes.bi
'' Purpose: Observe actual declaration/prototype matching without repeating it.
'' Responsibilities: Native candidate, parameter comparison and return receipts.
'' This file intentionally does NOT contain: matching or overload scoring policy.
#ifndef __FB_SEMANTIC_PROTOTYPES_BI__
#define __FB_SEMANTIC_PROTOTYPES_BI__
#include once "core/fbint.bi"
#include once "symbols/symb.bi"

declare function fbSemanticModelPrototypeBegin( byval head as FBSYMBOL ptr, byval query as FBSYMBOL ptr, _
	byval options as integer ) as longint
declare function fbSemanticModelPrototypeCandidate( byval identity as longint, byval proc as FBSYMBOL ptr, _
	byval expected as integer, byval declared as integer, byval effective as integer, byval is_property as integer ) as longint
declare function fbSemanticModelPrototypeParameter( byval identity as longint, byval ordinal as longint, byval remaining as integer, _
	byref stage as const string, byval left_ as FBSYMBOL ptr, byval right_ as FBSYMBOL ptr, byval distinct as integer ) as integer
declare function fbSemanticModelPrototypeEnd( byval identity as longint, byval ordinal as longint, _
	byval result as FBSYMBOL ptr, byref outcome as const string ) as FBSYMBOL ptr
declare sub fbSemanticModelResetPrototypes( )
declare sub fbSemanticModelFinishPrototypes( byval commit as integer )
#endif
'' end of tooling/semantic-prototypes.bi
