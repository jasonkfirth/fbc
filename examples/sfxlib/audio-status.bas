#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )

SfxExampleBanner( "AUDIO STATUS" )
print "Status before playback:"; AUDIO STATUS()

if( AUDIO PLAY( filename ) = 0 ) then
	SfxExampleWait( 400 )
	print "Status while playing:"; AUDIO STATUS()
	AUDIO STOP
end if

print "Status after stop:"; AUDIO STATUS()
