' TEST_MODE : COMPILE_ONLY_OK

dim as string filename = "sound.wav"
dim as single samples(0 to 15)
dim as long result
dim as single level

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

music load filename
music play 1
music play filename
music loop 1
music loop filename
music pause
music pause 1
music resume
music resume 1
music stop
music stop 1
result = music status()
result = music current()
result = music position()

sfx load 1, filename
sfx play 1
sfx play 1, 2
sfx play 1, 2, 1.5
sfx loop 1
sfx loop 1, 2
sfx loop 1, 2, 0.5
sfx status
sfx status()
sfx status 1
sfx status channel, 2
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

audio play filename
audio loop filename
audio pause
audio pause()
audio resume
audio resume()
audio stop
audio stop()
result = audio status()

stream open filename
stream play
stream play()
stream pause
stream pause()
stream resume
stream resume()
stream seek 0
stream stop
stream stop()
result = stream position()

midi open 0
midi play filename
midi send &h90, 60, 100
midi pause
midi pause()
midi resume
midi resume()
midi stop
midi stop()
midi close
midi close()

device list
device list()
result = device select( 0 )
device info
device info()
device info 0
device info( 0 )

capture start
capture start()
result = capture status()
result = capture available()
result = capture save( filename )
result = capture read( @samples(0), 8 )
capture pause
capture pause()
capture resume
capture resume()
capture stop
capture stop()
