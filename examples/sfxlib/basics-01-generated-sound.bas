''
'' sfxlib basics 01: generated sound.
''
'' This example uses the direct synthesis commands.  These commands start a
'' sound and return to the program, so the example waits briefly after each one
'' to make the result easy to hear.
''

#include once "example_common.bi"

SfxExampleBanner( "SFXLIB BASICS 01: GENERATED SOUND" )

print "SOUND frequency, seconds"
sound 440, 0.50
SfxExampleWait( 650 )

print "SOUND channel, frequency, seconds, volume"
sound 1, 660, 0.35, 0.45
SfxExampleWait( 500 )

print "TONE channel, frequency, seconds"
tone 2, 330, 0.35
SfxExampleWait( 500 )

print "NOTE name, octave, seconds"
note "C", 4, 0.18
SfxExampleWait( 225 )
note "E", 4, 0.18
SfxExampleWait( 225 )
note "G", 4, 0.18
SfxExampleWait( 350 )

print "NOISE channel, seconds, volume"
noise 0, 0.30, 0.35
SfxExampleWait( 450 )

print "NOISE channel, update-rate, seconds, volume"
noise 0, 90, 0.25, 0.40
SfxExampleWait( 350 )
noise 0, 4500, 0.25, 0.28
SfxExampleWait( 450 )

print "Done."

'' end of basics-01-generated-sound.bas
