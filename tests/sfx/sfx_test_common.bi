''
'' FreeBASIC sfxlib tests
'' ----------------------
''
'' File: sfx_test_common.bi
''
'' Purpose:
''
''     Shared helpers for sfxlib runtime tests.
''
'' Responsibilities:
''
''     - select the null audio driver before sfxlib initializes
''     - configure bounded diagnostic dumps
''     - load generated sample dumps
''     - provide small signal-analysis helpers
''     - generate tiny PCM WAV files for SFX LOAD tests
''
'' This file intentionally does NOT contain:
''
''     - test cases
''     - platform audio device access
''     - full FFT implementation
''

#ifndef __SFX_TEST_COMMON_BI__
#define __SFX_TEST_COMMON_BI__

declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" ( byval frames as integer )
declare function fb_sfxDeviceCurrent cdecl alias "fb_sfxDeviceCurrent" ( ) as integer
declare function fb_sfxDeviceName cdecl alias "fb_sfxDeviceName" ( byval id as integer ) as zstring ptr

const SFX_TEST_SAMPLE_RATE = 44100.0
const SFX_TEST_PI = 3.1415926535897932384626433832795

sub SfxTestDeleteFile( byref filename as string )
	dim as integer f = freefile()

	if( open( filename for input as #f ) = 0 ) then
		close #f
		kill filename
	end if
end sub

sub SfxTestUseNullDriver()
	setenviron "SFXLIB_DRIVER=null"
end sub

sub SfxTestSetMixerDump( byref filename as string, byval frames as integer )
	SfxTestDeleteFile( filename )
	setenviron "SFXLIB_MIXER_DUMP=" + filename
	setenviron "SFXLIB_MIXER_DUMP_FRAMES=" + trim( str( frames ) )
end sub

sub SfxTestSetDriverDump( byref filename as string, byval frames as integer )
	SfxTestDeleteFile( filename )
	setenviron "SFXLIB_DRIVER_DUMP=" + filename
	setenviron "SFXLIB_DRIVER_DUMP_FRAMES=" + trim( str( frames ) )
end sub

function SfxTestLoadDump( byref filename as string, samples() as single ) as integer
	dim as integer f = freefile()
	dim as integer count = 0
	dim as string line_text

	if( open( filename for input as #f ) <> 0 ) then
		return 0
	end if

	do while( eof( f ) = 0 and count <= ubound( samples ) )
		line input #f, line_text

		if( len( line_text ) > 0 ) then
			samples( count ) = val( line_text )
			count += 1
		end if
	loop

	close #f

	return count
end function

function SfxTestRms( samples() as single, byval first_frame as integer, byval frames as integer ) as double
	dim as integer i
	dim as double sum_squares = 0.0

	if( frames <= 0 ) then
		return 0.0
	end if

	for i = 0 to frames - 1
		dim as double sample = samples( first_frame + i )
		sum_squares += sample * sample
	next

	return sqr( sum_squares / cdbl( frames ) )
end function

function SfxTestLowEnergyWindows _
	( _
		samples() as single, _
		byval first_frame as integer, _
		byval frames as integer, _
		byval window_frames as integer, _
		byval minimum_rms as double _
	) as integer

	dim as integer low_windows = 0
	dim as integer offset = 0

	if( window_frames <= 0 ) then
		return 0
	end if

	while( offset + window_frames <= frames )
		if( SfxTestRms( samples(), first_frame + offset, window_frames ) < minimum_rms ) then
			low_windows += 1
		end if

		offset += window_frames
	wend

	return low_windows
end function

function SfxTestEstimateZeroCrossHz _
	( _
		samples() as single, _
		byval first_frame as integer, _
		byval frames as integer _
	) as double

	dim as integer i
	dim as integer crossings = 0
	dim as single previous

	if( frames <= 1 ) then
		return 0.0
	end if

	previous = samples( first_frame )

	for i = 1 to frames - 1
		dim as single current = samples( first_frame + i )

		if( ( previous < 0.0 and current >= 0.0 ) or _
		    ( previous > 0.0 and current <= 0.0 ) ) then
			crossings += 1
		end if

		previous = current
	next

	return ( cdbl( crossings ) * 0.5 * SFX_TEST_SAMPLE_RATE ) / cdbl( frames )
end function

function SfxTestBandPower _
	( _
		samples() as single, _
		byval first_frame as integer, _
		byval frames as integer, _
		byval frequency as double _
	) as double

	dim as integer i
	dim as double re = 0.0
	dim as double im = 0.0

	if( frames <= 0 ) then
		return 0.0
	end if

	for i = 0 to frames - 1
		dim as double angle = ( 2.0 * SFX_TEST_PI * frequency * cdbl( i ) ) / SFX_TEST_SAMPLE_RATE
		dim as double sample = samples( first_frame + i )

		re += sample * cos( angle )
		im -= sample * sin( angle )
	next

	return sqr( re * re + im * im ) / cdbl( frames )
end function

function SfxTestCountChanges _
	( _
		samples() as single, _
		byval first_frame as integer, _
		byval frames as integer, _
		byval threshold as single _
	) as integer

	dim as integer i
	dim as integer changes = 0
	dim as single previous

	if( frames <= 1 ) then
		return 0
	end if

	previous = samples( first_frame )

	for i = 1 to frames - 1
		dim as single current = samples( first_frame + i )

		if( abs( current - previous ) > threshold ) then
			changes += 1
		end if

		previous = current
	next

	return changes
end function

sub SfxTestWriteByte( byval f as integer, byval value as integer )
	dim as ubyte b = value and &hff
	put #f, , b
end sub

sub SfxTestWrite16LE( byval f as integer, byval value as integer )
	if( value < 0 ) then
		value += 65536
	end if

	SfxTestWriteByte( f, value )
	SfxTestWriteByte( f, value \ 256 )
end sub

sub SfxTestWrite32LE( byval f as integer, byval value as integer )
	SfxTestWriteByte( f, value )
	SfxTestWriteByte( f, value \ 256 )
	SfxTestWriteByte( f, value \ 65536 )
	SfxTestWriteByte( f, value \ 16777216 )
end sub

sub SfxTestWriteText( byval f as integer, byref text as string )
	dim as integer i

	for i = 1 to len( text )
		SfxTestWriteByte( f, asc( mid( text, i, 1 ) ) )
	next
end sub

sub SfxTestWriteSineWav _
	( _
		byref filename as string, _
		byval sample_rate as integer, _
		byval frequency as double, _
		byval frames as integer _
	)

	dim as integer f = freefile()
	dim as integer data_bytes = frames * 2
	dim as integer i

	SfxTestDeleteFile( filename )

	open filename for binary as #f

	SfxTestWriteText( f, "RIFF" )
	SfxTestWrite32LE( f, 36 + data_bytes )
	SfxTestWriteText( f, "WAVE" )
	SfxTestWriteText( f, "fmt " )
	SfxTestWrite32LE( f, 16 )
	SfxTestWrite16LE( f, 1 )
	SfxTestWrite16LE( f, 1 )
	SfxTestWrite32LE( f, sample_rate )
	SfxTestWrite32LE( f, sample_rate * 2 )
	SfxTestWrite16LE( f, 2 )
	SfxTestWrite16LE( f, 16 )
	SfxTestWriteText( f, "data" )
	SfxTestWrite32LE( f, data_bytes )

	for i = 0 to frames - 1
		dim as double angle = ( 2.0 * SFX_TEST_PI * frequency * cdbl( i ) ) / cdbl( sample_rate )
		dim as integer sample = cint( sin( angle ) * 28000.0 )

		SfxTestWrite16LE( f, sample )
	next

	close #f
end sub

#endif

'' end of sfx_test_common.bi
