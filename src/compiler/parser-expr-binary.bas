'' Project: FreeBASIC Compiler
'' -------------------------
''
'' File: parser-expr-binary.bas
''
'' Purpose:
''
''     Parse relational and binary expressions using the compiler's
''     precedence rules and produce their resolved AST forms.
''
'' Responsibilities:
''
''     - parse the binary operator precedence levels
''     - construct compiler AST nodes for relational and binary operations
''     - retain parser-known source ranges for semantic expression records
''
'' This file intentionally does NOT contain:
''
''     - semantic sidecar serialization or schema validation
''     - compiler command-line option handling
''     - source-level editor refactoring rules
''
'' binary operators (+, \, MOD, ...) parsing
''
'' chng: sep/2004 written [v1ctor]


#include once "fb.bi"
#include once "fbint.bi"
#include once "parser.bi"
#include once "ast.bi"
#include once "rtl.bi"

declare function fbSemanticModelEnabled( ) as integer
declare function fbSemanticModelExpressionsOnlyEnabled( ) as integer
declare sub fbSemanticModelSetExpressionOperatorOverride _
	( _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval operator_override as integer _
	)
declare sub fbSemanticModelExportExpression _
	( _
		byval expr as ASTNODE ptr, _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byval nonphysical_tokens_at_end as longint, _
		byval semantic_operator_override as integer = -1 _
	)

'' A later operator in the same loop can fold the current AST. Export the
'' completed prefix first so the source range keeps its compiler-selected type.
private sub hSemanticModelExportCurrentExpression _
	( _
		byval expr as ASTNODE ptr, _
		byref source_start as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byval semantic_operator_override as integer = -1 _
	)
	if( expr = NULL ) then exit sub
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(expr, source_start, source_end, _
		nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ), _
		semantic_operator_override)
end sub

'' Some built-in operators are normalized to a different BOP by astNewBOP().
'' Preserve the parser-selected operation only when the resulting AST still
'' has an operator node whose opcode no longer represents that source operator.
private function hSemanticModelLoweredBinaryOperator _
	( _
		byval expr as ASTNODE ptr, _
		byval source_operator as integer _
	) as integer
	if( (expr = NULL) or (expr->class <> AST_NODECLASS_BOP) ) then return -1
	if( expr->op.op = source_operator ) then return -1
	return source_operator
end function

declare function cLogOrExpression _
	( _
		_
	) as ASTNODE ptr

declare function cLogAndExpression _
	( _
		_
	) as ASTNODE ptr

declare function cIsExpression _
	( _
		byval semantic_operator as integer ptr _
	) as ASTNODE ptr

'':::::
''Expression      =   LogExpression .
''
function cExpression _
	( _
		_
	) as ASTNODE ptr

	dim as integer last_isexpr = fbGetIsExpression( )
	dim as ASTNODE ptr expr
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	dim as integer export_semantics = fbSemanticModelEnabled( )

	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	fbSetIsExpression( TRUE )

	'' LogExpression
	expr = cBoolExpression( )
	if( export_semantics andalso (expr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(expr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

	fbSetIsExpression( last_isexpr )
	function = expr

end function

function cExpressionWithNIDXARRAY( byval allow_nidxarray as integer ) as ASTNODE ptr
	dim as integer previous_check_array = any
	previous_check_array = fbGetCheckArray( )
	fbSetCheckArray( not allow_nidxarray )
	function = cExpression( )
	fbSetCheckArray( previous_check_array )
end function

'':::::
''BoolExpression      =   LogExpression ( (ANDALSO | ORELSE ) LogExpression )* .
''
function cBoolExpression( ) as ASTNODE ptr
	dim as integer op = any, dtorlistcookie = any, hideconsterrors = any
	dim as ASTNODE ptr expr = any, logexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' LogExpression
	'' The first operand expression will always be executed
	logexpr = cLogExpression( )
	if( logexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' Logical operator
		select case as const lexGetToken( )
		case FB_TK_ANDALSO
			op = AST_OP_ANDALSO
		case FB_TK_ORELSE
			op = AST_OP_ORELSE
		case else
			exit do
		end select

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(logexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' LogExpression
		'' The second operand expression however only conditionally
		hideconsterrors = FALSE
		if( astIsCONST( logexpr ) ) then
			if( op = AST_OP_ANDALSO ) then
				hideconsterrors = astConstEqZero( logexpr )
			else
				hideconsterrors = not astConstEqZero( logexpr )
			end if
		end if

		if( hideconsterrors ) then
			astBeginHideConstErrors( )
		end if

		astDtorListScopeBegin( )
		expr = cLogExpression( )
		dtorlistcookie = astDtorListScopeEnd( )

		if( hideconsterrors ) then
			astEndHideConstErrors( )
		end if

		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		logexpr = astNewBOP( op, logexpr, expr, cptr( any ptr, dtorlistcookie ) )
		if( logexpr = NULL ) then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			logexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = logexpr
	if( export_semantics andalso (logexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(logexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if
end function

'':::::
''LogExpression      =   LogOrExpression ( (XOR | EQV | IMP) LogOrExpression )* .
''
function cLogExpression _
	( _
		_
	) as ASTNODE ptr

	dim as integer op = any
	dim as ASTNODE ptr expr = any, logexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' LogOrExpression
	logexpr = cLogOrExpression( )
	if( logexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' Logical operator
		select case as const lexGetToken( )
		case FB_TK_XOR
			op = AST_OP_XOR
		case FB_TK_EQV
			op = AST_OP_EQV
		case FB_TK_IMP
			op = AST_OP_IMP
		case else
			exit do
		end select

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(logexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' LogOrExpression
		expr = cLogOrExpression( )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		logexpr = astNewBOP( op, logexpr, expr )

		if( logexpr = NULL ) then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			logexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1

	loop

	function = logexpr
	if( export_semantics andalso (logexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(logexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''LogOrExpression    =   LogAndExpression ( OR LogAndExpression )* .
''
function cLogOrExpression _
	( _
		_
	) as ASTNODE ptr

	dim as ASTNODE ptr expr = any, logexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' LogAndExpression
	logexpr = cLogAndExpression( )
	if( logexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' OR?
		if( lexGetToken( ) <> FB_TK_OR ) then
			exit do
		end if

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(logexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' LogAndExpression
		expr = cLogAndExpression(  )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		logexpr = astNewBOP( AST_OP_OR, logexpr, expr )

		if( logexpr = NULL ) then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			logexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1

	loop

	function = logexpr
	if( export_semantics andalso (logexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(logexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''LogAndExpression   =   RelExpression ( AND RelExpression )* .
''
function cLogAndExpression _
	( _
		_
	) as ASTNODE ptr

	dim as ASTNODE ptr expr = any, logexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' RelExpression
	logexpr = cRelExpression( )
	if( logexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' AND?
		if( lexGetToken( ) <> FB_TK_AND ) then
			exit do
		end if

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(logexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' RelExpression
		expr = cRelExpression( )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		logexpr = astNewBOP( AST_OP_AND, logexpr, expr )

		if( logexpr = NULL ) then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			logexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1

	loop

	function = logexpr
	if( export_semantics andalso (logexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(logexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''RelExpression   =   IsExpression ( (EQ | GT | LT | NE | LE | GE) IsExpression )* .
''
function cRelExpression _
	( _
		_
	) as ASTNODE ptr

	dim as integer op = any
	dim as ASTNODE ptr expr = any, relexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_model_enabled = fbSemanticModelEnabled( )
	dim as integer semantic_operator_count = 0
	dim as integer semantic_operator_override = -1
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( semantic_model_enabled ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' IsExpression
	relexpr = cIsExpression( @semantic_operator_override )
	if( relexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' Relational operator
		select case as const lexGetToken( )
		case FB_TK_EQ
			parser.have_eq_outside_parens = (parser.prntcnt = 0)

			'' outside parentheses? (cParentExpression() would have
			'' unset this flag otherwise)
			if( fbGetEqInParensOnly( ) ) then
				exit do
			end if
			op = AST_OP_EQ
		case FB_TK_GT
			if( fbGetGtInParensOnly( ) ) then
				exit do
			end if
			op = AST_OP_GT
		case FB_TK_LT
			op = AST_OP_LT
		case FB_TK_NE
			op = AST_OP_NE
		case FB_TK_LE
			op = AST_OP_LE
		case FB_TK_GE
			op = AST_OP_GE
			assert( fbGetGtInParensOnly( ) = FALSE )
		case else
			exit do
		end select
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(relexpr, source_start, _
				nonphysical_tokens_at_start, semantic_operator_override)
		end if

		lexSkipToken( )

		'' IsExpression
		semantic_operator_override = -1
		expr = cIsExpression( @semantic_operator_override )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		relexpr = astNewBOP( op, relexpr, expr )
		semantic_operator_override = -1

		if( relexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			relexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = relexpr
	if( export_semantics andalso (relexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(relexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ), _
			semantic_operator_override)
	elseif( semantic_model_enabled andalso (relexpr <> NULL) and _
		(semantic_operator_override >= 0) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelSetExpressionOperatorOverride(source_start, source_end, _
			semantic_operator_override)
	end if

end function

'':::::
''IsExpression   =   CatExpression IS SymbolType .
''
function cIsExpression _
	( _
		byval semantic_operator as integer ptr _
	) as ASTNODE ptr

	'' CatExpression
	dim as ASTNODE ptr isexpr = cCatExpression(  )
	if( isexpr = NULL ) then
		return NULL
	end if
	if( semantic_operator <> NULL ) then *semantic_operator = -1

	'' IS?
	if( lexGetToken( ) <> FB_TK_IS ) then
		return isexpr
	end if

	'' must be a struct with RTTI info
	if( astGetDataType( isexpr ) = FB_DATATYPE_STRUCT ) then
		if( symbGetHasRTTI( astGetSubtype( isexpr ) ) = FALSE ) then
			errReport( FB_ERRMSG_TYPEHASNORTTI )
			'' error recovery: fake a node
			isexpr = astNewCONSTi( 0 )
		end if
	else
		errReport( FB_ERRMSG_TYPEMUSTBEAUDT )
		'' error recovery: fake a node
		isexpr = astNewCONSTi( 0 )
	end if

	'' IS
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' SymbolType
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any
	if( cSymbolType( dtype, subtype ) = FALSE ) then
		return NULL
	end if

	'' must be a struct type with RTTI info
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_STRUCT ) then
		if( symbGetHasRTTI( subtype ) = FALSE ) then
			errReport( FB_ERRMSG_TYPEHASNORTTI )
			'' error recovery: fake a node
			return astNewCONSTi( 0 )

		elseif( symbGetUDTBaseLevel( subtype, astGetSubtype( isexpr ) ) = 0 ) then
			errReport( FB_ERRMSG_TYPESARENOTRELATED )
			'' error recovery: fake a node
			return astNewCONSTi( 0 )
		end if
	else
		errReport( FB_ERRMSG_TYPEMUSTBEAUDT )
		'' error recovery: fake a node
		return astNewCONSTi( 0 )
	end if

	'' point to the RTTI table
	var expr = astNewVAR( subtype->udt.ext->rtti )

	'' do operation
	isexpr = astNewBOP( AST_OP_IS, isexpr, expr )

	if( isexpr = NULL ) Then
		errReport( FB_ERRMSG_TYPEMISMATCH )
		'' error recovery: fake a node
		isexpr = astNewCONSTi( 0 )
	elseif( semantic_operator <> NULL ) then
		*semantic_operator = AST_OP_IS
	end if

	function = isexpr

end function

'':::::
''CatExpression   =   AddExpression ( & AddExpression )* .
''
function cCatExpression _
	( _
		_
	) as ASTNODE ptr

	dim as ASTNODE ptr expr = any, catexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_model_enabled = fbSemanticModelEnabled( )
	dim as integer semantic_operator_count = 0
	dim as integer semantic_operator_override = -1
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( semantic_model_enabled ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' AddExpression
	catexpr = cAddExpression(  )
	if( catexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' &
		if( lexGetToken( ) <> CHAR_AMP ) then
			exit do
		end if

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(catexpr, source_start, _
				nonphysical_tokens_at_start, semantic_operator_override)
		end if

		lexSkipToken( )

		'' AddExpression
		expr = cAddExpression(  )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' concatenate
		catexpr = astNewBOP( AST_OP_CONCAT, catexpr, expr )

		if( catexpr = NULL ) then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a new node
			catexpr = astNewCONSTstr( NULL )
		end if
		semantic_operator_override = hSemanticModelLoweredBinaryOperator(catexpr, AST_OP_CONCAT)
		if( export_semantics ) then semantic_operator_count += 1

	loop

	function = catexpr
	if( export_semantics andalso (catexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(catexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ), _
			semantic_operator_override)
	elseif( semantic_model_enabled andalso (catexpr <> NULL) and _
		(semantic_operator_override >= 0) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelSetExpressionOperatorOverride(source_start, source_end, _
			semantic_operator_override)
	end if

end function

'':::::
''AddExpression   =   ShiftExpression ( ('+' | '-') ShiftExpression )* .
''
function cAddExpression _
	( _
		_
	) as ASTNODE ptr

	dim as integer op = any
	dim as ASTNODE ptr expr = any, addexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_model_enabled = fbSemanticModelEnabled( )
	dim as integer semantic_operator_count = 0
	dim as integer semantic_operator_override = -1
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( semantic_model_enabled ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' ShiftExpression
	addexpr = cShiftExpression(  )
	if( addexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' Add operator
		select case lexGetToken( )
		case CHAR_PLUS
			op = AST_OP_ADD
		case CHAR_MINUS
			op = AST_OP_SUB
		case else
			exit do
		end select

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(addexpr, source_start, _
				nonphysical_tokens_at_start, semantic_operator_override)
		end if

		lexSkipToken( )

		'' ShiftExpression
		expr = cShiftExpression( )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		addexpr = astNewBOP( op, _
		                     addexpr, _
		                     expr, _
		                     NULL, _
		                     AST_OPOPT_DEFAULT or AST_OPOPT_DOPTRARITH )

		if( addexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			addexpr = astNewCONSTi( 0 )
		end if
		semantic_operator_override = hSemanticModelLoweredBinaryOperator(addexpr, op)
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = addexpr
	if( export_semantics andalso (addexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(addexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ), _
			semantic_operator_override)
	elseif( semantic_model_enabled andalso (addexpr <> NULL) and _
		(semantic_operator_override >= 0) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelSetExpressionOperatorOverride(source_start, source_end, _
			semantic_operator_override)
	end if

end function

'':::::
''ShiftExpression  =   ModExpression ( (SHL | SHR) ModExpression )* .
''
function cShiftExpression _
	( _
		_
	) as ASTNODE ptr

	dim as integer op = any
	dim as ASTNODE ptr expr = any, shiftexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' ModExpression
	shiftexpr = cModExpression(  )
	if( shiftexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' Shift operator
		select case lexGetToken( )
		case FB_TK_SHL
			op = AST_OP_SHL
		case FB_TK_SHR
			op = AST_OP_SHR
		case else
			exit do
		end select

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(shiftexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' ModExpression
		expr = cModExpression(  )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		shiftexpr = astNewBOP( op, shiftexpr, expr )

		if( shiftexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			shiftexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = shiftexpr
	if( export_semantics andalso (shiftexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(shiftexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''ModExpression   =   IntDivExpression ( MOD IntDivExpression )* .
''
function cModExpression _
	( _
		_
	) as ASTNODE ptr

	dim as ASTNODE ptr expr = any, modexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' IntDivExpression
	modexpr = cIntDivExpression( )
	if( modexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' MOD
		if( lexGetToken( ) <> FB_TK_MOD ) then
			exit do
		end if

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(modexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' IntDivExpression
		expr = cIntDivExpression( )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		modexpr = astNewBOP( AST_OP_MOD, modexpr, expr )

		if( modexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			modexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = modexpr
	if( export_semantics andalso (modexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(modexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''IntDivExpression=   MultExpression ( '\' MultExpression )* .
''
function cIntDivExpression _
	( _
		_
	) as ASTNODE ptr

	dim as ASTNODE ptr expr = any, idivexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' MultExpression
	idivexpr = cMultExpression( )
	if( idivexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' '\'
		if( lexGetToken( ) <> CHAR_RSLASH ) then
			exit do
		end if

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(idivexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( )

		'' MultExpression
		expr = cMultExpression( )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		idivexpr = astNewBOP( AST_OP_INTDIV, idivexpr, expr )

		if( idivexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			idivexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = idivexpr
	if( export_semantics andalso (idivexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(idivexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''MultExpression  =   ExpExpression ( ('*' | '/') ExpExpression )* .
''
function cMultExpression _
	( _
		_
	) as ASTNODE ptr

	dim as integer op = any
	dim as ASTNODE ptr expr = any, mulexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_operator_count = 0
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' ExpExpression
	mulexpr = cExpExpression( )
	if( mulexpr = NULL ) then
		return NULL
	end if

	'' ( ... )*
	do
		'' Mult operator
		select case lexGetToken( )
		case CHAR_TIMES
			op = AST_OP_MUL
		case CHAR_SLASH
			op = AST_OP_DIV
		case else
			exit do
		end select

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(mulexpr, source_start, _
				nonphysical_tokens_at_start)
		end if

		lexSkipToken( )

		'' ExpExpression
		expr = cExpExpression(  )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		mulexpr = astNewBOP( op, mulexpr, expr )

		if( mulexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			mulexpr = astNewCONSTi( 0 )
		end if
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = mulexpr
	if( export_semantics andalso (mulexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(mulexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if

end function

'':::::
''ExpExpression   =   NegNotExpression ( '^' NegNotExpression )* .
''
function cExpExpression _
	( _
		_
	) as ASTNODE ptr

	dim as ASTNODE ptr expr = any, expexpr = any
	dim as integer export_semantics = fbSemanticModelExpressionsOnlyEnabled( )
	dim as integer semantic_model_enabled = fbSemanticModelEnabled( )
	dim as integer semantic_operator_count = 0
	dim as integer semantic_operator_override = -1
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start
	if( semantic_model_enabled ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' NegNotExpression
	expexpr = cNegNotExpression( )
	if( expexpr = NULL ) then
		return NULL
	end if

	'' ( '^' NegNotExpression )*
	do
		if( lexGetToken( ) <> CHAR_CART ) then
			exit do
		end if

		'' Self-assignment? Then don't parse a BOP
		if( hIsAssignToken( lexGetLookAhead( 1 ) ) ) then
			exit do
		end if
		if( export_semantics and (semantic_operator_count > 0) ) then
			hSemanticModelExportCurrentExpression(expexpr, source_start, _
				nonphysical_tokens_at_start, semantic_operator_override)
		end if

		lexSkipToken( )

		'' NegNotExpression
		expr = cNegNotExpression(  )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			exit do
		end if

		'' do operation
		expexpr = astNewBOP( AST_OP_POW, expexpr, expr )

		if( expexpr = NULL ) Then
			errReport( FB_ERRMSG_TYPEMISMATCH )
			'' error recovery: fake a node
			expexpr = astNewCONSTf( 0, FB_DATATYPE_DOUBLE )
		end if
		semantic_operator_override = hSemanticModelLoweredBinaryOperator(expexpr, AST_OP_POW)
		if( export_semantics ) then semantic_operator_count += 1
	loop

	function = expexpr
	if( export_semantics andalso (expexpr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(expexpr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ), _
			semantic_operator_override)
	elseif( semantic_model_enabled andalso (expexpr <> NULL) and _
		(semantic_operator_override >= 0) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelSetExpressionOperatorOverride(source_start, source_end, _
			semantic_operator_override)
	end if

end function

'' end of parser-expr-binary.bas
