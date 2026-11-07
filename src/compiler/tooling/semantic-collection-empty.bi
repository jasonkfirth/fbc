'' Project: FreeBASIC compiler - collection emptiness observations
'' File: tooling/semantic-collection-empty.bi
'' Purpose: Retain emptiness APIs for selected size and comparison calls.
'' Responsibilities: Scoped observation values and parser hook declarations.
'' This file intentionally does NOT own AST nodes or assert library semantics.
#ifndef __FB_SEMANTIC_COLLECTION_EMPTY_BI__
#define __FB_SEMANTIC_COLLECTION_EMPTY_BI__

type FB_SEMANTIC_COLLECTION_EMPTY
	receiver_id as longint
	payload(0 to 2) as string
	comparison_side as integer
end type

declare sub fbSemanticModelCollectionEmptyBegin(byref observation as FB_SEMANTIC_COLLECTION_EMPTY, _
	byval callee as FBSYMBOL ptr, byval receiver as ASTNODE ptr, byval source_type as FBSYMBOL ptr, byval comparison as integer = FALSE)
declare sub fbSemanticModelCollectionEmptyFinish(byref observation as FB_SEMANTIC_COLLECTION_EMPTY, byval binding as longint)

declare function fbSemanticModelCollectionEmptyUnaryCall(byval callee as FBSYMBOL ptr, byval receiver as ASTNODE ptr) as ASTNODE ptr
declare function fbSemanticModelCollectionEmptyComparisonCall(byval callee as FBSYMBOL ptr, byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr) as ASTNODE ptr
declare sub fbSemanticModelExportCollectionEmptyCount( )

#endif
'' end of tooling/semantic-collection-empty.bi
