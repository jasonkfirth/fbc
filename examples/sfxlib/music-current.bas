#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id
dim as long current_id

SfxExampleBanner( "MUSIC CURRENT" )

music_id = MUSIC PLAY( filename )
current_id = MUSIC CURRENT()

print "Started music id:"; music_id
print "MUSIC CURRENT() returned:"; current_id

MUSIC STOP
