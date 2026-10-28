'' Project: FreeBASIC compiler - native symbol filters
'' File: tooling/semantic-filters.bi
'' Purpose: Observe native candidate filters without repeating their predicates.
'' Responsibilities: Copied candidates, unchanged native predicates and returned symbols.
'' This file intentionally does NOT contain: name-search or overload policy.

#ifndef __FB_SEMANTIC_FILTERS_BI__
#define __FB_SEMANTIC_FILTERS_BI__
#include once "core/fbint.bi"
#include once "symbols/symb.bi"

declare function fbSemanticModelFilterBegin( byref kind as const string, byval class_ as integer, _
	byval dtype as integer, byval chain_ as FBSYMCHAIN ptr ) as longint
declare function fbSemanticModelFilterCandidate( byval sym as FBSYMBOL ptr, byval matches as integer, _
	byref reason as const string ) as integer
declare function fbSemanticModelFilterAccess( byval allowed as integer ) as integer
declare function fbSemanticModelFilterEnd( byval identity as longint, byval result as FBSYMBOL ptr, _
	byref outcome as const string ) as FBSYMBOL ptr
declare function fbSemanticModelLastFilter( ) as longint
declare sub fbSemanticModelQualifiedFilterUse( byval lookup_id as longint, byval filter_id as longint, _
	byval requested_class as integer, byval result as FBSYMBOL ptr )
declare sub fbSemanticModelResetFilters( )
declare sub fbSemanticModelFinishFilters( byval commit as integer )
#endif
'' end of tooling/semantic-filters.bi
