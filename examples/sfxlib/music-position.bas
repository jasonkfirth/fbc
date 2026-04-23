#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id

SfxExampleBanner( "MUSIC POSITION" )

music_id = MUSIC PLAY( filename )
if( music_id < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 300 )
	print "Position after 0.3 s:"; MUSIC POSITION()
	SfxExampleWait( 300 )
	print "Position after 0.6 s:"; MUSIC POSITION()
	MUSIC STOP
end if
