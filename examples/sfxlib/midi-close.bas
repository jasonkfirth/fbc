#include once "example_common.bi"

dim as long result

SfxExampleBanner( "MIDI CLOSE" )

if( MIDI OPEN( 0 ) <> 0 ) then
	print "No MIDI output device is available."
else
	print "Sending a short MIDI note before closing."
	MIDI SEND &H90, 60, 100
	SfxExampleWait( 400 )
	MIDI SEND &H80, 60, 0
	result = MIDI CLOSE()
	print "MIDI CLOSE returned"; result
end if
