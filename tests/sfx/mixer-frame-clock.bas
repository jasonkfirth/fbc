''
'' FreeBASIC Sound Library tests
'' --------------------------------
''
'' File: mixer-frame-clock.bas
''
'' Purpose:
''
''     Verify that voice time advances by exactly the number of frames sent to
''     the output driver across repeated non-block-aligned updates.
''
'' Responsibilities:
''
''     - render several notes whose intervals do not divide the mix block
''     - assert that every note ends at its requested output-frame boundary
''     - catch queued partial blocks that desynchronize mixer and output clocks
''
'' This file intentionally does NOT contain:
''
''     - platform audio timing
''     - MIDI parsing
''     - subjective waveform checks
''

' TEST_MODE : MULTI_MODULE_OK

#include once "sfx_test_common.bi"

const UPDATE_FRAMES = 51882
const NOTE_FRAMES = UPDATE_FRAMES - 1

SfxTestUseNullDriver()

for pass_index as integer = 1 to 4
	sound 0, 440 + pass_index, _
	      csng( cdbl( NOTE_FRAMES ) / SFX_TEST_SAMPLE_RATE ), _
	      0.25

	ASSERT( fb_sfxVoiceActiveCount() = 1 )
	fb_sfxUpdate( UPDATE_FRAMES )
	ASSERT( fb_sfxVoiceActiveCount() = 0 )
next

'' end of mixer-frame-clock.bas
