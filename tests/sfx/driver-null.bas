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

dim as zstring * 5 null_name = "null"
dim as FB_SFX_DRIVER_STATS stats

ASSERT( fb_sfxDriverStatsSnapshot( @null_name, @stats ) = 0 )
ASSERT( stats.write_calls > 0 )
ASSERT( stats.frames_requested > 0 )
ASSERT( stats.frames_accepted = stats.frames_requested )
ASSERT( stats.frames_dropped = 0 )
ASSERT( stats.short_writes = 0 )
ASSERT( stats.zero_writes = 0 )
ASSERT( stats.errors = 0 )

'' end of driver-null.bas
