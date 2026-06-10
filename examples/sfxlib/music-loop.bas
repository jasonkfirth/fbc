#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC LOOP" )
print "Starting looping music playback for a short demo."

result = MUSIC LOOP( filename )

if( result < 0 ) then
	print "MUSIC LOOP failed."
else
	print "MUSIC LOOP returned:"; result
	SfxExampleWait( 1500 )
	print "Stopping the loop."
	MUSIC STOP
end if
