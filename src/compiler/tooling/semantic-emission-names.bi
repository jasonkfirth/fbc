'' Project: FreeBASIC compiler - semantic emission identities
'' File: tooling/semantic-emission-names.bi
'' Purpose: Observe names already selected by a backend.
'' Responsibilities: Bounded name observations and module count lifecycle.
'' This file intentionally does NOT contain: mangling or linker resolution.

#ifndef __FB_SEMANTIC_EMISSION_NAMES_BI__
#define __FB_SEMANTIC_EMISSION_NAMES_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetEmissionNames( )
declare sub fbSemanticModelFinishEmissionNames( )
declare function fbSemanticModelObjectNameNeeded( byval sym as FBSYMBOL ptr ) as integer
declare sub fbSemanticModelEmissionName _
	( byval sym as FBSYMBOL ptr, byref stage as const string, byref selected_name as const string, _
	  byref state as const string = "selected", byref reason as const string = "" )
#endif

'' end of tooling/semantic-emission-names.bi
