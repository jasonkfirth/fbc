''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: primitive-benchmark.bas
''
'' Purpose:
''
''     Compare the public gfxlib primitive command paths under gfxlib2 and
''     gfxlib3 without measuring an application's game logic.
''
'' Responsibilities:
''
''     - force completion after each independently reported command family
''     - cover clear, PSET, LINE, BOX BF, CIRCLE, PAINT, text, and PUT
''     - select gfxlib3's OpenGL or Vulkan backend when requested at build time
''     - permit an Android gfxlib2 reference build with GFX2_REFERENCE
''
'' This file intentionally does NOT contain:
''
''     - a vendor-specific pass/fail performance threshold
''     - custom blenders, file I/O, or input timing
''     - a frame pacing benchmark
''

#ifdef __FB_ANDROID__
	'' GFX2_REFERENCE suppresses the Android gfxlib3 default for an apples-to-
	'' apples physical-device reference package. Desktop selection is unchanged.
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
	'' The connected API-24 device is a useful GLES correctness and throughput
	'' target, but a desktop-sized serial PAINT run would monopolize its Adreno
	'' 306 for minutes. Keep every family present at a fixed device-sized scale.
	const screen_width = 320
	const screen_height = 240
	const point_count = 40000
	const line_count = 1000
	const box_count = 1000
	const circle_count = 64
	const text_count = 256
	const sprite_count = 1024
	const circle_radius = 24
#else
	const screen_width = 1024
	const screen_height = 768
	const point_count = 200000
	const line_count = 6000
	const box_count = 6000
	const circle_count = 256
	const text_count = 1000
	const sprite_count = 4096
	const circle_radius = 96
#endif

dim as any ptr sprite_image
dim as integer result_pixel
dim as double section_started
dim as double clear_seconds, pset_seconds, line_seconds, box_seconds
dim as double circle_seconds, paint_seconds, text_seconds, put_seconds

if screenres( screen_width, screen_height, 32, 2, backend_flags ) <> 0 then end 1
screenset 1, 0
sprite_image = imagecreate( 16, 16, rgba( 60, 190, 245, 255 ), 32 )
if sprite_image = 0 then end 2
line sprite_image, ( 0, 0 )-( 15, 15 ), rgba( 255, 0, 255, 255 )

section_started = timer
for index as integer = 1 to 200
	cls rgb( index and 255, ( index * 3 ) and 255, ( index * 7 ) and 255 )
next
result_pixel = point( 0, 0 )
clear_seconds = timer - section_started

section_started = timer
for index as integer = 0 to point_count - 1
	pset ( ( index * 37 ) mod screen_width, ( index * 53 ) mod screen_height ), _
		rgb( index and 255, ( index * 5 ) and 255, ( index * 11 ) and 255 )
next
result_pixel = point( 0, 0 )
pset_seconds = timer - section_started

section_started = timer
for index as integer = 0 to line_count - 1
	line ( ( index * 29 ) mod screen_width, ( index * 47 ) mod screen_height ) - _
		( ( index * 71 + 311 ) mod screen_width, ( index * 13 + 211 ) mod screen_height ), _
		rgb( index and 255, ( index * 7 ) and 255, ( index * 17 ) and 255 )
next
result_pixel = point( 0, 0 )
line_seconds = timer - section_started

section_started = timer
for index as integer = 0 to box_count - 1
	dim as integer x = ( index * 31 ) mod ( screen_width - 32 )
	dim as integer y = ( index * 43 ) mod ( screen_height - 32 )
	line ( x, y )-( x + 31, y + 31 ), rgb( index and 255, _
		( index * 9 ) and 255, ( index * 19 ) and 255 ), bf
next
result_pixel = point( 0, 0 )
box_seconds = timer - section_started

section_started = timer
for index as integer = 0 to circle_count - 1
	circle ( ( screen_width \ 2 ) + ( ( index * 19 ) mod _
		( screen_width \ 4 ) ) - ( screen_width \ 8 ), _
		( screen_height \ 2 ) + ( ( index * 23 ) mod _
		( screen_height \ 4 ) ) - ( screen_height \ 8 ) ), _
		circle_radius + ( index and 15 ), _
		rgb( index and 255, ( index * 5 ) and 255, ( index * 13 ) and 255 ), , , , f
next
result_pixel = point( 0, 0 )
circle_seconds = timer - section_started

line ( 4, 4 )-( screen_width - 5, screen_height - 5 ), rgb( 0, 0, 255 ), b
section_started = timer
#ifdef __FB_ANDROID__
for index as integer = 1 to 4
#else
for index as integer = 1 to 16
#endif
	paint ( screen_width \ 2, screen_height \ 2 ), _
		rgb( index * 13, index * 7, index * 3 ), rgb( 0, 0, 255 )
next
result_pixel = point( 0, 0 )
paint_seconds = timer - section_started

section_started = timer
for index as integer = 0 to text_count - 1
	draw string ( ( index * 61 ) mod ( screen_width - 96 ), _
		( index * 17 ) mod ( screen_height - 16 ) ), "gfxlib primitive", _
		rgb( index and 255, ( index * 3 ) and 255, ( index * 7 ) and 255 )
next
result_pixel = point( 0, 0 )
text_seconds = timer - section_started

section_started = timer
for index as integer = 0 to sprite_count - 1
	put ( ( index * 17 ) mod ( screen_width - 16 ), _
		( index * 29 ) mod ( screen_height - 16 ) ), sprite_image, trans
next
result_pixel = point( 0, 0 )
put_seconds = timer - section_started

imagedestroy sprite_image
screen 0
print "primitive_benchmark_clear_seconds="; clear_seconds
print "primitive_benchmark_pset_seconds="; pset_seconds
print "primitive_benchmark_line_seconds="; line_seconds
print "primitive_benchmark_box_bf_seconds="; box_seconds
print "primitive_benchmark_circle_seconds="; circle_seconds
print "primitive_benchmark_paint_seconds="; paint_seconds
print "primitive_benchmark_text_seconds="; text_seconds
print "primitive_benchmark_put_trans_seconds="; put_seconds
print "primitive_benchmark_pixel="; result_pixel
end 0

'' end of primitive-benchmark.bas
