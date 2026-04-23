#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX STOP" )

SFX LOAD 1, filename
SFX LOOP 1, 1
SfxExampleWait( 500 )
SFX STOP 1
print "Effect 1 stopped."
print "SFX STATUS(1) ="; SFX STATUS( 1 )
