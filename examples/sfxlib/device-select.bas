#include once "example_common.bi"

dim as long result

SfxExampleBanner( "DEVICE SELECT" )
print "Selecting device 0, if one exists."

DEVICE LIST
result = DEVICE SELECT( 0 )
print "DEVICE SELECT returned"; result
