#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "STREAM PAUSE" )

if( STREAM OPEN( filename ) <> 0 ) then
	print "STREAM OPEN failed."
else
	STREAM PLAY
	SfxExampleWait( 400 )
	STREAM PAUSE
	print "Paused stream at position"; STREAM POSITION()
	STREAM STOP
end if
