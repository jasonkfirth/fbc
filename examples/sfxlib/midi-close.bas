#include once "example_common.bi"

dim as long result

SfxExampleBanner( "MIDI CLOSE" )

if( MIDI OPEN( 0 ) <> 0 ) then
	print "No MIDI output device is available."
else
	result = MIDI CLOSE()
	print "MIDI CLOSE returned"; result
end if
