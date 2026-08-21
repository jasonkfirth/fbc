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
''     - open a caller-clocked stream at the device sample rate
''     - write mono and stereo raw samples
''     - verify that invalid channel counts are rejected
''     - verify that the ordinary mixer does not pad a raw stream
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

ASSERT( sfxlib.RawOpen() = 44100 )
ASSERT( sfxlib.RawUnderruns() = 0 )

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
ASSERT( sfxlib.RawOpen() = 44100 )
ASSERT( sfxlib.RawWrite( @mono(0), RAW_FRAMES, 1 ) = RAW_FRAMES )

'' Requesting too much must drain only the caller-supplied raw frames.  The
'' ordinary mixer must not append a second block of generated silence.
fb_sfxUpdate RAW_FRAMES * 2

sfxlib.OutputCaptureStop()

ASSERT( sfxlib.OutputCaptureSave( strptr( output_file ) ) = 0 )

file_num = freefile()
ASSERT( open( output_file for binary access read as #file_num ) = 0 )
ASSERT( lof( file_num ) = 44 + ( RAW_FRAMES * 2 * 2 ) )
close #file_num

sfxlib.RawClose()

'' Closing raw mode restores the ordinary mixer path.  With no active voices
'' this produces one block of silence, but it must still advance the output.
ASSERT( sfxlib.OutputCaptureStart() = 0 )
ASSERT( sfxlib.OutputCaptureReserve( RAW_FRAMES ) = 0 )
fb_sfxUpdate RAW_FRAMES
sfxlib.OutputCaptureStop()
ASSERT( sfxlib.OutputCaptureSave( strptr( output_file ) ) = 0 )

file_num = freefile()
ASSERT( open( output_file for binary access read as #file_num ) = 0 )
ASSERT( lof( file_num ) = 44 + ( RAW_FRAMES * 2 * 2 ) )
close #file_num

SfxTestDeleteFile( output_file )

'' Failed ASSERT calls terminate before this point.  Keep the successful path
'' independent from any residual file status left by the platform runtime.
end 0

'' end of raw-write.bas
