'' Project: FreeBASIC compiler - preprocessor evaluation contexts
'' File: tooling/semantic-evaluations.bi
'' Purpose: Observe native in-memory evaluation and query lexer lifetimes.
'' Responsibilities: Copy input, nesting, replay and returned result receipts.
'' This file intentionally does NOT contain: expression evaluation or symbol lookup.

#ifndef __FB_SEMANTIC_EVALUATIONS_BI__
#define __FB_SEMANTIC_EVALUATIONS_BI__
#include once "lexer/lex.bi"

declare function fbSemanticModelEvaluationBegin _
	( byval parent_ctx as LEX_TKCTX ptr, byref purpose as const string, _
	  byval value as const any ptr, byval units as integer, byval wide as integer, _
	  byval replay_of as longint = 0 ) as longint
declare sub fbSemanticModelEvaluationEnd _
	( byval identity as longint, byval value as const any ptr, byval units as integer, _
	  byval wide as integer, byval error_code as integer, byref reason as const string = "completed" )
declare sub fbSemanticModelResetEvaluations( )
declare sub fbSemanticModelFinishEvaluations( byval commit as integer )

#endif
'' end of tooling/semantic-evaluations.bi
