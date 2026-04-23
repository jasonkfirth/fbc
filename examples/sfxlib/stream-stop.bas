#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "STREAM STOP" )

if( STREAM OPEN( filename ) <> 0 ) then
	print "STREAM OPEN failed."
else
	STREAM PLAY
	SfxExampleWait( 400 )
	STREAM STOP
	print "Position after stop:"; STREAM POSITION()
end if
