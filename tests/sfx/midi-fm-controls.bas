'
' FreeBASIC sfxlib tests
' ----------------------
'
' File: midi-fm-controls.bas
'
' Purpose:
'
'     Verify the percussion and commonly used controller behavior of the
'     software MIDI fallback.
'
' Responsibilities:
'
'     - exercise the complete General MIDI percussion key range
'     - verify channel volume changes the generated waveform
'     - verify pitch bend changes the heard pitch
'     - verify sustain holds and then releases a note
'
' This file intentionally does NOT contain:
'
'     - all-program coverage
'     - Standard MIDI File parsing tests
'     - native MIDI device tests
'

' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/midi-fm-controls.tmp"
const DUMP_FRAMES = 145000
const DRUM_FIRST_NOTE = 35
const DRUM_COUNT = 47
const DRUM_FRAMES = 2048
const ANALYSIS_FRAMES = 12000

redim as single samples( 0 to ANALYSIS_FRAMES - 1 )
redim as single previous_drum( 0 to DRUM_FRAMES - 1 )
dim as integer cursor = 0

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, DUMP_FRAMES )
setenviron "SFXLIB_MIDI_DRIVER=fm"

ASSERT( midi open( 0 ) = 0 )

' Channel 10 is MIDI channel index 9.
for drum as integer = 0 to DRUM_COUNT - 1
	ASSERT( midi send( &h99, DRUM_FIRST_NOTE + drum, 112 ) = 0 )
	fb_sfxUpdate( DRUM_FRAMES )
	cursor += DRUM_FRAMES
	ASSERT( midi send( &hb9, 120, 0 ) = 0 )
next

' A sustaining organ makes the volume ratio easy to measure.
ASSERT( midi send( &hc0, 16, 0 ) = 0 )
ASSERT( midi send( &hb0, 7, 127 ) = 0 )
ASSERT( midi send( &h90, 60, 112 ) = 0 )
fb_sfxUpdate( 4096 )
cursor += 4096

dim as integer loud_offset = cursor
fb_sfxUpdate( 2048 )
cursor += 2048

ASSERT( midi send( &hb0, 7, 32 ) = 0 )
dim as integer quiet_offset = cursor
fb_sfxUpdate( 2048 )
cursor += 2048
ASSERT( midi send( &hb0, 120, 0 ) = 0 )

' A flute-like preset has little enough modulation for stable pitch checks.
ASSERT( midi send( &hb0, 121, 0 ) = 0 )
ASSERT( midi send( &hc0, 73, 0 ) = 0 )
ASSERT( midi send( &h90, 69, 112 ) = 0 )
fb_sfxUpdate( 4096 )
cursor += 4096

dim as integer center_pitch_offset = cursor
fb_sfxUpdate( 4096 )
cursor += 4096

ASSERT( midi send( &he0, 127, 127 ) = 0 )
dim as integer bent_pitch_offset = cursor
fb_sfxUpdate( 4096 )
cursor += 4096
ASSERT( midi send( &hb0, 120, 0 ) = 0 )

' Sustain must defer Note Off until the pedal controller is released.
ASSERT( midi send( &hb0, 121, 0 ) = 0 )
ASSERT( midi send( &hc0, 16, 0 ) = 0 )
ASSERT( midi send( &hb0, 64, 127 ) = 0 )
ASSERT( midi send( &h90, 69, 112 ) = 0 )
fb_sfxUpdate( 2048 )
cursor += 2048
ASSERT( midi send( &h80, 69, 0 ) = 0 )

dim as integer sustained_offset = cursor
fb_sfxUpdate( 4096 )
cursor += 4096

ASSERT( midi send( &hb0, 64, 0 ) = 0 )
dim as integer release_offset = cursor
fb_sfxUpdate( 12000 )
cursor += 12000

ASSERT( midi close() = 0 )

dim as integer dump_input = freefile()
dim as integer frames = 0
dim as integer read_frames

ASSERT( open( DUMP_FILE for input as #dump_input ) = 0 )

for drum as integer = 0 to DRUM_COUNT - 1
	dim as single peak = 0.0
	dim as integer finite_samples = 1

	read_frames = SfxTestReadDumpSamples( dump_input, samples(), DRUM_FRAMES )
	ASSERT( read_frames = DRUM_FRAMES )
	frames += read_frames

	for frame as integer = 0 to DRUM_FRAMES - 1
		dim as single sample = samples( frame )

		if( sample <> sample ) then
			finite_samples = 0
		end if

		if( abs( sample ) > peak ) then
			peak = abs( sample )
		end if
	next

	ASSERT( finite_samples <> 0 )
	ASSERT( peak < 0.50 )
	ASSERT( SfxTestRms( samples(), 0, DRUM_FRAMES ) > 0.001 )
	ASSERT( SfxTestCountChanges( samples(), 0, DRUM_FRAMES, 0.00001 ) > 256 )

	if( drum > 0 ) then
		dim as integer differences = 0

		for frame as integer = 0 to DRUM_FRAMES - 1
			if( abs( samples( frame ) - previous_drum( frame ) ) > 0.00001 ) then
				differences += 1
			end if
		next

		ASSERT( differences > 256 )
	end if

	for frame as integer = 0 to DRUM_FRAMES - 1
		previous_drum( frame ) = samples( frame )
	next
next

' Skip the organ attack before measuring the two volume levels.
read_frames = SfxTestReadDumpSamples( dump_input, samples(), 4096 )
ASSERT( read_frames = 4096 )
frames += read_frames

read_frames = SfxTestReadDumpSamples( dump_input, samples(), 2048 )
ASSERT( read_frames = 2048 )
frames += read_frames
dim as double loud_rms = SfxTestRms( samples(), 0, read_frames )

read_frames = SfxTestReadDumpSamples( dump_input, samples(), 2048 )
ASSERT( read_frames = 2048 )
frames += read_frames
dim as double quiet_rms = SfxTestRms( samples(), 0, read_frames )

ASSERT( loud_rms > 0.01 )
ASSERT( quiet_rms < loud_rms * 0.40 )

' Skip the flute attack before measuring center and bent pitch.
read_frames = SfxTestReadDumpSamples( dump_input, samples(), 4096 )
ASSERT( read_frames = 4096 )
frames += read_frames

read_frames = SfxTestReadDumpSamples( dump_input, samples(), 4096 )
ASSERT( read_frames = 4096 )
frames += read_frames
dim as double center_pitch = SfxTestEstimateZeroCrossHz( samples(), 0, read_frames )

read_frames = SfxTestReadDumpSamples( dump_input, samples(), 4096 )
ASSERT( read_frames = 4096 )
frames += read_frames
dim as double bent_pitch = SfxTestEstimateZeroCrossHz( samples(), 0, read_frames )

ASSERT( center_pitch > 350.0 )
ASSERT( center_pitch < 550.0 )
ASSERT( bent_pitch > center_pitch * 1.05 )

' Skip the sustain attack, then measure the held note and release tail.
read_frames = SfxTestReadDumpSamples( dump_input, samples(), 2048 )
ASSERT( read_frames = 2048 )
frames += read_frames

read_frames = SfxTestReadDumpSamples( dump_input, samples(), 4096 )
ASSERT( read_frames = 4096 )
frames += read_frames
ASSERT( SfxTestRms( samples(), 0, read_frames ) > 0.01 )

read_frames = SfxTestReadDumpSamples( dump_input, samples(), 12000 )
ASSERT( read_frames = 12000 )
frames += read_frames
ASSERT( SfxTestRms( samples(), 9952, 2048 ) < 0.0005 )

do
	read_frames = SfxTestReadDumpSamples( dump_input, samples(), ANALYSIS_FRAMES )
	frames += read_frames
loop while( read_frames > 0 )

close #dump_input
ASSERT( frames >= cursor )

' end of midi-fm-controls.bas
