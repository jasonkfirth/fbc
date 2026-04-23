#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX STOP CHANNEL" )

SFX LOAD 1, filename
SFX LOOP 2, 1
SfxExampleWait( 500 )
SFX STOP CHANNEL, 2
print "Stopped channel 2."
