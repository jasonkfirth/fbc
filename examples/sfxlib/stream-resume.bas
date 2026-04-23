#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "STREAM RESUME" )

if( STREAM OPEN( filename ) <> 0 ) then
	print "STREAM OPEN failed."
else
	STREAM PLAY
	SfxExampleWait( 400 )
	STREAM PAUSE
	print "Paused at"; STREAM POSITION()
	SfxExampleWait( 300 )
	STREAM RESUME
	print "Resumed playback."
	SfxExampleWait( 500 )
	STREAM STOP
end if
