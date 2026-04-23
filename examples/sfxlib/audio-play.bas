#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )
dim as long result

SfxExampleBanner( "AUDIO PLAY" )
print "Playing an external audio file:"
print filename

result = AUDIO PLAY( filename )
print "AUDIO PLAY returned"; result

if( result = 0 ) then
	SfxExampleWait( 1200 )
	AUDIO STOP
end if
