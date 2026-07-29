''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: arc-benchmark.bas
''
'' Purpose:
''
''     Measure the public CIRCLE arc form separately from full ellipse
''     drawing. Arc construction has distinct compatibility and GPU command
''     behavior, so a filled-circle result is not representative.
''
'' Responsibilities:
''
''     - issue varied angular CIRCLE arcs through the public language API
''     - force an ordered readback after the final arc command
''     - select gfxlib2, forced OpenGL, or forced Vulkan without changing work
''
'' This file intentionally does NOT contain:
''
''     - a visual comparison or vendor-specific timing limit
''     - filled circles, which primitive-benchmark.bas already measures
''     - direct renderer or platform API calls
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
	const screen_width = 320
	const screen_height = 240
	const arc_count = 256
#else
	const screen_width = 1024
	const screen_height = 768
	const arc_count = 4096
#endif

const radians_per_degree = 3.14159265358979323846 / 180.0

dim as double started
dim as double elapsed
dim as integer ordered_pixel

if screenres( screen_width, screen_height, 32, 1, backend_flags ) <> 0 then end 1

started = timer
for index as integer = 0 to arc_count - 1
	dim as integer radius = 12 + ( index mod 96 )
	dim as integer center_x = ( index * 29 ) mod screen_width
	dim as integer center_y = ( index * 47 ) mod screen_height
	dim as double start_angle = ( index mod 360 ) * radians_per_degree
	dim as double end_angle = start_angle + ( 30 + ( index mod 240 ) ) * radians_per_degree

	circle ( center_x, center_y ), radius, _
		rgb( index and 255, ( index * 5 ) and 255, ( index * 13 ) and 255 ), _
		start_angle, end_angle
next
ordered_pixel = point( 0, 0 )
elapsed = timer - started

screen 0
print "arc_benchmark_seconds="; elapsed
print "arc_benchmark_arcs="; arc_count
print "arc_benchmark_pixel="; ordered_pixel
end 0

'' end of arc-benchmark.bas
