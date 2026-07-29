''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screen-state-benchmark.bas
''
'' Purpose:
''
''     Measure the public GPU-facing screen-state operations which are not
''     represented by primitive or FB.IMAGE transfer benchmarks.
''
'' Responsibilities:
''
''     - time PALETTE updates followed by an ordered presentation boundary
''     - alternate distinct sources so every copy requests different page data
''     - separate page producer time from completed SCREENSET/SCREENCOPY time
''     - separate FLIP producer time from its explicit completion boundary
''     - time SCREENLOCK/SCREENUNLOCK compatibility-shadow synchronization
''     - select gfxlib2, forced OpenGL, or forced Vulkan without changing work
''
'' This file intentionally does NOT contain:
''
''     - native compositor capture or frame-pacing assertions
''     - direct platform API calls
''     - a GPU primitive throughput workload
''
#ifdef __FB_ANDROID__
	#ifndef __FB_GFXLIB3__
		#define __FB_GFXLIB3__
	#endif
#endif
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
	const iteration_count = 64
#else
	const iteration_count = 256
#endif

dim as double started
dim as double palette_seconds
dim as double page_submit_seconds
dim as double page_seconds
dim as double flip_submit_seconds
dim as double flip_seconds
dim as double lock_seconds
dim as integer result_pixel
dim as ubyte ptr pixels

'' PALETTE is meaningful only for indexed display storage. ''
if screenres( 320, 240, 8, 1, backend_flags ) <> 0 then end 1

started = timer
for index as integer = 0 to iteration_count - 1
	palette index and 255, index and 255, ( index * 3 ) and 255, _
		( index * 7 ) and 255
next
screensync
palette_seconds = timer - started

screen 0
'' SCREENLOCK exposes direct true-colour pixels, matching its public ABI. ''
if screenres( 320, 240, 32, 3, backend_flags ) <> 0 then end 2

screenset 0, 0
line ( 0, 0 )-( 319, 239 ), rgb( 20, 40, 60 ), bf
screenset 1, 1
line ( 0, 0 )-( 319, 239 ), rgb( 80, 100, 120 ), bf
screenset 2, 2
line ( 0, 0 )-( 319, 239 ), rgb( 140, 160, 180 ), bf
started = timer
for index as integer = 0 to iteration_count - 1
	if ( index and 1 ) = 0 then
		screenset 0, 2
		screencopy 0, 2
	else
		screenset 1, 2
		screencopy 1, 2
	end if
next
page_submit_seconds = timer - started
screensync
screenset 2, 2
result_pixel = point( 160, 120 )
page_seconds = timer - started

started = timer
for index as integer = 0 to iteration_count - 1
	if ( index and 1 ) = 0 then
		flip 0, 2
	else
		flip 1, 2
	end if
next
flip_submit_seconds = timer - started
screensync
result_pixel = point( 160, 120 )
flip_seconds = timer - started

started = timer
for index as integer = 0 to iteration_count - 1
	screenlock
	pixels = screenptr
	if pixels = 0 then
	screenunlock
	screen 0
		end 3
	end if
	pixels[0] = ( index and 255 )
	screenunlock
next
'' Force the deferred CPU-shadow upload to complete before reporting timing. ''
screensync
result_pixel = point( 0, 0 )
lock_seconds = timer - started

screen 0
print "screen_state_palette_seconds="; palette_seconds
print "screen_state_page_submit_seconds="; page_submit_seconds
print "screen_state_page_seconds="; page_seconds
print "screen_state_flip_submit_seconds="; flip_submit_seconds
print "screen_state_flip_seconds="; flip_seconds
print "screen_state_lock_seconds="; lock_seconds
print "screen_state_pixel="; result_pixel
end 0

'' end of screen-state-benchmark.bas
