''
'' FreeBASIC Sound Library (sfxlib)
'' --------------------------------
''
'' File: voice-effects-stress.bas
''
'' Purpose:
''
''     Provide an audible and recorded 4, 16, and 32-voice synthesis test.
''
'' Responsibilities:
''
''     - start an exact simultaneous voice count for each test stage
''     - exercise four waves, four ADSR envelopes, instruments, and panning
''     - add a stereo ping-pong echo to the 32-voice stage
''     - save the final driver output and report device underruns
''
'' This file intentionally does NOT contain:
''
''     - platform-specific audio calls
''     - a claim that the echo is room-modeling reverb
''     - game-specific music sequencing
''

#include once "sfxlib_raw.bi"
#include once "sfxlib_effects.bi"

declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" ( byval frames as long )
declare function fb_sfxVoiceActiveCount cdecl alias "fb_sfxVoiceActiveCount" ( ) as long

const CHANNEL_COUNT = 16
const TEST_SAMPLE_RATE = 44100
const STAGE_MILLISECONDS = 1150
const GAP_MILLISECONDS = 300
const ECHO_TAIL_MILLISECONDS = 650

dim shared as integer use_null_driver

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

sub VoiceStressWait( byval milliseconds as integer )
	if( milliseconds <= 0 ) then
		exit sub
	end if

	if( use_null_driver <> 0 ) then
		fb_sfxUpdate( clng( ( cdbl( milliseconds ) * TEST_SAMPLE_RATE ) / 1000.0 ) )
	else
		sleep milliseconds, 1
	end if
end sub

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

function VoiceStressStartStage( byval voice_count as integer ) as integer
	dim as integer voice_index
	dim as integer channel_index
	dim as integer instrument_index
	dim as single voice_volume

	if( voice_count < 4 or voice_count > 32 or ( voice_count mod 4 ) <> 0 ) then
		return -1
	end if

	'' Square-root normalization keeps decorrelated layers near the same
	'' perceived level while still reducing the gain of every added voice.
	voice_volume = 0.36 / sqr( csng( voice_count ) )

	for voice_index = 0 to voice_count - 1
		channel_index = voice_index mod CHANNEL_COUNT
		instrument_index = ( voice_index mod 4 ) + 1

		instrument channel_index, instrument_index
		pan channel_index, -0.75 + ( 0.50 * csng( voice_index mod 4 ) )
		sound channel_index, _
		      VoiceStressFrequency( voice_index, voice_count ), _
		      1.0, _
		      voice_volume
	next

	if( fb_sfxVoiceActiveCount() <> voice_count ) then
		return -1
	end if

	return 0
end function

function VoiceStressPlayStage _
	( _
		byval voice_count as integer, _
		byref label_text as string _
	) as integer

	print label_text; ": "; voice_count; " simultaneous voices"

	if( VoiceStressStartStage( voice_count ) <> 0 ) then
		print "ERROR: requested "; voice_count; " voices, but the engine reported "; _
		      fb_sfxVoiceActiveCount()
		return -1
	end if

	VoiceStressWait( STAGE_MILLISECONDS )

	if( fb_sfxVoiceActiveCount() <> 0 ) then
		print "ERROR: voices remained active after the stage"
		return -1
	end if

	return 0
end function

dim as string output_file = trim( command( 1 ) )
dim as string requested_driver = lcase( trim( environ( "SFXLIB_DRIVER" ) ) )
dim as double render_started
dim as double render_seconds

if( output_file = "" ) then
	output_file = exepath() + "\voice-effects-stress.wav"
end if

use_null_driver = iif( requested_driver = "null", 1, 0 )

print
print "SFXLIB 4 / 16 / 32 VOICE EFFECTS STRESS"
print "Output capture: "; output_file

VoiceStressConfigureSynth()

if( sfxlib.OutputCaptureStart() <> 0 ) then
	print "ERROR: could not start output capture"
	end 1
end if

'' Six seconds at 48 kHz covers the complete program and avoids reallocating
'' the recording buffer while the real-time driver is running.
if( sfxlib.OutputCaptureReserve( 48000 * 6 ) <> 0 ) then
	print "ERROR: could not reserve the output capture"
	sfxlib.OutputCaptureStop()
	end 1
end if

render_started = timer

if( VoiceStressPlayStage( 4, "Baseline" ) <> 0 ) then
	end 1
end if
VoiceStressWait( GAP_MILLISECONDS )

if( VoiceStressPlayStage( 16, "Quadrupled" ) <> 0 ) then
	end 1
end if
VoiceStressWait( GAP_MILLISECONDS )

if( sfxlib.Echo( 0.20, 0.11, 0.32 ) <> 0 ) then
	print "ERROR: could not enable the echo effect"
	end 1
end if

if( sfxlib.EchoEnabled() = 0 ) then
	print "ERROR: echo did not report itself enabled"
	end 1
end if

if( VoiceStressPlayStage( 32, "Doubled again, with ADSR and echo" ) <> 0 ) then
	end 1
end if
VoiceStressWait( ECHO_TAIL_MILLISECONDS )

render_seconds = timer - render_started

sfxlib.OutputCaptureStop()
if( sfxlib.OutputCaptureSave( strptr( output_file ) ) <> 0 ) then
	print "ERROR: could not save output capture"
	end 1
end if

sfxlib.EchoReset()

print "Driver underruns: "; sfxlib.OutputUnderruns()
print "Captured: "; output_file

if( use_null_driver <> 0 and render_seconds > 0.0 ) then
	print "Offline render speed: ";
	print using "###0.0"; 4.70 / render_seconds;
	print "x real time"
end if

if( sfxlib.OutputUnderruns() <> 0 ) then
	print "ERROR: the output device starved during the stress test"
	end 1
end if

print "PASS: exact 4, 16, and 32-voice stages completed"

'' end of voice-effects-stress.bas
