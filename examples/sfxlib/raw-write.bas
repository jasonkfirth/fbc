''
'' FreeBASIC sfxlib examples
'' -------------------------
''
'' File: raw-write.bas
''
'' Purpose:
''
''     Demonstrate the opt-in sfxlib_raw.bi helper for writing raw
''     floating-point samples through the sfxlib driver path.
''
'' Responsibilities:
''
''     - generate a short sine wave in BASIC code
''     - write samples through sfxlib.RawWrite()
''     - pace writes when the runtime output queue is full
''
'' This file intentionally does NOT contain:
''
''     - BASIC sound-command syntax examples
''     - file decoding
''     - platform driver selection
''

#include once "sfxlib_raw.bi"

const SAMPLE_RATE = 44100
const FRAMES = SAMPLE_RATE \ 2
const FREQUENCY = 440.0
const PI_VALUE = 3.14159265358979323846

dim samples( 0 to FRAMES - 1 ) as single
dim as long frame_index
dim as long written

print
print "========================================"
print "SFXLIB RAW WRITE"
print "========================================"
print "Writing a short generated sine wave through sfxlib_raw.bi."

for frame_index = 0 to FRAMES - 1
	dim as double phase = (2.0 * PI_VALUE * FREQUENCY * frame_index) / SAMPLE_RATE
	samples(frame_index) = csng(sin( phase ) * 0.25)
next

frame_index = 0
while( frame_index < FRAMES )
	written = sfxlib.RawWrite( @samples(frame_index), FRAMES - frame_index, 1 )

	if( written < 0 ) then
		print "RawWrite failed."
		end 1
	end if

	if( written = 0 ) then
		sleep 10, 1
	else
		frame_index += written
	end if
wend

sleep 700, 1

print "Done."

'' end of raw-write.bas
