'' Project: FreeBASIC compiler - semantic expression associations
'' File: tooling/semantic-expressions.bi
'' Purpose: Associate parser-selected expression identities with surviving ASTs.
'' Responsibilities: Define checked module-lifetime expression/node observations.
'' This file intentionally does NOT contain: expression parsing or graph inference.

#ifndef __FB_SEMANTIC_EXPRESSIONS_BI__
#define __FB_SEMANTIC_EXPRESSIONS_BI__

#include once "ast/ast.bi"
#include once "lexer/lex.bi"

declare sub fbSemanticModelResetExpressions( )
declare sub fbSemanticModelAttachExpression(byval node as ASTNODE ptr, byval identity as longint)
declare sub fbSemanticModelExportExpressionLinks(byval node as ASTNODE ptr, byval identity as longint)
declare function fbSemanticModelCaptureOperands _
	( byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, byref kind as const string, _
	  byval op as integer, byref source as LEX_LOCATION ) as longint
declare sub fbSemanticModelAttachOperands(byval node as ASTNODE ptr, byval identity as longint)
declare sub fbSemanticModelExportOperands(byval node as ASTNODE ptr, byval identity as longint)

#endif

'' end of tooling/semantic-expressions.bi
