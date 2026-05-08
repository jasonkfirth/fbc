''
'' sfxlib basics 02: PLAY strings.
''
'' PLAY is useful when the sound is naturally musical.  It accepts the familiar
'' BASIC music-string controls for tempo, octave, note length, articulation,
'' rests, and octave movement.
''

#include once "example_common.bi"

SfxExampleBanner( "SFXLIB BASICS 02: PLAY STRINGS" )

print "A foreground melody.  PLAY waits until this one finishes."
play "T120 O4 L8 C D E F G A B >C"

SfxExampleWait( 300 )

print "The same idea with staccato and a lower octave."
play "T120 O3 MS L8 C D E F G A B >C"

SfxExampleWait( 300 )

print "Two strings play together on two channels."
play "T120 O4 L8 C E G >C <G E C", _
     "T120 O3 L4 C C G G C C G C"

SfxExampleWait( 300 )

print "Channel PLAY starts a background phrase, so the program can continue."
play 0, "MB T140 O5 L16 C D E G E D C R C E G >C"
print "The phrase is running while this text prints."
SfxExampleWait( 1700 )

print "Done."

'' end of basics-02-play-strings.bas
