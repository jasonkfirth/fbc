#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "clown-laugh.mp3" )
dim as long result

SfxExampleBanner( "AUDIO LOOP" )

result = AUDIO LOOP( filename )
print "AUDIO LOOP returned"; result

if( result = 0 ) then
	SfxExampleWait( 1500 )
	print "Stopping the loop."
	AUDIO STOP
end if
