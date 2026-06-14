#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC POSITION" )

result = MUSIC PLAY( filename )
if( result < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 1400 )
	print "Position after 1.4 s:"; MUSIC POSITION()
	SfxExampleWait( 500 )
	print "Position after 1.9 s:"; MUSIC POSITION()
	MUSIC STOP
end if
