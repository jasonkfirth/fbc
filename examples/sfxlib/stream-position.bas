#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "STREAM POSITION" )

if( STREAM OPEN( filename ) <> 0 ) then
	print "STREAM OPEN failed."
else
	STREAM PLAY
	SfxExampleWait( 300 )
	print "Position after 0.3 s:"; STREAM POSITION()
	SfxExampleWait( 300 )
	print "Position after 0.6 s:"; STREAM POSITION()
	STREAM STOP
end if
