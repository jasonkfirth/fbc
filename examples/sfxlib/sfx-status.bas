#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX STATUS" )
print "Global SFX status before playback:"; SFX STATUS()

SFX LOAD 1, filename
SFX PLAY 1
SfxExampleWait( 200 )
print "Global SFX status while active:"; SFX STATUS()
print "Effect 1 status while active:"; SFX STATUS( 1 )
SfxExampleWait( 700 )
