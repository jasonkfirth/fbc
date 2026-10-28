'' Project: FreeBASIC compiler - consumed lexer tokens
'' File: tooling/semantic-tokens.bi
'' Purpose: Copy actual committed parser tokens without requesting more input.
'' Responsibilities: Bounded native token payload and source/lookup provenance.
'' This file intentionally does NOT contain: tokenization or name resolution.
#ifndef __FB_SEMANTIC_TOKENS_BI__
#define __FB_SEMANTIC_TOKENS_BI__
#include once "lexer/lex.bi"
declare sub fbSemanticModelConsumedToken( byval token as FBTOKEN ptr )
declare sub fbSemanticModelResetTokens( )
declare sub fbSemanticModelFinishTokens( byval commit as integer )
#endif
'' end of tooling/semantic-tokens.bi
