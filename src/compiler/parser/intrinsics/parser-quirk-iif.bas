'' Project: FreeBASIC compiler - BASIC intrinsic syntax
'' -----------------------------------------
''
'' File: parser/intrinsics/parser-quirk-iif.bas
''
'' Purpose:
''
''     Parse quirk conditional statement (IIF) parsing.
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

'' quirk conditional statement (IIF) parsing
''
'' chng: sep/2004 written [v1ctor]


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "ast/ast.bi"

declare function fbSemanticModelEnabled( ) as integer
declare function fbSemanticModelExpressionsOnlyEnabled( ) as integer
declare function fbSemanticModelFullEnabled( ) as integer
declare sub fbSemanticModelIifInputs _
	( byval result as ASTNODE ptr, byval condition_id as longint, byval true_id as longint, _
	  byval false_id as longint, byref source_start as LEX_LOCATION, byref source_end as LEX_LOCATION, _
	  byval nonphysical_start as longint, byval nonphysical_end as longint )

'' cIIFFunct  =  IIF '(' condition-expr ',' true-expr ',' false-expr ')' .
function cIIFFunct() as ASTNODE ptr
	dim as ASTNODE ptr expr = any, truexpr = any, falsexpr = any
	dim as integer truecookie = any, falsecookie = any
	dim as integer export_semantics = fbSemanticModelEnabled( ) and _
		(fbSemanticModelExpressionsOnlyEnabled( ) = FALSE)
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start, nonphysical_tokens_at_end
	dim as AST_SEMANTIC_SOURCE_RANGE semantic_range = any
	dim as AST_SEMANTIC_SOURCE_RANGE ptr semantic_range_ptr = NULL
	dim as longint semantic_condition = 0, semantic_true = 0, semantic_false = 0

	function = NULL

	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	'' The condition expression is always executed,
	'' the true/false expressions only conditionally though.

	'' IIF
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' '('
	hMatchLPRNT( )

	'' condition-expr
	hMatchExpressionEx( expr, FB_DATATYPE_INTEGER )

	'' ','
	hMatchCOMMA( )

	'' true-expr
	astDtorListScopeBegin( )
	hMatchExpressionEx( truexpr, FB_DATATYPE_INTEGER )
	truecookie = astDtorListScopeEnd( )

	'' ','
	hMatchCOMMA( )

	'' false-expr
	astDtorListScopeBegin( )
	hMatchExpressionEx( falsexpr, astGetDataType( truexpr ) )
	falsecookie = astDtorListScopeEnd( )

	'' ')'
	hMatchRPRNT( )
	if( export_semantics ) then
		source_end = lexGetLastLocation( )
		nonphysical_tokens_at_end = lexGetNonphysicalTokenCount( )
		semantic_range.source_file = source_start.source_file
		semantic_range.start_line = source_start.start_line
		semantic_range.start_column = source_start.start_column
		semantic_range.end_line = source_end.end_line
		semantic_range.end_column = source_end.end_column
		semantic_range.start_is_physical = source_start.is_physical
		semantic_range.end_is_physical = source_end.is_physical
		semantic_range.nonphysical_at_start = nonphysical_tokens_at_start
		semantic_range.nonphysical_at_end = nonphysical_tokens_at_end
		semantic_range_ptr = @semantic_range
	end if

	'' Folding can delete an arm or reuse its AST as the result. Keep immutable
	'' original expression identities before either path consumes those nodes.
	if( fbSemanticModelFullEnabled( ) andalso (lex.ctx->semantic_probe = FALSE) ) then
		if( expr <> NULL ) then semantic_condition = expr->semantic_expression
		if( truexpr <> NULL ) then semantic_true = truexpr->semantic_expression
		if( falsexpr <> NULL ) then semantic_false = falsexpr->semantic_expression
	end if
	expr = astNewIIF( expr, truexpr, truecookie, falsexpr, falsecookie, semantic_range_ptr )
	if( expr = NULL ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, TRUE )
		'' error recovery: fake an expr
		expr = astNewCONSTi( 0 )
	else
		fbSemanticModelIifInputs(expr, semantic_condition, semantic_true, semantic_false, _
			source_start, source_end, nonphysical_tokens_at_start, nonphysical_tokens_at_end)
	end if

	function = expr
end function

'' end of parser/intrinsics/parser-quirk-iif.bas
