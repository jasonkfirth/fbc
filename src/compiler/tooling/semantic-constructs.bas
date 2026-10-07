'' Project: FreeBASIC compiler - semantic source constructs
'' File: tooling/semantic-constructs.bas
'' Purpose: Preserve statements and compound extents before their ASTs are flushed.
'' Responsibilities: Own observation stacks, source extents, and fact associations.
'' This file intentionally does NOT contain: grammar dispatch or control-flow inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-macros.bi"
#include once "tooling/semantic-coordinates.bi"

declare sub fbSemanticModelExportExpression _
	( byval expr as ASTNODE ptr, byref source_start as LEX_LOCATION, byref source_end as LEX_LOCATION, _
	  byval nonphysical_tokens_at_start as longint, byval nonphysical_tokens_at_end as longint, _
	  byval semantic_operator_override as integer = -1 )
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

'' -------------------------------------------------------------------------
'' Observation stacks
'' -------------------------------------------------------------------------

type SEMANTIC_SOURCE_STATEMENT
	identity as longint
	parent as longint
	compound as longint
	source as LEX_LOCATION
	token as integer
	counter_observed as integer
	counter_source as LEX_LOCATION
	step_observed as integer
	step_source as LEX_LOCATION
end type

type SEMANTIC_SOURCE_COMPOUND
	identity as longint
	statement as longint
	source as LEX_LOCATION
	select_owner as longint
	select_clauses as longint
	select_alternatives as integer
	select_else as integer
end type

const SEMANTIC_CONSTRUCT_MAX_DEPTH = 65536
dim shared as SEMANTIC_SOURCE_STATEMENT ptr semantic_statements
dim shared as integer semantic_statement_depth, semantic_statement_capacity
dim shared as SEMANTIC_SOURCE_COMPOUND ptr semantic_compounds
dim shared as integer semantic_compound_depth, semantic_compound_capacity

sub fbSemanticModelResetConstructs( )
	deallocate(semantic_statements)
	deallocate(semantic_compounds)
	semantic_statements = NULL
	semantic_compounds = NULL
	semantic_statement_depth = 0
	semantic_statement_capacity = 0
	semantic_compound_depth = 0
	semantic_compound_capacity = 0
end sub

private function hGrow( byref storage as any ptr, byref capacity as integer, byval depth as integer, byval element_bytes as integer ) as integer
	if( depth < capacity ) then return TRUE
	if( depth >= SEMANTIC_CONSTRUCT_MAX_DEPTH ) then
		fbSemanticModelFailAt("semantic-constructs.bas:61")
		return FALSE
	end if
	dim as integer next_capacity = iif(capacity = 0, 64, capacity * 2)
	dim as any ptr next_storage = reallocate(storage, next_capacity * element_bytes)
	if( next_storage = NULL ) then
		fbSemanticModelFailAt("semantic-constructs.bas:67")
		return FALSE
	end if
	storage = next_storage
	capacity = next_capacity
	return TRUE
end function

function fbSemanticModelCurrentStatement( ) as longint
	if( semantic_statement_depth = 0 ) then return 0
	return semantic_statements[semantic_statement_depth - 1].identity
end function

private function hCurrentCompound( ) as longint
	if( semantic_compound_depth = 0 ) then return 0
	return semantic_compounds[semantic_compound_depth - 1].identity
end function

private function hLocation( byref source as LEX_LOCATION ) as string
	return fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.start_line) + TABCHAR + _
		fbSemanticModelNumber(source.start_column) + TABCHAR + fbSemanticModelNumber(source.end_line) + TABCHAR + _
		fbSemanticModelNumber(source.end_column)
end function

private function hCompoundKind( byval token as integer ) as string
	select case token
	case FB_TK_IF: return "if"
	case FB_TK_FOR: return "for"
	case FB_TK_DO: return "do"
	case FB_TK_WHILE: return "while"
	case FB_TK_SELECT: return "select"
	case FB_TK_WITH: return "with"
	case FB_TK_SCOPE: return "scope"
	case FB_TK_NAMESPACE: return "namespace"
	case FB_TK_EXTERN: return "extern"
	case FB_TK_FUNCTION: return "procedure"
	case FB_TK_TYPE: return "type"
	case FB_TK_UNION: return "union"
	case FB_TK_ENUM: return "enum"
	case FB_TK_ASM: return "assembly"
	case else: return "unknown"
	end select
end function

'' -------------------------------------------------------------------------
'' Parser boundaries and fact ownership
'' -------------------------------------------------------------------------

function fbSemanticModelStatementBegin( byref source as LEX_LOCATION, byval token as integer, byval token_class as integer ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	if( (token = FB_TK_EOL) or (token = FB_TK_EOF) or (token = FB_TK_COMMENT) or (token = FB_TK_REM) or _
	    (token = FB_TK_ELSE) and (semantic_statement_depth > 0) ) then return 0
	if( hGrow(semantic_statements, semantic_statement_capacity, semantic_statement_depth, sizeof(SEMANTIC_SOURCE_STATEMENT)) = FALSE ) then return 0
	dim as longint parent = fbSemanticModelCurrentStatement( ), compound = hCurrentCompound( )
	dim as longint owner = fbSemanticModelSymbolId(parser.currproc)
	dim as longint context = fbSemanticModelCurrentContext( )
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	with semantic_statements[semantic_statement_depth]
		.identity = identity
		.parent = parent
		.compound = compound
		.source = source
		.token = token
		.counter_observed = FALSE
		.step_observed = FALSE
	end with
	semantic_statement_depth += 1
	fbSemanticModelAppendProvenance("ST" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(parent) + _
		TABCHAR + fbSemanticModelNumber(compound) + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(context) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + fbSemanticModelNumber(parser.stmt.cnt) + _
		TABCHAR + fbSemanticModelNumber(token) + TABCHAR + fbSemanticModelNumber(token_class) + TABCHAR + hLocation(source))
	fbSemanticModelMacroOrigin("statement", identity, source.macro_identity, "start")
	return identity
end function

sub fbSemanticModelStatementEnd( byval identity as longint, byref route as const string, byref ending as LEX_LOCATION, byval errors_before as integer )
	if( identity = 0 ) then exit sub
	if( (semantic_statement_depth = 0) or (fbSemanticModelCurrentStatement( ) <> identity) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:147")
		exit sub
	end if
	semantic_statement_depth -= 1
	dim as LEX_LOCATION start_site = semantic_statements[semantic_statement_depth].source
	dim as string outcome = iif(errGetCount( ) = errors_before, "parsed", "recovered")
	if( route = "unmatched" ) then outcome = "unmatched"
	fbSemanticModelAppendProvenance("STE" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + route + TABCHAR + outcome + TABCHAR + hLocation(ending))
	fbSemanticModelMacroOrigin("statement", identity, ending.macro_identity, "end")
	'' A macro in an operand can make the full ending nonphysical while the
	'' opening token remains written source. Preserve that token independently;
	'' consumers must not fabricate a whole-statement range from this receipt.
	'' Serialize after STE so existing streaming readers already know the end.
	if( fbSemanticModelFullEnabled( ) ) then
		fbSemanticModelExportCoordinates("statement", identity, "opening-token", start_site, start_site)
		if( semantic_statements[semantic_statement_depth].counter_observed ) then
			dim as LEX_LOCATION counter_site = semantic_statements[semantic_statement_depth].counter_source
			fbSemanticModelExportCoordinates("statement", identity, "for-counter", counter_site, counter_site)
		end if
		if( semantic_statements[semantic_statement_depth].step_observed ) then
			dim as LEX_LOCATION step_site = semantic_statements[semantic_statement_depth].step_source
			fbSemanticModelExportCoordinates("statement", identity, "for-step", step_site, step_site)
		end if
	end if
	fbSemanticModelExportCoordinates("statement", identity, "range", start_site, ending)
end sub

function fbSemanticModelConstructBegin( byval token as integer ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	if( hGrow(semantic_compounds, semantic_compound_capacity, semantic_compound_depth, sizeof(SEMANTIC_SOURCE_COMPOUND)) = FALSE ) then return 0
	dim as longint parent = hCurrentCompound( ), statement = fbSemanticModelCurrentStatement( )
	dim as longint owner = fbSemanticModelSymbolId(parser.currproc), context = fbSemanticModelCurrentContext( )
	dim as LEX_LOCATION source = lexGetLastLocation( )
	if( semantic_statement_depth > 0 ) then source = semantic_statements[semantic_statement_depth - 1].source
	dim as string kind = hCompoundKind(token)
	if( (token = FB_TK_TYPE) and (semantic_statement_depth > 0) ) then
		if( semantic_statements[semantic_statement_depth - 1].token = FB_TK_UNION ) then kind = "union"
	end if
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	with semantic_compounds[semantic_compound_depth]
		.identity = identity
		.statement = statement
		.source = source
		.select_owner = 0
		.select_clauses = 0
		.select_alternatives = 0
		.select_else = FALSE
	end with
	semantic_compound_depth += 1
	fbSemanticModelAppendProvenance("BLK" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(parent) + _
		TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(owner) + TABCHAR + kind + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(context) + TABCHAR + hLocation(source))
	fbSemanticModelMacroOrigin("construct", identity, source.macro_identity, "start")
	return identity
end function

sub fbSemanticModelConstructEnd( byval identity as longint, byref ending as LEX_LOCATION )
	if( identity = 0 ) then exit sub
	if( (semantic_compound_depth = 0) or (hCurrentCompound( ) <> identity) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:202")
		exit sub
	end if
	if( fbSemanticModelFullEnabled( ) ) then
		with semantic_compounds[semantic_compound_depth - 1]
			if( .select_owner > 0 ) then
				if( .select_alternatives <> 0 ) then
					fbSemanticModelFailAt("unfinished select alternatives")
				else
					fbSemanticModelAppendProvenance("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(.select_owner) + _
						TABCHAR + "select-case-end:" + fbSemanticModelNumber(identity) + TABCHAR + _
						fbSemanticModelEscape(fbSemanticModelNumber(.select_clauses) + TABCHAR + fbSemanticModelNumber(abs(.select_else))))
				end if
			end if
		end with
	end if
	semantic_compound_depth -= 1
	fbSemanticModelAppendProvenance("BEND" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + hLocation(ending))
	fbSemanticModelMacroOrigin("construct", identity, ending.macro_identity, "end")
	fbSemanticModelExportCoordinates("construct", identity, "range", semantic_compounds[semantic_compound_depth].source, ending)
end sub

sub fbSemanticModelAssociateStatement( byref domain as const string, byval subject as longint, byval statement as longint )
	if( statement = 0 ) then statement = fbSemanticModelCurrentStatement( )
	if( (statement = 0) or (subject = 0) ) then exit sub
	fbSemanticModelAppendProvenance("OWN" + TABCHAR + domain + TABCHAR + fbSemanticModelNumber(subject) + TABCHAR + fbSemanticModelNumber(statement))
end sub

sub fbSemanticModelStatementOperation( byref operation as const string )
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (statement = 0) or (fbSemanticModelEnabled( ) = FALSE) ) then exit sub
	'' The caller is the grammar route that actually accepted this builtin,
	'' rather than a spelling lookup that could misclassify a user identifier.
	fbSemanticModelAppendProvenance("SOP" + TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + operation)
end sub

'' -------------------------------------------------------------------------
'' Accepted SELECT CASE grammar inputs
'' -------------------------------------------------------------------------

'' Keep parser alternatives before branch lowering consumes their ASTs. Clause
'' ordinals belong to the actual SELECT stack, including nested and expanded
'' statements. A separate end receipt makes missing clauses distinguishable
'' from a short complete selection. This state is owned by the parser thread.
private function hSelectObservation( byval construct as longint ) as SEMANTIC_SOURCE_COMPOUND ptr
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return NULL
	if( lex.ctx->semantic_probe ) then return NULL
	if( (construct = 0) or (semantic_compound_depth = 0) or (hCurrentCompound( ) <> construct) ) then
		fbSemanticModelFailAt("invalid select observation owner")
		return NULL
	end if
	return @semantic_compounds[semantic_compound_depth - 1]
end function

sub fbSemanticModelSelectInput( byval construct as longint, byval expression as longint, byval storage as FBSYMBOL ptr, byval is_const as integer )
	dim as SEMANTIC_SOURCE_COMPOUND ptr observed = hSelectObservation(construct)
	if( observed = NULL ) then exit sub
	if( (observed->select_owner <> 0) or (expression <= 0) or (storage = NULL) ) then
		fbSemanticModelFailAt("invalid select input")
		exit sub
	end if
	dim as longint owner = fbSemanticModelSymbolId(parser.currproc)
	if( owner = 0 ) then owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	dim as longint variable = fbSemanticModelSymbolId(storage)
	if( (owner = 0) or (variable = 0) ) then
		fbSemanticModelFailAt("unavailable select input identity")
		exit sub
	end if
	observed->select_owner = owner
	dim as string value = fbSemanticModelNumber(observed->statement) + TABCHAR + fbSemanticModelNumber(expression) + _
		TABCHAR + fbSemanticModelNumber(variable) + TABCHAR + iif(is_const, "constant", "normal")
	fbSemanticModelAppendProvenance("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + "select-case-input:" + fbSemanticModelNumber(construct) + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelSelectClause( byval construct as longint, byval is_else as integer, byval alternatives as integer )
	dim as SEMANTIC_SOURCE_COMPOUND ptr observed = hSelectObservation(construct)
	if( observed = NULL ) then exit sub
	if( (observed->select_owner = 0) or observed->select_else or (alternatives < 0) or _
		(alternatives <> observed->select_alternatives) or ((alternatives = 0) <> (is_else <> 0)) ) then
		fbSemanticModelFailAt("invalid select clause")
		exit sub
	end if
	observed->select_clauses += 1
	observed->select_else = (is_else <> 0)
	dim as string value = fbSemanticModelNumber(construct) + TABCHAR + fbSemanticModelNumber(observed->select_clauses) + _
		TABCHAR + fbSemanticModelNumber(abs(is_else <> 0)) + TABCHAR + fbSemanticModelNumber(alternatives)
	fbSemanticModelAppendProvenance("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(observed->select_owner) + _
		TABCHAR + "select-case-clause:" + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelEscape(value))
	observed->select_alternatives = 0
end sub

sub fbSemanticModelSelectAlternative( byval construct as longint, byval ordinal as integer, byref kind as const string, byval operation as integer, byval first_expression as longint, byval last_expression as longint, byval is_last as integer )
	dim as SEMANTIC_SOURCE_COMPOUND ptr observed = hSelectObservation(construct)
	if( observed = NULL ) then exit sub
	if( (observed->select_owner = 0) or observed->select_else or (ordinal <> observed->select_alternatives + 1) or _
		(first_expression <= 0) or ((kind <> "value") and (kind <> "range") and (kind <> "is")) or _
		((kind = "range") <> (last_expression > 0)) ) then
		fbSemanticModelFailAt("invalid select alternative")
		exit sub
	end if
	observed->select_alternatives = ordinal
	dim as string value = fbSemanticModelNumber(construct) + TABCHAR + kind + TABCHAR + fbSemanticModelNumber(operation) + _
		TABCHAR + fbSemanticModelNumber(first_expression) + TABCHAR + fbSemanticModelNumber(last_expression) + _
		TABCHAR + fbSemanticModelNumber(abs(is_last <> 0))
	fbSemanticModelAppendProvenance("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(observed->select_owner) + _
		TABCHAR + "select-case-alternative:" + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + ":" + _
		fbSemanticModelNumber(ordinal) + TABCHAR + fbSemanticModelEscape(value))
end sub

'' Branch lowering can consume a constant condition or replace its original
'' type with a comparison. Record the accepted input while it still exists.
'' Bare DO/LOOP statements retain zero explicitly; missing observations cannot
'' then be mistaken for intentionally unconditional grammar.
sub fbSemanticModelLoopCondition( byval expr as ASTNODE ptr, byref kind as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( lex.ctx->semantic_probe ) then exit sub
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (semantic_statement_depth = 0) or (statement = 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:235")
		exit sub
	end if
	dim as integer expected_token
	select case kind
	case "while": expected_token = FB_TK_WHILE
	case "do", "do-while", "do-until": expected_token = FB_TK_DO
	case "loop", "loop-while", "loop-until": expected_token = FB_TK_LOOP
	case else
		fbSemanticModelFailAt("semantic-constructs.bas:244")
		exit sub
	end select
	if( semantic_statements[semantic_statement_depth - 1].token <> expected_token ) then
		fbSemanticModelFailAt("semantic-constructs.bas:248")
		exit sub
	end if
	dim as longint expression_id = 0
	if( expr <> NULL ) then expression_id = expr->semantic_expression
	if( ((kind = "do") or (kind = "loop")) <> (expression_id = 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:254")
		exit sub
	end if
	dim as longint owner = fbSemanticModelSymbolId(parser.currproc)
	if( owner = 0 ) then
		fbSemanticModelFailAt("semantic-constructs.bas:259")
		exit sub
	end if
	dim as string prefix = "K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + TABCHAR
	dim as string statement_text = fbSemanticModelNumber(statement)
	fbSemanticModelAppendProvenance(prefix + "loop-condition-kind:" + statement_text + TABCHAR + kind)
	fbSemanticModelAppendProvenance(prefix + "loop-condition-expression:" + statement_text + TABCHAR + fbSemanticModelNumber(expression_id))
	if( expression_id > 0 ) then
		fbSemanticModelAppendProvenance("H" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
			TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR + _
			"loop-condition" + TABCHAR + statement_text)
	end if
end sub

sub fbSemanticModelForCounter _
	( byval counter as FBSYMBOL ptr, byref source as LEX_LOCATION, byval declared_here as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( counter = NULL ) then exit sub
	if( semantic_statement_depth = 0 ) then
		fbSemanticModelFailAt("semantic-constructs.bas:278")
		exit sub
	end if
	if( semantic_statements[semantic_statement_depth - 1].token <> FB_TK_FOR ) then
		fbSemanticModelFailAt("semantic-constructs.bas:282")
		exit sub
	end if
	dim as longint statement = fbSemanticModelCurrentStatement( )
	dim as longint variable = fbSemanticModelSymbolId(counter)
	'' A variable can control several loops. Keep each accepted statement in
	'' its own optional property instead of refreshing away earlier uses.
	'' Generic symbol K keys and statement LOC/MR roles remain old-client safe.
	fbSemanticModelAppendProvenance("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(variable) + _
		TABCHAR + "for-counter:" + fbSemanticModelNumber(statement) + TABCHAR + iif(declared_here, "local", "existing"))
	'' Statement locations follow STE, as required by existing streaming readers.
	semantic_statements[semantic_statement_depth - 1].counter_observed = TRUE
	semantic_statements[semantic_statement_depth - 1].counter_source = source
	fbSemanticModelMacroOrigin("statement", statement, source.macro_identity, "for-counter")
end sub

'' Keep the selected STEP token separate from its value expression. A macro
'' or a continuation can place the clause and its value at different sites.
sub fbSemanticModelForStepSource( byref source as LEX_LOCATION )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (semantic_statement_depth = 0) or (statement = 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:304")
		exit sub
	end if
	if( semantic_statements[semantic_statement_depth - 1].token <> FB_TK_FOR ) then
		fbSemanticModelFailAt("semantic-constructs.bas:308")
		exit sub
	end if
	semantic_statements[semantic_statement_depth - 1].step_observed = TRUE
	semantic_statements[semantic_statement_depth - 1].step_source = source
	fbSemanticModelMacroOrigin("statement", statement, source.macro_identity, "for-step")
end sub

'' Start and TO expressions are converted to the counter type and can then be
'' destroyed. Retain the original typed input instead of reconstructing it from
'' generated counter assignments or a temporary holding the converted limit.
sub fbSemanticModelScalarForBound _
	( byval counter as FBSYMBOL ptr, byval expr as ASTNODE ptr, byref role as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (counter = NULL) or (expr = NULL) ) then exit sub
	select case symbGetType(counter)
	case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, _
	     FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
	     FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE, FB_DATATYPE_ENUM
	case else
		exit sub
	end select
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (semantic_statement_depth = 0) or (statement = 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:332")
		exit sub
	end if
	if( (semantic_statements[semantic_statement_depth - 1].token <> FB_TK_FOR) or _
	    ((role <> "start") and (role <> "limit")) or (expr->semantic_expression <= 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:337")
		exit sub
	end if
	fbSemanticModelAppendProvenance("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(counter)) + TABCHAR + _
		"for-" + role + "-expression:" + fbSemanticModelNumber(statement) + TABCHAR + _
		fbSemanticModelNumber(expr->semantic_expression))
end sub

'' Numerical FOR steps are adapted to the counter's width before lowering.
'' An unsigned counter still accepts a signed negative step. Consumers need
'' its original expression, rather than guessing a direction from counter type.
'' UDT iteration operators and pointer counters have separate grammar contracts.
sub fbSemanticModelScalarForStep _
	( byval counter as FBSYMBOL ptr, byval expr as ASTNODE ptr, byval explicit_step as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (counter = NULL) or (expr = NULL) ) then exit sub
	select case symbGetType(counter)
	case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, _
	     FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
	     FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE, FB_DATATYPE_ENUM
	case else
		exit sub
	end select
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (semantic_statement_depth = 0) or (statement = 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:363")
		exit sub
	end if
	if( semantic_statements[semantic_statement_depth - 1].token <> FB_TK_FOR ) then
		fbSemanticModelFailAt("semantic-constructs.bas:367")
		exit sub
	end if
	dim as longint expression_id = 0
	if( explicit_step ) then
		expression_id = expr->semantic_expression
		if( expression_id <= 0 ) then
			fbSemanticModelFailAt("semantic-constructs.bas:374")
			exit sub
		end if
	end if
	dim as string subject = "K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(counter)) + TABCHAR
	dim as string statement_text = fbSemanticModelNumber(statement)
	fbSemanticModelAppendProvenance(subject + "for-step-explicit:" + statement_text + TABCHAR + iif(explicit_step, "1", "0"))
	fbSemanticModelAppendProvenance(subject + "for-step-expression:" + statement_text + TABCHAR + fbSemanticModelNumber(expression_id))
end sub

'' The selected constant, when present, is what advances the counter. Floating
'' fractions can round to zero and wide integer steps can change direction when
'' converted. A logical E/C snapshot records that result without inventing a
'' source extent. Dynamic steps retain their selected type and a zero sentinel.
sub fbSemanticModelScalarForStepSelection _
	( byval counter as FBSYMBOL ptr, byval expr as ASTNODE ptr, byval dtype as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( counter = NULL ) then exit sub
	select case symbGetType(counter)
	case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_SHORT, FB_DATATYPE_USHORT, _
	     FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
	     FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT, FB_DATATYPE_SINGLE, FB_DATATYPE_DOUBLE, FB_DATATYPE_ENUM
	case else
		exit sub
	end select
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (statement = 0) or (semantic_statement_depth = 0) ) then
		fbSemanticModelFailAt("semantic-constructs.bas:401")
		exit sub
	end if
	if( semantic_statements[semantic_statement_depth - 1].token <> FB_TK_FOR ) then
		fbSemanticModelFailAt("semantic-constructs.bas:405")
		exit sub
	end if
	dim as longint expression_id = 0
	if( expr <> NULL ) then
		if( (expr->class <> AST_NODECLASS_CONST) or (astGetFullType(expr) <> dtype) ) then
			fbSemanticModelFailAt("semantic-constructs.bas:411")
			exit sub
		end if
		dim as LEX_LOCATION anchor = lexGetLastLocation( )
		anchor.is_physical = FALSE
		fbSemanticModelExportExpression(expr, anchor, anchor, 0, 0)
		expression_id = expr->semantic_expression
		if( expression_id <= 0 ) then
			fbSemanticModelFailAt("semantic-constructs.bas:419")
			exit sub
		end if
	end if
	dim as string prefix = "K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(counter)) + TABCHAR
	dim as string statement_text = fbSemanticModelNumber(statement)
	fbSemanticModelAppendProvenance(prefix + "for-step-selected-dtype:" + statement_text + TABCHAR + fbSemanticModelNumber(dtype))
	fbSemanticModelAppendProvenance(prefix + "for-step-selected-expression:" + statement_text + TABCHAR + fbSemanticModelNumber(expression_id))
end sub

'' end of tooling/semantic-constructs.bas
