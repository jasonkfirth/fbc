#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id

SfxExampleBanner( "MUSIC STOP" )

music_id = MUSIC PLAY( filename )
if( music_id < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 800 )
	MUSIC STOP( music_id )
	print "Stopped music id:"; music_id
	print "MUSIC STATUS() ="; MUSIC STATUS()
end if
