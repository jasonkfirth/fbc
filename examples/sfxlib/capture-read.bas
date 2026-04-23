#include once "example_common.bi"

dim buffer( 0 to 1023 ) as single
dim as long frames
dim as long wanted

SfxExampleBanner( "CAPTURE READ" )

if( CAPTURE START() <> 0 ) then
	print "Capture device not available on this system."
else
	SfxExampleWait( 700 )
	wanted = CAPTURE AVAILABLE()
	if( wanted > 1024 ) then
		wanted = 1024
	end if
	if( wanted > 0 ) then
		frames = CAPTURE READ( @buffer(0), wanted )
	else
		frames = 0
	end if
	print "Frames read:"; frames
	CAPTURE STOP
end if
