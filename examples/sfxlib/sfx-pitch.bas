''
'' SFX pitch example.
''
'' One loaded sample can be played back at different pitch ratios.  A pitch
'' of 1.0 uses the file's original speed, 0.5 plays an octave lower and slower,
'' and 2.0 plays an octave higher and faster.
''

#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "buzzer.wav" )

SfxExampleBanner( "SFX PITCH" )

SFX LOAD 1, filename

print "Original sample."
SFX PLAY 0, 1, 1.0
SfxExampleWait( 550 )

print "Same sample, lower."
SFX PLAY 0, 1, 0.5
SfxExampleWait( 900 )

print "Same sample, higher."
SFX PLAY 0, 1, 2.0
SfxExampleWait( 450 )

print "Same sample layered at three pitches."
SFX PLAY 0, 1, 0.75
SFX PLAY 1, 1, 1.00
SFX PLAY 2, 1, 1.50
SfxExampleWait( 750 )

print "Looped lower pitch, then stop the channel."
SFX LOOP 0, 1, 0.75
SfxExampleWait( 700 )
SFX STOP CHANNEL, 0
SfxExampleWait( 150 )

print "Done."

'' end of sfx-pitch.bas
