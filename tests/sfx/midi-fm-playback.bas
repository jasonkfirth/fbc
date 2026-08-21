'
' FreeBASIC sfxlib tests
' ----------------------
'
' File: midi-fm-playback.bas
'
' Purpose:
'
'     Verify that Standard MIDI File playback reaches the software FM
'     fallback and produces audio through the normal mixer.
'
' Responsibilities:
'
'     - create a bounded format 0 Standard MIDI File
'     - play its events through the software MIDI route
'     - advance the null-driver mixer close to the MIDI wall clock
'     - verify an audible note and a completed release tail
'
' This file intentionally does NOT contain:
'
'     - broad Standard MIDI File parser coverage
'     - platform MIDI device tests
'

' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

declare function fb_sfxMidiPlaying cdecl alias "fb_sfxMidiPlaying" ( ) as long

const MIDI_FILE = "sfx/midi-fm-playback.mid"
const DUMP_FILE = "sfx/midi-fm-playback.tmp"
const MAX_FRAMES = 50000
const RELEASE_FRAMES = 12000
const TAIL_FRAMES = 4096

sub WriteMidi16BE( byval f as integer, byval value as integer )
	SfxTestWriteByte( f, value \ 256 )
	SfxTestWriteByte( f, value )
end sub

sub WriteMidi32BE( byval f as integer, byval value as integer )
	SfxTestWriteByte( f, value \ 16777216 )
	SfxTestWriteByte( f, value \ 65536 )
	SfxTestWriteByte( f, value \ 256 )
	SfxTestWriteByte( f, value )
end sub

sub WriteTestMidi()
	dim as integer f = freefile()

	SfxTestDeleteFile( MIDI_FILE )
	open MIDI_FILE for binary as #f

	SfxTestWriteText( f, "MThd" )
	WriteMidi32BE( f, 6 )
	WriteMidi16BE( f, 0 )
	WriteMidi16BE( f, 1 )
	WriteMidi16BE( f, 96 )

	SfxTestWriteText( f, "MTrk" )
	WriteMidi32BE( f, 15 )

	' Program 80, A4 note-on, one quarter-note gate, note-off, end of track.
	SfxTestWriteByte( f, 0 )
	SfxTestWriteByte( f, &hc0 )
	SfxTestWriteByte( f, 80 )

	SfxTestWriteByte( f, 0 )
	SfxTestWriteByte( f, &h90 )
	SfxTestWriteByte( f, 69 )
	SfxTestWriteByte( f, 112 )

	SfxTestWriteByte( f, 96 )
	SfxTestWriteByte( f, &h80 )
	SfxTestWriteByte( f, 69 )
	SfxTestWriteByte( f, 0 )

	SfxTestWriteByte( f, 0 )
	SfxTestWriteByte( f, &hff )
	SfxTestWriteByte( f, &h2f )
	SfxTestWriteByte( f, 0 )

	close #f
end sub

redim as single tail_samples( 0 to TAIL_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, MAX_FRAMES )
setenviron "SFXLIB_MIDI_DRIVER=fm"
WriteTestMidi()

ASSERT( midi open( 0 ) = 0 )
ASSERT( midi play( MIDI_FILE ) = 0 )

dim as integer wait_steps = 0

do while( fb_sfxMidiPlaying() <> 0 and wait_steps < 200 )
	' Ten milliseconds at 44.1 kHz is approximately 441 output frames.
	fb_sfxUpdate( 441 )
	sleep 10
	wait_steps += 1
loop

ASSERT( fb_sfxMidiPlaying() = 0 )

' Give the synth enough mixer time to finish its program-family release.
fb_sfxUpdate( RELEASE_FRAMES )
ASSERT( midi close() = 0 )

dim as integer dump_input = freefile()
dim as integer frames = 0
dim as integer body_frames = 0
dim as double body_sum_squares = 0.0
dim as string line_text

ASSERT( open( DUMP_FILE for input as #dump_input ) = 0 )

do while( eof( dump_input ) = 0 )
	line input #dump_input, line_text

	if( len( line_text ) > 0 ) then
		dim as single sample = val( line_text )
		dim as integer tail_index = frames mod TAIL_FRAMES

		if( frames >= TAIL_FRAMES ) then
			body_sum_squares += cdbl( tail_samples( tail_index ) ) * _
			                    cdbl( tail_samples( tail_index ) )
			body_frames += 1
		end if

		tail_samples( tail_index ) = sample
		frames += 1
	end if
loop

close #dump_input

' Mixer diagnostics are deliberately simple text output.  Slow filesystems
' can let the wall-clock MIDI worker finish after fewer update calls, but at
' least one block must have been generated while the note was playing.
ASSERT( frames > RELEASE_FRAMES )
ASSERT( body_frames = frames - TAIL_FRAMES )
ASSERT( sqr( body_sum_squares / cdbl( body_frames ) ) > 0.025 )

dim as double tail_sum_squares = 0.0

for frame as integer = 0 to TAIL_FRAMES - 1
	tail_sum_squares += cdbl( tail_samples( frame ) ) * _
	                    cdbl( tail_samples( frame ) )
next

ASSERT( sqr( tail_sum_squares / cdbl( TAIL_FRAMES ) ) < 0.0005 )

SfxTestDeleteFile( MIDI_FILE )

' end of midi-fm-playback.bas
