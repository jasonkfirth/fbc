#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC PAUSE" )

result = MUSIC PLAY( filename )
if( result < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 1600 )
	MUSIC PAUSE
	print "Paused current music."
	print "MUSIC STATUS() ="; MUSIC STATUS()
	SfxExampleWait( 500 )
	MUSIC STOP
end if
