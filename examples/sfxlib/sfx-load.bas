#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX LOAD" )
print "Loading a short WAV file as effect id 1."
print filename

SFX LOAD 1, filename
print "Effect 1 is ready for SFX PLAY."
