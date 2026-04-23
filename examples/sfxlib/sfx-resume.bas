#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX RESUME" )

SFX LOAD 1, filename
SFX LOOP 1, 1
SfxExampleWait( 400 )
SFX PAUSE 1
print "Paused effect 1."
SfxExampleWait( 300 )
SFX RESUME 1
print "Resumed effect 1."
SfxExampleWait( 500 )
SFX STOP 1
