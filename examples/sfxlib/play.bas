'' PLAY example
'' Build a small two-part arrangement of "Good Morning to All".
'' The melody and bass line are sent to separate channels so the
'' example demonstrates the polyphonic side of PLAY.

#include once "example_common.bi"

SfxExampleBanner( "PLAY" )
print "Playing a short two-channel arrangement of ""Good Morning to All""."

play 0, "T112 O4 L8 G G A G >C B  G G A G >D C  G G >G E C B A  F F E C D C"
play 1, "T112 O3 L4 C C C C C C  C C G G G G  C C C C F F  C C G C G C"

SfxExampleWait( 5000 )
