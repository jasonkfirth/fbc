'' Project: FreeBASIC compiler - semantic lifetime observations
'' File: tooling/semantic-lifetimes.bi
'' Purpose: Observe normal lexical destruction without selecting storage again.
'' Responsibilities: Define teardown group, selected action and retained node hooks.
'' This file intentionally does NOT contain: destructor selection or AST lowering.

#ifndef __FB_SEMANTIC_LIFETIMES_BI__
#define __FB_SEMANTIC_LIFETIMES_BI__

#include once "ast/ast.bi"

declare sub fbSemanticModelResetLifetimes( )
declare function fbSemanticModelLexicalCleanupBegin( byval owner as FBSYMBOL ptr ) as longint
declare sub fbSemanticModelLexicalCleanup( byval group_id as longint, byval ordinal as integer, byval variable as FBSYMBOL ptr, byval action as ASTNODE ptr )
declare sub fbSemanticModelLexicalCleanupEnd( byval group_id as longint, byval count as integer )
declare sub fbSemanticModelLexicalCleanupNode( byval node as ASTNODE ptr, byval identity as longint )
declare sub fbSemanticModelFinishLifetimes( byval commit as integer )

#endif

'' end of tooling/semantic-lifetimes.bi
