'' Project: FreeBASIC compiler - semantic expression associations
'' File: tooling/semantic-expressions.bas
'' Purpose: Preserve observed source expressions through AST cloning and lowering.
'' Responsibilities: Own immutable association chains and export surviving links.
'' This file intentionally does NOT contain: inferred expression parents or execution claims.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-expressions.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-macros.bi"

'' Several precedence results can describe the same AST node. Immutable
'' module-lifetime chains retain them all, and cloned ASTs can share a chain
'' without introducing ownership into the compiler's ordinary AST lifecycle.
type SEMANTIC_EXPRESSION_LINK
	expression as longint
	previous as longint
end type

'' One source result can be observed on more than one AST allocation while
'' parser layers deduplicate their facts. Bound these associations separately
'' from the one-million-expression and sixteen-million-detail budgets.
const SEMANTIC_EXPRESSION_MAX_LINKS = 4000000
dim shared as SEMANTIC_EXPRESSION_LINK ptr expression_links
dim shared as integer expression_link_count, expression_link_capacity

'' AST builders can consume or rewrite both operands, including folding a
'' complete operation into an existing constant allocation. Keep their typed
'' source identities before entering the builder. Fixed buffers make this
'' observation array safe to relocate without BASIC string ownership changes.
type SEMANTIC_EXPRESSION_OPERANDS
	left as longint
	right as longint
	result as longint
	kind as zstring * 24
	code as zstring * 64
	source as LEX_LOCATION
end type

const SEMANTIC_EXPRESSION_MAX_OPERANDS = 1000000
dim shared as SEMANTIC_EXPRESSION_OPERANDS ptr expression_operands
dim shared as integer expression_operand_count, expression_operand_capacity

sub fbSemanticModelResetExpressions( )
	deallocate(expression_links)
	expression_links = NULL
	expression_link_count = 0
	expression_link_capacity = 0
	deallocate(expression_operands)
	expression_operands = NULL
	expression_operand_count = 0
	expression_operand_capacity = 0
end sub

sub fbSemanticModelAttachExpression(byval node as ASTNODE ptr, byval identity as longint)
	if( (node = NULL) or (identity = 0) ) then exit sub
	node->semantic_expression = identity
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint link = node->semantic_expressions
	while( link <> 0 )
		if( (link < 1) or (link > expression_link_count) ) then
			fbSemanticModelFail( )
			exit sub
		end if
		if( expression_links[link - 1].expression = identity ) then exit sub
		link = expression_links[link - 1].previous
	wend
	if( expression_link_count >= SEMANTIC_EXPRESSION_MAX_LINKS ) then
		fbSemanticModelFail( )
		exit sub
	end if
	if( expression_link_count = expression_link_capacity ) then
		dim as integer capacity = iif(expression_link_capacity = 0, 128, expression_link_capacity * 2)
		if( capacity > SEMANTIC_EXPRESSION_MAX_LINKS ) then capacity = SEMANTIC_EXPRESSION_MAX_LINKS
		dim as SEMANTIC_EXPRESSION_LINK ptr storage = reallocate(expression_links, capacity * sizeof(SEMANTIC_EXPRESSION_LINK))
		if( storage = NULL ) then
			fbSemanticModelFail( )
			exit sub
		end if
		expression_links = storage
		expression_link_capacity = capacity
	end if
	expression_links[expression_link_count].expression = identity
	expression_links[expression_link_count].previous = node->semantic_expressions
	expression_link_count += 1
	node->semantic_expressions = expression_link_count
end sub

sub fbSemanticModelExportExpressionLinks(byval node as ASTNODE ptr, byval identity as longint)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint link = node->semantic_expressions
	while( link <> 0 )
		if( (link < 1) or (link > expression_link_count) ) then
			fbSemanticModelFail( )
			exit sub
		end if
		fbSemanticModelAppendDetail("H" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
			TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_links[link - 1].expression) + _
			TABCHAR + "source-expression" + TABCHAR + "0")
		link = expression_links[link - 1].previous
	wend
end sub

function fbSemanticModelCaptureOperands _
	( byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, byref kind as const string, _
	  byval op as integer, byref source as LEX_LOCATION ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	dim as longint left_id = 0, right_id = 0
	if( left_expr <> NULL ) then left_id = left_expr->semantic_expression
	if( right_expr <> NULL ) then right_id = right_expr->semantic_expression
	'' Bindings-only exports never assign expression IDs. Missing observations
	'' remain zero instead of being guessed from a node address or source range.
	if( (left_id = 0) and (right_id = 0) ) then return 0
	dim as string code = fbSemanticModelOperatorCode(op)
	select case kind
	case "group": code = "parentheses"
	case "cast": code = "cast"
	end select
	if( (len(kind) >= 24) or (len(code) >= 64) or (len(code) = 0) ) then
		fbSemanticModelFail( )
		return 0
	end if
	if( expression_operand_count >= SEMANTIC_EXPRESSION_MAX_OPERANDS ) then
		fbSemanticModelFail( )
		return 0
	end if
	if( expression_operand_count = expression_operand_capacity ) then
		dim as integer capacity = iif(expression_operand_capacity = 0, 128, expression_operand_capacity * 2)
		if( capacity > SEMANTIC_EXPRESSION_MAX_OPERANDS ) then capacity = SEMANTIC_EXPRESSION_MAX_OPERANDS
		dim as SEMANTIC_EXPRESSION_OPERANDS ptr storage = reallocate(expression_operands, capacity * sizeof(SEMANTIC_EXPRESSION_OPERANDS))
		if( storage = NULL ) then
			fbSemanticModelFail( )
			return 0
		end if
		expression_operands = storage
		expression_operand_capacity = capacity
	end if
	with expression_operands[expression_operand_count]
		.left = left_id
		.right = right_id
		.result = 0
		.kind = kind
		.code = code
		.source = source
		.source.is_physical = fbSemanticModelLocationIsPhysical(source)
	end with
	expression_operand_count += 1
	return expression_operand_count
end function

sub fbSemanticModelAttachOperands(byval node as ASTNODE ptr, byval identity as longint)
	if( node = NULL ) then exit sub
	if( identity = 0 ) then
		node->semantic_operands = 0
		exit sub
	end if
	if( (identity < 1) or (identity > expression_operand_count) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	node->semantic_operands = identity
end sub

sub fbSemanticModelExportOperands(byval node as ASTNODE ptr, byval identity as longint)
	if( (node = NULL) or (node->semantic_operands = 0) ) then exit sub
	dim as longint index = node->semantic_operands - 1
	if( (index < 0) or (index >= expression_operand_count) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	with expression_operands[index]
		'' Clones and precedence unwinding share this actual source operation.
		'' Bind it once to its first observed typed result; later allocations
		'' retain their existing source-expression links without inventing ops.
		if( .result <> 0 ) then exit sub
		.result = identity
		dim as longint source_id = .source.source_context
		if( source_id = 0 ) then source_id = fbSemanticModelCurrentSource( )
		fbSemanticModelAppendProvenance("EX" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + .kind + _
			TABCHAR + .code + TABCHAR + fbSemanticModelNumber(.left) + TABCHAR + fbSemanticModelNumber(.right) + _
			TABCHAR + fbSemanticModelNumber(source_id) + TABCHAR + fbSemanticModelNumber(abs(.source.is_physical)) + _
			TABCHAR + fbSemanticModelEscape(.source.source_file) + TABCHAR + fbSemanticModelNumber(.source.start_line) + _
			TABCHAR + fbSemanticModelNumber(.source.start_column) + TABCHAR + fbSemanticModelNumber(.source.end_line) + _
			TABCHAR + fbSemanticModelNumber(.source.end_column))
		fbSemanticModelExportCoordinates("expression", identity, "operator", .source, .source)
		fbSemanticModelMacroOrigin("expression", identity, .source.macro_identity, "operator")
	end with
end sub

'' end of tooling/semantic-expressions.bas
