#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "good-morning-to-all.ogg" )
dim as long result

SfxExampleBanner( "MUSIC LOAD" )
print "Loading the current music asset from:"
print filename

result = MUSIC LOAD( filename )

if( result < 0 ) then
	print "MUSIC LOAD failed."
else
	print "MUSIC LOAD returned:"; result
	print "Playing the current music briefly."
	MUSIC PLAY
	SfxExampleWait( 1800 )
	MUSIC STOP
end if
