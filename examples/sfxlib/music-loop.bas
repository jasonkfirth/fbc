#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id

SfxExampleBanner( "MUSIC LOOP" )
print "Starting looping music playback for a short demo."

music_id = MUSIC LOOP( filename )

if( music_id < 0 ) then
	print "MUSIC LOOP failed."
else
	print "Looping music id:"; music_id
	SfxExampleWait( 1500 )
	print "Stopping the loop."
	MUSIC STOP
end if
