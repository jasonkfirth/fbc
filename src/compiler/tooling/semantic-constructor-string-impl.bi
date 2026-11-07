'' Project: FreeBASIC compiler - constructor lookup observations
'' File: tooling/semantic-constructor-string-impl.bi
'' Purpose: Retain compiler-selected string alternatives for float arguments.
'' Responsibilities: Unmodified argument tails, access checks and provenance.
'' This file intentionally does NOT allocate ASTs, literals or replacement calls.

dim shared as longint constructor_string_count
const SEMANTIC_CONSTRUCTOR_STRING_LIMIT = 1000000

private sub hResetConstructorString( )
	constructor_string_count = 0
end sub

sub fbSemanticModelConstructorStringCount( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"constructor-string-query-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(constructor_string_count))
end sub

sub fbSemanticModelConstructorString(byval head as FBSYMBOL ptr, byval selected as FBSYMBOL ptr, _
	byval argument_count as integer, byval arguments as FB_CALL_ARG ptr, byval options as FB_SYMBFINDOPT)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (head = NULL) or (selected = NULL) or (arguments = NULL) or (argument_count < 1) ) then exit sub
	if( (symbIsConstructor(head) = FALSE) or (symbIsConstructor(selected) = FALSE) ) then exit sub
	dim as ASTNODE ptr original = arguments->expr
	if( original = NULL ) then exit sub
	if( original->class <> AST_NODECLASS_CONST ) then exit sub
	if( (astGetDataType(original) <> FB_DATATYPE_SINGLE) and (astGetDataType(original) <> FB_DATATYPE_DOUBLE) ) then exit sub
	'' Source parsing normally captures this expression before lookup. Do not
	'' manufacture a new source occurrence from a speculative conversion probe.
	if( original->semantic_expression <= 0 ) then exit sub
	constructor_string_count += 1
	if( constructor_string_count > SEMANTIC_CONSTRUCTOR_STRING_LIMIT ) then
		fbSemanticModelFailAt("constructor string query limit exceeded"): exit sub
	end if

	'' The same CHAR literal shape is built by astNewCONSTstr/symbAllocStrConst.
	'' These stack descriptors are lookup inputs only. Registering an actual
	'' string literal here would mutate the symbol pool and possibly codegen.
	'' Overload selection uses its type, literal attributes and passing mode;
	'' the replacement spelling/value is deliberately not a compiler policy.
	dim as FBSYMBOL literal_symbol
	dim as ASTNODE literal_node
	literal_symbol.class = FB_SYMBCLASS_VAR
	literal_symbol.typ = FB_DATATYPE_CHAR
	literal_symbol.attrib = FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_CONST or FB_SYMBATTRIB_LITERAL
	literal_symbol.lgt = 2
	literal_symbol.var_.littext = @"0"
	literal_node.class = AST_NODECLASS_VAR
	literal_node.dtype = FB_DATATYPE_CHAR
	literal_node.sym = @literal_symbol
	dim as FB_CALL_ARG first_argument = *arguments
	first_argument.expr = @literal_node
	dim as integer saved_probe = lex.ctx->semantic_probe
	lex.ctx->semantic_probe = TRUE
	dim as FB_ERRMSG error_number = FB_ERRMSG_OK
	dim as FBSYMBOL ptr alternative = symbFindClosestOvlProc(head, argument_count, @first_argument, _
		@error_number, options or FB_SYMBFINDOPT_NO_ERROR)
	dim as string status = "selected"
	if( alternative = NULL ) then
		status = iif(error_number = FB_ERRMSG_AMBIGUOUSCALLTOPROC, "ambiguous", "no-match")
	elseif( symbCheckAccess(alternative) = FALSE ) then
		status = "inaccessible"
	end if
	lex.ctx->semantic_probe = saved_probe

	dim as longint expression_id = original->semantic_expression
	dim as longint selected_id = fbSemanticModelSymbolId(selected)
	dim as longint alternative_id = fbSemanticModelSymbolId(alternative)
	dim as longint type_id = fbSemanticModelSymbolId(symbGetParent(selected))
	dim as longint caller_id = fbSemanticModelSymbolId(parser.currproc)
	dim as string identity = fbSemanticModelNumber(constructor_string_count)
	dim as string payload = "1" + TABCHAR + fbSemanticModelNumber(selected_id) + TABCHAR + _
		fbSemanticModelNumber(type_id) + TABCHAR + fbSemanticModelNumber(argument_count) + TABCHAR + _
		fbSemanticModelNumber(arguments->mode) + TABCHAR + fbSemanticModelNumber(alternative_id) + TABCHAR + _
		status + TABCHAR + fbSemanticModelNumber(error_number) + TABCHAR + fbSemanticModelNumber(caller_id) + TABCHAR + _
		fbSemanticModelTypeNumber(astGetFullType(original))
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR + _
		"constructor-string-query:" + identity + TABCHAR + fbSemanticModelEscape(payload))
	fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR + _
		"symbol" + TABCHAR + fbSemanticModelNumber(selected_id) + TABCHAR + "constructor-string-original:" + identity + TABCHAR + "0")
	if( alternative_id ) then
		fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR + _
			"symbol" + TABCHAR + fbSemanticModelNumber(alternative_id) + TABCHAR + "constructor-string-alternative:" + identity + TABCHAR + "0")
	end if
end sub

'' end of tooling/semantic-constructor-string-impl.bi
