' TEST_MODE : MULTI_MODULE_TEST

''
'' FreeBASIC sfxlib tests
'' ----------------------
''
'' File: convert-float-s16.bas
''
'' Purpose:
''
''     Verify the shared PCM conversion helpers used by platform audio
''     drivers and architecture-specific SIMD backends.
''
'' Responsibilities:
''
''     - check clipping for values outside the mixer range
''     - check the deliberate -1.0 to -32767 mapping
''     - check NaN and infinity handling
''     - compare vector-sized buffers and scalar tails with scalar helpers
''     - exhaustively verify every signed 16-bit input value
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
		byval samples as long _
	)

declare sub fb_sfxConvertFloatToS32 cdecl alias "fb_sfxConvertFloatToS32" _
	( _
		byval src as single ptr, _
		byval dst as long ptr, _
		byval samples as long _
	)

declare sub fb_sfxConvertS16ToFloat cdecl alias "fb_sfxConvertS16ToFloat" _
	( _
		byval src as short ptr, _
		byval dst as single ptr, _
		byval samples as long _
	)

declare function fb_sfxFloatToS16 cdecl alias "fb_sfxFloatToS16" _
	( byval value as single ) as short

declare function fb_sfxFloatToS32 cdecl alias "fb_sfxFloatToS32" _
	( byval value as single ) as long

declare function fb_sfxS16ToFloat cdecl alias "fb_sfxS16ToFloat" _
	( byval value as short ) as single

function near_equal( byval lhs as single, byval rhs as single ) as integer
	function = ( abs( lhs - rhs ) < 0.00001f )
end function

dim src(0 to 9) as single
dim dst(0 to 9) as short
dim pcm(0 to 4) as short
dim restored(0 to 4) as single

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

pcm(0) = -32768
pcm(1) = -32767
pcm(2) = 0
pcm(3) = 32766
pcm(4) = 32767

fb_sfxConvertS16ToFloat( @pcm(0), @restored(0), 5 )

ASSERT( near_equal( restored(0), -1.0f ) )
ASSERT( near_equal( restored(1), -32767.0f / 32768.0f ) )
ASSERT( near_equal( restored(2), 0.0f ) )
ASSERT( near_equal( restored(3), 32766.0f / 32768.0f ) )
ASSERT( near_equal( restored(4), 1.0f ) )

const sample_count = 4099

dim source_many(0 to sample_count - 1) as single
dim pcm16_many(0 to sample_count - 1) as short
dim pcm32_many(0 to sample_count - 1) as long

for i as integer = 0 to sample_count - 1
	dim pattern as integer = ((i * 7919) mod 20001) - 10000
	source_many(i) = csng(pattern) / 4096.0f
next

source_many(0) = 0.0f / 0.0f
source_many(1) = 1.0f / 0.0f
source_many(2) = -1.0f / 0.0f
source_many(3) = -1.0f
source_many(4) = 1.0f
source_many(5) = -0.0f

fb_sfxConvertFloatToS16( @source_many(0), @pcm16_many(0), sample_count )
fb_sfxConvertFloatToS32( @source_many(0), @pcm32_many(0), sample_count )

for i as integer = 0 to sample_count - 1
	ASSERT( pcm16_many(i) = fb_sfxFloatToS16(source_many(i)) )
	ASSERT( pcm32_many(i) = fb_sfxFloatToS32(source_many(i)) )
next

const pcm_value_count = 65536

dim pcm_all(0 to pcm_value_count - 1) as short
dim restored_all(0 to pcm_value_count - 1) as single

for i as integer = 0 to pcm_value_count - 1
	pcm_all(i) = cshort(i - 32768)
next

fb_sfxConvertS16ToFloat( @pcm_all(0), @restored_all(0), pcm_value_count )

for i as integer = 0 to pcm_value_count - 1
	ASSERT( restored_all(i) = fb_sfxS16ToFloat(pcm_all(i)) )
next

'' end of convert-float-s16.bas
