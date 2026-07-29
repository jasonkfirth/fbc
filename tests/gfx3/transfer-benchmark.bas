''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: transfer-benchmark.bas
''
'' Purpose:
''
''     Compare every built-in PUT operation and GET under gfxlib2 and the
''     selected gfxlib3 renderer.  A CPU FB.IMAGE is deliberately reused so
''     the benchmark measures the compatible transfer command rather than
''     allocation or application-side image construction.
''
'' Responsibilities:
''
''     - time each standard PUT mode with an ordered completed readback
''     - time GET as its own required device-to-CPU synchronization family
''     - retain a final pixel readback so queued work is never reported early
''
'' This file intentionally does NOT contain:
''
''     - PUT CUSTOM, whose user callback is necessarily CPU code
''     - image file decoding, allocation, or presentation timing
''     - vendor-specific performance thresholds
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
	const transfer_count = 512
	const get_count = 64
#else
	const screen_width = 1024
	const screen_height = 768
	const transfer_count = 4096
	const get_count = 512
#endif

dim as any ptr sprite_image
dim as any ptr captured_image
dim as integer ordered_pixel
dim as double section_started
dim as double pset_seconds, preset_seconds, and_seconds, or_seconds
dim as double xor_seconds, trans_seconds, alpha_seconds, blend_seconds
dim as double add_seconds, get_seconds

if screenres( screen_width, screen_height, 32, 1, backend_flags ) <> 0 then end 1

sprite_image = imagecreate( 32, 32, rgba( 80, 150, 220, 128 ), 32 )
captured_image = imagecreate( 32, 32, 0, 32 )
if sprite_image = 0 orelse captured_image = 0 then end 2

line sprite_image, ( 0, 0 )-( 31, 31 ), rgba( 255, 0, 255, 255 ), b
line sprite_image, ( 4, 4 )-( 27, 27 ), rgba( 40, 210, 90, 180 ), bf

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, pset
next
ordered_pixel = point( 16, 16 )
pset_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, preset
next
ordered_pixel = point( 16, 16 )
preset_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, and
next
ordered_pixel = point( 16, 16 )
and_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, or
next
ordered_pixel = point( 16, 16 )
or_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, xor
next
ordered_pixel = point( 16, 16 )
xor_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, trans
next
ordered_pixel = point( 16, 16 )
trans_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, alpha
next
ordered_pixel = point( 16, 16 )
alpha_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, alpha, 128
next
ordered_pixel = point( 16, 16 )
blend_seconds = timer - section_started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
section_started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, add, 128
next
ordered_pixel = point( 16, 16 )
add_seconds = timer - section_started

section_started = timer
for index as integer = 0 to get_count - 1
	dim as integer x = ( index * 37 ) mod ( screen_width - 32 )
	dim as integer y = ( index * 43 ) mod ( screen_height - 32 )
	get ( x, y )-( x + 31, y + 31 ), captured_image
next
ordered_pixel = point( 16, 16, captured_image )
get_seconds = timer - section_started

'' Directly writable FB.IMAGE pixels must invalidate the cached GPU copy.
pset sprite_image, ( 6, 6 ), rgba( 21, 43, 65, 87 )
put ( 0, 0 ), sprite_image, pset
if point( 6, 6 ) <> rgba( 21, 43, 65, 87 ) then end 3

imagedestroy captured_image
imagedestroy sprite_image
screen 0
print "transfer_benchmark_pset_seconds="; pset_seconds
print "transfer_benchmark_preset_seconds="; preset_seconds
print "transfer_benchmark_and_seconds="; and_seconds
print "transfer_benchmark_or_seconds="; or_seconds
print "transfer_benchmark_xor_seconds="; xor_seconds
print "transfer_benchmark_trans_seconds="; trans_seconds
print "transfer_benchmark_alpha_seconds="; alpha_seconds
print "transfer_benchmark_blend_seconds="; blend_seconds
print "transfer_benchmark_add_seconds="; add_seconds
print "transfer_benchmark_get_seconds="; get_seconds
print "transfer_benchmark_pixel="; ordered_pixel
end 0

'' end of transfer-benchmark.bas
