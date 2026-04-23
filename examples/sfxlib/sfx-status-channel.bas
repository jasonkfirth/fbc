#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )
dim as long state

SfxExampleBanner( "SFX STATUS CHANNEL" )

SFX LOAD 1, filename
SFX LOOP 2, 1
SfxExampleWait( 200 )
state = SFX STATUS( CHANNEL, 2 )
print "Channel 2 status while active:"; state
SFX STOP CHANNEL, 2
