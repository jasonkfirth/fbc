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
#include once "symbols/symb.bi"

declare sub fbSemanticModelExportCurrentExpressionPrefix(byval expr as ASTNODE ptr)
declare sub fbSemanticModelExportExpression _
	( byval expr as ASTNODE ptr, byref source_start as LEX_LOCATION, byref source_end as LEX_LOCATION, _
	  byval nonphysical_tokens_at_start as longint, byval nonphysical_tokens_at_end as longint, _
	  byval semantic_operator_override as integer = -1 )

'' -------------------------------------------------------------------------
'' Parser-selected numeric suffix spellings
'' -------------------------------------------------------------------------

sub fbSemanticModelNumericLiteral( byval token as FBTOKEN ptr )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (token = NULL) ) then exit sub
	if( lex.ctx->semantic_probe or (len(token->numeric_suffix) = 0) ) then exit sub
	'' Canonical numeric token text drops the suffix and can fold leading zeros.
	'' Record only the suffix actually consumed by the native number lexer, and
	'' only when the parser constructs a numeric literal. Expanded argument text,
	'' skipped branches and assembler operands do not establish literal meaning.
	'' The global namespace owns these module observations; no synthetic symbol
	'' or AST allocation is introduced. Unknown K properties remain compatible
	'' with existing schema-27 readers, while CAP advertises complete retention.
	dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	if( (owner = 0) or (identity = 0) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:37")
		exit sub
	end if
	dim as string property_name = "parsed-numeric-suffix-" + fbSemanticModelNumber(identity)
	dim as string value = fbSemanticModelNumber(token->source.source_context) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(token->source.macro_identity) + TABCHAR + _
		fbSemanticModelNumber(token->dtype) + TABCHAR + token->numeric_suffix
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + property_name + TABCHAR + fbSemanticModelEscape(value))
	fbSemanticModelExportCoordinates("source-context", token->source.source_context, property_name, token->source, token->source)
	fbSemanticModelMacroOrigin("symbol", owner, token->source.macro_identity, property_name)
end sub

'' Literal identity is a parser fact, independent of the constant's value.
'' Named constants and folded calculations can have the same C value but do
'' not become direct literals. Canonical token text excludes type suffixes;
'' the completeness flag prevents exact-decimal proofs on truncated text.
sub fbSemanticModelNumericLiteralExpression(byval expr as ASTNODE ptr, byval token as FBTOKEN ptr)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (expr = NULL) or (token = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( (token->class <> FB_TKCLASS_NUMLITERAL) or (expr->class <> AST_NODECLASS_CONST) ) then exit sub
	dim as longint counter = lex.ctx->nonphysical_token_count
	fbSemanticModelExportExpression(expr, token->source, token->source, counter, counter)
	dim as longint identity = expr->semantic_expression
	if( identity = 0 ) then exit sub
	dim as string token_text = token->text
	if( (len(token_text) = 0) or (len(token_text) > FB_MAXLITLEN) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:65")
		exit sub
	end if
	dim as string literal_kind = "integer"
	dim as string literal_base = "decimal"
	if( typeGetClass(token->dtype) = FB_DATACLASS_FPOINT ) then literal_kind = "float"
	select case ucase(left(token_text, 2))
	case "&H": literal_base = "hex"
	case "&O": literal_base = "octal"
	case "&B": literal_base = "binary"
	end select
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "numeric-literal-kind" + TABCHAR + literal_kind)
	fbSemanticModelAppendDetail(prefix + "numeric-literal-base" + TABCHAR + literal_base)
	fbSemanticModelAppendDetail(prefix + "numeric-literal-text" + TABCHAR + fbSemanticModelEscape(token_text))
	fbSemanticModelAppendDetail(prefix + "numeric-literal-text-complete" + TABCHAR + _
		fbSemanticModelNumber(abs(len(token_text) < FB_MAXNUMLEN)))
end sub

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
	numeric_inputs as integer
	selected_left_dtype as integer
	selected_right_dtype as integer
	selected_numeric as integer
	selected_left_expression as longint
	selected_right_expression as longint
end type

const SEMANTIC_EXPRESSION_MAX_OPERANDS = 1000000
dim shared as SEMANTIC_EXPRESSION_OPERANDS ptr expression_operands
dim shared as integer expression_operand_count, expression_operand_capacity

'' Size queries can be observed again while precedence layers unwind. Keep
'' their original IDs separately from operand/result values. Association chains
'' can outlive a folded operand, but never own or retain an AST allocation.
dim shared as longint ptr size_query_ids
dim shared as integer size_query_count, size_query_capacity
dim shared as longint size_query_work_left
const SEMANTIC_SIZE_QUERY_WORK_LIMIT = 8000000
dim shared as longint pointer_observation_work_left

'' The index owns no ASTs and uses at most one byte per expression. A cloned
'' TYPEINI can select more than one copy contract; keep that ambiguity instead
'' of letting the last lowering overwrite the first observation.
dim shared as ubyte ptr string_initializer_copies
dim shared as integer string_initializer_copy_capacity

sub fbSemanticModelResetExpressions( )
	deallocate(string_initializer_copies)
	string_initializer_copies = NULL
	string_initializer_copy_capacity = 0
	deallocate(expression_links)
	expression_links = NULL
	expression_link_count = 0
	expression_link_capacity = 0
	deallocate(expression_operands)
	expression_operands = NULL
	expression_operand_count = 0
	expression_operand_capacity = 0
	deallocate(size_query_ids)
	size_query_ids = NULL
	size_query_count = 0
	size_query_capacity = 0
	size_query_work_left = SEMANTIC_SIZE_QUERY_WORK_LIMIT
	pointer_observation_work_left = SEMANTIC_SIZE_QUERY_WORK_LIMIT
end sub

private function hSizeQueryIdentity(byval identity as longint) as integer
	dim as integer first = 0
	dim as integer last = size_query_count - 1
	while( first <= last )
		dim as integer middle = first + (last - first) \ 2
		if( size_query_ids[middle] = identity ) then return TRUE
		if( size_query_ids[middle] < identity ) then
			first = middle + 1
		else
			last = middle - 1
		end if
	wend
	return FALSE
end function

private sub hRememberSizeQuery(byval identity as longint)
	'' Expression identities are published monotonically. Reject a broken
	'' ordering rather than make binary lookup silently miss an observation.
	if( size_query_count > 0 ) then
		if( size_query_ids[size_query_count - 1] = identity ) then exit sub
		if( size_query_ids[size_query_count - 1] > identity ) then
			fbSemanticModelFailAt("semantic-expressions.bas:169")
			exit sub
		end if
	end if
	if( size_query_count >= SEMANTIC_EXPRESSION_MAX_OPERANDS ) then
		fbSemanticModelFailAt("semantic-expressions.bas:174")
		exit sub
	end if
	if( size_query_count = size_query_capacity ) then
		dim as integer capacity = iif(size_query_capacity = 0, 128, size_query_capacity * 2)
		if( capacity > SEMANTIC_EXPRESSION_MAX_OPERANDS ) then capacity = SEMANTIC_EXPRESSION_MAX_OPERANDS
		dim as longint ptr storage = reallocate(size_query_ids, capacity * sizeof(longint))
		if( storage = NULL ) then
			fbSemanticModelFailAt("semantic-expressions.bas:182")
			exit sub
		end if
		size_query_ids = storage
		size_query_capacity = capacity
	end if
	size_query_ids[size_query_count] = identity
	size_query_count += 1
end sub

sub fbSemanticModelExportSizeQuerySources(byval node as ASTNODE ptr, byval identity as longint)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (node = NULL) or (size_query_count = 0) ) then exit sub
	dim as longint link = node->semantic_expressions
	while( link <> 0 )
		size_query_work_left -= 1
		if( (link < 1) or (link > expression_link_count) or (size_query_work_left < 0) ) then
			fbSemanticModelFailAt("semantic-expressions.bas:198")
			exit sub
		end if
		dim as longint source_id = expression_links[link - 1].expression
		if( (source_id < identity) and hSizeQueryIdentity(source_id) ) then
			'' This is contributing parser provenance, not equality of values.
			'' Folding can reuse an operand allocation for a different result.
			fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
				TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(source_id) + _
				TABCHAR + "size-query-source" + TABCHAR + "0")
		end if
		link = expression_links[link - 1].previous
	wend
end sub

'' Keep one predecessor instead of repeatedly exporting every contribution.
'' This is parser provenance. Reusing an operand during folding can change
'' the value and type, so the relationship is never an equivalence claim.
sub fbSemanticModelExportExpressionSource(byval node as ASTNODE ptr, byval identity as longint)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (node = NULL) ) then exit sub
	dim as longint link = node->semantic_expressions
	if( link = 0 ) then exit sub
	if( (link < 1) or (link > expression_link_count) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:221")
		exit sub
	end if
	dim as longint source_id = expression_links[link - 1].expression
	if( source_id >= identity ) then exit sub
	fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(source_id) + _
		TABCHAR + "parsed-expression-source" + TABCHAR + "0")
end sub

sub fbSemanticModelAttachExpression(byval node as ASTNODE ptr, byval identity as longint)
	if( (node = NULL) or (identity = 0) ) then exit sub
	node->semantic_expression = identity
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint link = node->semantic_expressions
	while( link <> 0 )
		if( (link < 1) or (link > expression_link_count) ) then
			fbSemanticModelFailAt("semantic-expressions.bas:238")
			exit sub
		end if
		if( expression_links[link - 1].expression = identity ) then exit sub
		link = expression_links[link - 1].previous
	wend
	if( expression_link_count >= SEMANTIC_EXPRESSION_MAX_LINKS ) then
		fbSemanticModelFailAt("semantic-expressions.bas:245")
		exit sub
	end if
	if( expression_link_count = expression_link_capacity ) then
		dim as integer capacity = iif(expression_link_capacity = 0, 128, expression_link_capacity * 2)
		if( capacity > SEMANTIC_EXPRESSION_MAX_LINKS ) then capacity = SEMANTIC_EXPRESSION_MAX_LINKS
		dim as SEMANTIC_EXPRESSION_LINK ptr storage = reallocate(expression_links, capacity * sizeof(SEMANTIC_EXPRESSION_LINK))
		if( storage = NULL ) then
			fbSemanticModelFailAt("semantic-expressions.bas:253")
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
			fbSemanticModelFailAt("semantic-expressions.bas:270")
			exit sub
		end if
		fbSemanticModelAppendDetail("H" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
			TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_links[link - 1].expression) + _
			TABCHAR + "source-expression" + TABCHAR + "0")
		link = expression_links[link - 1].previous
	wend
end sub

private function hPrimitiveNumeric(byval dtype as integer, byval subtype as FBSYMBOL ptr = NULL) as integer
	if( typeGetPtrCnt(dtype) or (subtype <> NULL) ) then return FALSE
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_BOOLEAN, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, _
		FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
		FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE
		return TRUE
	end select
	return FALSE
end function

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
		fbSemanticModelFailAt("semantic-expressions.bas:307")
		return 0
	end if
	if( expression_operand_count >= SEMANTIC_EXPRESSION_MAX_OPERANDS ) then
		fbSemanticModelFailAt("semantic-expressions.bas:311")
		return 0
	end if
	if( expression_operand_count = expression_operand_capacity ) then
		dim as integer capacity = iif(expression_operand_capacity = 0, 128, expression_operand_capacity * 2)
		if( capacity > SEMANTIC_EXPRESSION_MAX_OPERANDS ) then capacity = SEMANTIC_EXPRESSION_MAX_OPERANDS
		dim as SEMANTIC_EXPRESSION_OPERANDS ptr storage = reallocate(expression_operands, capacity * sizeof(SEMANTIC_EXPRESSION_OPERANDS))
		if( storage = NULL ) then
			fbSemanticModelFailAt("semantic-expressions.bas:319")
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
		.numeric_inputs = FALSE
		.selected_left_dtype = 0
		.selected_right_dtype = 0
		.selected_numeric = FALSE
		.selected_left_expression = 0
		.selected_right_expression = 0
		if( (left_expr <> NULL) and (right_expr <> NULL) and (.left > 0) and (.right > 0) ) then
			.numeric_inputs = hPrimitiveNumeric(astGetFullType(left_expr), astGetSubType(left_expr)) and _
				hPrimitiveNumeric(astGetFullType(right_expr), astGetSubType(right_expr))
		end if
	end with
	expression_operand_count += 1
	return expression_operand_count
end function

'' Compound assignment inputs survive until the AST builder has selected the
'' operation. The result must be observed before assignment narrows its type.
'' These E ranges are logical operator anchors; EX retains the actual op= token.
function fbSemanticModelCaptureCompoundOperands _
	( byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, byval op as integer, _
	  byref source as LEX_LOCATION ) as longint
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (left_expr = NULL) or (right_expr = NULL) ) then return 0
	if( lex.ctx->semantic_probe ) then return 0
	if( left_expr->semantic_expression = 0 ) then
		dim as LEX_LOCATION anchor = source
		anchor.is_physical = FALSE
		fbSemanticModelExportExpression(left_expr, anchor, anchor, 0, 0)
	end if
	return fbSemanticModelCaptureOperands(left_expr, right_expr, "binary", op, source)
end function

sub fbSemanticModelSelectedNumericOperands _
	( byval operands as longint, byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr, _
	  byval left_dtype as integer, byval right_dtype as integer )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (operands = 0) ) then exit sub
	if( (operands < 1) or (operands > expression_operand_count) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:369")
		exit sub
	end if
	if( (left_expr = NULL) or (right_expr = NULL) ) then exit sub
	if( (expression_operands[operands - 1].numeric_inputs = FALSE) or _
		(hPrimitiveNumeric(left_dtype) = FALSE) or (hPrimitiveNumeric(right_dtype) = FALSE) ) then exit sub
	'' Reuse typed E/C observations for selected constants too. This preserves
	'' conversions such as a floating shift count before its later normalization.
	'' Logical operator anchors do not claim editable operand source extents.
	dim as LEX_LOCATION anchor = expression_operands[operands - 1].source
	anchor.is_physical = FALSE
	fbSemanticModelExportExpression(left_expr, anchor, anchor, 0, 0)
	dim as longint left_identity = left_expr->semantic_expression
	fbSemanticModelExportExpression(right_expr, anchor, anchor, 0, 0)
	dim as longint right_identity = right_expr->semantic_expression
	if( (left_identity = 0) or (right_identity = 0) ) then exit sub
	with expression_operands[operands - 1]
		.selected_left_dtype = left_dtype
		.selected_right_dtype = right_dtype
		.selected_left_expression = left_identity
		.selected_right_expression = right_identity
		.selected_numeric = TRUE
	end with
end sub

sub fbSemanticModelCompoundResult(byval result as ASTNODE ptr, byval operands as longint)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) or (operands = 0) ) then exit sub
	if( (operands < 1) or (operands > expression_operand_count) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:397")
		exit sub
	end if
	dim as LEX_LOCATION anchor = expression_operands[operands - 1].source
	anchor.is_physical = FALSE
	fbSemanticModelAttachOperands(result, operands)
	fbSemanticModelExportExpression(result, anchor, anchor, 0, 0)
end sub

sub fbSemanticModelAttachOperands(byval node as ASTNODE ptr, byval identity as longint)
	if( node = NULL ) then exit sub
	if( identity = 0 ) then
		node->semantic_operands = 0
		exit sub
	end if
	if( (identity < 1) or (identity > expression_operand_count) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:413")
		exit sub
	end if
	node->semantic_operands = identity
end sub

sub fbSemanticModelExportOperands(byval node as ASTNODE ptr, byval identity as longint)
	if( (node = NULL) or (node->semantic_operands = 0) ) then exit sub
	dim as longint index = node->semantic_operands - 1
	if( (index < 0) or (index >= expression_operand_count) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:423")
		exit sub
	end if
	with expression_operands[index]
		'' Clones and precedence unwinding share this actual source operation.
		'' Bind it once to its first observed typed result; later allocations
		'' retain their existing source-expression links without inventing ops.
		if( .result <> 0 ) then exit sub
		.result = identity
		if( .selected_numeric ) then
			fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
				TABCHAR + "numeric-selected-left-dtype" + TABCHAR + fbSemanticModelNumber(.selected_left_dtype))
			fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
				TABCHAR + "numeric-selected-right-dtype" + TABCHAR + fbSemanticModelNumber(.selected_right_dtype))
			fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
				TABCHAR + "numeric-selected-left-expression" + TABCHAR + fbSemanticModelNumber(.selected_left_expression))
			fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
				TABCHAR + "numeric-selected-right-expression" + TABCHAR + fbSemanticModelNumber(.selected_right_expression))
		end if
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

'' Record accepted source assignments before their original operand types are
'' lost to conversion, constant folding, or TYPEINI lowering. The caller keeps
'' an expression identity, never an AST pointer that a builder may have freed.
'' Numeric scalar assignments have one selected destination; aggregate LET
'' overloads and generated assignments are excluded. Parsed compound updates
'' supply their arithmetic result before destination conversion too.
sub fbSemanticModelAssignmentTarget _
	( byval expression_id as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byref assignment_kind as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( expression_id <= 0 ) then exit sub
	if( typeGetPtrCnt(dtype) <> 0 ) then exit sub
	if( subtype <> NULL ) then exit sub
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_BOOLEAN, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, _
		FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, _
		FB_DATATYPE_LONG, FB_DATATYPE_ULONG, FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, _
		FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE
	case else
		exit sub
	end select
	'' K expression properties preserve the original NT value role. Consumers
	'' must support expression properties, rather than overwrite the RHS type
	'' with its destination. Y supplies the target-selected storage widths.
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + _
		TABCHAR + "assignment-target-dtype" + TABCHAR + fbSemanticModelNumber(dtype))
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + _
		TABCHAR + "assignment-kind" + TABCHAR + assignment_kind)
end sub

'' Fixed character capacity belongs to the selected declaration, not its
'' primitive dtype. TYPEINI and static emission can consume the literal and
'' target independently. Keep their accepted association before that happens.
'' The symbol's length is one element's storage, including a Z/W terminator;
'' array dimensions do not multiply this initializer's element capacity.
sub fbSemanticModelStringInitializer _
	( byval expression_id as longint, byval target as FBSYMBOL ptr, byval dtype as integer, byval initializer as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( lex.ctx->semantic_probe or (expression_id <= 0) or (target = NULL) ) then exit sub
	if( typeGetPtrCnt(dtype) <> 0 ) then exit sub
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR, FB_DATATYPE_FIXSTR
	case else: exit sub
	end select
	if( (symbIsVar(target) = FALSE) and (symbIsField(target) = FALSE) ) then exit sub
	if( symbIsRef(target) ) then exit sub
	if( (initializer = NULL) or (initializer->class <> AST_NODECLASS_TYPEINI_ASSIGN) ) then exit sub
	if( expression_id > SEMANTIC_EXPRESSION_MAX_OPERANDS ) then
		fbSemanticModelFailAt("string initializer copy identity limit")
		exit sub
	end if
	if( expression_id >= string_initializer_copy_capacity ) then
		dim as integer capacity = iif(string_initializer_copy_capacity = 0, 128, string_initializer_copy_capacity)
		while( capacity <= expression_id )
			capacity *= 2
		wend
		if( capacity > SEMANTIC_EXPRESSION_MAX_OPERANDS + 1 ) then capacity = SEMANTIC_EXPRESSION_MAX_OPERANDS + 1
		dim as ubyte ptr storage = reallocate(string_initializer_copies, capacity)
		if( storage = NULL ) then
			fbSemanticModelFailAt("string initializer copy allocation")
			exit sub
		end if
		clear storage[string_initializer_copy_capacity], 0, capacity - string_initializer_copy_capacity
		string_initializer_copies = storage
		string_initializer_copy_capacity = capacity
	end if
	string_initializer_copies[expression_id] or= 1
	'' TYPEINI_ASSIGN already carries the parser result in the common semantic
	'' field. This association remains separate from the initializer RHS chains.
	initializer->semantic_expression = expression_id

	dim as longint target_id = fbSemanticModelSymbolId(target)
	if( target_id = 0 ) then exit sub
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "string-initializer-symbol" + TABCHAR + fbSemanticModelNumber(target_id))
	fbSemanticModelAppendDetail(prefix + "string-initializer-dtype" + TABCHAR + fbSemanticModelNumber(dtype))
	fbSemanticModelAppendDetail(prefix + "string-initializer-bytes" + TABCHAR + fbSemanticModelNumber(target->lgt))
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(target_id) + _
		TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR + "string-initializer" + TABCHAR + "0")
end sub

'' Static emission copies the literal payload. Runtime character input is
'' zero terminated, while String descriptors and fixed String inputs retain
'' their counted length. These are selected lowering facts, not storage-name
'' guesses. Unselected/dead initializers retain an explicit unknown contract.
sub fbSemanticModelStringInitializerCopy(byval initializer as ASTNODE ptr, byval is_static as integer)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( initializer = NULL ) then exit sub
	if( initializer->class <> AST_NODECLASS_TYPEINI_ASSIGN ) then exit sub
	dim as longint identity = initializer->semantic_expression
	if( (identity <= 0) or (identity >= string_initializer_copy_capacity) ) then exit sub
	if( (string_initializer_copies[identity] and 1) = 0 ) then exit sub
	dim as integer kind = 64
	if( initializer->l <> NULL ) then
		select case astGetFullType(initializer->l)
		case FB_DATATYPE_CHAR: kind = iif(is_static, 16, 2)
		case FB_DATATYPE_STRING, FB_DATATYPE_FIXSTR: if( is_static = FALSE ) then kind = 4
		case FB_DATATYPE_WCHAR: kind = iif(is_static, 32, 8)
		end select
	end if
	string_initializer_copies[identity] or= kind
end sub

sub fbSemanticModelExportStringInitializerCopies( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	for identity as integer = 1 to string_initializer_copy_capacity - 1
		if( (string_initializer_copies[identity] and 1) = 0 ) then continue for
		dim as string kind = "ambiguous"
		select case string_initializer_copies[identity]
		case 1: kind = "unselected"
		case 3: kind = "runtime-terminated"
		case 5: kind = "runtime-counted"
		case 9: kind = "runtime-wide"
		case 17: kind = "static-bytes"
		case 33: kind = "static-wide"
		end select
		fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + _
			TABCHAR + "string-initializer-copy" + TABCHAR + kind)
	next
end sub

'' LEN/SIZEOF can discard an unevaluated operand and replace it with a size
'' constant. Keep the parser's selected input type and expression identity;
'' neither the constant value nor the generated AST can recover that meaning.
sub fbSemanticModelSizeQuery _
	( byval expression_id as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byval operand_id as longint, _
	  byref query_kind as const string, byref input_kind as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( expression_id <= 0 ) then exit sub
	hRememberSizeQuery(expression_id)
	dim as longint subtype_id = fbSemanticModelSymbolId(subtype)
	if( subtype <> NULL ) then fbSemanticModelExportSymbolDetails(subtype)
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + _
		fbSemanticModelNumber(expression_id) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "size-query-kind" + TABCHAR + query_kind)
	fbSemanticModelAppendDetail(prefix + "size-query-dtype" + TABCHAR + fbSemanticModelNumber(dtype))
	fbSemanticModelAppendDetail(prefix + "size-query-subtype" + TABCHAR + fbSemanticModelNumber(subtype_id))
	fbSemanticModelAppendDetail(prefix + "size-query-operand" + TABCHAR + fbSemanticModelNumber(operand_id))
	fbSemanticModelAppendDetail(prefix + "size-query-input" + TABCHAR + input_kind)
end sub

'' -------------------------------------------------------------------------
'' Accepted address and allocation selections before lowering
'' -------------------------------------------------------------------------

'' These snapshots retain values and identities, never owned AST pointers.
'' TEMP is a compiler storage attribute. A source name or generated spelling
'' does not establish temporary lifetime. Walk only the selected storage base.
function fbSemanticModelAddressIsTemporary(byval node as ASTNODE ptr) as integer
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return FALSE
	if( lex.ctx->semantic_probe ) then return FALSE
	dim as integer depth = 0
	dim as integer follow_address = FALSE
	while( node <> NULL )
		pointer_observation_work_left -= 1
		depth += 1
		if( (pointer_observation_work_left < 0) or (depth > 128) ) then
			fbSemanticModelFailAt("semantic-expressions.bas:548")
			return FALSE
		end if
		if( follow_address ) then
			select case node->class
			case AST_NODECLASS_ADDROF
				node = node->l
				follow_address = FALSE
			case AST_NODECLASS_BOP
				if( (node->op.op <> AST_OP_ADD) and (node->op.op <> AST_OP_SUB) ) then return FALSE
				if( typeIsPtr(astGetFullType(node)) = FALSE ) then return FALSE
				node = node->l
			case else
				return FALSE
			end select
			continue while
		end if
		select case node->class
		case AST_NODECLASS_TYPEINI, AST_NODECLASS_CALLCTOR
			return TRUE
		case AST_NODECLASS_VAR
			if( node->sym = NULL ) then return FALSE
			return symbIsTemp(node->sym)
		case AST_NODECLASS_FIELD, AST_NODECLASS_CONV
			node = node->l
		case AST_NODECLASS_DEREF
			'' A temporary pointer value does not own its pointed-to storage.
			'' Only direct addressing preserves the constructed storage base.
			node = astSkipNoConvCAST(node->l)
			follow_address = TRUE
		case AST_NODECLASS_IDX
			node = node->r
		case AST_NODECLASS_LINK
			select case node->link.ret
			case AST_LINK_RETURN_LEFT: node = node->l
			case AST_LINK_RETURN_RIGHT: node = node->r
			case else: return FALSE
			end select
		case else
			return FALSE
		end select
	wend
	return FALSE
end function

sub fbSemanticModelPointerAddress _
	( byval result as ASTNODE ptr, byval operand_id as longint, byval dtype as integer, _
	  byval subtype as FBSYMBOL ptr, byval temporary_input as integer, byref address_kind as const string )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	fbSemanticModelExportCurrentExpressionPrefix(result)
	dim as longint identity = result->semantic_expression
	if( (identity <= 0) or (operand_id <= 0) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:601")
		exit sub
	end if
	dim as longint subtype_id = fbSemanticModelSymbolId(subtype)
	if( subtype <> NULL ) then fbSemanticModelExportSymbolDetails(subtype)
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "pointer-address-kind" + TABCHAR + address_kind)
	fbSemanticModelAppendDetail(prefix + "pointer-address-dtype" + TABCHAR + fbSemanticModelNumber(dtype))
	fbSemanticModelAppendDetail(prefix + "pointer-address-subtype" + TABCHAR + fbSemanticModelNumber(subtype_id))
	fbSemanticModelAppendDetail(prefix + "pointer-address-operand" + TABCHAR + fbSemanticModelNumber(operand_id))
	fbSemanticModelAppendDetail(prefix + "pointer-address-temporary" + TABCHAR + fbSemanticModelNumber(abs(temporary_input <> FALSE)))
end sub

'' astBuildMultiDeref can cancel an address expression or consume its AST.
'' Preserve the parsed input identity before that lowering. The result carries
'' its own source range; no AST pointer survives this observation.
sub fbSemanticModelPointerDereference _
	( byval result as ASTNODE ptr, byval operand_id as longint, byval dereferences as integer )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	dim as longint identity = result->semantic_expression
	if( (identity <= 0) or (operand_id <= 0) or (dereferences < 1) or (dereferences > 8) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:623")
		exit sub
	end if
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "pointer-dereference-operand" + TABCHAR + fbSemanticModelNumber(operand_id))
	fbSemanticModelAppendDetail(prefix + "pointer-dereference-count" + TABCHAR + fbSemanticModelNumber(dereferences))
end sub

'' Built-in indexing scales its input and may fold the resulting address.
'' Record the original pointer and index before that transformation. A field
'' following [] can have a different result type from the indexed pointee.
function fbSemanticModelPointerIndexPrefix _
	( byval expr as ASTNODE ptr, byref source_start as LEX_LOCATION, _
	  byval nonphysical_tokens as longint ) as longint
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (expr = NULL) ) then return 0
	if( lex.ctx->semantic_probe ) then return 0
	fbSemanticModelExportCurrentExpressionPrefix(expr)
	'' Assignment targets enter the variable parser without cExpression's
	'' active range. Preserve their accepted AST with the parser's observed
	'' prefix anchor, so a later read can be related to an actual prior store.
	if( expr->semantic_expression = 0 ) then
		dim as LEX_LOCATION source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(expr, source_start, source_end, _
			nonphysical_tokens, lexGetNonphysicalTokenCount( ))
	end if
	return expr->semantic_expression
end function

sub fbSemanticModelPointerIndex _
	( byval result as ASTNODE ptr, byval operand_id as longint, byval index_id as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	dim as longint identity = result->semantic_expression
	if( (identity <= 0) or (operand_id <= 0) or (index_id <= 0) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:657")
		exit sub
	end if
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "pointer-index-operand" + TABCHAR + fbSemanticModelNumber(operand_id))
	fbSemanticModelAppendDetail(prefix + "pointer-index-index" + TABCHAR + fbSemanticModelNumber(index_id))
end sub

'' Index expressions are captured before integer conversion and byte-offset
'' construction. The parser keeps only their immutable IDs, never their ASTs.
'' Export the selected element as the group owner so SIZEOF/LEN provenance can
'' distinguish an unevaluated access from an emitted read or write.
function fbSemanticModelSelectedArrayIndex _
	( byval expr as ASTNODE ptr, byref source_start as LEX_LOCATION, byval nonphysical_start as longint ) as longint
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (expr = NULL) ) then return 0
	if( lex.ctx->semantic_probe ) then return 0
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(expr, source_start, source_end, nonphysical_start, lexGetNonphysicalTokenCount( ))
	if( expr->semantic_expression <= 0 ) then fbSemanticModelFailAt("semantic-expressions.bas:675")
	return expr->semantic_expression
end function

sub fbSemanticModelArraySubscripts _
	( byval result as ASTNODE ptr, byval array_symbol as FBSYMBOL ptr, _
	  indices() as longint, selected_indices() as longint, byval rank as integer, _
	  byref source_start as LEX_LOCATION, byval nonphysical_start as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) or (rank = 0) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( (array_symbol = NULL) or (rank < 1) or (rank > FB_MAXARRAYDIMS) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:686")
		exit sub
	end if
	if( (lbound(indices) <> 0) or (ubound(indices) < rank - 1) or _
		(lbound(selected_indices) <> 0) or (ubound(selected_indices) < rank - 1) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:691")
		exit sub
	end if
	for dimension as integer = 0 to rank - 1
		if( (indices(dimension) <= 0) or (selected_indices(dimension) <= 0) ) then
			fbSemanticModelFailAt("semantic-expressions.bas:696")
			exit sub
		end if
	next
	'' An earlier field or subscript can already own a prefix observation.
	'' Statement-side lvalues have no active cExpression frame. Explicit
	'' parser locations cover writes as well as expression-side reads.
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(result, source_start, source_end, nonphysical_start, lexGetNonphysicalTokenCount( ))
	dim as longint identity = result->semantic_expression
	dim as longint symbol_id = fbSemanticModelSymbolId(array_symbol)
	if( (identity <= 0) or (symbol_id <= 0) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:708")
		exit sub
	end if
	fbSemanticModelExportSymbolDetails(array_symbol)
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "array-subscript-symbol" + TABCHAR + fbSemanticModelNumber(symbol_id))
	fbSemanticModelAppendDetail(prefix + "array-subscript-rank" + TABCHAR + fbSemanticModelNumber(rank))
	for dimension as integer = 0 to rank - 1
		fbSemanticModelAppendDetail(prefix + "array-subscript-index:" + fbSemanticModelNumber(dimension) + TABCHAR + fbSemanticModelNumber(indices(dimension)))
		fbSemanticModelAppendDetail(prefix + "array-subscript-selected-index:" + fbSemanticModelNumber(dimension) + TABCHAR + fbSemanticModelNumber(selected_indices(dimension)))
	next
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbol_id) + TABCHAR + _
		"expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + "array-subscript" + TABCHAR + "0")
end sub

'' A fixed array query can fold to a plain constant. Keep its intrinsic kind
'' and resolved array independently of that value, including the dimension
'' before and after INTEGER conversion. Runtime queries use the same group.
sub fbSemanticModelArrayBound _
	( byval result as ASTNODE ptr, byval array_symbol as FBSYMBOL ptr, byval tk as integer, _
	  byval original_dimension as longint, byval selected_dimension as longint, _
	  byref source_start as LEX_LOCATION, byval nonphysical_start as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( (array_symbol = NULL) or (original_dimension < 0) or (selected_dimension <= 0) or _
		((tk <> FB_TK_LBOUND) and (tk <> FB_TK_UBOUND)) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:734")
		exit sub
	end if
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(result, source_start, source_end, nonphysical_start, lexGetNonphysicalTokenCount( ))
	dim as longint identity = result->semantic_expression
	dim as longint symbol_id = fbSemanticModelSymbolId(array_symbol)
	if( (identity <= 0) or (symbol_id <= 0) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:742")
		exit sub
	end if
	fbSemanticModelExportSymbolDetails(array_symbol)
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "array-bound-kind" + TABCHAR + iif(tk = FB_TK_LBOUND, "lower", "upper"))
	fbSemanticModelAppendDetail(prefix + "array-bound-symbol" + TABCHAR + fbSemanticModelNumber(symbol_id))
	fbSemanticModelAppendDetail(prefix + "array-bound-dimension" + TABCHAR + fbSemanticModelNumber(original_dimension))
	'' A zero dimension ID means the optional source argument was omitted; the
	'' compiler-selected dimension still has its own positive expression ID.
	fbSemanticModelAppendDetail(prefix + "array-bound-dimension-explicit" + TABCHAR + _
		iif(original_dimension > 0, "1", "0"))
	fbSemanticModelAppendDetail(prefix + "array-bound-selected-dimension" + TABCHAR + fbSemanticModelNumber(selected_dimension))
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(symbol_id) + TABCHAR + _
		"expression" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + "array-bound-query" + TABCHAR + "0")
end sub

sub fbSemanticModelNewStorage _
	( byval temporary as FBSYMBOL ptr, byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval elements as ASTNODE ptr, byval do_clear as integer, byval is_array as integer, _
	  byval placement as ASTNODE ptr )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (temporary = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	dim as longint identity = fbSemanticModelSymbolId(temporary)
	dim as longint subtype_id = fbSemanticModelSymbolId(subtype)
	fbSemanticModelExportSymbolDetails(temporary)
	if( subtype <> NULL ) then fbSemanticModelExportSymbolDetails(subtype)
	dim as string count_text = "unknown"
	if( elements <> NULL ) then
		if( elements->class = AST_NODECLASS_CONST ) then count_text = ltrim(str(culngint(elements->val.i)))
	end if
	dim as longint placement_id = 0
	if( placement <> NULL ) then
		if( placement->semantic_expression = 0 ) then fbSemanticModelExportCurrentExpressionPrefix(placement)
		placement_id = placement->semantic_expression
		if( placement_id = 0 ) then
			fbSemanticModelFailAt("semantic-expressions.bas:774")
			exit sub
		end if
	end if
	dim as string prefix = "K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "memory-new-kind" + TABCHAR + iif(is_array, "array", "scalar"))
	fbSemanticModelAppendDetail(prefix + "memory-new-dtype" + TABCHAR + fbSemanticModelNumber(dtype))
	fbSemanticModelAppendDetail(prefix + "memory-new-subtype" + TABCHAR + fbSemanticModelNumber(subtype_id))
	fbSemanticModelAppendDetail(prefix + "memory-new-elements" + TABCHAR + count_text)
	fbSemanticModelAppendDetail(prefix + "memory-new-clear" + TABCHAR + fbSemanticModelNumber(abs(do_clear <> FALSE)))
	fbSemanticModelAppendDetail(prefix + "memory-new-placement" + TABCHAR + fbSemanticModelNumber(abs(placement <> NULL)))
	fbSemanticModelAppendDetail(prefix + "memory-new-placement-operand" + TABCHAR + fbSemanticModelNumber(placement_id))
end sub

sub fbSemanticModelDeleteStorage(byval operand as ASTNODE ptr, byval is_array as integer)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (operand = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( operand->semantic_expression = 0 ) then fbSemanticModelExportCurrentExpressionPrefix(operand)
	if( operand->semantic_expression = 0 ) then
		fbSemanticModelFailAt("semantic-expressions.bas:793")
		exit sub
	end if
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + _
		fbSemanticModelNumber(operand->semantic_expression) + TABCHAR + "memory-release-kind" + TABCHAR + _
		iif(is_array, "array", "scalar"))
end sub

'' GET/PUT lower a typed object to a BYREF AS ANY runtime argument. Preserve
'' that object before lowering so consumers can review the actual disk layout.
'' String overloads transfer character contents; this receipt alone never
'' asserts that a descriptor or a pointed-to allocation was transferred.
function fbSemanticModelFileTransferInput( byval operand as ASTNODE ptr ) as longint
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (operand = NULL) ) then return 0
	if( lex.ctx->semantic_probe ) then return 0
	if( operand->semantic_expression = 0 ) then fbSemanticModelExportCurrentExpressionPrefix(operand)
	if( operand->semantic_expression = 0 ) then
		fbSemanticModelFailAt("semantic-expressions.bas:810")
		return 0
	end if
	return operand->semantic_expression
end function

sub fbSemanticModelFileTransfer _
	( byval call_expr as ASTNODE ptr, byval operand_id as longint, _
	  byref transfer_kind as const string, byval is_array as integer )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (call_expr = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	if( operand_id <= 0 ) then
		fbSemanticModelFailAt("semantic-expressions.bas:822")
		exit sub
	end if
	'' Statement intrinsics run after cExpression() has closed its active
	'' range. Anchor the selected call at the last accepted token; the separate
	'' operand receipt supplies the object's precise source range.
	dim as LEX_LOCATION anchor = lexGetLastLocation( )
	fbSemanticModelExportExpression(call_expr, anchor, anchor, 0, 0)
	if( call_expr->semantic_expression = 0 ) then
		fbSemanticModelFailAt("semantic-expressions.bas:831")
		exit sub
	end if
	'' This call identity later links to an executable N snapshot. A parsed
	'' GET/PUT inside SIZEOF has no such snapshot and cannot prove a transfer.
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(call_expr->semantic_expression) + TABCHAR
	fbSemanticModelAppendDetail(prefix + "file-transfer-kind" + TABCHAR + transfer_kind)
	fbSemanticModelAppendDetail(prefix + "file-transfer-storage" + TABCHAR + iif(is_array, "array", "scalar"))
	fbSemanticModelAppendDetail(prefix + "file-transfer-operand" + TABCHAR + fbSemanticModelNumber(operand_id))
end sub

'' -------------------------------------------------------------------------
'' Parsed string intrinsics before folding and argument lowering
'' -------------------------------------------------------------------------

function fbSemanticModelOriginalExpression _
	( byval operand as ASTNODE ptr, byref source_start as LEX_LOCATION, _
	  byval nonphysical_tokens_at_start as longint, byref source_end as LEX_LOCATION, _
	  byval nonphysical_tokens_at_end as longint ) as longint
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (operand = NULL) ) then return 0
	if( lex.ctx->semantic_probe ) then return 0
	if( operand->semantic_expression = 0 ) then _
		fbSemanticModelExportExpression(operand, source_start, source_end, _
			nonphysical_tokens_at_start, nonphysical_tokens_at_end)
	if( operand->semantic_expression <= 0 ) then
		dim as string failure_reason = "original expression identity unavailable at " + _
			source_start.source_file + ":" + fbSemanticModelNumber(source_start.start_line) + ":" + _
			fbSemanticModelNumber(source_start.start_column) + "-" + _
			fbSemanticModelNumber(source_end.end_line) + ":" + _
			fbSemanticModelNumber(source_end.end_column) + " macro=" + _
			fbSemanticModelNumber(source_start.macro_identity) + "/" + _
			fbSemanticModelNumber(source_end.macro_identity) + " physical=" + _
			fbSemanticModelNumber(abs(source_start.is_physical <> FALSE)) + "/" + _
			fbSemanticModelNumber(abs(source_end.is_physical <> FALSE)) + " ast=" + _
			fbSemanticModelNumber(operand->class) + " dtype=" + _
			fbSemanticModelNumber(astGetFullType(operand))
		fbSemanticModelFailAt(failure_reason)
	end if
	return operand->semantic_expression
end function

'' CHR can consume every argument while producing a literal. TRIM lowers ANY
'' to a runtime selection. Capture only immutable E identities before either
'' path consumes AST storage; this module never owns the parser's argument ASTs.
sub fbSemanticModelStringIntrinsic _
	( byval result as ASTNODE ptr, byref kind as const string, byval count as integer, _
	  arguments() as longint, byval is_any as integer, byref source_start as LEX_LOCATION, _
	  byval nonphysical_start as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (result = NULL) ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	'' The grammar permits at most 32 CHR arguments and two TRIM arguments.
	if( (count < 1) or (count > 32) or (lbound(arguments) <> 0) or (ubound(arguments) < count - 1) ) then
		fbSemanticModelFailAt("semantic-expressions.bas:865")
		exit sub
	end if
	for ordinal as integer = 0 to count - 1
		if( arguments(ordinal) <= 0 ) then
			fbSemanticModelFailAt("semantic-expressions.bas:870")
			exit sub
		end if
	next
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(result, source_start, source_end, nonphysical_start, lexGetNonphysicalTokenCount( ))
	if( result->semantic_expression = 0 ) then
		fbSemanticModelFailAt("semantic-expressions.bas:877")
		exit sub
	end if
	dim as string prefix = "K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(result->semantic_expression) + TABCHAR
	'' The module marker preserves occurrence membership independently of the
	'' property group, including intrinsics folded to ordinary string literals.
	dim as longint module_owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	if( module_owner = 0 ) then
		fbSemanticModelFailAt("semantic-expressions.bas:885")
		exit sub
	end if
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(module_owner) + _
		TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(result->semantic_expression) + TABCHAR + "parsed-string-intrinsic" + TABCHAR + "0")
	fbSemanticModelAppendDetail(prefix + "string-intrinsic-kind" + TABCHAR + kind)
	fbSemanticModelAppendDetail(prefix + "string-intrinsic-count" + TABCHAR + fbSemanticModelNumber(count))
	fbSemanticModelAppendDetail(prefix + "string-intrinsic-any" + TABCHAR + fbSemanticModelNumber(abs(is_any <> FALSE)))
	for ordinal as integer = 0 to count - 1
		fbSemanticModelAppendDetail(prefix + "string-intrinsic-argument-" + fbSemanticModelNumber(ordinal + 1) + _
			TABCHAR + fbSemanticModelNumber(arguments(ordinal)))
	next
end sub

'' end of tooling/semantic-expressions.bas
