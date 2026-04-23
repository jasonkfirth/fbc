#include once "example_common.bi"

''
'' This example is a guided tour of the major command groups provided by
'' sfxlib.  It is deliberately "big but simple":
'' - each feature area gets its own small subroutine
'' - commands are run in a straightforward sequence
'' - the program prints the return values and status values it sees
'' That makes it useful both as a demo and as a starting point for readers
'' who want to copy one section into their own program.
''

declare sub DemoDevice()
declare sub DemoSynthesis()
declare sub DemoMusic( byref music_file as string )
declare sub DemoSfx( byref sfx_file as string )
declare sub DemoAudioAndStream( byref audio_file as string )
declare sub DemoMidi( byref midi_file as string )
declare sub DemoCapture( byref capture_output as string )

dim as string music_file = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as string sfx_file = SfxExampleMedia( "buzzer.wav" )
dim as string audio_file = SfxExampleMedia( "clown-laugh.mp3" )
dim as string midi_file = SfxExampleMedia( "harmonized-scale.mid" )
dim as string capture_output = exepath() & SFX_EXAMPLE_PATHSEP & "showcase-capture.wav"

SfxExampleBanner( "SFXLIB SHOWCASE" )
print "This program walks through the major sfxlib feature areas in one go."
print "It is meant to be simple, sequential, and easy to read."

''
'' Run the feature groups one after another.
'' The earlier groups focus on device selection and generated sound.
'' The later groups demonstrate file-based playback and audio capture.
''
DemoDevice()
DemoSynthesis()
DemoMusic( music_file )
DemoSfx( sfx_file )
DemoAudioAndStream( audio_file )
DemoMidi( midi_file )
DemoCapture( capture_output )

print
print "Showcase complete."

sub DemoDevice()
	dim as long result

	SfxExampleBanner( "DEVICE COMMANDS" )
	'' DEVICE LIST prints the available audio devices, if any.
	DEVICE LIST
	print
	print "Default device information:"
	'' DEVICE INFO with no argument reports the current/default device.
	DEVICE INFO
	print
	print "Trying DEVICE SELECT( 0 )."
	'' DEVICE SELECT chooses a device by index.
	'' Many systems will use device 0 for the default output.
	result = DEVICE SELECT( 0 )
	print "DEVICE SELECT returned"; result
end sub

sub DemoSynthesis()
	SfxExampleBanner( "SYNTHESIS COMMANDS" )

	'' channel(), octave(), tempo(), volume(), and balance() are both getters
	'' and setters in this command set.  The examples below first print the
	'' old value, then set a new one, then print the new value.
	print "channel() before ="; channel()
	channel 1
	print "channel() after  ="; channel()

	print "octave() before ="; octave()
	octave 5
	print "octave() after  ="; octave()

	print "tempo() before ="; tempo()
	tempo 132
	print "tempo() after  ="; tempo()

	print "volume() before ="; volume()
	volume 0.65
	print "volume() after  ="; volume()

	print "balance() before ="; balance()
	balance -0.20
	print "balance() after  ="; balance()

	'' pan(channel) controls left/right placement for one channel.
	pan 1, -0.35
	print "pan(1) ="; pan(1)

	'' These four commands define a simple instrument:
	'' - WAVE chooses the waveform
	'' - ENVELOPE shapes the attack/decay/sustain/release
	'' - INSTRUMENT ties the settings together
	'' - VOICE chooses which instrument is active
	wave 1, 2
	envelope 1, 0.01, 0.10, 0.55, 0.15
	instrument 1, 1, 1
	voice 1

	'' The next few commands generate sound directly:
	'' - SOUND plays a frequency for a duration
	'' - TONE is a related tone generator command
	'' - NOISE produces noise instead of a pitched tone
	'' - NOTE plays note names such as C or E
	'' - PLAY uses a compact string notation for short musical phrases
	print "Generating SOUND, TONE, NOISE, NOTE, and PLAY."
	sound 440, 0.20
	SfxExampleWait( 250 )
	tone 1, 660, 0.15
	SfxExampleWait( 250 )
	noise 0, 0.15, 0.45
	SfxExampleWait( 250 )
	note "C", 5, 0.15
	SfxExampleWait( 175 )
	note 1, "E", 5, 0.15
	SfxExampleWait( 175 )
	play 0, "T132 O4 L8 C E G >C <G E C"
	play 1, "T132 O3 L4 C C G G C C G C"
	SfxExampleWait( 1800 )
end sub

sub DemoMusic( byref music_file as string )
	dim as long loaded_music
	dim as long active_music

	SfxExampleBanner( "MUSIC COMMANDS" )
	print "Music file:"; music_file

	'' MUSIC LOAD prepares a music file and returns an id.
	'' This example prints the id so the reader can see what came back.
	loaded_music = MUSIC LOAD( music_file )
	print "MUSIC LOAD returned"; loaded_music
	print "MUSIC STATUS before playback:"; MUSIC STATUS()

	'' MUSIC PLAY starts playback immediately and returns the active music id.
	active_music = MUSIC PLAY( music_file )
	print "MUSIC PLAY returned"; active_music

	if( active_music >= 0 ) then
		'' While music is running we can inspect state with CURRENT, POSITION,
		'' and STATUS, then pause/resume by id.
		SfxExampleWait( 350 )
		print "MUSIC CURRENT() ="; MUSIC CURRENT()
		print "MUSIC POSITION() ="; MUSIC POSITION()
		print "MUSIC STATUS() ="; MUSIC STATUS()
		MUSIC PAUSE( active_music )
		print "Paused music."
		SfxExampleWait( 250 )
		MUSIC RESUME( active_music )
		print "Resumed music."
		SfxExampleWait( 350 )
		MUSIC STOP
		print "MUSIC STATUS after stop:"; MUSIC STATUS()
	end if

	'' MUSIC LOOP works like MUSIC PLAY, but restarts automatically.
	print "Starting a short MUSIC LOOP demo."
	active_music = MUSIC LOOP( music_file )
	print "MUSIC LOOP returned"; active_music
	if( active_music >= 0 ) then
		SfxExampleWait( 400 )
		MUSIC STOP
	end if
end sub

sub DemoSfx( byref sfx_file as string )
	dim as long state

	SfxExampleBanner( "SFX COMMANDS" )
	print "Effect file:"; sfx_file

	'' SFX LOAD stores a short effect into an effect slot.
	'' Here we use effect slot 1.
	SFX LOAD 1, sfx_file
	print "Playing effect 1 on the default channel."
	'' SFX PLAY 1 means "play effect id 1 using the default channel choice".
	SFX PLAY 1
	SfxExampleWait( 300 )

	print "Looping effect 1 on channel 2."
	'' SFX LOOP 2, 1 means "play effect id 1 on channel 2 in a loop".
	SFX LOOP 2, 1
	SfxExampleWait( 250 )
	'' SFX STATUS can report the state of a specific channel.
	state = SFX STATUS( CHANNEL, 2 )
	print "SFX STATUS( CHANNEL, 2 ) ="; state

	'' Channel-oriented pause/resume/stop commands are useful when several
	'' effects might be playing at once.
	SFX PAUSE CHANNEL, 2
	print "Paused channel 2."
	SfxExampleWait( 250 )
	SFX RESUME CHANNEL, 2
	print "Resumed channel 2."
	SfxExampleWait( 300 )
	SFX STOP CHANNEL, 2
	print "Stopped channel 2."
end sub

sub DemoAudioAndStream( byref audio_file as string )
	dim as long result

	SfxExampleBanner( "AUDIO AND STREAM COMMANDS" )
	print "Audio file:"; audio_file
	'' AUDIO commands are the "simple one-at-a-time" playback interface.
	print "AUDIO STATUS before playback:"; AUDIO STATUS()

	result = AUDIO PLAY( audio_file )
	print "AUDIO PLAY returned"; result
	if( result = 0 ) then
		SfxExampleWait( 300 )
		print "AUDIO STATUS while playing:"; AUDIO STATUS()
		AUDIO PAUSE
		print "AUDIO STATUS after pause:"; AUDIO STATUS()
		SfxExampleWait( 200 )
		AUDIO RESUME
		SfxExampleWait( 250 )
		AUDIO STOP
	end if

	'' AUDIO LOOP keeps replaying the file until AUDIO STOP is called.
	result = AUDIO LOOP( audio_file )
	print "AUDIO LOOP returned"; result
	if( result = 0 ) then
		SfxExampleWait( 350 )
		AUDIO STOP
	end if

	'' STREAM commands expose a little more control.  You open a stream first,
	'' then use PLAY, POSITION, SEEK, PAUSE, RESUME, and STOP on that stream.
	result = STREAM OPEN( audio_file )
	print "STREAM OPEN returned"; result
	if( result = 0 ) then
		STREAM PLAY
		SfxExampleWait( 300 )
		print "STREAM POSITION() ="; STREAM POSITION()
		result = STREAM SEEK( 500 )
		print "STREAM SEEK returned"; result
		print "STREAM POSITION() after seek ="; STREAM POSITION()
		STREAM PAUSE
		SfxExampleWait( 200 )
		STREAM RESUME
		SfxExampleWait( 250 )
		STREAM STOP
	end if
end sub

sub DemoMidi( byref midi_file as string )
	dim as long result

	SfxExampleBanner( "MIDI COMMANDS" )
	print "MIDI file:"; midi_file

	'' MIDI OPEN chooses a MIDI output device.  Device 0 is the usual default.
	result = MIDI OPEN( 0 )
	print "MIDI OPEN returned"; result
	if( result <> 0 ) then
		print "No MIDI output device is available."
		exit sub
	end if

	'' MIDI SEND sends raw MIDI bytes.  Here we send note-on and note-off for
	'' middle C so the reader can see the low-level interface too.
	result = MIDI SEND( &H90, 60, 100 )
	print "MIDI SEND note-on returned"; result
	SfxExampleWait( 250 )
	result = MIDI SEND( &H80, 60, 0 )
	print "MIDI SEND note-off returned"; result

	'' MIDI PLAY loads and plays a .mid file through the selected device.
	result = MIDI PLAY( midi_file )
	print "MIDI PLAY returned"; result
	if( result = 0 ) then
		SfxExampleWait( 400 )
		result = MIDI PAUSE()
		print "MIDI PAUSE returned"; result
		SfxExampleWait( 250 )
		result = MIDI RESUME()
		print "MIDI RESUME returned"; result
		SfxExampleWait( 350 )
		result = MIDI STOP()
		print "MIDI STOP returned"; result
	end if

	'' Always close the MIDI device when finished.
	MIDI CLOSE
	print "MIDI CLOSE complete."
end sub

sub DemoCapture( byref capture_output as string )
	dim buffer( 0 to 1023 ) as single
	dim as long result
	dim as long available_frames
	dim as long frames_read

	SfxExampleBanner( "CAPTURE COMMANDS" )
	'' CAPTURE commands record audio from an input device, if one exists.
	print "CAPTURE STATUS before start:"; CAPTURE STATUS()

	result = CAPTURE START()
	print "CAPTURE START returned"; result
	if( result <> 0 ) then
		print "Capture device not available on this system."
		exit sub
	end if

	'' After capture has been running for a short time, AVAILABLE tells us how
	'' many frames can be read right now.
	SfxExampleWait( 400 )
	available_frames = CAPTURE AVAILABLE()
	print "CAPTURE AVAILABLE() ="; available_frames
	print "CAPTURE STATUS while active:"; CAPTURE STATUS()

	'' CAPTURE READ writes sample data into a buffer supplied by the program.
	'' We cap the read size here so the example stays simple.
	if( available_frames > 1024 ) then
		available_frames = 1024
	end if
	if( available_frames > 0 ) then
		frames_read = CAPTURE READ( @buffer(0), available_frames )
	else
		frames_read = 0
	end if
	print "CAPTURE READ returned"; frames_read

	'' The capture device can also be paused and resumed.
	CAPTURE PAUSE
	print "CAPTURE STATUS after pause:"; CAPTURE STATUS()
	SfxExampleWait( 200 )
	CAPTURE RESUME
	print "CAPTURE STATUS after resume:"; CAPTURE STATUS()
	SfxExampleWait( 250 )

	'' CAPTURE SAVE writes the currently captured audio to a file on disk.
	'' Save before stopping so the backend can pull a final block of audio
	'' even on systems where capture is not filled asynchronously.
	result = CAPTURE SAVE( capture_output )
	print "CAPTURE SAVE returned"; result
	print "Capture output file:"; capture_output

	CAPTURE STOP
	print "CAPTURE STATUS after stop:"; CAPTURE STATUS()
end sub
