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
declare sub fbSemanticModelExportExpressionSource(byval node as ASTNODE ptr, byval identity as longint)
declare sub fbSemanticModelExportSizeQuerySources(byval node as ASTNODE ptr, byval identity as longint)
declare function fbSemanticModelExpressionCheckpoint( ) as longint
declare sub fbSemanticModelUnevaluatedQuery(byval checkpoint as longint, byref query_kind as const string)
declare function fbSemanticModelCaptureOperands _
	( byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, byref kind as const string, _
	  byval op as integer, byref source as LEX_LOCATION ) as longint
declare sub fbSemanticModelAttachOperands(byval node as ASTNODE ptr, byval identity as longint)
declare sub fbSemanticModelExportOperands(byval node as ASTNODE ptr, byval identity as longint)
declare function fbSemanticModelCaptureCompoundOperands _
	( byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, byval op as integer, _
	  byref source as LEX_LOCATION ) as longint
declare sub fbSemanticModelCompoundResult(byval result as ASTNODE ptr, byval operands as longint)
declare sub fbSemanticModelSelectedNumericOperands _
	( byval operands as longint, byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, _
	  byval left_dtype as integer, byval right_dtype as integer )

declare sub fbSemanticModelAssignmentTarget _
	( byval expression_id as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byref assignment_kind as const string )
declare sub fbSemanticModelStringInitializer _
	( byval expression_id as longint, byval target as FBSYMBOL ptr, byval dtype as integer, byval initializer as ASTNODE ptr )

declare sub fbSemanticModelStringInitializerCopy(byval initializer as ASTNODE ptr, byval is_static as integer)
declare sub fbSemanticModelExportStringInitializerCopies( )

declare sub fbSemanticModelSizeQuery _
	( byval expression_id as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byval operand_id as longint, _
	  byref query_kind as const string, byref input_kind as const string )

declare sub fbSemanticModelPointerAddress _
	( byval result as ASTNODE ptr, byval operand_id as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byval temporary_input as integer, byref address_kind as const string )
declare function fbSemanticModelAddressIsTemporary(byval node as ASTNODE ptr) as integer
declare sub fbSemanticModelPointerDereference _
	( byval result as ASTNODE ptr, byval operand_id as longint, byval dereferences as integer )
declare sub fbSemanticModelPointerIndex _
	( byval result as ASTNODE ptr, byval operand_id as longint, byval index_id as longint )
declare sub fbSemanticModelArraySubscripts _
	( byval result as ASTNODE ptr, byval array_symbol as FBSYMBOL ptr, _
	  indices() as longint, selected_indices() as longint, byval rank as integer, _
	  byref source_start as LEX_LOCATION, byval nonphysical_start as longint )
declare function fbSemanticModelSelectedArrayIndex _
	( byval expr as ASTNODE ptr, byref source_start as LEX_LOCATION, byval nonphysical_start as longint ) as longint
declare sub fbSemanticModelArrayBound _
	( byval result as ASTNODE ptr, byval array_symbol as FBSYMBOL ptr, byval tk as integer, _
	  byval original_dimension as longint, byval selected_dimension as longint, _
	  byref source_start as LEX_LOCATION, byval nonphysical_start as longint )
declare function fbSemanticModelPointerIndexPrefix _
	( byval expr as ASTNODE ptr, byref source_start as LEX_LOCATION, _
	  byval nonphysical_tokens as longint ) as longint
declare sub fbSemanticModelNewStorage _
	( byval temporary as FBSYMBOL ptr, byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval elements as ASTNODE ptr, byval do_clear as integer, byval is_array as integer, _
	  byval placement as ASTNODE ptr )
declare sub fbSemanticModelDeleteStorage(byval operand as ASTNODE ptr, byval is_array as integer)

declare function fbSemanticModelFileTransferInput( byval operand as ASTNODE ptr ) as longint
declare sub fbSemanticModelFileTransfer _
	( byval call_expr as ASTNODE ptr, byval operand_id as longint, _
	  byref transfer_kind as const string, byval is_array as integer )

declare function fbSemanticModelOriginalExpression _
	( byval operand as ASTNODE ptr, byref source_start as LEX_LOCATION, _
	  byval nonphysical_tokens_at_start as longint, byref source_end as LEX_LOCATION, _
	  byval nonphysical_tokens_at_end as longint ) as longint
declare sub fbSemanticModelStringIntrinsic _
	( byval result as ASTNODE ptr, byref kind as const string, byval count as integer, _
	  arguments() as longint, byval is_any as integer, byref source_start as LEX_LOCATION, _
	  byval nonphysical_start as longint )

#endif

'' end of tooling/semantic-expressions.bi
