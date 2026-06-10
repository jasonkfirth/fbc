#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC POSITION" )

result = MUSIC PLAY( filename )
if( result < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 300 )
	print "Position after 0.3 s:"; MUSIC POSITION()
	SfxExampleWait( 300 )
	print "Position after 0.6 s:"; MUSIC POSITION()
	MUSIC STOP
end if
