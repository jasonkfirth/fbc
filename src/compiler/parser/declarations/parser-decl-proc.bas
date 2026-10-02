'' Project: FreeBASIC compiler - BASIC declaration grammar
'' -----------------------------------------
''
'' File: parser/declarations/parser-decl-proc.bas
''
'' Purpose:
''
''     Parse proc (SUB or FUNCTION) declarations.
''
'' Responsibilities:
''
''     - parse the declaration family described below
''     - resolve types and symbols and build typed initializers
''
'' This file intentionally does NOT contain:
''
''     - symbol table allocation or target instruction selection
''

'' proc (SUB or FUNCTION) declarations
''
'' chng: sep/2004 written [v1ctor]

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "ast/ast.bi"

'' ProcDecl  =  DECLARE SUB|FUNCTION|OPERATOR ProcHeader .
sub cProcDecl( )
	dim as integer tk = any

	if( cCompStmtIsAllowed( FB_CMPSTMT_MASK_DECL ) = FALSE ) then
		hSkipStmt( )
		exit sub
	end if

	'' DECLARE
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' SUB|FUNCTION|OPERATOR
	tk = lexGetToken( )
	select case( tk )
	case FB_TK_SUB, FB_TK_FUNCTION, FB_TK_OPERATOR
		dim as LEX_LOCATION declaration_start = lexGetCurrentLocation( )
		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' ProcHeader
		cProcHeader( 0, 0, FALSE, FB_PROCOPT_ISPROTO, tk, @declaration_start )

	case else
		errReport( FB_ERRMSG_SYNTAXERROR )
		'' error recovery: skip stmt
		hSkipStmt( )
	end select
end sub

'' end of parser/declarations/parser-decl-proc.bas
