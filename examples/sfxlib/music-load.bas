#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long music_id

SfxExampleBanner( "MUSIC LOAD" )
print "Loading a reusable music asset from:"
print filename

music_id = MUSIC LOAD( filename )

if( music_id < 0 ) then
	print "MUSIC LOAD failed."
else
	print "Loaded music id:"; music_id
	print "Playing the loaded music id briefly."
	MUSIC PLAY music_id
	SfxExampleWait( 1800 )
	MUSIC STOP music_id
end if
