#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX RESUME CHANNEL" )

SFX LOAD 1, filename
SFX LOOP 2, 1
SfxExampleWait( 400 )
SFX PAUSE CHANNEL, 2
print "Channel 2 paused."
SfxExampleWait( 300 )
SFX RESUME CHANNEL, 2
print "Channel 2 resumed."
SfxExampleWait( 500 )
SFX STOP CHANNEL, 2
