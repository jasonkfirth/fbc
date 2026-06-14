#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long state
dim as long result

SfxExampleBanner( "MUSIC STATUS" )

state = MUSIC STATUS()
print "Status before playback:"; state

result = MUSIC PLAY( filename )
if( result >= 0 ) then
	SfxExampleWait( 1600 )
	print "Status while playing:"; MUSIC STATUS()
	MUSIC STOP
end if

print "Status after stop:"; MUSIC STATUS()
