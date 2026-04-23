#include once "example_common.bi"

dim as long frames

SfxExampleBanner( "CAPTURE AVAILABLE" )

if( CAPTURE START() <> 0 ) then
	print "Capture device not available on this system."
else
	SfxExampleWait( 500 )
	frames = CAPTURE AVAILABLE()
	print "Frames available:"; frames
	CAPTURE STOP
end if
