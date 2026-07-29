''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-primitive-benchmark.bas
''
'' Purpose:
''
''     Measure the GPU primitive command path on the connected Android GLES
''     device at a size which completes safely on older mobile hardware.
''
'' Responsibilities:
''
''     - force gfxlib3 through the source-level Android compatibility route
''     - exercise the same public command families as the desktop benchmark
''     - leave an ordered POINT readback after each family
''
'' This file intentionally does NOT contain:
''
''     - a desktop gfxlib2 comparison, which is not available on Android
''     - Vulkan requirements on a GLES-only device
''     - a large PAINT workload which would prevent timely device teardown
''

#ifndef __FB_GFXLIB3__
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const screen_width = 320
const screen_height = 240
dim as any ptr sprite
dim as integer ordered_pixel
dim as double started
dim as double elapsed( 0 to 7 )

if screenres( screen_width, screen_height, 32, 1, 0 ) <> 0 then end 1
sprite = imagecreate( 16, 16, rgba( 60, 190, 245, 255 ), 32 )
if sprite = 0 then end 2
line sprite, ( 0, 0 )-( 15, 15 ), rgba( 255, 0, 255, 255 )

started = timer
for index as integer = 0 to 19999
	pset ( ( index * 37 ) mod screen_width, ( index * 53 ) mod screen_height ), _
		rgb( index and 255, ( index * 5 ) and 255, ( index * 11 ) and 255 )
next
ordered_pixel = point( 0, 0 )
elapsed( 0 ) = timer - started

started = timer
for index as integer = 0 to 499
	line ( ( index * 29 ) mod screen_width, ( index * 47 ) mod screen_height ) - _
		( ( index * 71 + 111 ) mod screen_width, ( index * 13 + 91 ) mod screen_height ), _
		rgb( index and 255, ( index * 7 ) and 255, ( index * 17 ) and 255 )
next
ordered_pixel = point( 0, 0 )
elapsed( 1 ) = timer - started

started = timer
for index as integer = 0 to 499
	dim as integer x = ( index * 31 ) mod ( screen_width - 16 )
	dim as integer y = ( index * 43 ) mod ( screen_height - 16 )
	line ( x, y )-( x + 15, y + 15 ), rgb( index and 255, _
		( index * 9 ) and 255, ( index * 19 ) and 255 ), bf
next
ordered_pixel = point( 0, 0 )
elapsed( 2 ) = timer - started

started = timer
for index as integer = 0 to 31
	circle ( 160 + ( ( index * 19 ) mod 80 ) - 40, _
		120 + ( ( index * 23 ) mod 60 ) - 30 ), 24 + ( index and 7 ), _
		rgb( index and 255, ( index * 5 ) and 255, ( index * 13 ) and 255 ), , , , f
next
ordered_pixel = point( 0, 0 )
elapsed( 3 ) = timer - started

line ( 4, 4 )-( 315, 235 ), rgb( 0, 0, 255 ), b
started = timer
paint ( 160, 120 ), rgb( 160, 80, 40 ), rgb( 0, 0, 255 )
ordered_pixel = point( 0, 0 )
elapsed( 4 ) = timer - started

started = timer
for index as integer = 0 to 127
	draw string ( ( index * 61 ) mod 224, ( index * 17 ) mod 224 ), _
		"gfxlib primitive", rgb( index and 255, ( index * 3 ) and 255, _
		( index * 7 ) and 255 )
next
ordered_pixel = point( 0, 0 )
elapsed( 5 ) = timer - started

started = timer
for index as integer = 0 to 1023
	put ( ( index * 17 ) mod 304, ( index * 29 ) mod 224 ), sprite, trans
next
ordered_pixel = point( 0, 0 )
elapsed( 6 ) = timer - started

imagedestroy sprite
screen 0
print "android_primitive_benchmark_pset_seconds="; elapsed( 0 )
print "android_primitive_benchmark_line_seconds="; elapsed( 1 )
print "android_primitive_benchmark_box_bf_seconds="; elapsed( 2 )
print "android_primitive_benchmark_circle_seconds="; elapsed( 3 )
print "android_primitive_benchmark_paint_seconds="; elapsed( 4 )
print "android_primitive_benchmark_text_seconds="; elapsed( 5 )
print "android_primitive_benchmark_put_trans_seconds="; elapsed( 6 )
print "android_primitive_benchmark_pixel="; ordered_pixel
end 0

'' end of android-primitive-benchmark.bas
