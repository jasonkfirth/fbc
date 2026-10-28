'' Project: FreeBASIC compiler - procedure runtime state
'' File: tooling/semantic-procedure-state.bi
'' Purpose: Observe native hidden error-handler and GOSUB ownership.
'' Responsibilities: Copy selected storage and generated-action targets before teardown.
'' This file intentionally does NOT contain: runtime error or GOSUB policy.
#ifndef __FB_SEMANTIC_PROCEDURE_STATE_BI__
#define __FB_SEMANTIC_PROCEDURE_STATE_BI__
#include once "core/fbint.bi"
#include once "symbols/symb.bi"

declare sub fbSemanticModelProcedureState( byval proc as FBSYMBOL ptr, byref kind as const string, _
	byref mode as const string = "none", byval temporary as FBSYMBOL ptr = NULL, byval target as FBSYMBOL ptr = NULL )
declare sub fbSemanticModelResetProcedureState( )
declare sub fbSemanticModelFinishProcedureState( byval commit as integer )
#endif
'' end of tooling/semantic-procedure-state.bi
