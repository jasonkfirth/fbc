''
'' FreeBASIC Compiler
'' ------------------
''
'' File: parser-quirk-sfx.bas
''
'' Purpose:
''
''     Parse multi-word sfxlib statements and function forms.
''
'' Responsibilities:
''
''     - dispatch MUSIC/SFX/AUDIO/STREAM/MIDI/DEVICE/CAPTURE
''     - normalize optional parenthesized command syntax
''     - build rtl call trees for the corresponding sfxlib entry points
''
'' This file intentionally does NOT contain:
''
''     - sfxlib runtime implementation
''     - single-word intrinsic registration
''     - backend-specific audio behavior
''
'' chng: apr/2026 written [codex]

#include once "fb.bi"
#include once "fbint.bi"
#include once "parser.bi"
#include once "ast.bi"
#include once "rtl.bi"
#include once "error.bi"


'' ----------------------------------------------------------------------------
'' Small AST helpers
'' ----------------------------------------------------------------------------

private function hSfxCall0 _
	( _
		byval proc as FBSYMBOL ptr _
	) as ASTNODE ptr

	function = astNewCALL( proc )
end function

private function hSfxCall1 _
	( _
		byval proc as FBSYMBOL ptr, _
		byval arg1 as ASTNODE ptr _
	) as ASTNODE ptr

	dim as ASTNODE ptr call_ = astNewCALL( proc )

	if( astNewARG( call_, arg1 ) = NULL ) then
		return NULL
	end if

	function = call_
end function

private function hSfxCall2 _
	( _
		byval proc as FBSYMBOL ptr, _
		byval arg1 as ASTNODE ptr, _
		byval arg2 as ASTNODE ptr _
	) as ASTNODE ptr

	dim as ASTNODE ptr call_ = astNewCALL( proc )

	if( astNewARG( call_, arg1 ) = NULL ) then
		return NULL
	end if

	if( astNewARG( call_, arg2 ) = NULL ) then
		return NULL
	end if

	function = call_
end function

private function hSfxCall3 _
	( _
		byval proc as FBSYMBOL ptr, _
		byval arg1 as ASTNODE ptr, _
		byval arg2 as ASTNODE ptr, _
		byval arg3 as ASTNODE ptr _
	) as ASTNODE ptr

	dim as ASTNODE ptr call_ = astNewCALL( proc )

	if( astNewARG( call_, arg1 ) = NULL ) then
		return NULL
	end if

	if( astNewARG( call_, arg2 ) = NULL ) then
		return NULL
	end if

	if( astNewARG( call_, arg3 ) = NULL ) then
		return NULL
	end if

	function = call_
end function

private sub hSfxOptBeginArgs _
	( _
		byref had_parens as integer _
	)

	had_parens = hMatch( CHAR_LPRNT )
end sub

private sub hSfxOptEndArgs _
	( _
		byval had_parens as integer _
	)

	if( had_parens ) then
		hMatchRPRNT( )
	end if
end sub

private function hSfxTypeIsString _
	( _
		byval expr as ASTNODE ptr _
	) as integer

	if( expr = NULL ) then
		return FALSE
	end if

	function = symbIsString( astGetDataType( expr ) )
end function

private function hSfxVoidInExpr( ) as ASTNODE ptr
	errReport( FB_ERRMSG_SYNTAXERROR )
	function = NULL
end function

private function hSfxAtStmtEnd( ) as integer
	select case as const lexGetToken( )
	case FB_TK_STMTSEP, FB_TK_EOL, FB_TK_EOF, FB_TK_COMMENT, FB_TK_REM
		function = TRUE
	case else
		function = FALSE
	end select
end function

'' ----------------------------------------------------------------------------
'' MUSIC
'' ----------------------------------------------------------------------------

private function hParseMusic _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1
	dim as FBSYMBOL ptr proc

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "LOAD", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXMUSICLOAD ), expr1 )
	end if

	if( hMatchIdOrKw( "PLAY", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_INVALID )
		hSfxOptEndArgs( had_parens )

		if( hSfxTypeIsString( expr1 ) ) then
			proc = PROCLOOKUP( SFXMUSICPLAYFILE )
		else
			proc = PROCLOOKUP( SFXMUSICPLAY )
		end if

		return hSfxCall1( proc, expr1 )
	end if

	if( hMatchIdOrKw( "LOOP", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_INVALID )
		hSfxOptEndArgs( had_parens )

		if( hSfxTypeIsString( expr1 ) ) then
			proc = PROCLOOKUP( SFXMUSICLOOPFILE )
		else
			proc = PROCLOOKUP( SFXMUSICLOOP )
		end if

		return hSfxCall1( proc, expr1 )
	end if

	if( hMatchIdOrKw( "STOP", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then
			return hSfxVoidInExpr( )
		end if

		hSfxOptBeginArgs( had_parens )
		if( had_parens orelse (hSfxAtStmtEnd( ) = FALSE) ) then
			if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
				hSfxOptEndArgs( had_parens )
				return hSfxCall0( PROCLOOKUP( SFXMUSICSTOP ) )
			end if

			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXMUSICSTOPID ), expr1 )
		end if

		return hSfxCall0( PROCLOOKUP( SFXMUSICSTOP ) )
	end if

	if( hMatchIdOrKw( "PAUSE", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then
			return hSfxVoidInExpr( )
		end if

		hSfxOptBeginArgs( had_parens )
		if( had_parens orelse (hSfxAtStmtEnd( ) = FALSE) ) then
			if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
				hSfxOptEndArgs( had_parens )
				return hSfxCall0( PROCLOOKUP( SFXMUSICPAUSE ) )
			end if

			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXMUSICPAUSEID ), expr1 )
		end if

		return hSfxCall0( PROCLOOKUP( SFXMUSICPAUSE ) )
	end if

	if( hMatchIdOrKw( "RESUME", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then
			return hSfxVoidInExpr( )
		end if

		hSfxOptBeginArgs( had_parens )
		if( had_parens orelse (hSfxAtStmtEnd( ) = FALSE) ) then
			if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
				hSfxOptEndArgs( had_parens )
				return hSfxCall0( PROCLOOKUP( SFXMUSICRESUME ) )
			end if

			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXMUSICRESUMEID ), expr1 )
		end if

		return hSfxCall0( PROCLOOKUP( SFXMUSICRESUME ) )
	end if

	if( hMatchIdOrKw( "STATUS", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMUSICSTATUS ) )
	end if

	if( hMatchIdOrKw( "CURRENT", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMUSICCURRENT ) )
	end if

	if( hMatchIdOrKw( "POSITION", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMUSICPOSITION ) )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function


'' ----------------------------------------------------------------------------
'' SFX
'' ----------------------------------------------------------------------------

private function hParseSfx _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1, expr2

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "LOAD", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hMatchCOMMA( )
		hMatchExpressionEx( expr2, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall2( PROCLOOKUP( SFXSFXLOAD ), expr1, expr2 )
	end if

	if( hMatchIdOrKw( "PLAY", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		if( hMatch( CHAR_COMMA ) ) then
			hMatchExpressionEx( expr2, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall2( PROCLOOKUP( SFXSFXPLAYCHANNEL ), expr1, expr2 )
		end if
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSFXPLAY ), expr1 )
	end if

	if( hMatchIdOrKw( "LOOP", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		if( hMatch( CHAR_COMMA ) ) then
			hMatchExpressionEx( expr2, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall2( PROCLOOKUP( SFXSFXLOOPCHANNEL ), expr1, expr2 )
		end if
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSFXLOOP ), expr1 )
	end if

	if( hMatchIdOrKw( "STOP", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then
			return hSfxVoidInExpr( )
		end if

		hSfxOptBeginArgs( had_parens )
		if( had_parens = FALSE andalso hSfxAtStmtEnd( ) ) then
			return hSfxCall0( PROCLOOKUP( SFXSFXSTOPALL ) )
		end if

		if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
			hSfxOptEndArgs( had_parens )
			return hSfxCall0( PROCLOOKUP( SFXSFXSTOPALL ) )
		end if

		if( hMatchIdOrKw( "CHANNEL", LEXCHECK_POST_SUFFIX ) ) then
			hMatchCOMMA( )
			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXSFXSTOPCHANNEL ), expr1 )
		end if

		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSFXSTOP ), expr1 )
	end if

	if( hMatchIdOrKw( "PAUSE", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then
			return hSfxVoidInExpr( )
		end if

		hSfxOptBeginArgs( had_parens )
		if( had_parens = FALSE andalso hSfxAtStmtEnd( ) ) then
			return hSfxCall0( PROCLOOKUP( SFXSFXPAUSEALL ) )
		end if

		if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
			hSfxOptEndArgs( had_parens )
			return hSfxCall0( PROCLOOKUP( SFXSFXPAUSEALL ) )
		end if

		if( hMatchIdOrKw( "CHANNEL", LEXCHECK_POST_SUFFIX ) ) then
			hMatchCOMMA( )
			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXSFXPAUSECHANNEL ), expr1 )
		end if

		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSFXPAUSE ), expr1 )
	end if

	if( hMatchIdOrKw( "RESUME", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then
			return hSfxVoidInExpr( )
		end if

		hSfxOptBeginArgs( had_parens )
		if( had_parens = FALSE andalso hSfxAtStmtEnd( ) ) then
			return hSfxCall0( PROCLOOKUP( SFXSFXRESUMEALL ) )
		end if

		if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
			hSfxOptEndArgs( had_parens )
			return hSfxCall0( PROCLOOKUP( SFXSFXRESUMEALL ) )
		end if

		if( hMatchIdOrKw( "CHANNEL", LEXCHECK_POST_SUFFIX ) ) then
			hMatchCOMMA( )
			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXSFXRESUMECHANNEL ), expr1 )
		end if

		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSFXRESUME ), expr1 )
	end if

	if( hMatchIdOrKw( "STATUS", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		if( had_parens = FALSE andalso is_func = FALSE andalso hSfxAtStmtEnd( ) ) then
			return hSfxCall0( PROCLOOKUP( SFXSFXANYACTIVE ) )
		end if

		if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
			hSfxOptEndArgs( had_parens )
			return hSfxCall0( PROCLOOKUP( SFXSFXANYACTIVE ) )
		end if

		if( hMatchIdOrKw( "CHANNEL", LEXCHECK_POST_SUFFIX ) ) then
			hMatchCOMMA( )
			hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
			hSfxOptEndArgs( had_parens )
			return hSfxCall1( PROCLOOKUP( SFXSFXSTATUSCHANNEL ), expr1 )
		end if

		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSFXSTATUS ), expr1 )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function


'' ----------------------------------------------------------------------------
'' AUDIO
'' ----------------------------------------------------------------------------

private function hParseAudio _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "PLAY", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXAUDIOPLAY ), expr1 )
	end if

	if( hMatchIdOrKw( "LOOP", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXAUDIOLOOP ), expr1 )
	end if

	if( hMatchIdOrKw( "STOP", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXAUDIOSTOP ) )
	end if

	if( hMatchIdOrKw( "PAUSE", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXAUDIOPAUSE ) )
	end if

	if( hMatchIdOrKw( "RESUME", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXAUDIORESUME ) )
	end if

	if( hMatchIdOrKw( "STATUS", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXAUDIOSTATUS ) )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function


'' ----------------------------------------------------------------------------
'' STREAM
'' ----------------------------------------------------------------------------

private function hParseStream _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "OPEN", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSTREAMOPEN ), expr1 )
	end if

	if( hMatchIdOrKw( "PLAY", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXSTREAMPLAY ) )
	end if

	if( hMatchIdOrKw( "STOP", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXSTREAMSTOP ) )
	end if

	if( hMatchIdOrKw( "PAUSE", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXSTREAMPAUSE ) )
	end if

	if( hMatchIdOrKw( "RESUME", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXSTREAMRESUME ) )
	end if

	if( hMatchIdOrKw( "POSITION", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXSTREAMPOSITION ) )
	end if

	if( hMatchIdOrKw( "SEEK", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXSTREAMSEEK ), expr1 )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function


'' ----------------------------------------------------------------------------
'' MIDI
'' ----------------------------------------------------------------------------

private function hParseMidi _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1, expr2, expr3

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "OPEN", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXMIDIOPEN ), expr1 )
	end if

	if( hMatchIdOrKw( "CLOSE", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMIDICLOSE ) )
	end if

	if( hMatchIdOrKw( "PLAY", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXMIDIPLAY ), expr1 )
	end if

	if( hMatchIdOrKw( "STOP", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMIDISTOP ) )
	end if

	if( hMatchIdOrKw( "PAUSE", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMIDIPAUSE ) )
	end if

	if( hMatchIdOrKw( "RESUME", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXMIDIRESUME ) )
	end if

	if( hMatchIdOrKw( "SEND", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hMatchCOMMA( )
		hMatchExpressionEx( expr2, FB_DATATYPE_LONG )
		hMatchCOMMA( )
		hMatchExpressionEx( expr3, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall3( PROCLOOKUP( SFXMIDISEND ), expr1, expr2, expr3 )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function


'' ----------------------------------------------------------------------------
'' DEVICE
'' ----------------------------------------------------------------------------

private function hParseDevice _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "LIST", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXDEVICELIST ) )
	end if

	if( hMatchIdOrKw( "SELECT", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXDEVICESELECT ), expr1 )
	end if

	if( hMatchIdOrKw( "INFO", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )

		hSfxOptBeginArgs( had_parens )
		if( had_parens = FALSE andalso hSfxAtStmtEnd( ) ) then
			return hSfxCall0( PROCLOOKUP( SFXDEVICEINFOCURRENT ) )
		end if

		if( had_parens andalso lexGetToken( ) = CHAR_RPRNT ) then
			hSfxOptEndArgs( had_parens )
			return hSfxCall0( PROCLOOKUP( SFXDEVICEINFOCURRENT ) )
		end if

		hMatchExpressionEx( expr1, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXDEVICEINFO ), expr1 )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function


'' ----------------------------------------------------------------------------
'' CAPTURE
'' ----------------------------------------------------------------------------

private function hParseCapture _
	( _
		byval is_func as integer _
	) as ASTNODE ptr

	dim as integer had_parens
	dim as ASTNODE ptr expr1, expr2

	lexSkipToken( LEXCHECK_POST_SUFFIX )

	if( hMatchIdOrKw( "START", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXCAPTURESTART ) )
	end if

	if( hMatchIdOrKw( "STOP", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXCAPTURESTOP ) )
	end if

	if( hMatchIdOrKw( "PAUSE", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXCAPTUREPAUSE ) )
	end if

	if( hMatchIdOrKw( "RESUME", LEXCHECK_POST_SUFFIX ) ) then
		if( is_func ) then return hSfxVoidInExpr( )
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXCAPTURERESUME ) )
	end if

	if( hMatchIdOrKw( "STATUS", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXCAPTURESTATUS ) )
	end if

	if( hMatchIdOrKw( "SAVE", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, FB_DATATYPE_STRING )
		hSfxOptEndArgs( had_parens )
		return hSfxCall1( PROCLOOKUP( SFXCAPTURESAVE ), expr1 )
	end if

	if( hMatchIdOrKw( "AVAILABLE", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hSfxOptEndArgs( had_parens )
		return hSfxCall0( PROCLOOKUP( SFXCAPTUREAVAILABLE ) )
	end if

	if( hMatchIdOrKw( "READ", LEXCHECK_POST_SUFFIX ) ) then
		hSfxOptBeginArgs( had_parens )
		hMatchExpressionEx( expr1, typeAddrOf( FB_DATATYPE_SINGLE ) )
		hMatchCOMMA( )
		hMatchExpressionEx( expr2, FB_DATATYPE_LONG )
		hSfxOptEndArgs( had_parens )
		return hSfxCall2( PROCLOOKUP( SFXCAPTUREREAD ), expr1, expr2 )
	end if

	errReport( FB_ERRMSG_EXPECTEDIDENTIFIER )
	function = NULL
end function
'' ----------------------------------------------------------------------------
'' Public parser entry points
'' ----------------------------------------------------------------------------

function cSfxStmt _
	( _
		byval tk as FB_TOKEN _
	) as integer

	dim as ASTNODE ptr expr = NULL

	select case as const tk
	case FB_TK_MUSIC
		expr = hParseMusic( FALSE )

	case FB_TK_SFX
		expr = hParseSfx( FALSE )

	case FB_TK_AUDIO
		expr = hParseAudio( FALSE )

	case FB_TK_STREAM
		expr = hParseStream( FALSE )

	case FB_TK_MIDI
		expr = hParseMidi( FALSE )

	case FB_TK_DEVICE
		expr = hParseDevice( FALSE )

	case FB_TK_CAPTURE
		expr = hParseCapture( FALSE )

	case else
		return FALSE
	end select

	if( expr <> NULL ) then
		astAdd( expr )
	end if

	function = TRUE
end function

function cSfxFunct _
	( _
		byval tk as FB_TOKEN _
	) as ASTNODE ptr

	select case as const tk
	case FB_TK_MUSIC
		function = hParseMusic( TRUE )

	case FB_TK_SFX
		function = hParseSfx( TRUE )

	case FB_TK_AUDIO
		function = hParseAudio( TRUE )

	case FB_TK_STREAM
		function = hParseStream( TRUE )

	case FB_TK_MIDI
		function = hParseMidi( TRUE )

	case FB_TK_DEVICE
		function = hParseDevice( TRUE )

	case FB_TK_CAPTURE
		function = hParseCapture( TRUE )

	case else
		function = NULL
	end select
end function

'' end of parser-quirk-sfx.bas
