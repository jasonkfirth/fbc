''
'' FreeBASIC Compiler
'' ------------------
''
'' File: rtl-sfx.bas
''
'' Purpose:
''
''     Register the sfxlib runtime entry points used by the compiler.
''
'' Responsibilities:
''
''     - describe the public single-word sound procedures
''     - describe the hidden helper procedures used by multi-word syntax
''     - request automatic linkage of the fbsfx runtime library
''
'' This file intentionally does NOT contain:
''
''     - parser logic
''     - backend-specific sound code
''     - command execution behavior
''
'' chng: apr/2026 written [codex]

#include once "fb.bi"
#include once "fbint.bi"
#include once "ast.bi"
#include once "rtl.bi"

declare function hSfxlib_cb _
	( _
		byval sym as FBSYMBOL ptr _
	) as integer

'' ----------------------------------------------------------------------------
'' Public intrinsic procedures
'' ----------------------------------------------------------------------------

	dim shared as FB_RTL_PROCDEF funcdata( 0 to ... ) = _
	{ _
		/' sub sound overload( byval frequency as const long, byval duration as const single ) '/ _
		( _
			@"sound", @"fb_sfxSound", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub sound overload( byval channel as const long, byval frequency as const long, byval duration as const single, byval volume as const single ) '/ _
		( _
			@"sound", @"fb_sfxSoundChannel", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			4, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub noise overload( byval channel as const long, byval duration as const single, byval volume as const single ) '/ _
		( _
			@"noise", @"fb_sfxNoise", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			3, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub play overload( byref music as const string ) '/ _
		( _
			@"play", @"fb_sfxPlay", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub play overload( byval channel as const long, byref music as const string ) '/ _
		( _
			@"play", @"fb_sfxPlayChannel", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub tempo overload( byval bpm as const long ) '/ _
		( _
			@"tempo", @"fb_sfxTempo", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function tempo overload( ) as long '/ _
		( _
			@"tempo", @"fb_sfxTempoGet", _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			0 _
		), _
		/' sub channel overload( byval value as const long ) '/ _
		( _
			@"channel", @"fb_sfxChannelCmd", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function channel overload( ) as long '/ _
		( _
			@"channel", @"fb_sfxChannelCmdGet", _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			0 _
		), _
		/' sub octave overload( byval value as const long ) '/ _
		( _
			@"octave", @"fb_sfxOctave", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function octave overload( ) as long '/ _
		( _
			@"octave", @"fb_sfxOctaveGet", _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			0 _
		), _
		/' sub voice overload( byval instrument as const long ) '/ _
		( _
			@"voice", @"fb_sfxVoice", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function voice overload( ) as long '/ _
		( _
			@"voice", @"fb_sfxVoiceGet", _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			0 _
		), _
		/' sub volume overload( byval level as const single ) '/ _
		( _
			@"volume", @"fb_sfxVolume", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function volume overload( ) as single '/ _
		( _
			@"volume", @"fb_sfxVolumeGet", _
			FB_DATATYPE_SINGLE, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			0 _
		), _
		/' sub volume overload( byval channel as const long, byval level as const single ) '/ _
		( _
			@"volume", @"fb_sfxVolumeChannel", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function volume overload( byval channel as const long ) as single '/ _
		( _
			@"volume", @"fb_sfxVolumeChannelGet", _
			FB_DATATYPE_SINGLE, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub balance overload( byval position as const single ) '/ _
		( _
			@"balance", @"fb_sfxBalance", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function balance overload( ) as single '/ _
		( _
			@"balance", @"fb_sfxBalanceGet", _
			FB_DATATYPE_SINGLE, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			0 _
		), _
		/' sub pan overload( byval channel as const long, byval position as const single ) '/ _
		( _
			@"pan", @"fb_sfxPan", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function pan overload( byval channel as const long ) as single '/ _
		( _
			@"pan", @"fb_sfxPanGet", _
			FB_DATATYPE_SINGLE, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub note overload( byref name as const string, byval octave as const long, byval duration as const single ) '/ _
		( _
			@"note", @"fb_sfxNote", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			3, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub note overload( byval channel as const long, byref name as const string, byval octave as const long, byval duration as const single ) '/ _
		( _
			@"note", @"fb_sfxNoteChannel", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			4, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub wave overload( byval id as const long, byval waveform as const long ) '/ _
		( _
			@"wave", @"fb_sfxWaveCmd", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub envelope overload( byval id as const long, byval attack as const single, byval decay as const single, byval sustain as const single, byval release as const single ) '/ _
		( _
			@"envelope", @"fb_sfxEnvelopeCmd", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			5, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub instrument overload( byval id as const long, byval wave_id as const long, byval env_id as const long ) '/ _
		( _
			@"instrument", @"fb_sfxInstrumentDefine", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			3, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub instrument overload( byval channel as const long, byval instrument_id as const long ) '/ _
		( _
			@"instrument", @"fb_sfxInstrumentAssign", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub rest overload( byval duration as const single ) '/ _
		( _
			@"rest", @"fb_sfxRest", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub rest overload( byval channel as const long, byval duration as const single ) '/ _
		( _
			@"rest", @"fb_sfxRestChannel", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub tone overload( byval channel as const long, byval frequency as const long, byval duration as const single ) '/ _
		( _
			@"tone", @"fb_sfxTone", _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_OVER, _
			3, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' EOL '/ _
		( _
			NULL _
		) _
	}

'' ----------------------------------------------------------------------------
'' Hidden command helpers
'' ----------------------------------------------------------------------------

	dim shared as FB_RTL_PROCDEF cmddata( 0 to ... ) = _
	{ _
		/' sub fb_sfxSoundStop( ) '/ _
		( _
			@FB_RTL_SFXSOUNDSTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxSoundStopChannel( byval channel as const long ) '/ _
		( _
			@FB_RTL_SFXSOUNDSTOPCHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxNoiseStopAll( ) '/ _
		( _
			@FB_RTL_SFXNOISESTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxNoiseStop( byval channel as const long ) '/ _
		( _
			@FB_RTL_SFXNOISESTOPCHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxToneStopAll( ) '/ _
		( _
			@FB_RTL_SFXTONESTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxToneStop( byval channel as const long ) '/ _
		( _
			@FB_RTL_SFXTONESTOPCHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxPlayStop( ) '/ _
		( _
			@FB_RTL_SFXPLAYSTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxPlayPause( ) '/ _
		( _
			@FB_RTL_SFXPLAYPAUSE, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxPlayResume( ) '/ _
		( _
			@FB_RTL_SFXPLAYRESUME, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxPlayStatus( ) as long '/ _
		( _
			@FB_RTL_SFXPLAYSTATUS, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMusicLoad( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXMUSICLOAD, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMusicPlayCmd( byval id as const long ) as long '/ _
		( _
			@FB_RTL_SFXMUSICPLAY, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMusicPlayFile( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXMUSICPLAYFILE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMusicLoopCmd( byval id as const long ) as long '/ _
		( _
			@FB_RTL_SFXMUSICLOOP, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMusicLoopFile( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXMUSICLOOPFILE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxMusicPause( ) '/ _
		( _
			@FB_RTL_SFXMUSICPAUSE, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxMusicPauseId( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXMUSICPAUSEID, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxMusicResume( ) '/ _
		( _
			@FB_RTL_SFXMUSICRESUME, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxMusicResumeId( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXMUSICRESUMEID, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxMusicStop( ) '/ _
		( _
			@FB_RTL_SFXMUSICSTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxMusicStopId( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXMUSICSTOPID, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMusicStatus( ) as long '/ _
		( _
			@FB_RTL_SFXMUSICSTATUS, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMusicCurrent( ) as long '/ _
		( _
			@FB_RTL_SFXMUSICCURRENT, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMusicPosition( ) as long '/ _
		( _
			@FB_RTL_SFXMUSICPOSITION, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxSfxLoad( byval id as const long, byref filename as const string ) '/ _
		( _
			@FB_RTL_SFXSFXLOAD, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxPlay( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXPLAY, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxPlayChannel( byval channel as const long, byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXPLAYCHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxLoop( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXLOOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxLoopChannel( byval channel as const long, byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXLOOPCHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			2, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxStop( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXSTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxStopChannel( byval channel as const long ) '/ _
		( _
			@FB_RTL_SFXSFXSTOPCHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxStopAll( ) '/ _
		( _
			@FB_RTL_SFXSFXSTOPALL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxSfxPause( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXPAUSE, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxPauseChannel( byval channel as const long ) '/ _
		( _
			@FB_RTL_SFXSFXPAUSECHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxPauseAll( ) '/ _
		( _
			@FB_RTL_SFXSFXPAUSEALL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxSfxResume( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXSFXRESUME, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxResumeChannel( byval channel as const long ) '/ _
		( _
			@FB_RTL_SFXSFXRESUMECHANNEL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxSfxResumeAll( ) '/ _
		( _
			@FB_RTL_SFXSFXRESUMEALL, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxSfxStatus( byval id as const long ) as long '/ _
		( _
			@FB_RTL_SFXSFXSTATUS, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxSfxStatusChannel( byval channel as const long ) as long '/ _
		( _
			@FB_RTL_SFXSFXSTATUSCHANNEL, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxSfxAnyActive( ) as long '/ _
		( _
			@FB_RTL_SFXSFXANYACTIVE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxAudioPlay( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXAUDIOPLAY, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxAudioLoop( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXAUDIOLOOP, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxAudioStop( ) '/ _
		( _
			@FB_RTL_SFXAUDIOSTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxAudioPause( ) '/ _
		( _
			@FB_RTL_SFXAUDIOPAUSE, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxAudioResume( ) '/ _
		( _
			@FB_RTL_SFXAUDIORESUME, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxAudioStatus( ) as long '/ _
		( _
			@FB_RTL_SFXAUDIOSTATUS, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxStreamOpen( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXSTREAMOPEN, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxStreamPlay( ) as long '/ _
		( _
			@FB_RTL_SFXSTREAMPLAY, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxStreamStop( ) '/ _
		( _
			@FB_RTL_SFXSTREAMSTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxStreamPause( ) '/ _
		( _
			@FB_RTL_SFXSTREAMPAUSE, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxStreamResume( ) '/ _
		( _
			@FB_RTL_SFXSTREAMRESUME, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxStreamPosition( ) as long '/ _
		( _
			@FB_RTL_SFXSTREAMPOSITION, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxStreamSeek( byval position as const long ) as long '/ _
		( _
			@FB_RTL_SFXSTREAMSEEK, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMidiOpen( byval device as const long ) as long '/ _
		( _
			@FB_RTL_SFXMIDIOPEN, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMidiClose( ) as long '/ _
		( _
			@FB_RTL_SFXMIDICLOSE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMidiPlay( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXMIDIPLAY, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxMidiStop( ) as long '/ _
		( _
			@FB_RTL_SFXMIDISTOP, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMidiPause( ) as long '/ _
		( _
			@FB_RTL_SFXMIDIPAUSE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMidiResume( ) as long '/ _
		( _
			@FB_RTL_SFXMIDIRESUME, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxMidiSend( byval status as const long, byval data1 as const long, byval data2 as const long ) as long '/ _
		( _
			@FB_RTL_SFXMIDISEND, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			3, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxDeviceList( ) '/ _
		( _
			@FB_RTL_SFXDEVICELIST, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxDeviceSelect( byval id as const long ) as long '/ _
		( _
			@FB_RTL_SFXDEVICESELECT, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxDeviceInfo( byval id as const long ) '/ _
		( _
			@FB_RTL_SFXDEVICEINFO, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' sub fb_sfxDeviceInfoCurrent( ) '/ _
		( _
			@FB_RTL_SFXDEVICEINFOCURRENT, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxCaptureStart( ) as long '/ _
		( _
			@FB_RTL_SFXCAPTURESTART, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxCaptureStop( ) '/ _
		( _
			@FB_RTL_SFXCAPTURESTOP, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxCapturePause( ) '/ _
		( _
			@FB_RTL_SFXCAPTUREPAUSE, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' sub fb_sfxCaptureResume( ) '/ _
		( _
			@FB_RTL_SFXCAPTURERESUME, NULL, _
			FB_DATATYPE_VOID, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxCaptureStatus( ) as long '/ _
		( _
			@FB_RTL_SFXCAPTURESTATUS, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxCaptureSaveCmd( byref filename as const string ) as long '/ _
		( _
			@FB_RTL_SFXCAPTURESAVE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			1, _
			{ _
				( typeAddrOf( typeSetIsConst( FB_DATATYPE_CHAR ) ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' function fb_sfxCaptureAvailable( ) as long '/ _
		( _
			@FB_RTL_SFXCAPTUREAVAILABLE, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			0 _
		), _
		/' function fb_sfxCaptureReadSamples( byval buffer as single ptr, byval frames as const long ) as long '/ _
		( _
			@FB_RTL_SFXCAPTUREREAD, NULL, _
			FB_DATATYPE_LONG, FB_FUNCMODE_CDECL, _
			@hSfxlib_cb, FB_RTL_OPT_NONE, _
			2, _
			{ _
				( typeAddrOf( FB_DATATYPE_SINGLE ), FB_PARAMMODE_BYVAL, FALSE ), _
				( typeSetIsConst( FB_DATATYPE_LONG ), FB_PARAMMODE_BYVAL, FALSE ) _
			} _
		), _
		/' EOL '/ _
		( _
			NULL _
		) _
	}

'' ----------------------------------------------------------------------------
'' Module lifetime
'' ----------------------------------------------------------------------------

sub rtlSfxModInit( )

	rtlAddIntrinsicProcs( @funcdata(0) )
	rtlAddIntrinsicProcs( @cmddata(0) )

end sub

sub rtlSfxModEnd( )

	'' procs will be deleted when symbEnd is called

end sub

private function hSfxlib_cb( byval sym as FBSYMBOL ptr ) as integer
	env.clopt.fbsfx = TRUE
	function = TRUE
end function

'' end of rtl-sfx.bas
