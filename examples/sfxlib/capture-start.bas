#include once "example_common.bi"

dim as long result

SfxExampleBanner( "CAPTURE START" )

result = CAPTURE START()
print "CAPTURE START returned"; result

if( result = 0 ) then
	SfxExampleWait( 500 )
	print "Frames available after 0.5 s:"; CAPTURE AVAILABLE()
	CAPTURE STOP
else
	print "Capture device not available on this system."
end if
