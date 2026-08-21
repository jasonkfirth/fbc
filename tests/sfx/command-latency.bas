''
'' FreeBASIC Sound Library tests
'' --------------------------------
''
'' File: command-latency.bas
''
'' Purpose:
''
''     Ensure ordinary mixer updates do not pre-render future silence ahead
''     of a sound command.
''
'' Responsibilities:
''
''     - prime the null output driver with a small silent update
''     - queue a sound after that update
''     - verify the sound begins in the next submitted block
''
'' This file intentionally does NOT contain:
''
''     - real audio-device timing
''     - raw-stream queue coverage
''     - subjective latency thresholds
''

' TEST_MODE : MULTI_MODULE_OK

#include once "sfx_test_common.bi"

const PRIME_FRAMES = 256
const SOUND_FRAMES = 2048
const MAX_FRAMES = PRIME_FRAMES + SOUND_FRAMES

redim as single samples( 0 to MAX_FRAMES - 1 )

dim as string temp_dir = trim( environ( "TMP" ) )
dim as string dump_file

if( temp_dir = "" ) then
	temp_dir = trim( environ( "TEMP" ) )
end if

if( temp_dir = "" ) then
	temp_dir = trim( environ( "TMPDIR" ) )
end if

if( temp_dir = "" ) then
	temp_dir = curdir()

	if( temp_dir = "" ) then
		temp_dir = "."
	end if
end if

if( right( temp_dir, 1 ) <> "/" andalso right( temp_dir, 1 ) <> "\\" ) then
	temp_dir += "/"
end if

dump_file = temp_dir + "sfx-command-latency-driver.tmp"

SfxTestUseNullDriver()
SfxTestSetDriverDump( dump_file, MAX_FRAMES )

fb_sfxUpdate( PRIME_FRAMES )
sound 0, 440, 0.10, 0.60
fb_sfxUpdate( SOUND_FRAMES )

dim as integer frames = SfxTestLoadDump( dump_file, samples() )

ASSERT( frames = MAX_FRAMES )
ASSERT( SfxTestRms( samples(), 0, PRIME_FRAMES ) = 0.0 )
ASSERT( SfxTestRms( samples(), PRIME_FRAMES + 64, 1024 ) > 0.01 )

'' end of command-latency.bas
