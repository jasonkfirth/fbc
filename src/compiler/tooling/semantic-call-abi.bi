'' Project: FreeBASIC compiler - selected call ABI observations
'' File: tooling/semantic-call-abi.bi
'' Purpose: Copy native call-site context and selected backend argument components.
'' Responsibilities: Bounded immediate observations and explicit call closure.
'' This file intentionally does NOT contain: lowering or machine allocation.

#ifndef __FB_SEMANTIC_CALL_ABI_BI__
#define __FB_SEMANTIC_CALL_ABI_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetCallAbi( )
declare sub fbSemanticModelFinishCallAbi( byval commit as integer )
declare sub fbSemanticModelCallAbiContext( byval node as ASTNODE ptr, byval caller as FBSYMBOL ptr )
declare function fbSemanticModelCallAbiBegin( byval proc as FBSYMBOL ptr, byref stage as const string, _
	byval indirect as integer, byval demanded as integer, byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	byref result_type as const string, byref convention as const string, byval level as integer, _
	byval bytestopop as integer ) as longint
declare sub fbSemanticModelCallAbiArgument( byval identity as longint, byval param as FBSYMBOL ptr, _
	byval part as integer, byval parts as integer, byref kind as const string, _
	byval input_dtype as integer, byval input_subtype as FBSYMBOL ptr, _
	byval selected_dtype as integer, byval selected_subtype as FBSYMBOL ptr, _
	byref spelling as const string, byref attributes as const string, byref value as const string )
declare sub fbSemanticModelCallAbiEnd( byval identity as longint, byref text as const string )
declare sub fbSemanticModelCallAbiBuiltin( byval proc as FBSYMBOL ptr, byval level as integer )

#endif

'' end of tooling/semantic-call-abi.bi
