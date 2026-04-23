#include once "example_common.bi"

dim as long result

SfxExampleBanner( "MIDI SEND" )

if( MIDI OPEN( 0 ) <> 0 ) then
	print "No MIDI output device is available."
else
	result = MIDI SEND( &H90, 60, 100 )
	print "Note on result:"; result
	SfxExampleWait( 400 )
	result = MIDI SEND( &H80, 60, 0 )
	print "Note off result:"; result
	MIDI CLOSE
end if
