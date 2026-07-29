''
'' Project: FreeBASIC gfxlib3 benchmarks
'' -------------------------------------
''
'' File: screen-event-poll-benchmark.bas
''
'' Purpose:
''
''     Measure the empty SCREENEVENT polling workload used by real-time games.
''
'' Responsibilities:
''
''     - open the same fixed graphics mode under gfxlib2 and gfxlib3
''     - drain startup window events before the measured interval
''     - issue a bounded stream of empty public SCREENEVENT queries
''     - report completed calls without a hardware-specific threshold
''
'' This file intentionally does NOT contain:
''
''     - synthetic native message injection
''     - keyboard or mouse state benchmarks
''     - rendering commands inside the measured interval
''

#include once "fbgfx.bi"

#ifndef SCREEN_EVENT_POLL_COUNT
	const poll_count = 4096
#else
	const poll_count = SCREEN_EVENT_POLL_COUNT
#endif

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

if screenres( 320, 240, 32, 1, backend_flags ) <> 0 then end 1
screensync

dim as fb.EVENT event
while screenevent( @event )
wend

dim as integer event_count
dim as double started = timer
for index as integer = 1 to poll_count
	if screenevent( @event ) then event_count += 1
next
dim as double elapsed = timer - started

screen 0
print "screen_event_poll_seconds="; elapsed
print "screen_event_poll_count="; poll_count
print "screen_event_poll_calls_per_second="; poll_count / elapsed
print "screen_event_poll_events="; event_count

'' end of screen-event-poll-benchmark.bas
