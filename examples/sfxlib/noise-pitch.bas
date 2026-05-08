''
'' NOISE pitch example.
''
'' The three-argument form is white noise.  The four-argument form adds a
'' sample-and-hold update rate, which gives the noise an audible pitch color.
''

#include once "example_common.bi"

SfxExampleBanner( "NOISE PITCH" )

print "White noise: NOISE channel, seconds, volume"
noise 0, 0.35, 0.35
SfxExampleWait( 500 )

print "Low pitched noise."
noise 0, 80, 0.40, 0.45
SfxExampleWait( 550 )

print "Medium pitched noise."
noise 0, 600, 0.35, 0.35
SfxExampleWait( 500 )

print "High pitched noise."
noise 0, 6000, 0.25, 0.28
SfxExampleWait( 400 )

print "Done."

'' end of noise-pitch.bas
