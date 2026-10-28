'' Project: FreeBASIC compiler - selected backend ABI signatures
'' File: tooling/semantic-backend-abi.bi
'' Purpose: Copy ABI components chosen while native backends format signatures.
'' Responsibilities: Bounded signature/component capture and module closure.
'' This file intentionally does NOT contain: ABI selection or machine allocation.

#ifndef __FB_SEMANTIC_BACKEND_ABI_BI__
#define __FB_SEMANTIC_BACKEND_ABI_BI__
#include once "core/fbint.bi"

declare sub fbSemanticModelResetBackendAbi( )
declare sub fbSemanticModelFinishBackendAbi( byval commit as integer )
declare function fbSemanticModelBackendAbiBegin( byval proc as FBSYMBOL ptr, byref stage as const string, _
	byref role as const string, byref result_type as const string, byref convention as const string ) as longint
declare sub fbSemanticModelBackendAbiParameter( byval signature as longint, byval param as FBSYMBOL ptr, _
	byval part as integer, byval parts as integer, byref kind as const string, byref spelling as const string, _
	byval dtype as integer = FB_DATATYPE_INVALID, byval subtype as FBSYMBOL ptr = NULL, byref attributes as const string = "" )
declare sub fbSemanticModelBackendAbiEnd( byval signature as longint, byref header as const string )

#endif

'' end of tooling/semantic-backend-abi.bi
