' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

SfxTestUseNullDriver()

device list

dim as integer current = fb_sfxDeviceCurrent()
ASSERT( current >= 0 )

dim as zstring ptr driver_name = fb_sfxDeviceName( current )
ASSERT( driver_name <> 0 )
ASSERT( lcase( *driver_name ) = "null" )

sound 20000, 1

'' end of driver-null.bas
