''
'' FreeBASIC Sound Library tests
'' --------------------------------
''
'' File: voice-effects-stress.bas
''
'' Purpose:
''
''     Verify exact 4, 16, and 32-voice synthesis while exercising waves,
''     envelopes, instruments, panning, and the global echo effect.
''
'' Responsibilities:
''
''     - assert the requested number of simultaneous live voices
''     - render each stage through the deterministic null driver
''     - prove that the echo produces a decaying tail
''     - reject dropped, short, failed, or underrun output writes
''
'' This file intentionally does NOT contain:
''
''     - real audio-device access
''     - subjective listening checks
''     - timing assumptions based on wall-clock sleeps
''

' TEST_MODE : MULTI_MODULE_OK

#include once "sfx_test_common.bi"
#include once "sfxlib_effects.bi"

const CHANNEL_COUNT = 16
const STAGE_FRAMES = 44100
const GAP_FRAMES = 4410
const TAIL_FRAMES = 22050
const TOTAL_FRAMES = ( STAGE_FRAMES * 3 ) + ( GAP_FRAMES * 2 ) + TAIL_FRAMES
const MAX_FRAMES = TOTAL_FRAMES + 4096

function VoiceStressFrequency _
	( _
		byval voice_index as integer, _
		byval voice_count as integer _
	) as integer

	dim as integer base_frequency
	dim as integer copy_count = voice_count \ 4
	dim as integer copy_index = voice_index \ 4
	dim as integer detune = ( copy_index * 2 ) - ( copy_count - 1 )

	select case voice_index mod 4
	case 0
		base_frequency = 220
	case 1
		base_frequency = 277
	case 2
		base_frequency = 330
	case else
		base_frequency = 440
	end select

	return base_frequency + detune
end function

function VoiceStressReadRms _
	( _
		byval f as integer, _
		byval requested_frames as integer, _
		byref read_frames as integer _
	) as double

	dim as double sum_squares = 0.0
	dim as string line_text

	read_frames = 0

	do while( eof( f ) = 0 and read_frames < requested_frames )
		line input #f, line_text

		if( len( line_text ) > 0 ) then
			dim as double sample = val( line_text )

			sum_squares += sample * sample
			read_frames += 1
		end if
	loop

	if( read_frames = 0 ) then
		return 0.0
	end if

	return sqr( sum_squares / cdbl( read_frames ) )
end function

sub VoiceStressConfigureSynth()
	wave 1, 0
	wave 2, 1
	wave 3, 2
	wave 4, 3

	envelope 1, 0.010, 0.080, 0.55, 0.15
	envelope 2, 0.020, 0.120, 0.45, 0.18
	envelope 3, 0.040, 0.100, 0.60, 0.20
	envelope 4, 0.005, 0.150, 0.40, 0.12

	instrument 1, 1, 1
	instrument 2, 2, 2
	instrument 3, 3, 3
	instrument 4, 4, 4
end sub

sub VoiceStressStartStage( byval voice_count as integer )
	dim as integer voice_index
	dim as integer channel_index
	dim as integer instrument_index
	dim as single voice_volume = 0.36 / sqr( csng( voice_count ) )

	ASSERT( voice_count >= 4 )
	ASSERT( voice_count <= 32 )
	ASSERT( ( voice_count mod 4 ) = 0 )

	for voice_index = 0 to voice_count - 1
		channel_index = voice_index mod CHANNEL_COUNT
		instrument_index = ( voice_index mod 4 ) + 1

		'' Each SOUND captures the current instrument definition. Two voices
		'' may therefore share one logical channel without sharing oscillator
		'' or envelope state.
		instrument channel_index, instrument_index
		pan channel_index, -0.75 + ( 0.50 * csng( voice_index mod 4 ) )
		sound channel_index, _
		      VoiceStressFrequency( voice_index, voice_count ), _
		      1.0, _
		      voice_volume
	next

	ASSERT( fb_sfxVoiceActiveCount() = voice_count )
end sub

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

dump_file = temp_dir + "voice-effects-stress-driver.tmp"

SfxTestUseNullDriver()
SfxTestSetDriverDump( dump_file, MAX_FRAMES )
VoiceStressConfigureSynth()

'' Invalid parameters must not silently enable a partially configured effect.
ASSERT( sfxlib.Echo( -0.01, 0.11, 0.32 ) = -1 )
ASSERT( sfxlib.Echo( 0.20, 0.001, 0.32 ) = -1 )
ASSERT( sfxlib.Echo( 0.20, 0.11, 0.99 ) = -1 )
ASSERT( sfxlib.EchoEnabled() = 0 )

VoiceStressStartStage( 4 )
fb_sfxUpdate( STAGE_FRAMES )
ASSERT( fb_sfxVoiceActiveCount() = 0 )
fb_sfxUpdate( GAP_FRAMES )

VoiceStressStartStage( 16 )
fb_sfxUpdate( STAGE_FRAMES )
ASSERT( fb_sfxVoiceActiveCount() = 0 )
fb_sfxUpdate( GAP_FRAMES )

ASSERT( sfxlib.Echo( 0.20, 0.11, 0.32 ) = 0 )
ASSERT( sfxlib.EchoEnabled() <> 0 )
VoiceStressStartStage( 32 )
fb_sfxUpdate( STAGE_FRAMES )
ASSERT( fb_sfxVoiceActiveCount() = 0 )
fb_sfxUpdate( TAIL_FRAMES )

dim as zstring * 5 null_name = "null"
dim as FB_SFX_DRIVER_STATS stats
dim as integer dump_input = freefile()
dim as integer frames = 0
dim as integer read_frames
dim as double stage_rms( 0 to 2 )
dim as double tail_rms

ASSERT( open( dump_file for input as #dump_input ) = 0 )

for stage as integer = 0 to 2
	dim as integer leading_frames = 4096
	dim as integer trailing_frames = 4096

	VoiceStressReadRms( dump_input, leading_frames, read_frames )
	ASSERT( read_frames = leading_frames )
	frames += read_frames

	stage_rms( stage ) = VoiceStressReadRms( _
		dump_input, STAGE_FRAMES - leading_frames - trailing_frames, read_frames )
	ASSERT( read_frames = STAGE_FRAMES - leading_frames - trailing_frames )
	frames += read_frames

	VoiceStressReadRms( dump_input, trailing_frames, read_frames )
	ASSERT( read_frames = trailing_frames )
	frames += read_frames

	if( stage < 2 ) then
		VoiceStressReadRms( dump_input, GAP_FRAMES, read_frames )
		ASSERT( read_frames = GAP_FRAMES )
		frames += read_frames
	end if
next

tail_rms = VoiceStressReadRms( dump_input, 12000, read_frames )
ASSERT( read_frames = 12000 )
frames += read_frames

do
	VoiceStressReadRms( dump_input, 4096, read_frames )
	frames += read_frames
loop while( read_frames > 0 )

close #dump_input

ASSERT( frames >= TOTAL_FRAMES )
ASSERT( fb_sfxDriverStatsSnapshot( @null_name, @stats ) = 0 )
ASSERT( stats.frames_requested = TOTAL_FRAMES )
ASSERT( stats.frames_accepted = stats.frames_requested )
ASSERT( stats.frames_dropped = 0 )
ASSERT( stats.underruns = 0 )
ASSERT( stats.short_writes = 0 )
ASSERT( stats.zero_writes = 0 )
ASSERT( stats.errors = 0 )

ASSERT( stage_rms( 0 ) > 0.005 )
ASSERT( stage_rms( 1 ) > 0.005 )
ASSERT( stage_rms( 2 ) > 0.005 )

'' SOUND ends exactly at the tail boundary. Nonzero energy afterward can only
'' come from the delay line, so this also proves that the effect was processed.
ASSERT( tail_rms > 0.0001 )

sfxlib.EchoReset()
ASSERT( sfxlib.EchoEnabled() = 0 )

'' end of voice-effects-stress.bas
