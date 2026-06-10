#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC PLAY" )
print "Starting music playback directly from a file."

result = MUSIC PLAY( filename )

if( result < 0 ) then
	print "MUSIC PLAY failed."
else
	print "MUSIC PLAY returned:"; result
	SfxExampleWait( 1500 )
	print "Stopping the demo track."
	MUSIC STOP
end if
