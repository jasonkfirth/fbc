#include once "example_common.bi"

SfxExampleBanner( "CAPTURE PAUSE" )

if( CAPTURE START() <> 0 ) then
	print "Capture device not available on this system."
else
	SfxExampleWait( 400 )
	CAPTURE PAUSE
	print "Capture paused."
	CAPTURE STOP
end if
