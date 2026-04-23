#include once "example_common.bi"

dim as long result

SfxExampleBanner( "MIDI OPEN" )
print "Opening MIDI device 0."

result = MIDI OPEN( 0 )
print "MIDI OPEN returned"; result

if( result = 0 ) then
	MIDI CLOSE
end if
