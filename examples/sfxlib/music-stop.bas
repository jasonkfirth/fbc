#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC STOP" )

result = MUSIC PLAY( filename )
if( result < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 800 )
	MUSIC STOP
	print "Stopped current music."
	print "MUSIC STATUS() ="; MUSIC STATUS()
end if
