''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: pset-benchmark.bas
''
'' Purpose:
''
''     Measure a completed public PSET stream independently from line, text,
''     or presentation work.
''
'' Responsibilities:
''
''     - issue a representative changing-colour PSET stream
''     - force command completion with one ordered POINT readback
''     - run unchanged against gfxlib2, OpenGL gfxlib3, and Vulkan gfxlib3
''
'' This file intentionally does NOT contain:
''
''     - SCREENLOCK, alpha, or POINT readback loops
''     - line and rectangle primitives
''     - renderer-private entry points
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
	const screen_width = 320
	const screen_height = 240
	const pset_count = 40000
#else
	const screen_width = 1024
	const screen_height = 768
	const pset_count = 200000
#endif

dim as integer final_pixel
dim as double started

if screenres( screen_width, screen_height, 32, 1, backend_flags ) <> 0 then end 1

started = timer
for index as integer = 0 to pset_count - 1
	pset ( ( index * 17 ) mod screen_width, _
		( index * 29 ) mod screen_height ), _
		rgb( index and 255, ( index * 7 ) and 255, ( index * 17 ) and 255 )
next
final_pixel = point( 0, 0 )

screen 0
print "pset_benchmark_seconds="; timer - started
print "pset_benchmark_count="; pset_count
print "pset_benchmark_pixel="; final_pixel
end 0

'' end of pset-benchmark.bas
