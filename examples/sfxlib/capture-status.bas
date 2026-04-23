#include once "example_common.bi"

dim as long state

SfxExampleBanner( "CAPTURE STATUS" )
print "Status before capture:"; CAPTURE STATUS()

if( CAPTURE START() <> 0 ) then
	print "Capture device not available on this system."
else
	SfxExampleWait( 300 )
	state = CAPTURE STATUS()
	print "Status while capturing:"; state
	CAPTURE STOP
	print "Status after stop:"; CAPTURE STATUS()
end if
