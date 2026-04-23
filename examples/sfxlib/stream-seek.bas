#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )
dim as long result

SfxExampleBanner( "STREAM SEEK" )

if( STREAM OPEN( filename ) <> 0 ) then
	print "STREAM OPEN failed."
else
	STREAM PLAY
	SfxExampleWait( 300 )
	print "Position before seek:"; STREAM POSITION()
	result = STREAM SEEK( 500 )
	print "STREAM SEEK returned"; result
	print "Position after seek:"; STREAM POSITION()
	STREAM STOP
end if
