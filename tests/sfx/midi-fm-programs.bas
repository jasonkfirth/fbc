'
' FreeBASIC sfxlib tests
' ----------------------
'
' File: midi-fm-programs.bas
'
' Purpose:
'
'     Verify that every General MIDI program number produces a useful,
'     bounded waveform through the software MIDI fallback.
'
' Responsibilities:
'
'     - exercise all 128 melodic program numbers
'     - verify that every preset is audible and changes over time
'     - verify that neighboring presets do not collapse to one waveform
'     - reject non-finite, clipped, or unexpectedly loud output
'
' This file intentionally does NOT contain:
'
'     - subjective instrument-quality judgments
'     - percussion tests
'     - native MIDI device tests
'

' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/midi-fm-programs.tmp"
const PROGRAM_COUNT = 128
const PROGRAM_FRAMES = 4096
const TOTAL_FRAMES = PROGRAM_COUNT * PROGRAM_FRAMES

redim as single samples( 0 to TOTAL_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, TOTAL_FRAMES )
setenviron "SFXLIB_MIDI_DRIVER=fm"

ASSERT( midi open( 0 ) = 0 )

for program as integer = 0 to PROGRAM_COUNT - 1
	ASSERT( midi send( &hc0, program, 0 ) = 0 )
	ASSERT( midi send( &h90, 60, 112 ) = 0 )

	fb_sfxUpdate( PROGRAM_FRAMES )

	' All Sound Off prevents a long release from entering the next preset.
	ASSERT( midi send( &hb0, 120, 0 ) = 0 )
next

ASSERT( midi close() = 0 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )

ASSERT( frames >= TOTAL_FRAMES )

for program as integer = 0 to PROGRAM_COUNT - 1
	dim as integer first_frame = program * PROGRAM_FRAMES
	dim as double rms = SfxTestRms( samples(), first_frame, PROGRAM_FRAMES )
	dim as single peak = 0.0
	dim as integer finite_samples = 1

	for frame as integer = 0 to PROGRAM_FRAMES - 1
		dim as single sample = samples( first_frame + frame )

		if( sample <> sample ) then
			finite_samples = 0
		end if

		if( abs( sample ) > peak ) then
			peak = abs( sample )
		end if
	next

	ASSERT( finite_samples <> 0 )
	ASSERT( rms > 0.00025 )
	ASSERT( rms < 0.25 )
	ASSERT( peak < 0.50 )
	ASSERT( SfxTestCountChanges( samples(), first_frame, PROGRAM_FRAMES, 0.00001 ) > 256 )

	if( program > 0 ) then
		dim as integer differences = 0
		dim as integer previous_frame = first_frame - PROGRAM_FRAMES

		for frame as integer = 0 to PROGRAM_FRAMES - 1
			if( abs( samples( first_frame + frame ) - _
			         samples( previous_frame + frame ) ) > 0.00001 ) then
				differences += 1
			end if
		next

		ASSERT( differences > 256 )
	end if
next

' end of midi-fm-programs.bas
