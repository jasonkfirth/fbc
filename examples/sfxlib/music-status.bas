#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long state
dim as long music_id

SfxExampleBanner( "MUSIC STATUS" )

state = MUSIC STATUS()
print "Status before playback:"; state

music_id = MUSIC PLAY( filename )
if( music_id >= 0 ) then
	SfxExampleWait( 500 )
	print "Status while playing:"; MUSIC STATUS()
	MUSIC STOP
end if

print "Status after stop:"; MUSIC STATUS()
