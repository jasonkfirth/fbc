'' Project: FreeBASIC compiler - native array mutation inputs
'' File: tooling/semantic-array-mutations.bi
'' Purpose: Preserve selected source ERASE and REDIM targets before lowering.
'' Responsibilities: Shared observer entry points and per-module count lifecycle.
'' This file intentionally does NOT contain: array semantics or linter policies.

#ifndef __FB_SEMANTIC_ARRAY_MUTATIONS_BI__
#define __FB_SEMANTIC_ARRAY_MUTATIONS_BI__
#include once "ast/ast.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticModelResetArrayMutations( )
declare sub fbSemanticModelFinishArrayMutations( )
declare sub fbSemanticModelArrayMutations(byval expr as ASTNODE ptr, byval sym as FBSYMBOL ptr, _
    byref kind as const string, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
    byval expanded as integer)

#endif
'' end of tooling/semantic-array-mutations.bi
