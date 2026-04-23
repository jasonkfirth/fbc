#include once "example_common.bi"

SfxExampleBanner( "CAPTURE RESUME" )

if( CAPTURE START() <> 0 ) then
	print "Capture device not available on this system."
else
	SfxExampleWait( 400 )
	CAPTURE PAUSE
	print "Capture paused."
	SfxExampleWait( 200 )
	CAPTURE RESUME
	print "Capture resumed."
	CAPTURE STOP
end if
