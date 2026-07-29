''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-benchmark.bas
''
'' Purpose:
''
''     Measure completed PAINT operations without the timing being obscured by
''     the unrelated primitive families in primitive-benchmark.bas.
''
'' Responsibilities:
''
''     - draw one fully enclosed rectangular PAINT region
''     - repeat the compatible solid PAINT operation at a fixed scale
''     - report application-thread submission and ordered completion separately
''     - force the final GPU result through an ordered POINT readback
''     - select gfxlib2, forced OpenGL, or forced Vulkan without changing work
''
'' This file intentionally does NOT contain:
''
''     - a renderer-specific fast-path switch
''     - a timing result without a completion boundary
''     - an irregular-region workload, which requires a separate fixture
''
#ifdef __FB_ANDROID__
	#ifndef GFX2_REFERENCE
	#ifndef __FB_GFXLIB3__
		#define __FB_GFXLIB3__
	#endif
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
	const paint_count = 4
#else
	const screen_width = 1024
	const screen_height = 768
	const paint_count = 16
#endif

dim as double started
dim as double first_seconds
dim as double repeat_submit_seconds
dim as double repeat_seconds
dim as integer first_pixel
dim as integer final_pixel
dim as integer border_pixel

if screenres( screen_width, screen_height, 32, 1, backend_flags ) <> 0 then end 1

line ( 4, 4 )-( screen_width - 5, screen_height - 5 ), rgb( 0, 0, 255 ), b

started = timer
paint ( screen_width \ 2, screen_height \ 2 ), rgb( 31, 17, 9 ), rgb( 0, 0, 255 )
first_pixel = point( screen_width \ 2, screen_height \ 2 )
first_seconds = timer - started

started = timer
for index as integer = 2 to paint_count
	paint ( screen_width \ 2, screen_height \ 2 ), _
		rgb( index * 13, index * 7, index * 3 ), rgb( 0, 0, 255 )
next
repeat_submit_seconds = timer - started
final_pixel = point( screen_width \ 2, screen_height \ 2 )
border_pixel = point( 4, 4 )
repeat_seconds = timer - started

screen 0
print "paint_benchmark_first_seconds="; first_seconds
print "paint_benchmark_repeat_submit_seconds="; repeat_submit_seconds
print "paint_benchmark_repeat_seconds="; repeat_seconds
print "paint_benchmark_count="; paint_count
print "paint_benchmark_first_pixel="; first_pixel
print "paint_benchmark_final_pixel="; final_pixel
print "paint_benchmark_border_pixel="; border_pixel
end 0

'' end of paint-benchmark.bas
