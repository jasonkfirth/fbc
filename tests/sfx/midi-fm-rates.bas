'
' FreeBASIC sfxlib tests
' ----------------------
'
' File: midi-fm-rates.bas
'
' Purpose:
'
'     Verify that the software MIDI fallback remains audible and bounded at
'     common output sample rates from 8 kHz through 192 kHz.
'
' Responsibilities:
'
'     - render low, sustained, slow-attack, bright, and percussion voices
'     - exercise the high-note FM bandwidth guard at every sample rate
'     - reject silent, non-finite, clipped, or static output
'
' This file intentionally does NOT contain:
'
'     - physical audio device reconfiguration
'     - subjective listening tests
'     - native MIDI device tests
'

' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

declare function fb_sfxTestSetSampleRate cdecl alias "fb_sfxTestSetSampleRate" _
	( byval sample_rate as long ) as long
declare sub fb_sfxForegroundFeedBegin cdecl alias "fb_sfxForegroundFeedBegin" ( )
declare sub fb_sfxForegroundFeedEnd cdecl alias "fb_sfxForegroundFeedEnd" ( )

const DUMP_FILE = "sfx/midi-fm-rates.tmp"
const DUMP_FRAMES = 250000
const RATE_COUNT = 9
const CASE_COUNT = 5
const WINDOW_COUNT = RATE_COUNT * CASE_COUNT

dim as integer sample_rates( 0 to RATE_COUNT - 1 ) = _
	{ 8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000, 192000 }
dim as integer offsets( 0 to WINDOW_COUNT - 1 )
dim as integer lengths( 0 to WINDOW_COUNT - 1 )
redim as single samples( 0 to DUMP_FRAMES - 1 )
dim as integer cursor = 0
dim as integer window_index = 0

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, DUMP_FRAMES )
setenviron "SFXLIB_MIDI_DRIVER=fm"

' The Linux audio worker normally advances the mixer in the background.  This
' test records exact frame offsets, so reserve the feed before initialization
' starts that worker and keep every rendered frame under the test's control.
fb_sfxForegroundFeedBegin()

ASSERT( midi open( 0 ) = 0 )

for rate_index as integer = 0 to RATE_COUNT - 1
	dim as integer sample_rate = sample_rates( rate_index )
	dim as integer case_frames = sample_rate \ 10

	ASSERT( fb_sfxTestSetSampleRate( sample_rate ) = 0 )

	for sound_case as integer = 0 to CASE_COUNT - 1
		offsets( window_index ) = cursor
		lengths( window_index ) = case_frames

		select case sound_case
		case 0
			ASSERT( midi send( &hc0, 0, 0 ) = 0 )
			ASSERT( midi send( &h90, 60, 112 ) = 0 )

		case 1
			ASSERT( midi send( &hc0, 43, 0 ) = 0 )
			ASSERT( midi send( &h90, 36, 112 ) = 0 )

		case 2
			ASSERT( midi send( &hc0, 95, 0 ) = 0 )
			ASSERT( midi send( &h90, 72, 112 ) = 0 )

		case 3
			' This bright high note is the alias-control stress case.
			ASSERT( midi send( &hc0, 103, 0 ) = 0 )
			ASSERT( midi send( &h90, 96, 112 ) = 0 )

		case 4
			ASSERT( midi send( &h99, 42, 112 ) = 0 )
		end select

		fb_sfxUpdate( case_frames )
		cursor += case_frames

		if( sound_case = 4 ) then
			ASSERT( midi send( &hb9, 120, 0 ) = 0 )
		else
			ASSERT( midi send( &hb0, 120, 0 ) = 0 )
		end if

		window_index += 1
	next
next

ASSERT( midi close() = 0 )
fb_sfxForegroundFeedEnd()

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )

ASSERT( cursor <= DUMP_FRAMES )
ASSERT( frames >= cursor )

for window_index = 0 to WINDOW_COUNT - 1
	dim as integer first_frame = offsets( window_index )
	dim as integer window_frames = lengths( window_index )
	dim as double rms = SfxTestRms( samples(), first_frame, window_frames )
	dim as single peak = 0.0
	dim as integer finite_samples = 1

	for frame as integer = 0 to window_frames - 1
		dim as single sample = samples( first_frame + frame )

		if( sample <> sample ) then
			finite_samples = 0
		end if

		if( abs( sample ) > peak ) then
			peak = abs( sample )
		end if
	next

	ASSERT( finite_samples <> 0 )
	ASSERT( rms > 0.00020 )
	ASSERT( rms < 0.25 )
	ASSERT( peak < 0.50 )
	ASSERT( SfxTestCountChanges( samples(), first_frame, window_frames, 0.000001 ) > window_frames \ 10 )
next

' end of midi-fm-rates.bas
