#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX PAUSE" )

SFX LOAD 1, filename
SFX LOOP 1, 1
SfxExampleWait( 400 )
SFX PAUSE 1
print "SFX STATUS(1) after pause ="; SFX STATUS( 1 )
SfxExampleWait( 300 )
SFX STOP 1
