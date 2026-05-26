''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: sfxlib-command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches every public sfxlib
''     command and function registered by the compiler.
''
'' Responsibilities:
''
''     - exercise single-word sound intrinsics from rtl-sfx.bas
''     - exercise multi-word syntax from parser-quirk-sfx.bas
''     - use generated local media files and the null audio driver
''     - make each command enter the runtime without relying on hardware
''
'' This file intentionally does NOT contain:
''
''     - CoreAudio hardware validation
''     - subjective audio quality checks
''     - platform build orchestration
''

#include once "../sfx/sfx_test_common.bi"

SfxTestUseNullDriver()

dim shared as integer failures

sub expect_nonnegative _
	( _
		byref label as string, _
		byval value as long _
	)

	if( value < 0 ) then
		print label; ": got "; value
		failures += 1
	end if
end sub

sub write_midi_byte( byval f as integer, byval value as integer )
	SfxTestWriteByte f, value
end sub

sub write_midi_text( byval f as integer, byref text as string )
	SfxTestWriteText f, text
end sub

sub write_midi_16be( byval f as integer, byval value as integer )
	write_midi_byte f, value \ 256
	write_midi_byte f, value
end sub

sub write_midi_32be( byval f as integer, byval value as integer )
	write_midi_byte f, value \ 16777216
	write_midi_byte f, value \ 65536
	write_midi_byte f, value \ 256
	write_midi_byte f, value
end sub

sub write_empty_midi( byref filename as string )
	dim as integer f = freefile()

	SfxTestDeleteFile filename

	open filename for binary as #f
	write_midi_text f, "MThd"
	write_midi_32be f, 6
	write_midi_16be f, 0
	write_midi_16be f, 1
	write_midi_16be f, 96
	write_midi_text f, "MTrk"
	write_midi_32be f, 4
	write_midi_byte f, 0
	write_midi_byte f, &hff
	write_midi_byte f, &h2f
	write_midi_byte f, 0
	close #f
end sub

dim as string wav_file = "sfxlib-command-sweep.wav"
dim as string midi_file = "sfxlib-command-sweep.mid"
dim as string capture_file = "sfxlib-command-sweep-capture.wav"
dim as single samples(0 to 31)
dim as long result
dim as single level

SfxTestWriteSineWav wav_file, 44100, 440.0, 2205
write_empty_midi midi_file
SfxTestDeleteFile capture_file

device list
device info
device info()
device info 0
device info( 0 )
result = device select( 0 )
expect_nonnegative "device select", result

beep 0.01, 0.0
sound 440, 0.01
sound 440, 18
sound 440, 18, 127
sound 1, 440, 0.01, 0.5
sound 1, 4096, 60
sound 1, 49152, 240, 1, 0, 100, 1, 0
sound 1, -15, 53, 20
sound 0, 100, 10, 8
sound 1, 121, 10, 8
sound 1000, 110, 0
sound 500, 110, 0, 131, 0, 196, 3

tone 1, 440, 0.01
noise 1, 0.01, 0.5
noise 1, 1200, 0.01, 0.5
note "C", 4, 0.01
note 1, "D#", 5, 0.01
rest 0.01
rest 1, 0.01
play "CDE"
play 1, "CDE"
play "CDE", "EFG"
play "CDE", "EFG", "GAB"

tempo 120
result = tempo()
channel 2
result = channel()
octave 4
result = octave()
voice 1
result = voice()
vol 8
volume 0.5
level = volume()
volume 1, 0.5
level = volume( 1 )
balance 0.0
level = balance()
pan 1, 0.0
level = pan( 1 )
wave 1, 0
envelope 1, 0.01, 0.10, 0.50, 0.20
instrument 1, 1, 1
instrument 1, 1

result = music load( wav_file )
expect_nonnegative "music load", result
music play result
music play wav_file
music loop result
music loop wav_file
music pause
music pause result
music resume
music resume result
music stop
music stop result
result = music status()
result = music current()
result = music position()

sfx load 1, wav_file
sfx play 1
sfx play 1, 2
sfx play 1, 2, 1.5
sfx loop 1
sfx loop 1, 2
sfx loop 1, 2, 0.5
sfx status
result = sfx status()
result = sfx status( 1 )
result = sfx status( channel, 2 )
sfx pause
sfx pause()
sfx pause 1
sfx pause channel, 2
sfx resume
sfx resume()
sfx resume 1
sfx resume channel, 2
sfx stop
sfx stop()
sfx stop 1
sfx stop channel, 2

result = audio play( wav_file )
expect_nonnegative "audio play", result
result = audio loop( wav_file )
expect_nonnegative "audio loop", result
audio pause
audio pause()
audio resume
audio resume()
audio stop
audio stop()
result = audio status()

result = stream open( wav_file )
expect_nonnegative "stream open", result
result = stream play()
expect_nonnegative "stream play", result
stream pause
stream pause()
stream resume
stream resume()
result = stream seek( 0 )
expect_nonnegative "stream seek", result
stream stop
stream stop()
result = stream position()

result = midi open( 0 )
midi play midi_file
midi send &h90, 60, 100
midi pause
midi pause()
midi resume
midi resume()
midi stop
midi stop()
midi close
midi close()

result = capture start()
result = capture status()
result = capture available()
result = capture save( capture_file )
result = capture read( @samples(0), 8 )
capture pause
capture pause()
capture resume
capture resume()
capture stop
capture stop()

fb_sfxUpdate 5000
sleep 20, 1

SfxTestDeleteFile wav_file
SfxTestDeleteFile midi_file
SfxTestDeleteFile capture_file

if( failures <> 0 ) then
	end 1
end if

end 0

'' end of sfxlib-command-sweep.bas
