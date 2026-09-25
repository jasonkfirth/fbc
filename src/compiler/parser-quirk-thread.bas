'' quick threadcall implementation
''
'' chng: oct/2011 written [jofers]

#include once "fb.bi"
#include once "fbint.bi"
#include once "parser.bi"

#include once "ast.bi"
#include once "rtl.bi"

declare function fbSemanticModelEnabled( ) as integer
declare sub fbSemanticModelExportExpression _
	( _
		byval expr as ASTNODE ptr, _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byval nonphysical_tokens_at_end as longint, _
		byval semantic_operator_override as integer = -1 _
	)

'':::::
'' ThreadCallFunc =   THREADCALL proc_call
''
function cThreadCallFunc() as ASTNODE ptr
	dim as FBSYMBOL ptr sym
	dim as FBSYMCHAIN ptr chain_
	dim as integer check_paren
	dim as FB_CALL_ARG_LIST arg_list = ( 0, NULL, NULL )
	dim as ASTNODE ptr childcall
	dim as ASTNODE ptr threadcall_expr
	dim as integer export_semantics = fbSemanticModelEnabled( )
	dim as LEX_LOCATION source_start, source_end
	dim as longint nonphysical_tokens_at_start

	if( export_semantics ) then
		lexGetToken( )
		source_start = lexGetCurrentLocation( )
		nonphysical_tokens_at_start = lexGetNonphysicalTokenCount( )
	end if

	function = NULL

	'' THREADCALL
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' proc
	chain_ = cIdentifier( NULL, FB_IDOPT_DEFAULT or FB_IDOPT_ALLOWSTRUCT )
	if( chain_ = NULL ) then
		exit function
	end if

	'' get symbol
	sym = symbFindByClass( chain_, FB_SYMBCLASS_PROC )
	if sym = NULL then
		errReport( FB_ERRMSG_EXPECTEDSUB )
		exit function
	end if

	'' must be a sub
	if( symbGetType( sym ) <> FB_DATATYPE_VOID ) then
		errReport( FB_ERRMSG_EXPECTEDSUB )
		exit function
	end if

	'' ID
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' '('?
	if( hMatch( CHAR_LPRNT ) = FALSE ) then
		dim params as integer
		params = symbGetProcParams( sym )
		if( params > 0 ) then
			errReport( FB_ERRMSG_EXPECTEDLPRNT )
			exit function
		end if
	else
		check_paren = TRUE
	end if

	'' arg_list
	childcall = cProcArgList( NULL, sym, NULL, @arg_list, 0 )

	'' ')'?
	if( check_paren = TRUE ) then
		if( lexGetToken( ) <> CHAR_RPRNT ) then
			errReport( FB_ERRMSG_EXPECTEDRPRNT )
			exit function
		end if
		lexSkipToken( )
	end if

	'' transform the call into a threadcall
	threadcall_expr = rtlThreadCall( childcall )
	function = threadcall_expr
	if( export_semantics andalso (threadcall_expr <> NULL) ) then
		source_end = lexGetLastLocation( )
		fbSemanticModelExportExpression(threadcall_expr, source_start, source_end, _
			nonphysical_tokens_at_start, lexGetNonphysicalTokenCount( ))
	end if
end function

'' end of parser-quirk-thread.bas
