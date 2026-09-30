'' quirk conditional statement (IIF) parsing
''
'' chng: sep/2004 written [v1ctor]


#include once "fb.bi"
#include once "fbint.bi"
#include once "parser.bi"
#include once "ast.bi"

declare function fbSemanticModelEnabled( ) as integer
declare function fbSemanticModelExpressionsOnlyEnabled( ) as integer

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
		semantic_range.source_file = strptr( source_start.source_file )
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

	expr = astNewIIF( expr, truexpr, truecookie, falsexpr, falsecookie, semantic_range_ptr )
	if( expr = NULL ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, TRUE )
		'' error recovery: fake an expr
		expr = astNewCONSTi( 0 )
	end if

	function = expr
end function
