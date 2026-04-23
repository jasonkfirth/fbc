#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "harmonized-scale.mid" )
dim as long result

SfxExampleBanner( "MIDI PLAY" )

if( MIDI OPEN( 0 ) <> 0 ) then
	print "No MIDI output device is available."
else
	result = MIDI PLAY( filename )
	print "MIDI PLAY returned"; result
		SfxExampleWait( 1200 )
		MIDI STOP
		MIDI CLOSE
	end if
