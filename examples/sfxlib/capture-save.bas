#include once "example_common.bi"

dim as string filename = exepath() & SFX_EXAMPLE_PATHSEP & "capture-example.wav"
dim as long result

SfxExampleBanner( "CAPTURE SAVE" )

if( CAPTURE START() = 0 ) then
	SfxExampleWait( 1000 )
	CAPTURE STOP
	result = CAPTURE SAVE( filename )
	print "CAPTURE SAVE returned"; result
	print "Output file:"; filename
else
	print "Capture device not available on this system."
end if
