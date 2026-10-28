'' Project: FreeBASIC compiler - native IR control flow
'' File: tooling/semantic-ir-flow.bi
'' Purpose: Observe actual outer IR dispatch and its procedure-local transfers.
'' Responsibilities: Copied actions, labels, tables, blocks and unknown destinations.
'' This file intentionally does NOT contain: IR emission or target selection.

#ifndef __FB_SEMANTIC_IR_FLOW_BI__
#define __FB_SEMANTIC_IR_FLOW_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetIRFlow( )
declare sub fbSemanticModelFinishIRFlow( byval commit as integer )
declare sub fbSemanticModelIRFlowBegin( byval proc as FBSYMBOL ptr, byval entry_label as FBSYMBOL ptr )
declare sub fbSemanticModelIRFlowEnd( byval proc as FBSYMBOL ptr, byval exit_label as FBSYMBOL ptr )
declare sub fbSemanticModelIRFlowAction( byref kind as const string, byval opcode as integer = 0, _
	byval label as FBSYMBOL ptr = NULL, byval callee as FBSYMBOL ptr = NULL, _
	byref role as const string = "", byval argument as longint = 0 )
declare sub fbSemanticModelIRFlowTable( byval values as ulongint ptr, byval labels as FBSYMBOL ptr ptr, _
	byval count as integer, byval fallback as FBSYMBOL ptr, byval bias as ulongint, byval span as ulongint )
declare sub fbSemanticModelIRFlowAddress( byval sym as FBSYMBOL ptr )

#endif

'' end of tooling/semantic-ir-flow.bi
