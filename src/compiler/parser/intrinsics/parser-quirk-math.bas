'' Project: FreeBASIC compiler - BASIC intrinsic syntax
'' -----------------------------------------
''
'' File: parser/intrinsics/parser-quirk-math.bas
''
'' Purpose:
''
''     Parse quirk math functions (ABS, SGN, FIX, LEN, ...) parsing.
''
'' Responsibilities:
''
''     - parse the intrinsic statement or function family described below
''     - validate arguments before building AST and runtime calls
''
'' This file intentionally does NOT contain:
''
''     - the runtime implementation of those intrinsics
''

'' quirk math functions (ABS, SGN, FIX, LEN, ...) parsing
''
'' chng: sep/2004 written [v1ctor]


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "runtime/rtl.bi"
#include once "ast/ast.bi"
#include once "tooling/semantic-hooks.bi"
#include once "tooling/semantic-expressions.bi"


declare sub fbSemanticModelExportExpression _
	( _
		byval expr as ASTNODE ptr, _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byval nonphysical_tokens_at_end as longint, _
		byval semantic_operator_override as integer, _
		byval force_nonphysical_range as integer = FALSE _
	)

private function hMathOp(byval op as AST_OP) as ASTNODE ptr
	dim as ASTNODE ptr expr = any
	dim as integer semantic_model_enabled = fbSemanticModelEnabled( )
	dim as integer has_closing_parenthesis = FALSE
	dim as integer is_overloaded_operator = FALSE
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start = 0
	dim as longint nonphysical_tokens_at_end = 0
	if( semantic_model_enabled ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' ABS|SGN|FIX|FRAC|INT|SIN|ASIN|COS|ACOS|TAN|ATN|SQR|LOG|EXP
	lexSkipToken( LEXCHECK_POST_SUFFIX )
	hMatchLPRNT( )
	hMatchExpressionEx( expr, FB_DATATYPE_INTEGER )
	has_closing_parenthesis = (lexGetToken( ) = CHAR_RPRNT)
	hMatchRPRNT( )

	dim as longint operands = fbSemanticModelCaptureOperands(expr, NULL, "intrinsic-unary", op, source_start)
	expr = astNewUOP( op, expr )
	fbSemanticModelAttachOperands(expr, operands)
	if( expr = NULL ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES )
		expr = astNewCONSTi( 0 )
	else
		if( semantic_model_enabled andalso has_closing_parenthesis ) then
			fbSemanticModelExportOperation(expr, op, source_start)
			'' Integer FIX/INT/FRAC results may be lowered to ordinary arithmetic
			'' nodes. Preserve the source operation while the parser still knows it.
			if( (expr->class = AST_NODECLASS_CALL) or _
				(expr->class = AST_NODECLASS_CALLCTOR) ) then
				if( (expr->sym <> NULL) andalso symbIsOperator(expr->sym) ) then
					is_overloaded_operator = TRUE
				end if
			end if
			dim as integer semantic_operator_override = op
			if( is_overloaded_operator ) then semantic_operator_override = -1
			source_end = lexGetLastLocation( )
			nonphysical_tokens_at_end = lexGetNonphysicalTokenCount( )
			fbSemanticModelExportExpression(expr, source_start, source_end, _
				nonphysical_tokens_at_start, nonphysical_tokens_at_end, _
				semantic_operator_override)
		end if
	end if

	function = expr
end function

private function hAtan2() as ASTNODE ptr
	dim as ASTNODE ptr expr = any, expr2 = any
	dim as integer semantic_model_enabled = fbSemanticModelEnabled( )
	dim as integer has_closing_parenthesis = FALSE
	dim as integer is_overloaded_operator = FALSE
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start = 0
	dim as longint nonphysical_tokens_at_end = 0
	if( semantic_model_enabled ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' ATAN2( Expression ',' Expression )
	lexSkipToken( LEXCHECK_POST_SUFFIX )
	hMatchLPRNT( )
	hMatchExpressionEx( expr, FB_DATATYPE_INTEGER )
	hMatchCOMMA( )
	hMatchExpressionEx( expr2, FB_DATATYPE_INTEGER )
	has_closing_parenthesis = (lexGetToken( ) = CHAR_RPRNT)
	hMatchRPRNT( )

	dim as longint operands = fbSemanticModelCaptureOperands(expr, expr2, "intrinsic-binary", AST_OP_ATAN2, source_start)
	expr = astNewBOP( AST_OP_ATAN2, expr, expr2 )
	fbSemanticModelAttachOperands(expr, operands)
	if( expr = NULL ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES )
		expr = astNewCONSTi( 0 )
	elseif( semantic_model_enabled andalso has_closing_parenthesis ) then
		fbSemanticModelExportOperation(expr, AST_OP_ATAN2, source_start)
		if( (expr->class = AST_NODECLASS_CALL) or _
			(expr->class = AST_NODECLASS_CALLCTOR) ) then
			if( (expr->sym <> NULL) andalso symbIsOperator(expr->sym) ) then
				is_overloaded_operator = TRUE
			end if
		end if
		dim as integer semantic_operator_override = AST_OP_ATAN2
		if( is_overloaded_operator ) then semantic_operator_override = -1
		source_end = lexGetLastLocation( )
		nonphysical_tokens_at_end = lexGetNonphysicalTokenCount( )
		fbSemanticModelExportExpression(expr, source_start, source_end, _
			nonphysical_tokens_at_start, nonphysical_tokens_at_end, _
			semantic_operator_override)
	end if

	function = expr
end function

private function hLen _
	( _
		byval expr as ASTNODE ptr, _
		byref lgt as longint _
	) as ASTNODE ptr

	dim as FBSYMBOL ptr litsym = any
	dim as ASTNODE ptr lenexpr = any

	select case( astGetDataType( expr ) )
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		return rtlStrLen( expr )

	case FB_DATATYPE_CHAR
		litsym = astGetStrLitSymbol( expr )
		if( litsym = NULL ) then
			return rtlStrLen( expr )
		end if

		'' String literal, evaluate at compile-time
		lgt = symbGetStrLength( litsym )

	case FB_DATATYPE_WCHAR
		litsym = astGetStrLitSymbol( expr )
		if( litsym = NULL ) then
			return rtlWstrLen( expr )
		end if

		'' String literal, evaluate at compile-time
		'' symbGetStrLength( litsym ) will return the number of codepoints
		'' that are used to store the escaped WSTRING literal, when what
		'' we really want is the number of codepoints unescaped.
		lgt = len( *hUnescapeW( symbGetVarLitTextW( litsym ) ) )

	case FB_DATATYPE_FIXSTR
		'' len( fixstr ) returns the N from STRING * N, i.e. it works like sizeof()
		'' length of the stored data should always be same as the size
		'' where the data is padded with spaces and there is no null terminator
		lgt = astSizeOf( expr )
		assert( lgt >= 0 )

	case FB_DATATYPE_STRUCT
		'' Check whether there is a matching len() UOP overload
		lenexpr = astNewUOP( AST_OP_LEN, expr )
		if( lenexpr <> NULL ) then
			return lenexpr
		end if

		lgt = astSizeOf( expr )

	case else
		'' For anything else, len() means sizeof()
		lgt = astSizeOf( expr )

	end select

	astDelTree( expr )
	return NULL
end function

private function hLenSizeof( byval tk as integer, byval isasm as integer ) as ASTNODE ptr
	dim as ASTNODE ptr expr = any
	dim as integer dtype = any
	dim as longint lgt = any
	dim as FBSYMBOL ptr subtype = any
	dim as integer semantic_enabled = fbSemanticModelEnabled( ) and (isasm = FALSE)
	dim as LEX_LOCATION semantic_start, semantic_end
	dim as longint semantic_nonphysical_start = 0, semantic_nonphysical_end = 0
	dim as integer semantic_dtype = 0, semantic_has_close = FALSE
	dim as FBSYMBOL ptr semantic_subtype = NULL
	dim as longint semantic_operand = 0
	dim as string semantic_input = "type"
	dim as string semantic_kind = iif(tk = FB_TK_LEN, "len", "sizeof")
	if( semantic_enabled ) then
		semantic_start = lexGetCurrentLocation( )
		semantic_nonphysical_start = lexGetNonphysicalTokenCount( )
	end if

	'' LEN | SIZEOF
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' '('
	hMatchLPRNT( )

	'' Type or an Expression
	expr = cTypeOrExpression( tk, dtype, subtype, lgt )

	'' Was it an expression?
	if( expr ) then
		'' Array without index makes this a SIZEOF()
		if( astIsNIDXARRAY( expr ) ) then
			tk = FB_TK_SIZEOF
		end if

	'' then must be a type
	elseif( tk = FB_TK_SIZEOF ) then
		dim is_fixlenstr as integer
		cUdtTypeMember( dtype, subtype, lgt, is_fixlenstr )

	elseif( tk = FB_TK_LEN ) then
		dim is_fixlenstr as integer
		cUdtTypeMember( dtype, subtype, lgt, is_fixlenstr )

		if( is_fixlenstr ) then
			'' assume that constant string has no embedded nulls
			select case typeGetDtAndPtrOnly( dtype )
			case FB_DATATYPE_CHAR, FB_DATATYPE_STRING, FB_DATATYPE_USTRING, FB_DATATYPE_FIXSTR
				lgt -= typeGetSize( FB_DATATYPE_CHAR )
				lgt /= typeGetSize( FB_DATATYPE_CHAR )
			case FB_DATATYPE_WCHAR
				lgt -= typeGetSize( env.target.wchar )
				lgt /= typeGetSize( env.target.wchar )
			end select
		end if
	end if

	if( semantic_enabled ) then
		if( expr <> NULL ) then
			semantic_dtype = astGetFullType(expr)
			semantic_subtype = astGetSubtype(expr)
			semantic_operand = expr->semantic_expression
			semantic_input = iif(astIsNIDXARRAY(expr), "array", "expression")
		else
			semantic_dtype = dtype
			semantic_subtype = subtype
		end if
		semantic_has_close = (lexGetToken( ) = CHAR_RPRNT)
	end if

	'' ')'
	if( lexGetToken( ) <> CHAR_RPRNT ) then
		errReport( FB_ERRMSG_EXPECTEDRPRNT )
		hSkipUntil( CHAR_RPRNT, TRUE )
	else
		if( isasm = FALSE ) then
			lexSkipToken( )
		end if
	end if

	if( expr ) then
		if( tk = FB_TK_LEN ) then
			'' len()
			'' If an expression is returned, then it's an
			'' fb_[W]StrLen() call, otherwise it's a sizeof() and
			'' the length is returned in lgt.
			expr = hLen( expr, lgt )
			if( expr = NULL ) then
				expr = astNewCONSTi( lgt )
			end if
		else
			'' sizeof()
			lgt = astSizeOf( expr )
			astDelTree( expr )
			expr = astNewCONSTi( lgt )
		end if
	else
		expr = astNewCONSTi( lgt )
	end if
	if( semantic_enabled andalso semantic_has_close andalso (expr <> NULL) ) then
		semantic_end = lexGetLastLocation( )
		semantic_nonphysical_end = lexGetNonphysicalTokenCount( )
		fbSemanticModelExportExpression(expr, semantic_start, semantic_end, _
			semantic_nonphysical_start, semantic_nonphysical_end, -1)
		fbSemanticModelSizeQuery(expr->semantic_expression, semantic_dtype, semantic_subtype, _
			semantic_operand, semantic_kind, semantic_input)
	end if

	function = expr
end function

'':::::
'' cMathFunct   =   ABS( Expression )
''              |   SGN( Expression )
''              |   FIX( Expression )
''              |   INT( Expression )
''              |   LEN( data type | Expression ) .
''
function cMathFunct _
	( _
		byval tk as FB_TOKEN, _
		byval isasm as integer _
	) as ASTNODE ptr

	function = FALSE

	select case as const tk
	'' ABS( Expression )
	case FB_TK_ABS
		function = hMathOp(AST_OP_ABS)

	'' SGN( Expression )
	case FB_TK_SGN
		function = hMathOp(AST_OP_SGN)

	'' FIX( Expression )
	case FB_TK_FIX
		function = hMathOp(AST_OP_FIX)

	'' FRAC( Expression )
	case FB_TK_FRAC
		function = hMathOp(AST_OP_FRAC)

	'' INT( Expression )
	case FB_TK_INT
		function = hMathOp(AST_OP_FLOOR)

	'' SIN/COS/...( Expression )
	case FB_TK_SIN
		function = hMathOp(AST_OP_SIN)

	case FB_TK_ASIN
		function = hMathOp(AST_OP_ASIN)

	case FB_TK_COS
		function = hMathOp(AST_OP_COS)

	case FB_TK_ACOS
		function = hMathOp(AST_OP_ACOS)

	case FB_TK_TAN
		function = hMathOp(AST_OP_TAN)

	case FB_TK_ATN
		function = hMathOp(AST_OP_ATAN)

	case FB_TK_SQR
		function = hMathOp(AST_OP_SQRT)

	case FB_TK_LOG
		function = hMathOp(AST_OP_LOG)

	case FB_TK_EXP
		function = hMathOp(AST_OP_EXP)

	'' ATAN2( Expression ',' Expression )
	case FB_TK_ATAN2
		function = hAtan2()

	'' LEN|SIZEOF( data type | Expression{idx-less arrays too} )
	case FB_TK_LEN, FB_TK_SIZEOF
		function = hLenSizeof( tk, isasm )

	end select

end function

'' end of parser/intrinsics/parser-quirk-math.bas
