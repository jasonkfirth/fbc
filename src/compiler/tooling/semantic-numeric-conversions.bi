'' Project: FreeBASIC semantic numeric conversions
'' File: tooling/semantic-numeric-conversions.bi
'' Purpose: Observe actual scalar conversion inputs before folding or lowering.
'' Responsibilities: Expose one internal constructor hook with explicit source intent.
'' This file intentionally does NOT contain: value evaluation or diagnostic selection.

#pragma once
#include once "ast/ast.bi"

declare sub fbSemanticModelNumericConversion _
	( byval expr as ASTNODE ptr, byval target_dtype as integer, _
	  byval target_subtype as FBSYMBOL ptr, byval options as AST_CONVOPT )

declare sub fbSemanticModelResetNumericConversions( )
declare sub fbSemanticModelFinishNumericConversions( )

declare sub fbSemanticModelNumericConversionSource _
	( byval expr as ASTNODE ptr, byval target_dtype as integer, _
	  byval target_subtype as FBSYMBOL ptr, byval options as AST_CONVOPT, _
	  byval original_expression as longint )

'' end of tooling/semantic-numeric-conversions.bi
