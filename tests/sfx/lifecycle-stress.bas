' TEST_MODE : MULTI_MODULE_TEST

''
'' FreeBASIC sfxlib tests
'' ----------------------
''
'' File: lifecycle-stress.bas
''
'' Purpose:
''
''     Stress repeated sfxlib initialization, playback, shutdown, and
''     reinitialization through the null driver.
''
'' Responsibilities:
''
''     - keep the test silent by forcing the null driver
''     - exercise generated sound, PLAY queues, and retained stop commands
''     - verify driver diagnostics across repeated init/exit cycles
''
'' This file intentionally does NOT contain:
''
''     - real audio device access
''     - subjective playback checks
''     - platform-specific driver recovery tests
''

#include once "sfx_test_common.bi"

declare function fb_sfxInit cdecl alias "fb_sfxInit" ( ) as long
declare sub fb_sfxExit cdecl alias "fb_sfxExit" ( )
declare sub fb_sfxDriverStatsReset cdecl alias "fb_sfxDriverStatsReset" ( byval driver_name as zstring ptr )

const CYCLES = 8
const VOICES_PER_CYCLE = 24
const UPDATE_FRAMES = 12000

dim as zstring * 5 null_name = "null"
dim as FB_SFX_DRIVER_STATS stats
dim as integer cycle_index
dim as integer voice_index
dim as integer channel_index

SfxTestUseNullDriver()

for cycle_index = 0 to CYCLES - 1
	ASSERT( fb_sfxInit() = 0 )

	device list

	dim as long current = fb_sfxDeviceCurrent()
	ASSERT( current >= 0 )

	dim as zstring ptr driver_name = fb_sfxDeviceName( current )
	ASSERT( driver_name <> 0 )
	ASSERT( lcase( *driver_name ) = "null" )

	fb_sfxDriverStatsReset( @null_name )

	for voice_index = 0 to VOICES_PER_CYCLE - 1
		channel_index = voice_index mod 16

		sound channel_index, 220 + (voice_index * 17), 0.04, 0.35
		play channel_index, "MB T240 L32 O4 C"
	next

	fb_sfxUpdate( UPDATE_FRAMES )

	ASSERT( fb_sfxDriverStatsSnapshot( @null_name, @stats ) = 0 )
	ASSERT( stats.write_calls > 0 )
	ASSERT( stats.frames_requested > 0 )
	ASSERT( stats.frames_accepted = stats.frames_requested )
	ASSERT( stats.frames_accepted >= UPDATE_FRAMES )
	ASSERT( stats.frames_dropped = 0 )
	ASSERT( stats.short_writes = 0 )
	ASSERT( stats.zero_writes = 0 )
	ASSERT( stats.errors = 0 )

	sfx stop
	music stop

	fb_sfxUpdate( 1024 )
	fb_sfxExit()
next

ASSERT( fb_sfxInit() = 0 )
fb_sfxExit()

'' end of lifecycle-stress.bas
