#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id

SfxExampleBanner( "MUSIC PLAY" )
print "Starting music playback directly from a file."

music_id = MUSIC PLAY( filename )

if( music_id < 0 ) then
	print "MUSIC PLAY failed."
else
	print "Active music id:"; music_id
	SfxExampleWait( 1500 )
	print "Stopping the demo track."
	MUSIC STOP
end if
