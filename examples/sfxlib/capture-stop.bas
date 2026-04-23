#include once "example_common.bi"

SfxExampleBanner( "CAPTURE STOP" )

if( CAPTURE START() = 0 ) then
	SfxExampleWait( 500 )
	CAPTURE STOP
	print "Capture stopped cleanly."
else
	print "Capture device not available on this system."
end if
