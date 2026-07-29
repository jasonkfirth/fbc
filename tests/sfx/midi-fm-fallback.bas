'
' FreeBASIC sfxlib tests
' ----------------------
'
' File: midi-fm-fallback.bas
'
' Purpose:
'
'     Verify that the C software MIDI fallback produces a bounded waveform
'     and releases it to silence through the normal sfxlib mixer path.
'
' Responsibilities:
'
'     - force the software MIDI backend on a host with native MIDI support
'     - send program, controller, note-on, and note-off messages
'     - check that the mixer receives an audible waveform
'     - check that the release envelope reaches silence
'
' This file intentionally does NOT contain:
'
'     - Standard MIDI File parsing tests
'     - platform MIDI device tests
'

' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/midi-fm-fallback.tmp"
const MAX_FRAMES = 24000

redim as single samples( 0 to MAX_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, MAX_FRAMES )
setenviron "SFXLIB_MIDI_DRIVER=fm"

ASSERT( midi open( 0 ) = 0 )
ASSERT( midi send( &hc0, 80, 0 ) = 0 )
ASSERT( midi send( &hb0, 7, 110 ) = 0 )
ASSERT( midi send( &h90, 69, 112 ) = 0 )

fb_sfxUpdate( 4096 )

ASSERT( midi send( &h80, 69, 0 ) = 0 )
fb_sfxUpdate( 16000 )

ASSERT( midi close() = 0 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )

ASSERT( frames >= 20000 )
ASSERT( SfxTestRms( samples(), 512, 2048 ) > 0.015 )
ASSERT( SfxTestCountChanges( samples(), 512, 2048, 0.001 ) > 256 )
ASSERT( SfxTestRms( samples(), frames - 2048, 2048 ) < 0.0005 )

' end of midi-fm-fallback.bas
