'' Project: FreeBASIC compiler - source preprocessing
'' -----------------------------------------
''
'' File: preprocessor/pp-cond.bas
''
'' Purpose:
''
''     Evaluate preprocessing conditions and skip inactive source branches.
''
'' Responsibilities:
''
''     - evaluate directives and expand macros through the lexer
''     - track conditional compilation and preprocessing state
''
'' This file intentionally does NOT contain:
''
''     - BASIC procedure grammar or target code generation
''

'' pre-processor conditional (#if, #else, #elseif, #endif) parsing
''
'' chng: dec/2004 written [v1ctor]

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "lexer/lex.bi"
#include once "parser/parser.bi"
#include once "preprocessor/pp.bi"
#include once "tooling/semantic-preprocessor.bi"

const FB_PP_MAXRECLEVEL = 64

type LEXPP_REC
	istrue      as integer
	elsecnt     as integer
end type

declare sub ppSkip( )

'' globals
	dim shared pptb(1 to FB_PP_MAXRECLEVEL) as LEXPP_REC

sub ppCondInit( )
	pp.level = 0
end sub

sub ppCondEnd( )
end sub

private function ppExpression( byref valid as integer ) as integer
	dim as ASTNODE ptr expr = any
	valid = TRUE

	fbSetIsPP( TRUE )
	expr = cExpression( )
	fbSetIsPP( FALSE )

	if( expr = NULL ) then
		errReport( FB_ERRMSG_EXPECTEDEXPRESSION )
		valid = FALSE
		expr = astNewCONSTi( 0 )
	end if

	'' String literals evaluate to FALSE
	if( astGetStrLitSymbol( expr ) <> NULL ) then
		astDelTree( expr )
		expr = astNewCONSTi( 0 )

	'' Besides that, it must be a numeric constant (that includes the
	'' result of defined() or BOPs)
	elseif( astIsCONST( expr ) = FALSE ) then
		errReport( FB_ERRMSG_EXPECTEDCONST )
		valid = FALSE
		astDelTree( expr )
		expr = astNewCONSTi( 0 )
	end if

	function = not astConstEqZero( expr )
	'' Preprocessing consumes this tree; it does not transfer ownership to
	'' the program's statement list, including the error-recovery constant.
	astDelTree( expr )
end function

private function hDefinedQuery( ) as FBSYMBOL ptr
	dim as LEX_LOCATION source = lexGetCurrentLocation( )
	dim as string spelling = *lexGetText( )
	dim as FBSYMBOL ptr sym = cIdentifierOrUDTMember( )
	dim as LEX_LOCATION ending = lexGetLastLocation( )
	if( source.source_file = ending.source_file ) then
		source.end_line = ending.end_line
		source.end_column = ending.end_column
		source.raw_end_line = ending.raw_end_line
		source.raw_end_column = ending.raw_end_column
		source.raw_valid and= ending.raw_valid
		source.is_physical and= ending.is_physical
	end if
	fbSemanticModelPPDefined(sym, spelling, source)
	return sym
end function

sub ppCondIf( )
	dim as integer istrue = any
	dim as integer valid = TRUE
	dim as LEX_LOCATION source = lexGetCurrentLocation( )
	dim as longint branchid = fbSemanticModelPPBranch(lexGetToken( LEXCHECK_KWDNAMESPC ), source, TRUE)

	istrue = FALSE

	select case as const lexGetToken( LEXCHECK_KWDNAMESPC )
	'' IFDEF ID
	case FB_TK_PP_IFDEF
		lexSkipToken( LEXCHECK_NODEFINE or LEXCHECK_POST_SUFFIX )
		istrue = (hDefinedQuery( ) <> NULL)

	'' IFNDEF ID
	case FB_TK_PP_IFNDEF
		lexSkipToken( LEXCHECK_NODEFINE or LEXCHECK_POST_SUFFIX )
		istrue = (hDefinedQuery( ) = NULL)

	'' IF Expression
	case FB_TK_PP_IF
		lexSkipToken( LEXCHECK_POST_SUFFIX )

		istrue = ppExpression( valid )

	end select

	pp.level += 1
	if( pp.level > FB_PP_MAXRECLEVEL ) then
		errReport( FB_ERRMSG_RECLEVELTOODEEP )
		errHideFurtherErrors( )
		exit sub
	end if

	pptb(pp.level).istrue = istrue
	pptb(pp.level).elsecnt = 0
	dim as LEX_LOCATION ending = lexGetLastLocation( )
	fbSemanticModelPPDecision(branchid, iif(valid, "evaluated", "invalid"), istrue, istrue, ending)

	if( istrue = FALSE ) then
		ppSkip( )
	end if
end sub

sub ppCondElse( )
	dim as integer istrue = any
	dim as integer valid = TRUE

	istrue = FALSE

	if( pp.level = 0 ) then
		errReport( FB_ERRMSG_ILLEGALOUTSIDECOMP )
		'' error recovery: skip statement
		hSkipStmt( )
		exit sub
	end if

	if( pptb(pp.level).elsecnt > 0 ) then
		errReport( FB_ERRMSG_SYNTAXERROR )
		'' error recovery: skip statement
		hSkipStmt( )
		exit sub
	end if
	dim as integer token = lexGetToken( LEXCHECK_KWDNAMESPC )
	dim as LEX_LOCATION source = lexGetCurrentLocation( )
	dim as longint branchid = fbSemanticModelPPBranch(token, source, FALSE)

	select case as const lexGetToken( LEXCHECK_KWDNAMESPC )

	'' ELSEIF | ELSEIFDEF | ELSEIFNDEF?
	case FB_TK_PP_ELSEIF, FB_TK_PP_ELSEIFDEF, FB_TK_PP_ELSEIFNDEF

		select case as const lexGetToken( LEXCHECK_KWDNAMESPC )
		case FB_TK_PP_ELSEIF
			lexSkipToken( LEXCHECK_POST_SUFFIX )
			istrue = ppExpression( valid )
		case FB_TK_PP_ELSEIFDEF
			lexSkipToken( LEXCHECK_NODEFINE or LEXCHECK_POST_SUFFIX )
			istrue = (hDefinedQuery( ) <> NULL)
		case FB_TK_PP_ELSEIFNDEF
			lexSkipToken( LEXCHECK_NODEFINE or LEXCHECK_POST_SUFFIX )
			istrue = (hDefinedQuery( ) = NULL)
		end select

		if( pptb(pp.level).istrue ) then
			dim as LEX_LOCATION ending = lexGetLastLocation( )
			fbSemanticModelPPDecision(branchid, iif(valid, "evaluated", "invalid"), istrue, FALSE, ending)
			ppSkip( )
			exit sub
		end if

		pptb(pp.level).istrue = istrue

	'' ELSE
	case else
		lexSkipToken( LEXCHECK_POST_SUFFIX )

		pptb(pp.level).elsecnt += 1
		pptb(pp.level).istrue = not pptb(pp.level).istrue

	end select
	dim as LEX_LOCATION ending = lexGetLastLocation( )
	dim as string evaluation = iif(token = FB_TK_PP_ELSE, "unconditional", iif(valid, "evaluated", "invalid"))
	fbSemanticModelPPDecision(branchid, evaluation, istrue, pptb(pp.level).istrue, ending)

	if( pptb(pp.level).istrue = FALSE ) then
		ppSkip( )
	end if
end sub

sub ppCondEndIf( )
	dim as LEX_LOCATION source = lexGetCurrentLocation( )
	'' ENDIF
	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( pp.level > 0 ) then
		fbSemanticModelPPEnd(source)
		pp.level -= 1
	else
		errReport( FB_ERRMSG_ILLEGALOUTSIDECOMP )
	end if
end sub

'':::::
sub ppAssert( )
	dim as integer istrue = any
	dim as integer valid = any

	'' ASSERT Expression

	istrue = ppExpression( valid )

	if( istrue = FALSE ) then
		errReport( FB_ERRMSG_PPASSERT_FAILED )
	end if

end sub

'':::::
private sub ppSkip( )
	dim as integer iflevel = any
	dim as longint branchid = fbSemanticModelPPCurrentBranch( )

	pp.skipping = TRUE

	'' Comment?
	cComment( )

	'' emit the current line in text form
	hEmitCurrLine( )

	'' EOL
	if( lexGetToken( ) <> FB_TK_EOL ) then
		errReport( FB_ERRMSG_EXPECTEDEOL )
		'' error recovery: skip until next line
		hSkipUntil( FB_TK_EOL, TRUE )
	else
		lexSkipToken( )
	end if

	iflevel = pp.level
	dim as LEX_LOCATION inactive_start = lexGetLastLocation( )
	'' The consumed EOL retains the previous logical line; lexSkipToken()
	'' advances the scanner afterwards. Start at the next whole source line,
	'' preserving leading whitespace and continuations in the inactive region.
	if( inactive_start.is_physical ) then
		inactive_start.start_line += 1
	else
		inactive_start = lexGetCurrentLocation( )
	end if
	inactive_start.start_column = 0
	inactive_start.raw_start_line = lex.ctx->raw_linenum
	inactive_start.raw_start_column = 0

	'' skip lines until a #ENDIF or #ELSE at same level is found
	'' we only care about lines that would affect the level of
	'' PP processing, like multiline comments and #if* pp tokens.
	'' we don't care about invalid directives or line continuation
	'' characters.
	do
		select case lexGetToken( )
		case CHAR_SHARP
			dim as LEX_LOCATION boundary = lexGetCurrentLocation( )
			lexSkipToken( LEXCHECK_KWDNAMESPC )

			select case as const lexGetToken( LEXCHECK_KWDNAMESPC )
			case FB_TK_PP_IF, FB_TK_PP_IFDEF, FB_TK_PP_IFNDEF
				dim as LEX_LOCATION source = lexGetCurrentLocation( )
				dim as longint skipped_branch = fbSemanticModelPPBranch(lexGetToken( LEXCHECK_KWDNAMESPC ), source, TRUE)
				fbSemanticModelPPDecision(skipped_branch, "parent-inactive", FALSE, FALSE, source)
				iflevel += 1

			case FB_TK_PP_ELSE, FB_TK_PP_ELSEIF, _
			     FB_TK_PP_ELSEIFDEF, FB_TK_PP_ELSEIFNDEF
				select case( iflevel )
				case pp.level
					fbSemanticModelPPSkipped(branchid, inactive_start, boundary)
					pp.skipping = FALSE
					ppCondElse( )
					exit sub
				case 0
					errReport( FB_ERRMSG_ILLEGALOUTSIDECOMP )
				case else
					dim as LEX_LOCATION source = lexGetCurrentLocation( )
					dim as longint skipped_branch = fbSemanticModelPPBranch(lexGetToken( LEXCHECK_KWDNAMESPC ), source, FALSE)
					fbSemanticModelPPDecision(skipped_branch, "parent-inactive", FALSE, FALSE, source)
				end select

			case FB_TK_PP_ENDIF
				select case( iflevel )
				case pp.level
					fbSemanticModelPPSkipped(branchid, inactive_start, boundary)
					pp.skipping = FALSE
					ppCondEndIf( )
					exit sub
				case 0
					errReport( FB_ERRMSG_ILLEGALOUTSIDECOMP )
				case else
					dim as LEX_LOCATION source = lexGetCurrentLocation( )
					fbSemanticModelPPEnd(source)
					iflevel -= 1
				end select

			end select

		case FB_TK_EOF
			dim as LEX_LOCATION boundary = lexGetCurrentLocation( )
			fbSemanticModelPPSkipped(branchid, inactive_start, boundary)
			errReport( FB_ERRMSG_EXPECTEDPPENDIF )
			exit do

		end select

		lexSkipLine( )

		if( lexGetToken( ) = FB_TK_EOL ) then
			lexSkipToken( )
		end if
	loop

	pp.skipping = FALSE
end sub

'' end of preprocessor/pp-cond.bas
