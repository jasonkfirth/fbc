#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "AUDIO STOP" )

if( AUDIO PLAY( filename ) = 0 ) then
	SfxExampleWait( 500 )
	AUDIO STOP
	print "AUDIO STATUS() after stop ="; AUDIO STATUS()
else
	print "AUDIO PLAY failed."
end if
