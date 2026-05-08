''
'' sfxlib basics 03: channels and mix controls.
''
'' Channels let one program keep separate volume, pan, and instrument choices
'' for different sound sources.
''

#include once "example_common.bi"

SfxExampleBanner( "SFXLIB BASICS 03: CHANNELS AND MIX CONTROLS" )

print "Master volume controls the whole mix."
volume 0.35
sound 440, 0.30
SfxExampleWait( 450 )

volume 0.85
sound 440, 0.30
SfxExampleWait( 500 )

print "Per-channel volume controls one channel."
volume 0, 0.25
volume 1, 0.85
sound 0, 440, 0.35, 1.00
sound 1, 660, 0.35, 1.00
SfxExampleWait( 550 )

print "Pan separates channels left and right on stereo output."
volume 0, 0.75
volume 1, 0.75
pan 0, -1.00
pan 1, 1.00
sound 0, 392, 0.45, 0.80
sound 1, 523, 0.45, 0.80
SfxExampleWait( 650 )

print "Balance moves the entire mix."
balance -0.65
sound 0, 392, 0.25, 0.80
sound 1, 523, 0.25, 0.80
SfxExampleWait( 400 )

balance 0.65
sound 0, 392, 0.25, 0.80
sound 1, 523, 0.25, 0.80
SfxExampleWait( 400 )

balance 0.00
pan 0, 0.00
pan 1, 0.00
volume 1.00
volume 0, 1.00
volume 1, 1.00

print "Done."

'' end of basics-03-channels-mix.bas
