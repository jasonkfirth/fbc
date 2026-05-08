'' WAVE example

#include once "example_common.bi"

SfxExampleBanner( "WAVE" )

print "Default waveform."
sound 392, 0.20
SfxExampleWait( 300 )

print "Waveform 1: square."
wave 1, 1
instrument 1, 1, 0
voice 1
sound 392, 0.20
SfxExampleWait( 300 )

print "Waveform 2: triangle."
wave 2, 2
instrument 2, 2, 0
voice 2
sound 392, 0.20
SfxExampleWait( 300 )

print "Waveform 3: sawtooth."
wave 3, 3
instrument 3, 3, 0
voice 3
sound 392, 0.20
SfxExampleWait( 300 )

print "Waveform 4: noise."
wave 4, 4
instrument 4, 4, 0
voice 4
sound 392, 0.20
SfxExampleWait( 300 )

'' end of wave.bas
