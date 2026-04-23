#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id

SfxExampleBanner( "MUSIC RESUME" )

music_id = MUSIC PLAY( filename )
if( music_id < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 700 )
	MUSIC PAUSE( music_id )
	print "Music paused."
	SfxExampleWait( 400 )
	MUSIC RESUME( music_id )
	print "Music resumed."
	SfxExampleWait( 700 )
	MUSIC STOP
end if
