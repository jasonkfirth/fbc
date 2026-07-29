'
' FreeBASIC sfxlib tests
' ----------------------
'
' File: midi-fm-polyphony.bas
'
' Purpose:
'
'     Verify bounded mixing and deterministic voice stealing when the
'     software MIDI fallback reaches its 32-voice limit.
'
' Responsibilities:
'
'     - render a full 32-voice chord
'     - force another 32 note allocations without releasing the first set
'     - reject silence, non-finite output, and excessive peaks
'     - verify that All Sound Off produces immediate silence
'
' This file intentionally does NOT contain:
'
'     - per-program coverage
'     - controller tests
'     - native MIDI device tests
'

' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/midi-fm-polyphony.tmp"
const FIRST_CHORD_FRAMES = 4096
const SECOND_CHORD_FRAMES = 4096
const SILENT_FRAMES = 2048
const TOTAL_FRAMES = FIRST_CHORD_FRAMES + SECOND_CHORD_FRAMES + SILENT_FRAMES

redim as single samples( 0 to TOTAL_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, TOTAL_FRAMES )
setenviron "SFXLIB_MIDI_DRIVER=fm"

ASSERT( midi open( 0 ) = 0 )
ASSERT( midi send( &hc0, 0, 0 ) = 0 )

for note_number as integer = 36 to 67
	ASSERT( midi send( &h90, note_number, 104 ) = 0 )
next

fb_sfxUpdate( FIRST_CHORD_FRAMES )

' These notes must replace the oldest voices without exceeding the pool.
for note_number as integer = 68 to 99
	ASSERT( midi send( &h90, note_number, 104 ) = 0 )
next

fb_sfxUpdate( SECOND_CHORD_FRAMES )
ASSERT( midi send( &hb0, 120, 0 ) = 0 )
fb_sfxUpdate( SILENT_FRAMES )

ASSERT( midi close() = 0 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )
dim as single peak = 0.0
dim as integer finite_samples = 1

ASSERT( frames >= TOTAL_FRAMES )

for frame as integer = 0 to TOTAL_FRAMES - 1
	dim as single sample = samples( frame )

	if( sample <> sample ) then
		finite_samples = 0
	end if

	if( abs( sample ) > peak ) then
		peak = abs( sample )
	end if
next

ASSERT( finite_samples <> 0 )
ASSERT( peak < 0.90 )
ASSERT( SfxTestRms( samples(), 512, 2048 ) > 0.005 )
ASSERT( SfxTestRms( samples(), FIRST_CHORD_FRAMES + 512, 2048 ) > 0.005 )
ASSERT( SfxTestRms( samples(), FIRST_CHORD_FRAMES + SECOND_CHORD_FRAMES, SILENT_FRAMES ) < 0.00001 )

' end of midi-fm-polyphony.bas
