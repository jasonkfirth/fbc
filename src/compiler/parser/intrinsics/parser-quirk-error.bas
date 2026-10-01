'' Project: FreeBASIC compiler - BASIC intrinsic syntax
'' -----------------------------------------
''
'' File: parser/intrinsics/parser-quirk-error.bas
''
'' Purpose:
''
''     Parse quirk error statements (ERROR, ERR) parsing.
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

'' quirk error statements (ERROR, ERR) parsing
''
'' chng: sep/2004 written [v1ctor]


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "runtime/rtl.bi"
#include once "ast/ast.bi"

'' ERROR Expression
function cErrorStmt() as integer
	function = FALSE

	'' ERROR
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' Expression
	dim as ASTNODE ptr expr
	hMatchExpressionEx(expr, FB_DATATYPE_INTEGER)

	rtlErrorThrow(expr, lexLineNum(), env.inf.name)

	function = TRUE
end function

'' ERR '=' Expression
function cErrSetStmt() as integer
	function = FALSE

	'' ERR
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' '='
	if( cAssignToken( ) = FALSE ) then
		errReport( FB_ERRMSG_EXPECTEDEQ )
	end if

	'' Expression
	dim as ASTNODE ptr expr
	hMatchExpressionEx(expr, FB_DATATYPE_INTEGER)

	rtlErrorSetnum(expr)

	function = TRUE
end function

'' ERR()
function cErrorFunct() as ASTNODE ptr
	'' ERR
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' ('(' ')')?
	if( hMatch( CHAR_LPRNT ) ) then
		'' ')'
		hMatchRPRNT( )
	end if

	function = rtlErrorGetNum( )
end function

'' end of parser/intrinsics/parser-quirk-error.bas
