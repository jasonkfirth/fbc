#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )
dim as long result

SfxExampleBanner( "STREAM OPEN" )
print "Opening a stream so later commands can control it."

result = STREAM OPEN( filename )
print "STREAM OPEN returned"; result

if( result = 0 ) then
	print "Playing the opened stream briefly."
	STREAM PLAY
	SfxExampleWait( 900 )
	STREAM STOP
end if
