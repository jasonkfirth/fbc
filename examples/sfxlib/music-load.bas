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
	print "This id can be passed to MUSIC PLAY or MUSIC LOOP."
end if
