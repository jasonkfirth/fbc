''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: coordinate-state-benchmark.bas
''
'' Purpose:
''
''     Measure the public VIEW, WINDOW, and PMAP compatibility paths with the
''     same source for gfxlib2 and gfxlib3.
''
'' Responsibilities:
''
''     - time GPU-visible VIEW fill and border work
''     - time WINDOW coordinate-state changes
''     - time PMAP coordinate conversion while a window is active
''     - time POINTCOORD reads of the current graphics pen
''     - force an ordered pixel readback before reporting results
''
'' This file intentionally does NOT contain:
''
''     - direct renderer-private coordinate APIs
''     - input or native-window behaviour
''     - CPU image or SCREENLOCK transfer work
''

#if defined( __FB_ANDROID__ ) and not defined( GFX2_REFERENCE )
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
    const iteration_count = 512
#endif

dim as double started
dim as double view_seconds
dim as double window_seconds
dim as double pmap_seconds
dim as double pointcoord_seconds
dim as double mapped_sum
dim as double cursor_sum
dim as integer ordered_pixel

if screenres( 320, 240, 32, 1, backend_flags ) <> 0 then end 1

started = timer
for index as integer = 0 to iteration_count - 1
    view screen ( 16, 12 )-( 303, 227 ), _
        rgb( index and 255, ( index * 3 ) and 255, ( index * 7 ) and 255 ), _
        rgb( 220, 240, 255 )
    view
next
ordered_pixel = point( 16, 12 )
view_seconds = timer - started

started = timer
for index as integer = 0 to iteration_count - 1
    window screen ( 0, 0 )-( 319, 239 )
    window
next
window_seconds = timer - started

started = timer
for index as integer = 0 to iteration_count - 1
    window ( -160.0, -120.0 )-( 160.0, 120.0 )
    mapped_sum += pmap( 160.0, 0 ) + pmap( 120.0, 1 )
    window
next
pmap_seconds = timer - started

pset ( 123, 87 ), rgb( 40, 80, 120 )
ordered_pixel = point( 123, 87 )
started = timer
for index as integer = 0 to iteration_count * 16 - 1
    cursor_sum += pointcoord( 0 ) + pointcoord( 1 )
next
pointcoord_seconds = timer - started

screen 0
#ifdef __FB_GFXLIB3__
print "coordinate_state_renderer=gfxlib3"
#else
print "coordinate_state_renderer=gfxlib2"
#endif
print "coordinate_state_view_seconds="; view_seconds
print "coordinate_state_window_seconds="; window_seconds
print "coordinate_state_pmap_seconds="; pmap_seconds
print "coordinate_state_pointcoord_seconds="; pointcoord_seconds
print "coordinate_state_pmap_sum="; mapped_sum
print "coordinate_state_cursor_sum="; cursor_sum
print "coordinate_state_pixel="; ordered_pixel
end 0

'' end of coordinate-state-benchmark.bas
