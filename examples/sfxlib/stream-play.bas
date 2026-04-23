#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )
dim as long result

SfxExampleBanner( "STREAM PLAY" )

if( STREAM OPEN( filename ) <> 0 ) then
	print "STREAM OPEN failed."
else
	result = STREAM PLAY()
	print "STREAM PLAY returned"; result
	SfxExampleWait( 800 )
	STREAM STOP
end if
