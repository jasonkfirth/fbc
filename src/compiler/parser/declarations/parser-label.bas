'' Project: FreeBASIC compiler - BASIC declaration grammar
'' -----------------------------------------
''
'' File: parser/declarations/parser-label.bas
''
'' Purpose:
''
''     Parse label declarations.
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

'' label declarations
''
'' chng: sep/2004 written [v1ctor]


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "tooling/semantic-constructs.bi"
#include once "ast/ast.bi"

declare sub fbSemanticModelExportBinding _
	( _
		byval sym as FBSYMBOL ptr, _
		byref source as LEX_LOCATION, _
		byval is_declaration as integer _
	)

'':::::
''Label           =   NUM_LIT
''                |   ID ':' .
''
function cLabel as integer
	dim as LEX_LOCATION statement_source = lexGetCurrentLocation( )
	dim as integer statement_errors = errGetCount( )
	dim as longint statement = 0
	dim as integer statement_token = lexGetToken( ), statement_class = lexGetClass( )
	dim as FBSYMBOL ptr l = NULL
	dim as FBSYMCHAIN ptr chain_ = any
	dim as LEX_LOCATION semantic_site = lexGetCurrentLocation( )

	function = FALSE

	'' NUM_LIT
	select case as const lexGetClass( )
	case FB_TKCLASS_NUMLITERAL

		'' between SELECT CASE and first CASE? ... that's not allowed
		if( cCompStmtIsAllowed( FB_CMPSTMT_MASK_CODE ) = FALSE ) then
			hSkipStmt( )
			exit function
		end if

		if( fbLangOptIsSet( FB_LANG_OPT_NUMLABEL ) = FALSE ) then
			errReportNotAllowed( FB_LANG_OPT_NUMLABEL )
			'' error recovery: skip stmt
			hSkipStmt( )
		else
			l = symbAddLabel( lexGetText( ), _
							  FB_SYMBOPT_DECLARING or FB_SYMBOPT_CREATEALIAS )
			if( l = NULL ) then
				errReport( FB_ERRMSG_DUPDEFINITION )
				'' error recovery: skip stmt
				hSkipStmt( )
			else
				statement = fbSemanticModelStatementBegin(statement_source, statement_token, statement_class)
				lexSkipToken( )
			end if

			'' fake a ':'
			parser.stmt.cnt += 1
		end if

	'' ID (labels can't be quirk-keywords)
	case FB_TKCLASS_IDENTIFIER
		'' ':'
		if( lexGetLookAhead( 1 ) = FB_TK_STMTSEP ) then

			'' between SELECT CASE and first CASE? ... that's not allowed
			if( cCompStmtIsAllowed( FB_CMPSTMT_MASK_CODE ) = FALSE ) then
				hSkipStmt( )
				exit function
			end if

			'' ambiguity: it could be a proc call followed by a ':' stmt separator..
			'' no need to call Identifier(), ':' wouldn't follow 'ns.symbol' ids
			chain_ = lexGetSymChain( )
			if( symbFindByClass( chain_, FB_SYMBCLASS_PROC ) <> NULL ) then
				exit function
			end if

			l = symbAddLabel( lexGetText( ), _
							  FB_SYMBOPT_DECLARING or FB_SYMBOPT_CREATEALIAS )
			if( l = NULL ) then
				errReport( FB_ERRMSG_DUPDEFINITION )
			end if
			if( l <> NULL ) then statement = fbSemanticModelStatementBegin(statement_source, statement_token, statement_class)

			lexSkipToken( LEXCHECK_POST_SUFFIX )

			'' skip ':'
			lexSkipToken( )
		end if
	end select

	if( l <> NULL ) then
		fbSemanticModelExportBinding(l, semantic_site, TRUE)
		astAdd( astNewLABEL( l ) )

		symbSetLastLabel( l )

		function = TRUE
		dim as LEX_LOCATION ending = lexGetLastLocation( )
		fbSemanticModelStatementEnd(statement, "label", ending, statement_errors)
	end if

end function

'' end of parser/declarations/parser-label.bas
