'' Project: FreeBASIC compiler - frontend ABI snapshots
'' File: tooling/semantic-abi.bi
'' Purpose: Expose compiler-known procedure result and formal parameter policy.
'' Responsibilities: Capture-time ABI interfaces and module closure.
'' This file intentionally does NOT contain: backend register or stack allocation.

#ifndef __FB_SEMANTIC_ABI_BI__
#define __FB_SEMANTIC_ABI_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetAbi( )
declare sub fbSemanticModelFinishAbi( byval commit as integer )
declare sub fbSemanticModelProcedureAbi( byval proc as FBSYMBOL ptr, byval identity as longint )
declare sub fbSemanticModelParameterAbi( byval proc as FBSYMBOL ptr, byval param as FBSYMBOL ptr, _
	byval identity as longint, byval ordinal as integer, byval instance as integer )

#endif

'' end of tooling/semantic-abi.bi
