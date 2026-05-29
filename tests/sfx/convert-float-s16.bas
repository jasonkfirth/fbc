' TEST_MODE : MULTI_MODULE_TEST

''
'' FreeBASIC sfxlib tests
'' ----------------------
''
'' File: convert-float-s16.bas
''
'' Purpose:
''
''     Verify the shared float-to-signed-16 conversion helper used by
''     platform audio drivers.
''
'' Responsibilities:
''
''     - check clipping for values outside the mixer range
''     - check the deliberate -1.0 to -32767 mapping
''     - check NaN and infinity handling
''
'' This file intentionally does NOT contain:
''
''     - platform driver tests
''     - mixer signal generation
''     - playback timing checks
''

#inclib "sfx"

declare sub fb_sfxConvertFloatToS16 cdecl alias "fb_sfxConvertFloatToS16" _
	( _
		byval src as single ptr, _
		byval dst as short ptr, _
		byval samples as integer _
	)

dim src(0 to 9) as single
dim dst(0 to 9) as short

src(0) = -2.0f
src(1) = -1.0f
src(2) = -0.5f
src(3) = 0.0f
src(4) = 0.5f
src(5) = 1.0f
src(6) = 2.0f
src(7) = 0.0f / 0.0f
src(8) = 1.0f / 0.0f
src(9) = -1.0f / 0.0f

fb_sfxConvertFloatToS16( @src(0), @dst(0), 10 )

ASSERT( dst(0) = -32767 )
ASSERT( dst(1) = -32767 )
ASSERT( dst(2) = -16383 )
ASSERT( dst(3) = 0 )
ASSERT( dst(4) = 16383 )
ASSERT( dst(5) = 32767 )
ASSERT( dst(6) = 32767 )
ASSERT( dst(7) = 0 )
ASSERT( dst(8) = 32767 )
ASSERT( dst(9) = -32767 )

'' end of convert-float-s16.bas
