#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX PLAY" )

SFX LOAD 1, filename
print "Playing effect 1 on the default channel."
SFX PLAY 1
SfxExampleWait( 500 )

print "Playing effect 1 on channel 2."
SFX PLAY 2, 1
SfxExampleWait( 700 )
