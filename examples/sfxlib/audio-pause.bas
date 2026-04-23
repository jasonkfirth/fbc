#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "AUDIO PAUSE" )

if( AUDIO PLAY( filename ) = 0 ) then
	SfxExampleWait( 500 )
	AUDIO PAUSE
	print "AUDIO STATUS() after pause ="; AUDIO STATUS()
	SfxExampleWait( 300 )
	AUDIO STOP
else
	print "AUDIO PLAY failed."
end if
