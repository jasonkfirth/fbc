#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "AUDIO RESUME" )

if( AUDIO PLAY( filename ) = 0 ) then
	SfxExampleWait( 500 )
	AUDIO PAUSE
	print "Playback paused."
	SfxExampleWait( 300 )
	AUDIO RESUME
	print "Playback resumed."
	SfxExampleWait( 700 )
	AUDIO STOP
else
	print "AUDIO PLAY failed."
end if
