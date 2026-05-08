''
'' sfxlib basics 04: waves, envelopes, and instruments.
''
'' A WAVE chooses the oscillator shape.  An ENVELOPE shapes loudness over time.
'' An INSTRUMENT ties those two definitions together so a channel can reuse them.
''

#include once "example_common.bi"

SfxExampleBanner( "SFXLIB BASICS 04: INSTRUMENTS" )

print "Default sound."
sound 330, 0.45
SfxExampleWait( 650 )

print "Instrument 1: square wave with a quick attack and short release."
wave 1, 1
envelope 1, 0.01, 0.08, 0.45, 0.12
instrument 1, 1, 1
instrument 0, 1
sound 0, 330, 0.45, 0.80
SfxExampleWait( 650 )

print "Instrument 2: saw wave with a slower fade."
wave 2, 3
envelope 2, 0.08, 0.18, 0.35, 0.35
instrument 2, 2, 2
instrument 0, 2
sound 0, 330, 0.70, 0.80
SfxExampleWait( 950 )

print "Different instruments on different channels."
instrument 0, 1
instrument 1, 2
sound 0, 262, 0.60, 0.75
sound 1, 392, 0.60, 0.75
SfxExampleWait( 850 )

print "Done."

'' end of basics-04-instruments.bas
