#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX LOOP" )

SFX LOAD 1, filename
SFX LOOP 2, 1
print "Looping effect 1 on channel 2 for a short demo."
SfxExampleWait( 1200 )
SFX STOP 2
