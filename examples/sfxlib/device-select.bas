#include once "example_common.bi"

dim as long result

SfxExampleBanner( "DEVICE SELECT" )
print "Selecting device 0, if one exists."

DEVICE LIST
result = DEVICE SELECT( 0 )
print "DEVICE SELECT returned"; result

if( result = 0 ) then
	print "Playing a short confirmation tone on the selected device."
	sound 660, 0.35
	SfxExampleWait( 500 )
end if
