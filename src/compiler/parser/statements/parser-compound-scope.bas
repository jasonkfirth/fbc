'' Project: FreeBASIC compiler - BASIC statement grammar
'' -----------------------------------------
''
'' File: parser/statements/parser-compound-scope.bas
''
'' Purpose:
''
''     Parse sCOPE..END SCOPE compound statement parsing.
''
'' Responsibilities:
''
''     - parse statement syntax and track compound statement nesting
''     - build control flow and preserve source-level diagnostics
''
'' This file intentionally does NOT contain:
''
''     - symbol storage allocation or machine instruction encoding
''

'' SCOPE..END SCOPE compound statement parsing
''
'' chng: sep/2004 written [v1ctor]

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "ast/ast.bi"

'' ScopeStmtBegin  =  SCOPE .
sub cScopeStmtBegin( )
	dim as ASTNODE ptr n = any
	dim as FB_CMPSTMTSTK ptr stk = any

	if( fbLangOptIsSet( FB_LANG_OPT_SCOPE ) = FALSE ) then
		errReportNotAllowed( FB_LANG_OPT_SCOPE )
		'' error recovery: skip the whole compound stmt
		hSkipCompound( FB_TK_SCOPE )
		exit sub
	end if

	'' SCOPE
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	n = astScopeBegin( )
	if( n = NULL ) then
		errReport( FB_ERRMSG_RECLEVELTOODEEP )
	end if

	''
	stk = cCompStmtPush( FB_TK_SCOPE )
	stk->scopenode = n

	'' deprecated quirk: implicit vars inside implicit scope blocks
	'' must be allocated in the function scope
	stk->scp.lastis_scope = fbGetIsScope( )
	fbSetIsScope( TRUE )
end sub

'' ScopeStmtEnd  =  END SCOPE .
sub cScopeStmtEnd( )
	dim as FB_CMPSTMTSTK ptr stk = any

	stk = cCompStmtGetTOS( FB_TK_SCOPE )
	if( stk = NULL ) then
		hSkipStmt( )
		exit sub
	end if

	'' END SCOPE
	lexSkipToken( LEXCHECK_POST_SUFFIX )
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	fbSetIsScope( stk->scp.lastis_scope )

	if( stk->scopenode <> NULL ) then
		astScopeEnd( stk->scopenode )
	end if

	'' pop from stmt stack
	cCompStmtPop( stk )
end sub

'' end of parser/statements/parser-compound-scope.bas
