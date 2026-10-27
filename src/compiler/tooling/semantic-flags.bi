'' Project: FreeBASIC compiler - normalized semantic flags
'' File: tooling/semantic-flags.bi
'' Purpose: Observe named symbol flags and primitive type policies.
'' Responsibilities: Dictionary, snapshot and module closure interfaces.
'' This file intentionally does NOT contain: compiler policy changes or layout guesses.

#ifndef __FB_SEMANTIC_FLAGS_BI__
#define __FB_SEMANTIC_FLAGS_BI__
#include once "core/fbint.bi"
declare sub fbSemanticModelBeginFlags( )
declare sub fbSemanticModelFinishFlags( byval commit as integer )
declare sub fbSemanticModelSymbolFlags( byval sym as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelAggregateFlags( byval sym as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelProcedureFlags( byval sym as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelReturnMethod( byval sym as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelAggregateReturn( byval classification as integer, byval identity as longint )
declare sub fbSemanticModelPrimitivePolicy( byval dtype as integer )
#endif

'' end of tooling/semantic-flags.bi
