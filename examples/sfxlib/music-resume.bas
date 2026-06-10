#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC RESUME" )

result = MUSIC PLAY( filename )
if( result < 0 ) then
	print "Unable to start music playback."
else
	SfxExampleWait( 700 )
	MUSIC PAUSE
	print "Music paused."
	SfxExampleWait( 400 )
	MUSIC RESUME
	print "Music resumed."
	SfxExampleWait( 700 )
	MUSIC STOP
end if
