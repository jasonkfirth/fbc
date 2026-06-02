' TEST_MODE : MULTI_MODULE_TEST

''
'' FreeBASIC sfxlib tests
'' ----------------------
''
'' File: capture-buffer.bas
''
'' Purpose:
''
''     Verify the platform-independent capture buffer without using
''     microphone hardware.
''
'' Responsibilities:
''
''     - write and read raw captured PCM frames
''     - verify wraparound order
''     - verify capacity limiting and clear behavior
''     - verify public float reads convert the captured samples
''
'' This file intentionally does NOT contain:
''
''     - platform capture driver tests
''     - microphone permission handling
''     - WAV capture-save command coverage
''

#include once "sfx_test_common.bi"

#inclib "sfx"

declare function fb_sfxInit cdecl alias "fb_sfxInit" ( ) as long
declare sub fb_sfxExit cdecl alias "fb_sfxExit" ( )
declare sub fb_sfxCaptureBufferClear cdecl alias "fb_sfxCaptureBufferClear" ( )
declare function fb_sfxCaptureBufferWrite cdecl alias "fb_sfxCaptureBufferWrite" _
	( byval samples as short ptr, byval frames as long ) as long
declare function fb_sfxCaptureBufferRead cdecl alias "fb_sfxCaptureBufferRead" _
	( byval samples as short ptr, byval frames as long ) as long
declare function fb_sfxCaptureWrite cdecl alias "fb_sfxCaptureWrite" _
	( byval samples as short ptr, byval frames as long ) as long
declare function fb_sfxCaptureRead cdecl alias "fb_sfxCaptureRead" _
	( byval samples as single ptr, byval frames as long ) as long

const CAPTURE_SAMPLES = 65536
const CAPTURE_CHANNELS = 2
const CAPTURE_FRAMES = (CAPTURE_SAMPLES - 1) \ CAPTURE_CHANNELS
const CAPTURE_STOPPED = 0
const CAPTURE_RUNNING = 1
const CAPTURE_PAUSED = 2

function near_equal( byval lhs as single, byval rhs as single ) as integer
	function = ( abs( lhs - rhs ) < 0.0001f )
end function

SfxTestUseNullDriver()
ASSERT( fb_sfxInit() = 0 )

fb_sfxCaptureBufferClear()
ASSERT( fb_sfxCaptureAvailable() = 0 )
ASSERT( fb_sfxCaptureStatus() = CAPTURE_STOPPED )

fb_sfxCaptureResume()
ASSERT( fb_sfxCaptureStatus() = CAPTURE_RUNNING )
fb_sfxCapturePause()
ASSERT( fb_sfxCaptureStatus() = CAPTURE_PAUSED )
fb_sfxCaptureBufferClear()

dim as short first_write(0 to 7) = { 10, 11, 20, 21, 30, 31, 40, 41 }
dim as short second_write(0 to 9) = { 50, 51, 60, 61, 70, 71, 80, 81, 90, 91 }
dim as short raw_read(0 to 11)

ASSERT( fb_sfxCaptureBufferWrite( @first_write(0), 4 ) = 4 )
ASSERT( fb_sfxCaptureAvailable() = 4 )
ASSERT( fb_sfxCaptureBufferRead( @raw_read(0), 3 ) = 3 )
ASSERT( raw_read(0) = 10 )
ASSERT( raw_read(1) = 11 )
ASSERT( raw_read(4) = 30 )
ASSERT( raw_read(5) = 31 )
ASSERT( fb_sfxCaptureAvailable() = 1 )

ASSERT( fb_sfxCaptureBufferWrite( @second_write(0), 5 ) = 5 )
ASSERT( fb_sfxCaptureAvailable() = 6 )
ASSERT( fb_sfxCaptureBufferRead( @raw_read(0), 6 ) = 6 )
ASSERT( raw_read(0) = 40 )
ASSERT( raw_read(1) = 41 )
ASSERT( raw_read(2) = 50 )
ASSERT( raw_read(3) = 51 )
ASSERT( raw_read(10) = 90 )
ASSERT( raw_read(11) = 91 )
ASSERT( fb_sfxCaptureAvailable() = 0 )

redim as short fill_samples(0 to (CAPTURE_FRAMES * CAPTURE_CHANNELS) - 1)
dim as integer i

for i = 0 to ubound( fill_samples )
	fill_samples(i) = i and &h7fff
next

ASSERT( fb_sfxCaptureBufferWrite( @fill_samples(0), CAPTURE_FRAMES + 128 ) = CAPTURE_FRAMES )
ASSERT( fb_sfxCaptureAvailable() = CAPTURE_FRAMES )
ASSERT( fb_sfxCaptureBufferWrite( @first_write(0), 1 ) = 0 )
ASSERT( fb_sfxCaptureBufferWrite( @first_write(0), &h7fffffff ) = 0 )

fb_sfxCaptureBufferClear()
ASSERT( fb_sfxCaptureAvailable() = 0 )
ASSERT( fb_sfxCaptureBufferRead( @raw_read(0), &h7fffffff ) = 0 )

dim as short pcm(0 to 3) = { -32767, 0, 16384, 32767 }
dim as single converted(0 to 3)

ASSERT( fb_sfxCaptureWrite( @pcm(0), 2 ) = 2 )
ASSERT( fb_sfxCaptureRead( @converted(0), 2 ) = 2 )
ASSERT( near_equal( converted(0), -1.0f ) )
ASSERT( near_equal( converted(1), 0.0f ) )
ASSERT( converted(2) > 0.49f andalso converted(2) < 0.51f )
ASSERT( near_equal( converted(3), 1.0f ) )

fb_sfxExit()

'' end of capture-buffer.bas
