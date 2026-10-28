'' Project: FreeBASIC compiler - semantic effects
'' File: tooling/semantic-effects.bi
'' Purpose: Preserve compiler-owned runtime effect contracts and uncertainty.
'' Responsibilities: Declaration snapshots, selected calls and IR action effects.
'' This file intentionally does NOT contain: optimization or alias-set inference.

#ifndef __FB_SEMANTIC_EFFECTS_BI__
#define __FB_SEMANTIC_EFFECTS_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetEffects( )
declare sub fbSemanticModelFinishEffects( byval commit as integer )
declare sub fbSemanticModelEffectDeclaration( byval sym as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelEffectCall( byval sym as FBSYMBOL ptr, byval indirect as integer, _
	byval graph as longint, byval action as longint, byval load_id as longint )
declare sub fbSemanticModelEffectOperation( byref kind as const string, byval opcode as integer, _
	byref role as const string, byval graph as longint, byval action as longint, byval load_id as longint )

#endif

'' end of tooling/semantic-effects.bi
