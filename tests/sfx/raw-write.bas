' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC sfxlib tests
'' ----------------------
''
'' File: raw-write.bas
''
'' Purpose:
''
''     Verify the opt-in sfxlib_raw.bi raw output queue declaration.
''
'' Responsibilities:
''
''     - force the null driver before sfxlib initializes
''     - write mono and stereo raw samples
''     - verify that invalid channel counts are rejected
''     - verify output-side WAV recording for raw writes
''
'' This file intentionally does NOT contain:
''
''     - subjective audio checks
''     - platform audio device access
''     - BASIC command syntax coverage
''

#include once "sfx_test_common.bi"
#include once "sfxlib_raw.bi"

const RAW_FRAMES = 32

dim as single mono(0 to RAW_FRAMES - 1)
dim as single stereo(0 to (RAW_FRAMES * 2) - 1)
dim as long frame_index
dim as string output_file = "raw-write-output.wav"
dim as integer file_num

SfxTestUseNullDriver()
SfxTestDeleteFile( output_file )

for frame_index = 0 to RAW_FRAMES - 1
	mono(frame_index) = 0.25
	stereo((frame_index * 2) + 0) = -0.25
	stereo((frame_index * 2) + 1) = 0.25
next

ASSERT( sfxlib.RawWrite( @mono(0), RAW_FRAMES, 1 ) = RAW_FRAMES )
ASSERT( sfxlib.RawWrite( @stereo(0), RAW_FRAMES, 2 ) = RAW_FRAMES )
ASSERT( sfxlib.RawWrite( @mono(0), RAW_FRAMES, 3 ) = -1 )

fb_sfxUpdate RAW_FRAMES * 2

ASSERT( sfxlib.OutputCaptureStart() = 0 )
ASSERT( sfxlib.OutputCaptureReserve( RAW_FRAMES ) = 0 )
ASSERT( sfxlib.RawWrite( @mono(0), RAW_FRAMES, 1 ) = RAW_FRAMES )

fb_sfxUpdate RAW_FRAMES

sfxlib.OutputCaptureStop()

ASSERT( sfxlib.OutputCaptureSave( strptr( output_file ) ) = 0 )

file_num = freefile()
ASSERT( open( output_file for binary access read as #file_num ) = 0 )
ASSERT( lof( file_num ) > 44 )
close #file_num

SfxTestDeleteFile( output_file )

'' end of raw-write.bas
