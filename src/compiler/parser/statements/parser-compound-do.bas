'' Project: FreeBASIC compiler - BASIC statement grammar
'' -----------------------------------------
''
'' File: parser/statements/parser-compound-do.bas
''
'' Purpose:
''
''     Parse dO..LOOP compound statement parsing.
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

'' DO..LOOP compound statement parsing
''
'' chng: sep/2004 written [v1ctor]

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "ast/ast.bi"
#include once "tooling/semantic-hooks.bi"

'' DoStmtBegin  =  DO ((WHILE | UNTIL) Expression)? .
sub cDoStmtBegin( )
	dim as ASTNODE ptr expr = any
	dim as integer iswhile = any, isuntil = any
	dim as FBSYMBOL ptr il = any, el = any, cl = any
	dim as FB_CMPSTMTSTK ptr stk = any
	dim as AST_SEMANTIC_SOURCE_RANGE semantic_source_range

	'' DO
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' add ini and end labels (will be used by any EXIT DO)
	il = symbAddLabel( NULL )
	el = symbAddLabel( NULL, FB_SYMBOPT_NONE )

	'' emit ini label
	astAdd( astNewLABEL( il ) )

	'' ((WHILE | UNTIL) Expression)?
	iswhile = FALSE
	isuntil = fALSE

	select case lexGetToken( )
	case FB_TK_WHILE
		iswhile = TRUE
	case FB_TK_UNTIL
		isuntil = TRUE
	end select

	if( iswhile or isuntil ) then
		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' Expression
		expr = cExpression( @semantic_source_range )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			'' error recovery: fake a node
			expr = astNewCONSTi( 0 )
		end if

		'' branch
		fbSemanticModelLoopCondition( expr, iif(iswhile, "do-while", "do-until") )
		expr = astBuildBranch( expr, el, (not iswhile), FALSE, @semantic_source_range )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_INVALIDDATATYPES )
			'' error recovery: fake a node
			expr = astNewNOP( )
		end if

		astAdd( expr )
		cl = il

	else
		expr = NULL
		fbSemanticModelLoopCondition( NULL, "do" )
		cl = symbAddLabel( NULL, FB_SYMBOPT_NONE )
	end if

	'' push to stmt stack
	stk = cCompStmtPush( FB_TK_DO )
	stk->scopenode = astScopeBegin( )
	stk->do.attop = (expr <> NULL)
	stk->do.inilabel = il
	stk->do.cmplabel = cl
	stk->do.endlabel = el
end sub

'' DoStmtEnd  =  LOOP ((WHILE | UNTIL) Expression)? .
sub cDoStmtEnd( )
	dim as ASTNODE ptr expr = any
	dim as integer iswhile = any, isuntil = any
	dim as FB_CMPSTMTSTK ptr stk = any
	dim as AST_SEMANTIC_SOURCE_RANGE semantic_source_range

	stk = cCompStmtGetTOS( FB_TK_DO )
	if( stk = NULL ) then
		hSkipStmt( )
		exit sub
	end if

	'' LOOP
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' ((WHILE | UNTIL | SttSeparator) Expression)?
	iswhile = FALSE
	isuntil = fALSE

	select case lexGetToken( )
	case FB_TK_WHILE
		iswhile = TRUE
	case FB_TK_UNTIL
		isuntil = TRUE
	end select

	if( (iswhile or isuntil) and (stk->do.attop) ) then
		errReport( FB_ERRMSG_SYNTAXERROR )
	end if

	'' end scope
	if( stk->scopenode <> NULL ) then
		astScopeEnd( stk->scopenode )
	end if

	'' emit comp label, if needed
	if( stk->do.cmplabel <> stk->do.inilabel ) then
		astAdd( astNewLABEL( stk->do.cmplabel ) )
	end if

	'' bottom check?
	if( iswhile or isuntil ) then
		lexSkipToken( LEXCHECK_POST_SUFFIX )

		'' Expression
		expr = cExpression( @semantic_source_range )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
			'' error recovery: fake a node
			expr = astNewCONSTi( 0 )
		end if

		'' branch
		fbSemanticModelLoopCondition( expr, iif(iswhile, "loop-while", "loop-until") )
		expr = astBuildBranch( expr, stk->do.inilabel, iswhile, FALSE, @semantic_source_range )
		if( expr = NULL ) then
			errReport( FB_ERRMSG_INVALIDDATATYPES )
			'' error recovery: fake a node
			expr = astNewNOP( )
		end if

		astAdd( expr )
	else
		'' top check
		fbSemanticModelLoopCondition( NULL, "loop" )
		astAdd( astNewBRANCH( AST_OP_JMP, stk->do.inilabel ) )
	end if

	'' end label (loop exit)
	astAdd( astNewLABEL( stk->do.endlabel ) )

	'' pop from stmt stack
	cCompStmtPop( stk )
end sub

'' end of parser/statements/parser-compound-do.bas
