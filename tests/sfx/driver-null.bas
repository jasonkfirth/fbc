' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

type FB_SFX_DRIVER_STATS
	as ulongint write_calls
	as ulongint frames_requested
	as ulongint frames_accepted
	as ulongint frames_dropped
	as ulongint underruns
	as ulongint overruns
	as ulongint short_writes
	as ulongint zero_writes
	as ulongint errors
	as ulongint recoveries
	as integer current_queue_fill
	as integer max_queue_fill
	as integer last_error
end type

declare function fb_sfxDriverStatsSnapshot cdecl alias "fb_sfxDriverStatsSnapshot" _
	( _
		byval driver_name as zstring ptr, _
		byval stats as FB_SFX_DRIVER_STATS ptr _
	) as integer

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
