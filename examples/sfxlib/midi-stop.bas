#include once "example_common.bi"

dim as string filename = SfxExampleMedia( "harmonized-scale.mid" )
dim as long result

SfxExampleBanner( "MIDI STOP" )

if( MIDI OPEN( 0 ) <> 0 ) then
	print "No MIDI output device is available."
else
	if( MIDI PLAY( filename ) = 0 ) then
		SfxExampleWait( 700 )
		result = MIDI STOP()
		print "MIDI STOP returned"; result
	end if
	MIDI CLOSE
end if
