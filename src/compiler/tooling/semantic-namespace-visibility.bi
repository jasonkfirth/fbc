'' Project: FreeBASIC compiler - semantic namespace visibility
'' File: tooling/semantic-namespace-visibility.bi
'' Purpose: Observe accepted USING operands and native import relationships.
'' Responsibilities: Source requests, relationship lifetime and module closure.
'' This file intentionally does NOT contain: name lookup or import policy.

#ifndef __FB_SEMANTIC_NAMESPACE_VISIBILITY_BI__
#define __FB_SEMANTIC_NAMESPACE_VISIBILITY_BI__
#include once "core/fbint.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticModelResetNamespaceVisibility( )
declare sub fbSemanticModelFinishNamespaceVisibility( byval commit as integer )
declare function fbSemanticModelUsingBegin _
	( byval ns as FBSYMBOL ptr, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	  byval first_nonphysical as longint, byval last_nonphysical as longint, _
	  byval first_binding as longint, byval last_binding as longint ) as longint
declare sub fbSemanticModelUsingEnd( byval identity as longint, byval native_result as integer )
declare sub fbSemanticModelNamespaceImportAdded( byval imported as FBSYMBOL ptr )
declare sub fbSemanticModelNamespaceImportRemoved( byval imported as FBSYMBOL ptr, byval hashonly as integer )
declare sub fbSemanticModelNamespaceRefresh( byval ns as FBSYMBOL ptr, byval entering as integer )
declare sub fbSemanticModelBindingScopeContext( byval binding as longint )
#endif

'' end of tooling/semantic-namespace-visibility.bi
