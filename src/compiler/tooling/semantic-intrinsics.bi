'' Project: FreeBASIC compiler - semantic intrinsic dispatch
'' File: tooling/semantic-intrinsics.bi
'' Purpose: Observe the native statement and function dispatchers.
'' Responsibilities: Carry copied entry context to the actual parser return.
'' This file intentionally does NOT contain: intrinsic selection or AST ownership.

#ifndef __FB_SEMANTIC_INTRINSICS_BI__
#define __FB_SEMANTIC_INTRINSICS_BI__

#include once "ast/ast.bi"
#include once "lexer/lex.bi"

type SEMANTIC_INTRINSIC_SITE

	enabled as integer
	errors_before as integer
	statement as longint
	configuration as longint
	source as LEX_LOCATION
end type

declare sub fbSemanticModelIntrinsicBegin( byref site as SEMANTIC_INTRINSIC_SITE )
declare sub fbSemanticModelIntrinsicEnd( byref site as SEMANTIC_INTRINSIC_SITE, _
	byref kind as const string, byref route as const string, byval token as integer, _
	byval accepted as integer, byval node as ASTNODE ptr = NULL )
declare sub fbSemanticModelResetIntrinsics( )
declare sub fbSemanticModelFinishIntrinsics( byval commit as integer )

#endif
'' end of tooling/semantic-intrinsics.bi
