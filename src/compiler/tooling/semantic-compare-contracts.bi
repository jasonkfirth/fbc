'' Project: FreeBASIC compiler - comparison contract observations
'' File: tooling/semantic-compare-contracts.bi
'' Purpose: Keep original operands alive during one selected member call.
'' Responsibilities: Scoped AST ownership and observational parser hooks.
'' This file intentionally does NOT assert library equality semantics.

#ifndef __FB_SEMANTIC_COMPARE_CONTRACTS_BI__
#define __FB_SEMANTIC_COMPARE_CONTRACTS_BI__

type FB_SEMANTIC_COMPARE_PROBE
	receiver as ASTNODE ptr
	callee as FBSYMBOL ptr
	receiver_id as longint
	argument_id as longint
	payload as string
	declare constructor( )
	declare destructor( )
end type

declare sub fbSemanticModelCompareBegin(byref observation as FB_SEMANTIC_COMPARE_PROBE, _
	byval callee as FBSYMBOL ptr, byval receiver as ASTNODE ptr, byval source_type as FBSYMBOL ptr)
declare sub fbSemanticModelCompareArgument(byref observation as FB_SEMANTIC_COMPARE_PROBE, _
	byval parameter as FBSYMBOL ptr, byval argument as ASTNODE ptr, byval argument_mode as integer)
declare sub fbSemanticModelCompareFinish(byref observation as FB_SEMANTIC_COMPARE_PROBE, byval binding as longint)
declare sub fbSemanticModelExportCompareContractCount( )

#endif
'' end of tooling/semantic-compare-contracts.bi
