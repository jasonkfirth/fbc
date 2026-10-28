'' Project: FreeBASIC compiler - BASIC statement grammar
'' -----------------------------------------
''
'' File: parser/statements/parser-compound-extern.bas
''
'' Purpose:
''
''     Parse eXTERN..END EXTERN compound statement parsing.
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

'' EXTERN..END EXTERN compound statement parsing
''
'' chng: may/2006 written [v1ctor]

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "tooling/semantic-extern.bi"
#include once "tooling/semantic-hooks.bi"

'' ExternStmtBegin  =  EXTERN "mangling_spec" (LIB LITSTR)? .
sub cExternStmtBegin( )
	dim as FB_CMPSTMTSTK ptr stk = any
	dim as integer mangling = any
	dim as const zstring ptr litstr = any
	dim as integer semantic_observe = fbSemanticModelExternEnabled( )
	dim as integer semantic_present, semantic_recovered
	dim as longint semantic_context
	dim as string semantic_spelling
	dim as LEX_LOCATION semantic_source

	if( fbLangOptIsSet( FB_LANG_OPT_EXTERN ) = FALSE ) then
		errReportNotAllowed( FB_LANG_OPT_EXTERN )
		'' error recovery: skip the whole compound stmt
		hSkipCompound( FB_TK_EXTERN )
		exit sub
	end if

	if( cCompStmtIsAllowed( FB_CMPSTMT_MASK_EXTERN ) = FALSE ) then
		'' error recovery: skip the whole compound stmt
		hSkipCompound( FB_TK_EXTERN )
		exit sub
	end if

	'' EXTERN
	if( semantic_observe ) then
		semantic_source = lex.ctx->head->source
		semantic_context = fbSemanticModelCurrentContext( )
	end if
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' "mangling spec"
	if( lexGetClass( ) <> FB_TKCLASS_STRLITERAL ) then
		errReport( FB_ERRMSG_SYNTAXERROR )
		'' error recovery: assume it's "C"
		litstr = @"c"
		semantic_recovered = TRUE
	else
		litstr = lexGetText( )
		semantic_present = TRUE
		'' lexSkipToken() may recycle the literal's storage. Copy the native
		'' spelling now, before selecting or consuming its mangling mode.
		if( semantic_observe ) then semantic_spelling = *litstr
	end if

	select case lcase( *litstr )
	case "c"
		mangling = FB_MANGLING_CDECL
		lexSkipToken( )

	case "windows"
		mangling = FB_MANGLING_STDCALL
		lexSkipToken( )

	case "windows-ms"
		mangling = FB_MANGLING_STDCALL_MS
		lexSkipToken( )

	case "c++"
		mangling = FB_MANGLING_CPP
		lexSkipToken( )

	case "rtlib"
		mangling = FB_MANGLING_RTLIB
		lexSkipToken( )

	case else
		errReport( FB_ERRMSG_SYNTAXERROR )
		'' error recovery: assume it's "C"
		mangling = FB_MANGLING_CDECL
		semantic_recovered = TRUE
		lexSkipToken( )

	end select

	'' [LIB "string"]
	cLibAttribute()

	stk = cCompStmtPush( FB_TK_EXTERN, _
	                     FB_CMPSTMT_MASK_ALL and (not FB_CMPSTMT_MASK_CODE) _
	                     and (not FB_CMPSTMT_MASK_DATA) )

	stk->ext.lastmang = parser.mangling
	parser.mangling = mangling
	fbSemanticModelExternTransition("begin", stk->semantic_identity, stk->ext.lastmang, _
		semantic_context, semantic_source, semantic_spelling, semantic_present, semantic_recovered)
end sub

'' ExternStmtEnd  =  END EXTERN .
sub cExternStmtEnd( )
	dim as FB_CMPSTMTSTK ptr stk = any

	stk = cCompStmtGetTOS( FB_TK_EXTERN )
	if( stk = NULL ) then
		hSkipStmt( )
		exit sub
	end if

	'' END EXTERN
	dim as longint semantic_context
	dim as LEX_LOCATION semantic_source
	if( fbSemanticModelExternEnabled( ) ) then
		semantic_source = lex.ctx->head->source
		semantic_context = fbSemanticModelCurrentContext( )
	end if
	lexSkipToken( LEXCHECK_POST_SUFFIX )
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	'' pop from stmt stack
	dim as integer semantic_previous = parser.mangling
	parser.mangling = stk->ext.lastmang
	fbSemanticModelExternTransition("end", stk->semantic_identity, semantic_previous, semantic_context, semantic_source)
	cCompStmtPop( stk )
end sub

'' end of parser/statements/parser-compound-extern.bas
