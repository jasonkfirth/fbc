#include once "example_common.bi"

dim as long result

SfxExampleBanner( "MIDI OPEN" )
print "Opening MIDI device 0."

result = MIDI OPEN( 0 )
print "MIDI OPEN returned"; result

if( result = 0 ) then
	print "Sending a short MIDI note before closing."
	MIDI SEND &H90, 60, 100
	SfxExampleWait( 400 )
	MIDI SEND &H80, 60, 0
	MIDI CLOSE
end if
