''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: draw-benchmark.bas
''
'' Purpose:
''
''     Compare the compatible QB DRAW command language under gfxlib2 and the
''     selected gfxlib3 backend.  DRAW parsing belongs to the Basic-side
''     compatibility layer; its emitted line commands must remain GPU work.
''
'' Responsibilities:
''
''     - exercise DRAW movement, colour, and repeated line emission
''     - force an ordered completed pixel readback before reporting time
''     - keep the command string fixed so application string construction is
''       not mistaken for renderer time
''
'' This file intentionally does NOT contain:
''
''     - DRAW STRING glyph throughput, covered by primitive-benchmark.bas
''     - file I/O, input, or frame-pacing measurements
''     - a vendor-specific performance threshold
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
	const draw_count = 2000
#else
	const draw_count = 10000
#endif

dim as double started
dim as double elapsed
dim as integer ordered_pixel
dim as string draw_program

if screenres( 320, 240, 32, 1, backend_flags ) <> 0 then end 1

draw_program = "BM 32,32 C" & rgb( 30, 180, 240 ) & " R192 D128 L192 U128"
started = timer
for index as integer = 1 to draw_count
	draw draw_program
next
ordered_pixel = point( 32, 32 )
elapsed = timer - started

screen 0
print "draw_benchmark_seconds="; elapsed
print "draw_benchmark_commands="; draw_count
print "draw_benchmark_pixel="; ordered_pixel
end 0

'' end of draw-benchmark.bas
